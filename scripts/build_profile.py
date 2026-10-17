#!/usr/bin/env python3
"""Build Profile using unmodified Delphi 6 RTM units and U1 Personal DCC32.

Every build and Wine prefix belongs under a new ~/.ai-tmp directory. Exit zero
requires complete original-file equality. Only resource-directory clocks are
restored, and the unstamped compiler output is retained alongside the result.
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
from verify_profile import compare, summarize


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--work', type=Path,
                        help='reuse a build directory under ~/.ai-tmp/<scratch>/')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    source, toolchain = repo / 'src/profile', repo / 'toolchains/d6rtm'
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = (Path(tempfile.mkdtemp(prefix='nemu-profile-build.', dir=parent))
               if not args.work else args.work.resolve().parent)
    work = args.work.resolve() if args.work else scratch / 'build'
    if not scratch.is_relative_to(parent.resolve()) or scratch == parent.resolve():
        parser.error('--work must be under ~/.ai-tmp/<scratch>/')
    work.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    print(f'Build directory: {work}\nWine prefix: {env["WINEPREFIX"]}', flush=True)

    commands = []

    def run(label, argv):
        commands.append(dict(label=label, argv=argv))
        log = work / (label + '.log')
        with log.open('w') as output:
            process = subprocess.run(argv, cwd=work, env=env, stdout=output,
                                     stderr=subprocess.STDOUT, timeout=120)
        if process.returncode:
            raise RuntimeError(f'{log}: exit {process.returncode}\n'
                               f'{log.read_text(errors="replace")}')

    def tool(label, *options):
        run(label, [sys.executable, str(repo / 'scripts/run_toolchain.py'),
                    'd6rtm', 'dcc32-personal', *options])

    staged_sources = {}
    for path in sorted(source.iterdir()):
        if path.suffix not in ('.pas', '.dpr', '.dfm'):
            continue
        destination = work / (path.name + '.txt' if path.suffix == '.dfm' else path.name)
        if not destination.exists() or destination.read_bytes() != path.read_bytes():
            shutil.copyfile(path, destination)
        staged_sources[path.name] = destination
    # Always recompile the converter and all application units. The subsequent
    # make and cached link produce the original natural dependency traversal.
    shutil.copyfile(repo / 'scripts/delphi_dfm.dpr', work / 'delphi_dfm.dpr')
    tool('dfm-compiler', '-Q', '-B', 'delphi_dfm.dpr')
    for path in source.glob('*.dfm'):
        run('dfm-' + path.stem, ['wine', 'delphi_dfm.exe', 'resource',
                                path.name + '.txt', path.name])
    tool('compiler-units', '-Q', '-B', '-GD', 'profile.dpr')
    tool('compiler-make', '-Q', '-M', '-GD', 'profile.dpr')
    tool('compiler', '-Q', '-GD', 'profile.dpr')
    raw = work / 'profile.unstamped.dll'
    shutil.copyfile(work / 'profile.dll', raw)
    clock = '2001-09-24T21:04:20'
    stamps = stamp_resource_times(work / 'profile.dll', clock)
    result = compare(work / 'profile.dll')
    helpers = ['build_profile.py', 'run_toolchain.py', 'delphi_dfm.dpr',
               'stamp_pe_resources.py', 'verify_profile.py',
               'pe_fingerprints.py', 'inspect_delphi_forms.py']
    source_hashes = {name: sha256(path) for name, path in staged_sources.items()}
    tool_hashes = {str(p.relative_to(toolchain)): sha256(p)
                   for p in sorted(toolchain.rglob('*')) if p.is_file()}
    helper_hashes = {name: sha256(repo / 'scripts' / name) for name in helpers}
    unstamped_sha256 = sha256(raw)
    artifact_id = hashlib.sha256(json.dumps(dict(
        result=result, sources=source_hashes, tools=tool_hashes,
        helpers=helper_hashes, unstamped_sha256=unstamped_sha256,
        commands=commands), sort_keys=True).encode()).hexdigest()
    artifact = scratch / 'artifacts' / artifact_id
    result['build'] = dict(
        compiler='toolchains/d6rtm/bin/dcc32-personal.exe',
        configuration='Delphi 6 RTM RTL/VCL with General Update 1 Personal compiler',
        original_compiler_identified=False,
        unstamped_sha256=unstamped_sha256,
        resource_directory_time=clock, resource_timestamp_fields=len(stamps),
        resource_timestamp_file_offsets=stamps, source_sha256=source_hashes,
        toolchain_sha256=tool_hashes, build_helper_sha256=helper_hashes,
        artifact=str(artifact), clean_recompile=True,
        working_directory=str(work), wine_prefix=env['WINEPREFIX'], commands=commands)
    report = json.dumps(result, indent=2) + '\n'
    (work / 'comparison.json').write_text(report)
    if not artifact.exists():
        artifact.mkdir(parents=True)
        for name, path in staged_sources.items():
            destination = artifact / 'source-snapshot' / name
            destination.parent.mkdir(exist_ok=True)
            shutil.copyfile(path, destination)
        for name in helpers:
            destination = artifact / 'build-helpers' / name
            destination.parent.mkdir(exist_ok=True)
            shutil.copyfile(repo / 'scripts' / name, destination)
        shutil.copytree(work, artifact / 'build-tree')
        for name in ['profile.dll', 'profile.map', 'profile.unstamped.dll', 'comparison.json']:
            shutil.copyfile(work / name, artifact / name)
    if args.report:
        args.report.write_text(report)
    summarize(result)
    print(f'DLL: {artifact / "profile.dll"}\nReport: {artifact / "comparison.json"}')
    return 0 if result['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
