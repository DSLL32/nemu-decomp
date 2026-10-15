#!/usr/bin/env python3
"""Build six Delphi 6 references and identify matching Nemu plugin runtimes.

Requires the original Delphi ISO archive, all five staged updates, 7z,
unshield, Wine, and python3-capstone. Extracts and builds under a new ~/.ai-tmp
directory. Does not install candidates or execute the original Nemu plugins.
WINEPREFIX may be set to reuse an existing scratch prefix.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from check_toolchains import DELPHI
from pe_fingerprints import PE32
from run_toolchain import winpath
from unpack_toolchains import (D6_UPDATES, apply_d6_update, digest,
                               extract_d6_updates, install_d6_base)


CONFIGURATIONS = {
    "rtm": (),
    "u1": ("u1",),
    "u2": ("u2",),
    "u2r1": ("u2", "r1"),
    "u2r2": ("u2", "r2"),
    "u2r3": ("u2", "r2", "r3"),
}


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--staging", type=Path, default=repo / "toolchains-staging")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    for program in ("7z", "unshield", "wine"):
        if not shutil.which(program):
            parser.error(f"Missing program: {program}")
    parent = Path.home() / ".ai-tmp"
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="nemu-d6-check.", dir=parent))
    env = os.environ.copy()
    env.setdefault("WINEPREFIX", str(scratch / "wine"))
    env.setdefault("WINEARCH", "win32")
    env.setdefault("WINEDEBUG", "-all")
    print(f"Scratch: {scratch}\nWine prefix: {env['WINEPREFIX']}", flush=True)
    base = scratch / "base"
    install_d6_base(args.staging, scratch / "base-media", base)
    payloads = extract_d6_updates(args.staging, scratch / "updates", D6_UPDATES)
    targets = [repo / "master/Plugin" / n for n in ("debugger.dll", "dialogs.dll")]
    result = dict(
        scratch=str(scratch),
        method="Statically linked probes compiled from each candidate's DCC32 and DCUs; "
               "map-bounded normalized instruction matching, mapped direct-edge validation, "
               "and three RTTI/VMT-anchored TControl method checks.",
        limitations=[
            "Counts cover eligible reference blocks, not every instruction in each plugin.",
            "Short or structurally identical routines can have multiple matching addresses.",
            "Global data identities and untemplated/indirect callees are not fully validated.",
            "Library matches identify supplied RTL/VCL builds; they do not independently "
            "prove the exact compiler executable or all application compiler switches.",
            "A locally patched library reproducing the same code cannot be ruled out."],
        configurations={}, selections={})
    for name, updates in CONFIGURATIONS.items():
        work, tc = scratch / name / "probe", scratch / name / "tc"
        work.mkdir(parents=True)
        shutil.copytree(base, tc)
        for update in updates:
            apply_d6_update(update, payloads[update], tc)
        # Resources are irrelevant to code matching; compiler/resource smoke
        # checks are covered separately by check_toolchains.py.
        source = DELPHI.replace("{$R probe.res}", "")
        (work / "probe.dpr").write_text(source)
        candidate_env = env.copy()
        candidate_env["WINEPATH"] = winpath(tc / "bin") + ";" + winpath(tc / "runtime")
        with (work / "compile.log").open("w") as log:
            process = subprocess.run(
                ["wine", str(tc / "bin/dcc32.exe"), "-Q", "-B", "-GD",
                 "-U" + winpath(tc / "lib"), "-I" + winpath(tc / "lib"),
                 "-R" + winpath(tc / "lib"), "probe.dpr"],
                cwd=work, env=candidate_env, stdout=log, stderr=subprocess.STDOUT,
                timeout=120)
        if process.returncode:
            raise RuntimeError((work / "compile.log").read_text(errors="replace"))
        report_path = work.parent / "comparison.json"
        with (work / "comparison.log").open("w") as log:
            subprocess.run(
                [sys.executable, str(repo / "scripts/compare_delphi_rtl.py"),
                 str(work / "probe.dll"), str(work / "probe.map"),
                 *(str(t) for t in targets), "--validate-links", "--output", str(report_path),
                 "--matches", str(work.parent / "matches.json")],
                stdout=log, stderr=subprocess.STDOUT, check=True)
        report = json.loads(report_path.read_text())
        config = dict(updates=list(updates), reference=report["reference"],
                      map=report["map"], template_count=report["template_count"],
                      components={}, targets={})
        for component in ("bin/dcc32.exe", "runtime/rtl60.bpl", "runtime/vcl60.bpl"):
            path = tc / component
            config["components"][component] = dict(
                sha256=digest(path), versions=PE32(path).report()["versions"])
        for target in report["targets"]:
            target_name = Path(target["path"]).name
            config["targets"][target_name] = target
            validation = target["link_validation"]
            if (not target["counts"].get("absent", 0) and not validation["conflicts"]
                    and all(a["matches"] for a in validation["tcontrol_vmt_anchors"])):
                result["selections"].setdefault(target_name, []).append(name)
            print(f"{name} -> {target_name}: {target['counts']}; "
                  f"{len(validation['conflicts'])} direct-edge conflicts", flush=True)
        result["configurations"][name] = config
    text = json.dumps(result, indent=2) + "\n"
    (scratch / "results.json").write_text(text)
    if args.output:
        args.output.write_text(text)
    for target in targets:
        matches = result["selections"].get(target.name, [])
        if len(matches) != 1:
            raise RuntimeError(f"Expected one matching reference for {target.name}, got {matches}")
        print(f"Selected {matches[0]} for {target.name}", flush=True)


if __name__ == "__main__":
    main()
