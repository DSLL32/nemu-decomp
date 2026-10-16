#!/usr/bin/env python3
"""Build the dialogs reconstruction with Delphi 6 Personal Update 1.

Uses the reconstructed Theme Manager sources under src/dialogs/components.
Outputs and Wine prefixes live under ~/.ai-tmp.
Build success is not a claim of byte equality; verify_dialogs.py is the strict
byte-exact check. Requires Wine, tracked d6u1 and vc7 tools, python3-capstone.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from verify_dialogs import compare, summarize
from stamp_pe_resources import stamp_resource_times


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
    for path in (source / 'components').iterdir():
        if path.suffix in ('.pas', '.inc'):
            shutil.copyfile(path, work / path.name)
    shutil.copyfile(repo / 'scripts/delphi_dfm.dpr', work / 'delphi_dfm.dpr')
    tool('dfm-compiler', 'dcc32-personal', '-Q', '-B', 'delphi_dfm.dpr')
    for path in source.glob('*.dfm'):
        text_name = path.stem + '.dfm.txt'
        shutil.copyfile(path, work / text_name)
        run('dfm-' + path.stem, ['wine', 'delphi_dfm.exe', 'resource', text_name, path.name])
    # The old Borland resource compiler silently omits numeric type 24 here.
    # Compile the recovered manifest with the available XP-era SDK RC instead.
    run('resources', [sys.executable, str(repo / 'scripts/run_toolchain.py'),
                      'vc7', 'rc', '/fo', 'XPManifest.res', 'XPManifest.rc'])
    tool('compiler', 'dcc32-personal', '-Q', '-B', '-GD', 'dialogs.dpr')
    # Recompile UnitHeader with the other DCUs cached. Delphi orders unit
    # initialization and PACKAGEINFO differently for source and cached units;
    # this reproduces both original tables without changing the linked bytes.
    (work / "UnitHeader.dcu").unlink()
    tool('linker', 'dcc32-personal', '-Q', '-GD', 'dialogs.dpr')
    # Preserve the untouched compiler output. Only resource-directory clock
    # fields are normalized; code, data, resource payloads and layout are not.
    raw = work / 'dialogs.unstamped.dll'
    shutil.copyfile(work / 'dialogs.dll', raw)
    stamps = stamp_resource_times(work / 'dialogs.dll', '2002-04-02T21:52:38')
    result = compare(work / 'dialogs.dll', work / 'dialogs.map')
    result['build'] = dict(
        compiler='toolchains/d6u1/bin/dcc32-personal.exe',
        unstamped_sha256=hashlib.sha256(raw.read_bytes()).hexdigest(),
        resource_directory_time='2002-04-02T21:52:38',
        resource_timestamp_fields=len(stamps),
        note='Only resource-directory timestamps are normalized after linking. '
             'The untouched compiler output is saved as dialogs.unstamped.dll.')
    report = json.dumps(result, indent=2) + '\n'
    (work / 'comparison.json').write_text(report)
    if args.report:
        args.report.write_text(report)
    summarize(result)
    print(f'DLL: {work / "dialogs.dll"}\nReport: {work / "comparison.json"}')
    return 0 if result['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
