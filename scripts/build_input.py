#!/usr/bin/env python3
"""Build the input reconstruction with Delphi 6 Personal Update 2.

All generated files and Wine prefixes belong under ~/.ai-tmp. Use --work for
repeated integration builds. Exit zero requires complete original-file equality.
"""

import argparse
import hashlib
import json
import os
import re
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
    parser.add_argument('--work', type=Path,
                        help='reuse a build directory under ~/.ai-tmp/<scratch>/')
    parser.add_argument('--incremental', action='store_true')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    source = repo / 'src/input'
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-input-build.', dir=parent)) if not args.work else args.work.resolve().parent
    work = args.work.resolve() if args.work else scratch / 'build'
    if not scratch.is_relative_to(parent.resolve()) or scratch == parent.resolve():
        parser.error('--work must be under ~/.ai-tmp/<scratch>/')
    work.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    print(f'Build directory: {work}\nWine prefix: {env["WINEPREFIX"]}', flush=True)

    invocations = []

    def run(label, command):
        invocations.append(dict(label=label, argv=command))
        log = work / (label + '.log')
        with log.open('w') as output:
            process = subprocess.run(command, cwd=work, env=env, stdout=output,
                                     stderr=subprocess.STDOUT, timeout=120)
        if process.returncode:
            raise RuntimeError(f'{log}: exit {process.returncode}\n{log.read_text(errors="replace")}')

    def tool(label, *options):
        run(label, [sys.executable, str(repo / 'scripts/run_toolchain.py'),
                    'd6u2', 'dcc32-personal', *options])

    staged_sources = {}
    for folder in [source, source / 'components']:
        if not folder.exists():
            continue
        for path in folder.iterdir():
            if path.suffix in ('.pas', '.dpr', '.inc', '.rc', '.manifest'):
                destination = work / path.name
                if not destination.exists() or destination.read_bytes() != path.read_bytes():
                    shutil.copyfile(path, destination)
                staged_sources[str(path.relative_to(source))] = destination
    if not (work / 'delphi_dfm.exe').exists():
        shutil.copyfile(repo / 'scripts/delphi_dfm.dpr', work / 'delphi_dfm.dpr')
        tool('dfm-compiler', '-Q', '-B', 'delphi_dfm.dpr')
    for path in source.glob('*.dfm'):
        text_name = path.stem + '.dfm.txt'
        destination = work / text_name
        if not destination.exists() or destination.read_bytes() != path.read_bytes() or not (work / path.name).exists():
            shutil.copyfile(path, destination)
            run('dfm-' + path.stem, ['wine', 'delphi_dfm.exe', 'resource', text_name, path.name])
        staged_sources[path.name] = destination
    tool('compiler-units', '-Q', *([] if args.incremental else ['-B']),
         '-GD', 'Input.dpr')
    # Refresh cyclic unit interfaces on disk before the final cached link.
    # A plain link may rebuild stale units in memory without updating their DCUs.
    tool('compiler-make-1', '-Q', '-M', '-GD', 'Input.dpr')
    tool('compiler-make-2', '-Q', '-M', '-GD', 'Input.dpr')
    tool('compiler', '-Q', '-GD', 'Input.dpr')
    raw = work / 'input.unstamped.dll'
    shutil.copyfile(work / 'Input.dll', raw)
    stamps = stamp_resource_times(work / 'Input.dll', '2002-07-26T23:26:24')
    result = compare(work / 'Input.dll', work / 'Input.map')
    compiler = repo / 'toolchains/d6u2/bin/dcc32-personal.exe'
    used_units = re.findall(r'C=CODE.*?M=(\w+)', (work / 'Input.map').read_text())
    local_units = {Path(name).stem.lower() for name in staged_sources}
    library_units = {name: repo / 'toolchains/d6u2/lib' / (name.lower() + '.dcu')
                     for name in used_units if name.lower() not in local_units}
    source_hashes = {name: hashlib.sha256(path.read_bytes()).hexdigest()
                     for name, path in sorted(staged_sources.items())}
    tool_hashes = dict(
        compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),
        library_unit_sha256={name: hashlib.sha256(path.read_bytes()).hexdigest()
            for name, path in sorted(library_units.items())},
        build_helper_sha256={name: hashlib.sha256((repo / 'scripts' / name).read_bytes()).hexdigest()
            for name in ['build_input.py', 'run_toolchain.py', 'delphi_dfm.dpr',
                         'stamp_pe_resources.py', 'verify_input.py']})
    unstamped_sha256 = hashlib.sha256(raw.read_bytes()).hexdigest()
    artifact_id = hashlib.sha256((result['rebuilt_sha256'] + unstamped_sha256 + '\0' +
        json.dumps(dict(sources=source_hashes, tools=tool_hashes,
                        clean_recompile=not args.incremental,
                        commands=invocations), sort_keys=True)).encode()).hexdigest()
    artifact = scratch / 'artifacts' / artifact_id
    result['build'] = dict(compiler='toolchains/d6u2/bin/dcc32-personal.exe',
                          unstamped_sha256=unstamped_sha256,
                          resource_directory_time='2002-07-26T23:26:24',
                          resource_timestamp_fields=len(stamps),
                          resource_timestamp_file_offsets=stamps,
                          source_sha256=source_hashes,
                          artifact=str(artifact),
                          clean_recompile=not args.incremental,
                          working_directory=str(work),
                          wine_prefix=env['WINEPREFIX'],
                          commands=invocations, **tool_hashes)
    report = json.dumps(result, indent=2) + '\n'
    (work / 'comparison.json').write_text(report)
    if not artifact.exists():
        artifact.mkdir(parents=True)
        for name, path in staged_sources.items():
            destination = artifact / 'source-snapshot' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, destination)
        shutil.copytree(work, artifact / 'build-tree')
        for name in ['Input.dll', 'Input.map', 'input.unstamped.dll', 'comparison.json']:
            shutil.copyfile(work / name, artifact / name)
    if args.report:
        args.report.write_text(report)
    summarize(result)
    print(f'DLL: {artifact / "Input.dll"}\nReport: {artifact / "comparison.json"}')
    return 0 if result['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
