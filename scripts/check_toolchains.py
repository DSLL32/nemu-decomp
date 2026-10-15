#!/usr/bin/env python3
"""Compile/link probes for all three toolchains, including their build fingerprints.

All generated sources, outputs, logs, and (unless WINEPREFIX is supplied) the
Wine prefix live in a new ~/.ai-tmp directory. No target binaries are executed.
The generated VC7 console probe is executed to check its result.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from pe_fingerprints import PE32


ASM = """.386
.model flat, c
.code
add_three PROC arg:DWORD
 mov eax, arg
 add eax, 3
 ret
add_three ENDP
END
"""
VC6 = """#include <afxwin.h>
class ProbeApp : public CWinApp {};
ProbeApp app;
extern "C" int add_three(int);
extern "C" __declspec(dllexport) int probe(int n) {
    CString s("probe"); return add_three(n) + s.GetLength();
}
"""
VC7 = """#include <windows.h>
#include <stdio.h>
extern "C" int add_three(int);
int main() {
    printf("vc7 probe %d %lu\\n", add_three(39), GetCurrentProcessId());
    return add_three(39) != 42;
}
"""
DELPHI = """library probe;
uses Windows, SysUtils, Classes, Controls, Forms, StdCtrls;
{$R probe.res}
function ProbeValue: Integer; stdcall;
var b: TButton;
begin
  b := TButton.Create(nil);
  try
    b.Caption := IntToStr(TControl.InstanceSize);
    Result := Length(b.Caption);
  finally b.Free; end;
end;
exports ProbeValue;
begin
end.
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Also save the verification summary here")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    parent = Path.home() / ".ai-tmp"
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="nemu-check.", dir=parent))
    env = os.environ.copy()
    env.setdefault("WINEPREFIX", str(scratch / "wine"))
    env.setdefault("WINEARCH", "win32")
    env.setdefault("WINEDEBUG", "-all")
    print(f"Scratch: {scratch}\nWine prefix: {env['WINEPREFIX']}", flush=True)
    result = {}

    def run(work, log_name, command):
        # Wine services can inherit stdout handles. File logs avoid waiting for
        # an inherited pipe to close after the compiler itself has exited.
        log = work / (log_name + ".log")
        with log.open("w") as handle:
            process = subprocess.run(command, cwd=work, env=env, stdout=handle,
                                     stderr=subprocess.STDOUT, timeout=120)
        text = log.read_text(errors="replace")
        if process.returncode:
            raise RuntimeError(f"{log}: exit {process.returncode}\n{text}")
        return text

    for tc in ("vc6", "vc7", "d6"):
        work = scratch / tc
        work.mkdir()
        (work / "probe.rc").write_text('STRINGTABLE\nBEGIN\n  1 "Nemu toolchain probe"\nEND\n')

        def tool(name, *options):
            return run(work, name, [sys.executable, str(repo / "scripts/run_toolchain.py"),
                                    tc, name, *options])

        if tc in ("vc6", "vc7"):
            (work / "probe.asm").write_text(ASM)
            (work / "probe.cpp").write_text(VC6 if tc == "vc6" else VC7)
            tool("rc", "/fo", "probe.res", "probe.rc")
            tool("ml", "/nologo", "/c", "/coff", "/Foasm.obj", "probe.asm")
            if tc == "vc6":
                tool("cl", "/nologo", "/Od", "/Zi", "/MTd", "/GX", "/D_DEBUG", "/D_USRDLL",
                     "/LD", "probe.cpp", "asm.obj", "probe.res", "/link", "/out:probe.dll",
                     "/debug", "/incremental:yes")
                binary = work / "probe.dll"
                required = {"00312354", "000420ff"}
            else:
                tool("cl", "/nologo", "/O2", "/GL", "/MT", "/EHsc", "/Zi", "probe.cpp",
                     "asm.obj", "probe.res", "/link", "/out:probe.exe", "/LTCG", "/debug")
                binary = work / "probe.exe"
                required = {"002c24fa", "004024fa", "003d24fa", "004524fa"}
                output = run(work, "execution", ["wine", "probe.exe"])
                if "vc7 probe 42" not in output:
                    raise RuntimeError("VC7 console probe returned unexpected output")
            report = PE32(binary).report()
            ids = {r["comp_id"] for r in report["rich"]["records"]}
            if not report["rich"]["checksum_valid"] or not required <= ids:
                raise RuntimeError(f"Unexpected {tc} Rich fingerprint: {ids}")
            result[tc] = {"status": "passed", "required_rich_ids": sorted(required),
                          "observed_rich_records": report["rich"]["records"],
                          "pdb_generated": (work / "probe.pdb").is_file()}
            if not result[tc]["pdb_generated"]:
                raise RuntimeError(f"{tc} did not generate its linker PDB")
        else:
            (work / "probe.dpr").write_text(DELPHI)
            tool("brcc32", "probe.rc")
            tool("dcc32", "-Q", "-B", "-GD", "probe.dpr")
            report = PE32(work / "probe.dll").report()
            evidence = report["delphi"]["tcontrol"]
            if (evidence["instance_size"], evidence["preceding_dword"]) != (0x15c, 0x6f6f4207):
                raise RuntimeError(f"Unexpected Delphi VCL layout: {evidence}")
            if not any(e["name"] == "ProbeValue" for e in report["exports"]):
                raise RuntimeError("Delphi probe export missing")
            result[tc] = {"status": "passed", "tcontrol": evidence,
                          "map_generated": (work / "probe.map").is_file()}
            if not result[tc]["map_generated"]:
                raise RuntimeError("Delphi did not generate its detailed map")
        print(f"{tc}: passed", flush=True)
    text = json.dumps(result, indent=2) + "\n"
    (scratch / "results.json").write_text(text)
    if args.output:
        args.output.write_text(text)
    print("All compiler, resource, assembly, linking, and fingerprint checks passed.", flush=True)


if __name__ == "__main__":
    main()
