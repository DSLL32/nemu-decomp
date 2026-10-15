#!/usr/bin/env python3
"""Build the incomplete dialogs reconstruction with Delphi 6 Update 1.

Uses the explicitly provisional Theme Manager 1.10.1 sources, whose version
does not match the target. Outputs and Wine prefixes live under ~/.ai-tmp.
Build success is not a claim of byte equality; verify_dialogs.py is the strict
byte-exact check. Requires Wine, tracked d6u1 and vc7 tools, python3-capstone.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from verify_dialogs import compare, summarize


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, help='Also write the comparison report here')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    source = repo / 'src/dialogs'
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-dialogs-build.', dir=parent))
    work = scratch / 'build'
    work.mkdir()
    env = os.environ.copy()
    env.setdefault('WINEPREFIX', str(scratch / 'wine'))
    env.setdefault('WINEARCH', 'win32')
    env.setdefault('WINEDEBUG', '-all')
    print(f'Build directory: {work}\nWine prefix: {env["WINEPREFIX"]}', flush=True)
    print('Provisional dependency: Theme Manager 1.10.1; full DLL matching remains incomplete.', flush=True)

    def run(label, command):
        log = work / (label + '.log')
        with log.open('w') as output:
            process = subprocess.run(command, cwd=work, env=env, stdout=output,
                                     stderr=subprocess.STDOUT, timeout=120)
        if process.returncode:
            raise RuntimeError(f'{log}: exit {process.returncode}\n{log.read_text(errors="replace")}')

    def tool(label, name, *options):
        run(label, [sys.executable, str(repo / 'scripts/run_toolchain.py'),
                    'd6u1', name, *options])

    for path in source.iterdir():
        if path.suffix in ('.pas', '.dpr', '.rc', '.manifest'):
            shutil.copyfile(path, work / path.name)
    for path in (source / 'vendor/theme-manager-1.10.1').iterdir():
        if path.suffix in ('.pas', '.inc'):
            shutil.copyfile(path, work / path.name)
    shutil.copyfile(repo / 'scripts/delphi_dfm.dpr', work / 'delphi_dfm.dpr')
    tool('dfm-compiler', 'dcc32', '-Q', '-B', 'delphi_dfm.dpr')
    for path in source.glob('*.dfm'):
        text_name = path.stem + '.dfm.txt'
        shutil.copyfile(path, work / text_name)
        run('dfm-' + path.stem, ['wine', 'delphi_dfm.exe', 'resource', text_name, path.name])
    # The old Borland resource compiler silently omits numeric type 24 here.
    # Compile the recovered manifest with the available XP-era SDK RC instead.
    run('resources', [sys.executable, str(repo / 'scripts/run_toolchain.py'),
                      'vc7', 'rc', '/fo', 'XPManifest.res', 'XPManifest.rc'])
    tool('compiler', 'dcc32', '-Q', '-B', '-GD', 'dialogs.dpr')
    result = compare(work / 'dialogs.dll', work / 'dialogs.map')
    report = json.dumps(result, indent=2) + '\n'
    (work / 'comparison.json').write_text(report)
    if args.report:
        args.report.write_text(report)
    summarize(result)
    print(f'DLL: {work / "dialogs.dll"}\nReport: {work / "comparison.json"}')


if __name__ == '__main__':
    main()
