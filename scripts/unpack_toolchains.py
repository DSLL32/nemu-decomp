#!/usr/bin/env python3
"""Extract the Nemu toolchains from the staged media without running installers.

Requires Python 3, 7z, unshield, and msiinfo (Debian package msitools).
Existing target folders are never overwritten. Scratch space is allocated
under ~/.ai-tmp. Delphi updates go in d6u1 and d6u2.
"""

import argparse
import csv
from functools import cache
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

from pe_fingerprints import PE32


SP5 = "2001-03_X08-02111_X06-21660_CD_ROM.7z"
DELPHI = "Borland Delphi 6.0 Enterprise Edition (ISO).7z"
VS7_ROOT = "Program Files/Microsoft Visual Studio .NET/"
DELPHI_ROOT = "Install/program files/Borland/Delphi6/"
REQUIRED = [SP5, DELPHI, "en_vs6_ent_cd1.iso",
            "VSPROD1.iso", "VSPROD2.iso", "VSPROD3.iso",
            "delphi6-updates/D6_upd1_ent.exe", "delphi6-updates/D6_upd2_ent.exe"]
D6_UPDATES = {"u1": "D6_upd1_ent.exe", "u2": "D6_upd2_ent.exe",
              "r1": "D6_RTL1_ent.exe", "r2": "del6_ent_rtl2.exe", "r3": "d6_rtl3.zip"}


def extract(archive, destination, *members, compound=False):
    command = ["7z", "x", "-y", "-o" + str(destination)]
    if compound:
        command.append("-tCompound")
    result = subprocess.run(command + [str(archive), *members],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True)
    if result.returncode:
        raise RuntimeError(result.stdout)


def copy_tree(source, destination):
    """Use one casing, so SP5 overlays replace the RTM files on Linux."""
    for source_file in source.rglob("*"):
        if source_file.is_file():
            target = destination / str(source_file.relative_to(source)).lower()
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source_file, target)


def convert_mdf(source, target):
    # This specific SP5 disc uses 2352-byte Mode 1 sectors + 96-byte subchannels.
    with source.open("rb") as src, target.open("wb") as dst:
        while block := src.read(2448 * 4096):
            for pos in range(0, len(block), 2448):
                sector = block[pos:pos + 2448]
                if (len(sector) != 2448 or
                        sector[:12] != b"\0" + b"\xff" * 10 + b"\0" or
                        sector[15] != 1):
                    raise ValueError("Unexpected SP5 MDF sector format")
                dst.write(sector[16:2064])


def install_vc6(staging, scratch, destination):
    print("Extracting VC6 RTM, SP5, and Processor Pack", flush=True)
    base, patch, pp = (scratch / p for p in ("vc6-base", "sp5", "vc6-pp"))
    extract(staging / "en_vs6_ent_cd1.iso", base,
            *[f"VC98/{d}/*" for d in ("BIN", "INCLUDE", "LIB", "CRT", "MFC", "ATL")],
            *[f"COMMON/MSDEV98/BIN/{f}" for f in ("MSPDB60.DLL", "RC.EXE", "RCDLL.DLL")])
    extract(staging / SP5, scratch / "sp5-media",
            "2001-03_X08-02111_X06-21660_CD_ROM/image/CD_ROM.mdf")
    convert_mdf(scratch / "sp5-media/2001-03_X08-02111_X06-21660_CD_ROM/image/CD_ROM.mdf",
                scratch / "sp5.iso")
    extract(scratch / "sp5.iso", patch, "enu/vc98/*",
            "enu/common/msdev98/bin/*", "enu/PP/*")
    extract(patch / "enu/PP/vcpp.exe", pp)
    copy_tree(base / "VC98", destination)
    copy_tree(patch / "enu/vc98", destination)
    for source in (base / "COMMON/MSDEV98/BIN", patch / "enu/common/msdev98/bin"):
        for f in source.iterdir():
            if f.is_file() and f.name.lower() in ("mspdb60.dll", "rc.exe", "rcdll.dll"):
                shutil.copy2(f, destination / "bin" / f.name.lower())
    # File mappings from vcpp.inf, including its shortened source filenames.
    for name in ("c2.dll", "ml.exe", "ml.err", "h2inc.exe", "h2inc.err"):
        shutil.copy2(pp / name, destination / "bin" / name)
    headers = {h: h for h in ("dvec.h", "fpieee.h", "fvec.h", "ivec.h", "malloc.h",
                              "mm3dnow.h", "mmintrin.h")}
    headers.update({"emmintrin.h": "emmint_1.h", "xmmintrin.h": "xmmint_1.h"})
    for target, source in headers.items():
        shutil.copy2(pp / source, destination / "include" / target)
    shutil.copy2(pp / "align.c", destination / "crt/src/align.c")
    for name in ("PPreadme.htm", "MasmRef.doc", "procpack.chm"):
        shutil.copy2(pp / name, destination / name.lower())


def sdk_files(msi):
    tables = {}
    for table in ("File", "Component", "Directory", "Media"):
        text = subprocess.check_output(["msiinfo", "export", str(msi), table], text=True)
        rows = list(csv.reader(io.StringIO(text), delimiter="\t"))
        tables[table] = [dict(zip(rows[0], row)) for row in rows[3:]]
    dirs = {r["Directory"]: r for r in tables["Directory"]}
    components = {r["Component"]: r["Directory_"] for r in tables["Component"]}
    media = sorted(tables["Media"], key=lambda r: int(r["LastSequence"]))

    @cache
    def directory(key):
        row = dirs[key]
        name = row["DefaultDir"].split(":")[0].split("|")[-1]
        parent = directory(row["Directory_Parent"]) if row["Directory_Parent"] else Path()
        return parent / name

    result = []
    for row in tables["File"]:
        path = directory(components[row["Component_"]]) / row["FileName"].split("|")[-1]
        if "PlatformSDK" not in path.parts:
            continue
        relative = Path(*path.parts[path.parts.index("PlatformSDK") + 1:])
        cabinet = next(m["Cabinet"] for m in media
                       if int(m["LastSequence"]) >= int(row["Sequence"]))
        if not cabinet.startswith("#"):
            raise ValueError(f"Expected embedded SDK cabinet: {cabinet}")
        result.append((row["File"], relative, cabinet[1:], int(row["FileSize"])))
    if len(result) != 1525:
        raise ValueError(f"Unexpected SDK file count: {len(result)}")
    return result


def install_vc7(staging, scratch, destination):
    print("Extracting VC7 and restoring the MSI-embedded Platform SDK", flush=True)
    media_root = scratch / "vc7-media"
    members = [VS7_ROOT + f"Vc7/{d}/*" for d in ("bin", "include", "lib", "crt", "atlmfc")]
    members += [VS7_ROOT + "Common7/IDE/" + n for n in ("mspdb70.dll", "msobj10.dll")]
    members += ["Win/System/msvcr70.dll", "Win/System/msvcp70.dll", "vs_setup.msi"]
    for disc in range(1, 4):
        extract(staging / f"VSPROD{disc}.iso", media_root, *members)
    copy_tree(media_root / VS7_ROOT / "Vc7", destination)
    for name in ("mspdb70.dll", "msobj10.dll"):
        shutil.copy2(media_root / VS7_ROOT / "Common7/IDE" / name, destination / "bin" / name)
    for name in ("msvcr70.dll", "msvcp70.dll"):
        shutil.copy2(media_root / "Win/System" / name, destination / "bin" / name)
    msi = media_root / "vs_setup.msi"
    files = sdk_files(msi)
    cabinets = sorted({r[2] for r in files})
    extract(msi, scratch / "msi-streams", *cabinets, compound=True)
    for cabinet in cabinets:
        extract(scratch / "msi-streams" / cabinet, scratch / "sdk-cabs" / cabinet)
    for key, relative, cabinet, size in files:
        source = scratch / "sdk-cabs" / cabinet / key
        if source.stat().st_size != size:
            raise ValueError(f"SDK file size mismatch: {key}")
        target = destination / "platformsdk" / str(relative).lower()
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)


def install_d6_base(staging, scratch, destination):
    print("Extracting Delphi 6 compiler, RTL/VCL, and runtime reference packages", flush=True)
    media_root = scratch / "d6-media"
    extract(staging / DELPHI, media_root,
            "Borland Delphi 6.0 Enterprise Edition (ISO)/Borland Delphi 6.0.iso")
    iso = media_root / "Borland Delphi 6.0 Enterprise Edition (ISO)/Borland Delphi 6.0.iso"
    expanded = scratch / "d6"
    extract(iso, expanded, *[DELPHI_ROOT + d + "/*" for d in ("Bin", "Lib", "Source")],
            "Install/System32/*60.bpl", "Install/System32/*60.map")
    root = expanded / DELPHI_ROOT
    for name in ("Lib", "Source"):
        copy_tree(root / name, destination / name.lower())
    (destination / "bin").mkdir(parents=True, exist_ok=True)
    for name in ("DCC32.EXE", "brcc32.exe", "brc32.exe", "rlink32.dll", "make.exe",
                 "TDUMP.EXE", "convert.exe", "rw32core.dll"):
        shutil.copy2(root / "Bin" / name, destination / "bin" / name.lower())
    copy_tree(expanded / "Install/System32", destination / "runtime")


def extract_d6_updates(staging, scratch, names):
    metadata = json.loads((Path(__file__).resolve().parents[1] /
                           "toolchains/delphi-updates.json").read_text())
    hashes = {d["filename"]: d["sha256"] for d in metadata["downloads"]}
    result = {}
    for name in names:
        filename = D6_UPDATES[name]
        archive = staging / "delphi6-updates" / filename
        if digest(archive) != hashes[filename]:
            raise ValueError(f"Delphi update hash mismatch: {archive}")
        outer, payload = scratch / (name + "-outer"), scratch / (name + "-payload")
        extract(archive, outer)
        if name == "r3":
            result[name] = outer
        else:
            process = subprocess.run(["unshield", "-d", str(payload), "x",
                                      str(outer / "data1.cab")],
                                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     text=True)
            if process.returncode:
                raise RuntimeError(process.stdout)
            result[name] = payload
    return result


def apply_d6_update(name, payload, destination):
    if name == "r3":
        for source, target in (
                ("Classes.pas", "source/rtl/common/classes.pas"),
                ("Classes.dcu", "lib/classes.dcu"),
                ("debug/Classes.dcu", "lib/debug/classes.dcu"),
                ("rtl.dcp", "lib/rtl.dcp"),
                ("rtl60.bpl", "runtime/rtl60.bpl"),
                ("rtl60.map", "runtime/rtl60.map")):
            shutil.copy2(payload / source, destination / target)
        return
    # Select the English compiler/library payload, excluding localized copies
    # and IDE-only components. Normalize case before overlaying older files.
    program = payload / "Program_Built_Files"
    for source in program.rglob("*"):
        if not source.is_file():
            continue
        relative = Path(str(source.relative_to(program)).lower())
        if relative.parts[0] not in ("bin", "lib", "source"):
            continue
        target = destination / relative
        if relative.parts[0] == "bin" and not target.exists():
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    for source in (payload / "Windows_System_Files").iterdir():
        if source.suffix.lower() in (".bpl", ".map"):
            shutil.copy2(source, destination / "runtime" / source.name.lower())


def install_d6(staging, scratch, destination):
    base = scratch / "d6-base"
    install_d6_base(staging, scratch, base)
    payloads = extract_d6_updates(staging, scratch / "d6-updates", ("u1", "u2"))
    for name in ("u1", "u2"):
        target = destination / ("d6" + name)
        shutil.copytree(base, target)
        apply_d6_update(name, payloads[name], target)


def digest(path):
    with path.open("rb") as handle:
        return hashlib.file_digest(handle, "sha256").hexdigest()


def manifest(staging, destination):
    result = {"sources": {name: digest(staging / name) for name in REQUIRED}, "toolchains": {}}
    for name in ("vc6", "vc7", "d6u1", "d6u2"):
        root = destination / name
        files = sorted(p for p in root.rglob("*") if p.is_file())
        inventory = [(str(p.relative_to(root)), digest(p)) for p in files]
        tree = "".join(f"{sha}  {path}\n" for path, sha in inventory).encode()
        components = {}
        for filename in ("cl.exe", "c1.dll", "c1xx.dll", "c2.dll", "link.exe", "ml.exe",
                         "mspdb60.dll", "mspdb70.dll", "msobj10.dll", "rc.exe", "dcc32.exe",
                         "rlink32.dll", "brcc32.exe"):
            path = root / "bin" / filename
            if path.exists():
                components[filename] = {"sha256": digest(path), "versions": PE32(path).report()["versions"]}
        result["toolchains"][name] = dict(file_count=len(files),
            size_bytes=sum(p.stat().st_size for p in files),
            tree_sha256=hashlib.sha256(tree).hexdigest(), components=components)
        if name in ("d6u1", "d6u2"):
            for filename in ("rtl60.bpl", "vcl60.bpl"):
                path = root / "runtime" / filename
                components["runtime/" + filename] = {
                    "sha256": digest(path), "versions": PE32(path).report()["versions"]}
            result["toolchains"][name].update(
                configuration="Delphi 6 General Update " + name[-1],
                target="master/Plugin/" + ("debugger.dll" if name == "d6u2" else "dialogs.dll"),
                separate_rtl_updates=[])
    (destination / "manifest.json").write_text(json.dumps(result, indent=2) + "\n")


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--staging", type=Path, default=repo / "toolchains-staging")
    parser.add_argument("--dest", type=Path, default=repo / "toolchains")
    args = parser.parse_args()
    for program in ("7z", "msiinfo", "unshield"):
        if not shutil.which(program):
            parser.error(f"Missing program: {program}")
    for name in REQUIRED:
        if not (args.staging / name).is_file():
            parser.error(f"Missing media: {name}")
    for name in ("vc6", "vc7", "d6u1", "d6u2"):
        if (args.dest / name).exists():
            parser.error(f"Target already exists: {args.dest / name}")
    scratch_root = Path.home() / ".ai-tmp"
    scratch_root.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="nemu-unpack.", dir=scratch_root))
    print(f"Scratch: {scratch}", flush=True)
    args.dest.mkdir(parents=True, exist_ok=True)
    install_vc6(args.staging, scratch, args.dest / "vc6")
    install_vc7(args.staging, scratch, args.dest / "vc7")
    install_d6(args.staging, scratch, args.dest)
    manifest(args.staging, args.dest)
    print(f"Toolchains and manifest written to {args.dest}", flush=True)


if __name__ == "__main__":
    main()
