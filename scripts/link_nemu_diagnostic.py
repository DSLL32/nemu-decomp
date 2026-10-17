#!/usr/bin/env python3
"""Relink a successful Nemu64 build with relocation records for comparison.

This copies the complete build tree into a new ~/.ai-tmp directory and changes
only /FIXED to /FIXED:NO. It never forces unresolved symbols or runs the image.
Every existing section must retain the fixed build's exact layout and bytes.
The extra relocation directory makes instruction-operand comparison possible;
this diagnostic is separate from the executable being reconstructed.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
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
    for source in build['source_files']:
        data = (tree / source['path']).read_bytes()
        if hashlib.sha256(data).hexdigest() != source['sha256']:
            raise ValueError(f'Build source snapshot changed: {source["path"]}')
    command = list(build['commands'][-1]['command'])
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
    env = dict(os.environ, WINEPREFIX=build['wine_prefix'], WINEARCH='win32', WINEDEBUG='-all')
    print(f'Diagnostic snapshot: {scratch}', flush=True)
    with (scratch / 'link.log').open('w') as log:
        process = subprocess.run(command, cwd=project, env=env, stdout=log,
                                 stderr=subprocess.STDOUT, timeout=600)
    report = dict(kind='complete_relocation_diagnostic', parent_artifact=str(artifact),
                  parent_build=build, command=command, link_returncode=process.returncode,
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
        report.update(existing_sections=rows, relocation_directory=diagnostic.directory(5),
                      sha256=hashlib.sha256(diagnostic.data).hexdigest(),
                      status='verified' if all(r['layout_equal'] and r['raw_bytes_equal']
                                               for r in rows) else 'section_mismatch')
    (scratch / 'diagnostic.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Existing sections identical: {report["status"] == "verified"}', flush=True)
    return 0 if report['status'] == 'verified' else 1


if __name__ == '__main__':
    raise SystemExit(main())
