#!/usr/bin/env python3
"""Run a tool from vc6, vc6rtm, vc7, d6rtm, d6u1, or d6u2 with local search paths.

Example: python3 scripts/run_toolchain.py vc7 cl /c example.cpp
Set WINEPREFIX to reuse a prefix; otherwise a new one is allocated under
~/.ai-tmp and its location is printed. Arguments are passed without a shell.
"""

import argparse
import os
from pathlib import Path
import sys
import tempfile


def winpath(path):
    return "Z:" + str(path.resolve()).replace("/", "\\")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("toolchain", choices=("vc6", "vc6rtm", "vc7", "d6rtm", "d6u1", "d6u2"))
    parser.add_argument("tool", help="e.g. cl, link, ml, rc, dcc32, brcc32")
    parser.add_argument("args", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1] / "toolchains" / args.toolchain
    name = args.tool.lower()
    if not name.endswith(".exe"):
        name += ".exe"
    if Path(name).name != name or not (root / "bin" / name).is_file():
        parser.error(f"Tool not found: {root / 'bin' / name}")
    env = os.environ.copy()
    if not env.get("WINEPREFIX"):
        parent = Path.home() / ".ai-tmp"
        parent.mkdir(exist_ok=True)
        scratch = Path(tempfile.mkdtemp(prefix="nemu-wine.", dir=parent))
        env["WINEPREFIX"] = str(scratch / "wine")
        print(f"Wine prefix: {env['WINEPREFIX']}", file=sys.stderr, flush=True)
    env.setdefault("WINEARCH", "win32")
    env.setdefault("WINEDEBUG", "-all")
    env["WINEPATH"] = winpath(root / "bin")
    if args.toolchain == "vc6":
        include, lib = ("include", "mfc/include", "atl/include"), ("lib", "mfc/lib")
    elif args.toolchain == "vc6rtm":
        include, lib = ("include",), ()
    elif args.toolchain == "vc7":
        include = ("include", "atlmfc/include", "platformsdk/include")
        lib = ("lib", "atlmfc/lib", "platformsdk/lib")
    else:
        include, lib = (), ()
        env["WINEPATH"] += ";" + winpath(root / "runtime")
        if name in ("dcc32.exe", "dcc32-personal.exe"):
            args.args = ["-U" + winpath(root / "lib"), "-I" + winpath(root / "lib"),
                         "-R" + winpath(root / "lib"), *args.args]
    if include:
        env["INCLUDE"] = ";".join(winpath(root / p) for p in include)
        env["LIB"] = ";".join(winpath(root / p) for p in lib)
    os.execvpe("wine", ["wine", str(root / "bin" / name), *args.args], env)


if __name__ == "__main__":
    main()
