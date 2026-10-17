#!/usr/bin/env python3
"""Relink a successful Nemu64 build with relocation records for comparison.

This copies the complete build tree into a new ~/.ai-tmp directory and changes
only /FIXED to /FIXED:NO. It never forces unresolved symbols or runs the image.
Every existing section must retain the fixed build's exact layout and bytes.
The extra relocation directory makes instruction-operand comparison possible;
this diagnostic is separate from the executable being reconstructed.
The recorded linker environment is replayed in a fresh Wine prefix, after its
bootstrap server exits. A recorded linker clock must reproduce its timestamp.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

from pe_fingerprints import PE32


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build_report', type=Path, help='successful build.json')
    args = parser.parse_args()
    build = json.loads(args.build_report.read_text())
    if build['status'] not in ('different', 'byte_exact'):
        parser.error('the input must be a successful, unforced build')
    artifact = Path(build['immutable_artifact_dir'])
    tree = artifact / 'build-tree'
    if not tree.is_dir():
        tree = Path(build['project'])
    source_files = build.get('compiler_source_files', build['source_files'])
    for source in source_files:
        data = (tree / source['path']).read_bytes()
        if hashlib.sha256(data).hexdigest() != source['sha256']:
            raise ValueError(f'Build source snapshot changed: {source["path"]}')
    link_step = build['commands'][-1]
    command = list(link_step['command'])
    recorded_environment = link_step.get('environment', {})
    if not isinstance(recorded_environment, dict) or not all(
            isinstance(key, str) and isinstance(value, str)
            for key, value in recorded_environment.items()):
        parser.error('the recorded linker environment must contain string values')
    clock = build.get('link_clock', {})
    for library in clock.get('libraries', []):
        if hashlib.sha256(Path(library['path']).read_bytes()).hexdigest() != library['sha256']:
            raise ValueError(f'Link clock library changed: {library["path"]}')
    if clock.get('timestamp') is not None and recorded_environment != clock.get('environment'):
        parser.error('the linker step does not contain the recorded clock environment')
    if any('/FORCE' in argument.upper() for argument in command):
        parser.error('forced links are not supported')
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-complete-relocs.', dir=parent))
    project = scratch / 'src/nemu64'
    shutil.copytree(tree, project)
    for i, argument in enumerate(command):
        if argument.upper().startswith('/OUT:'):
            command[i] = '/OUT:Z:' + str(scratch / 'nemu64.exe').replace('/', '\\')
        elif argument.upper().startswith('/MAP:'):
            command[i] = '/MAP:Z:' + str(scratch / 'nemu64.map').replace('/', '\\')
        elif argument.upper() == '/FIXED':
            command[i] = '/FIXED:NO'
    if '/FIXED:NO' not in command:
        command.append('/FIXED:NO')
    if not any(argument.upper().startswith('/MAP:') for argument in command):
        command.append('/MAP:Z:' + str(scratch / 'nemu64.map').replace('/', '\\'))
    wine_environment = dict(WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    env = dict(os.environ, **wine_environment)
    print(f'Diagnostic snapshot: {scratch}', flush=True)
    bootstrap = []
    with (scratch / 'wine-init.log').open('w') as log:
        process = subprocess.run(['wineboot', '-u'], env=env, stdout=log,
                                 stderr=subprocess.STDOUT, timeout=120)
    bootstrap.append(dict(command=['wineboot', '-u'], returncode=process.returncode))
    if process.returncode:
        raise RuntimeError(f'Wine prefix initialization failed; see {scratch / "wine-init.log"}')
    # A pre-existing server retains its own clock. Stop only this new prefix's
    # bootstrap server before the environment-controlled linker starts one.
    process = subprocess.run(['wineserver', '-k'], env=env, capture_output=True)
    bootstrap.append(dict(command=['wineserver', '-k'], returncode=process.returncode))
    process = subprocess.run(['wineserver', '-w'], env=env, capture_output=True, timeout=30)
    bootstrap.append(dict(command=['wineserver', '-w'], returncode=process.returncode))
    process.check_returncode()
    with (scratch / 'link.log').open('w') as log:
        process = subprocess.run(command, cwd=project, env=dict(env, **recorded_environment), stdout=log,
                                 stderr=subprocess.STDOUT, timeout=600)
    report = dict(kind='complete_relocation_diagnostic', parent_artifact=str(artifact),
                  parent_build=build, command=command, link_returncode=process.returncode,
                  parent_build_report_sha256=hashlib.sha256(args.build_report.read_bytes()).hexdigest(),
                  environment=recorded_environment, wine_environment=wine_environment,
                  bootstrap_commands=bootstrap, clock_libraries=clock.get('libraries', []),
                  source_files_verified=source_files,
                  replay_script=str(Path(__file__).resolve()),
                  replay_script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  never_forced=True, status='link_failed')
    if process.returncode == 0:
        original = PE32(artifact / 'nemu64.exe')
        diagnostic = PE32(scratch / 'nemu64.exe')
        rows = []
        for section in original.sections:
            other = next((s for s in diagnostic.sections if s['name'] == section['name']), None)
            a = original.data[section['offset']:section['offset'] + section['raw_size']]
            b = (diagnostic.data[other['offset']:other['offset'] + other['raw_size']]
                 if other else b'')
            rows.append(dict(name=section['name'], layout_equal=section == other,
                             raw_bytes_equal=other is not None and a == b))
        timestamp = struct.unpack_from('<I', diagnostic.data, diagnostic.pe + 8)[0]
        expected_timestamp = clock.get('timestamp')
        clock_matches = expected_timestamp is None or timestamp == expected_timestamp
        sections_match = all(r['layout_equal'] and r['raw_bytes_equal'] for r in rows)
        report.update(existing_sections=rows, relocation_directory=diagnostic.directory(5),
                      sha256=hashlib.sha256(diagnostic.data).hexdigest(),
                      observed_timestamp=timestamp, expected_timestamp=expected_timestamp,
                      timestamp_matches_recorded_clock=(clock_matches if expected_timestamp is not None else None),
                      status=('verified' if clock_matches and sections_match else
                              'clock_mismatch' if not clock_matches else 'section_mismatch'))
    (scratch / 'diagnostic.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Existing sections identical: {report["status"] == "verified"}', flush=True)
    return 0 if report['status'] == 'verified' else 1


if __name__ == '__main__':
    raise SystemExit(main())
