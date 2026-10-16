#!/usr/bin/env python3
"""Build the debugger reconstruction with Delphi 6 Personal Update 2.

All generated files and Wine prefixes belong under ~/.ai-tmp. Use --work for
repeated integration builds. Exit zero requires complete original-file equality.
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

from stamp_pe_resources import stamp_resource_times
from verify_debugger import compare, summarize


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--work', type=Path)
    parser.add_argument('--incremental', action='store_true')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    source = repo / 'src/debugger'
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-debugger-build.', dir=parent)) if not args.work else args.work.resolve().parent
    work = args.work.resolve() if args.work else scratch / 'build'
    if not work.is_relative_to(parent.resolve()):
        parser.error('--work must be under ~/.ai-tmp')
    work.mkdir(parents=True, exist_ok=True)
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

    def tool(label, *options):
        run(label, [sys.executable, str(repo / 'scripts/run_toolchain.py'),
                    'd6u2', 'dcc32-personal', *options])

    for folder in [source, source / 'components']:
        for path in folder.iterdir():
            if path.suffix in ('.pas', '.dpr', '.inc', '.rc', '.manifest'):
                destination = work / path.name
                if not destination.exists() or destination.read_bytes() != path.read_bytes():
                    shutil.copyfile(path, destination)
    if not (work / 'delphi_dfm.exe').exists():
        shutil.copyfile(repo / 'scripts/delphi_dfm.dpr', work / 'delphi_dfm.dpr')
        tool('dfm-compiler', '-Q', '-B', 'delphi_dfm.dpr')
    for path in source.glob('*.dfm'):
        text_name = path.stem + '.dfm.txt'
        destination = work / text_name
        if not destination.exists() or destination.read_bytes() != path.read_bytes() or not (work / path.name).exists():
            shutil.copyfile(path, destination)
            run('dfm-' + path.stem, ['wine', 'delphi_dfm.exe', 'resource', text_name, path.name])
    run('resources', [sys.executable, str(repo / 'scripts/run_toolchain.py'),
                      'vc7', 'rc', '/fo', 'XPManifest.res', 'XPManifest.rc'])
    tool('compiler-units', '-Q', *([] if args.incremental else ['-B']),
         '-GD', 'debugger.dpr')
    # The original link traversed cached unit interfaces. Delphi's clean-build
    # traversal yields different package/initialization ordering with cycles.
    tool('compiler', '-Q', '-GD', 'debugger.dpr')
    raw = work / 'debugger.unstamped.dll'
    shutil.copyfile(work / 'debugger.dll', raw)
    stamps = stamp_resource_times(work / 'debugger.dll', '2002-06-25T22:18:06')
    result = compare(work / 'debugger.dll', work / 'debugger.map')
    result['build'] = dict(compiler='toolchains/d6u2/bin/dcc32-personal.exe',
                          unstamped_sha256=hashlib.sha256(raw.read_bytes()).hexdigest(),
                          resource_directory_time='2002-06-25T22:18:06',
                          resource_timestamp_fields=len(stamps))
    report = json.dumps(result, indent=2) + '\n'
    (work / 'comparison.json').write_text(report)
    if args.report:
        args.report.write_text(report)
    summarize(result)
    print(f'DLL: {work / "debugger.dll"}\nReport: {work / "comparison.json"}')
    return 0 if result['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
