#!/usr/bin/env python3
"""Build the input reconstruction with Delphi 6 Personal Update 2.

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
from verify_input import compare, summarize


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--work', type=Path)
    parser.add_argument('--incremental', action='store_true')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    source = repo / 'src/input'
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-input-build.', dir=parent)) if not args.work else args.work.resolve().parent
    work = args.work.resolve() if args.work else scratch / 'build'
    if not work.is_relative_to(parent.resolve()):
        parser.error('--work must be under ~/.ai-tmp')
    work.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
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
        if not folder.exists():
            continue
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
    tool('compiler-units', '-Q', *([] if args.incremental else ['-B']),
         '-GD', 'Input.dpr')
    # The original link traversed cached unit interfaces. Delphi's clean-build
    # traversal yields different package/initialization ordering with cycles.
    tool('compiler', '-Q', '-GD', 'Input.dpr')
    raw = work / 'input.unstamped.dll'
    shutil.copyfile(work / 'Input.dll', raw)
    stamps = stamp_resource_times(work / 'Input.dll', '2002-07-26T23:26:24')
    result = compare(work / 'Input.dll', work / 'Input.map')
    result['build'] = dict(compiler='toolchains/d6u2/bin/dcc32-personal.exe',
                          unstamped_sha256=hashlib.sha256(raw.read_bytes()).hexdigest(),
                          resource_directory_time='2002-07-26T23:26:24',
                          resource_timestamp_fields=len(stamps),
                          source_sha256={str(p.relative_to(source)): hashlib.sha256(p.read_bytes()).hexdigest()
                              for p in sorted(source.rglob('*'))
                              if p.suffix in ('.pas', '.dpr', '.dfm', '.inc')})
    report = json.dumps(result, indent=2) + '\n'
    (work / 'comparison.json').write_text(report)
    if args.report:
        args.report.write_text(report)
    summarize(result)
    print(f'DLL: {work / "Input.dll"}\nReport: {work / "comparison.json"}')
    return 0 if result['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
