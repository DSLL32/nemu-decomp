#!/usr/bin/env python3
"""Compile recovered Kaillera components and report remaining reconstruction.

The application is not fully reconstructed. This command produces a component
library and resources, not a replacement DLL, and exits nonzero. Instruction
coverage is diagnostic; verify_kaillera.py alone accepts full packed equality.
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

from check_kaillera import inspect
from verify_kaillera import ORIGINAL, ORIGINAL_SHA256

ROOT = Path(__file__).resolve().parents[1]
UNITS = ['reliableconnection', 'config', 'message', 'udpconnection',
         'jnetlib/asyncdns', 'jnetlib/connection', 'jnetlib/httpget', 'jnetlib/util',
         'connect', 'kailleraclient']
FLAGS = ['/nologo', '/c', '/O2', '/MD', '/GX', '/Gy', '/DWIN32', '/DNDEBUG']


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def winpath(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    parent = (Path.home() / '.ai-tmp').resolve()
    parent.mkdir(exist_ok=True)
    scratch = args.work.resolve() if args.work else Path(tempfile.mkdtemp(prefix='kaillera-build.', dir=parent))
    if not scratch.is_relative_to(parent) or scratch == parent:
        parser.error('--work must be a subdirectory of ~/.ai-tmp')
    source, output, logs = [scratch / name for name in ('source', 'output', 'logs')]
    if source.exists():
        shutil.rmtree(source)
    shutil.copytree(ROOT / 'src/kailleraclient', source)
    for directory in (output, logs):
        directory.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    report = dict(status='building', work=str(scratch), wine_prefix=env['WINEPREFIX'],
                  full_dll_byte_exact=False, sources={}, commands=[], compiler_flags=FLAGS,
                  executable_bytes_copied_from_reference=False, metadata_restoration=None)
    print(f'Build directory: {scratch}\nWine prefix: {env["WINEPREFIX"]}', flush=True)

    def save():
        data = json.dumps(report, indent=2) + '\n'
        (scratch / 'comparison.json').write_text(data)
        if args.report:
            args.report.write_text(data)

    def command(label, argv):
        log = logs / (label + '.log')
        row = dict(label=label, argv=argv, log=str(log))
        report['commands'].append(row)
        with log.open('w') as stream:
            result = subprocess.run(argv, cwd=source, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=240)
        row['returncode'] = result.returncode
        if result.returncode:
            raise RuntimeError(f'{label}: exit {result.returncode}; {log}\n' + log.read_text(errors='replace'))

    def tool(label, program, options):
        command(label, [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                        'vc6rtm', program, *options])

    try:
        report['sources'] = {str(path.relative_to(source)): digest(path)
                             for path in sorted(source.rglob('*')) if path.is_file()}
        tracked = [ROOT / 'scripts' / name for name in
                   ('build_kaillera.py', 'check_kaillera.py', 'verify_kaillera.py', 'run_toolchain.py')]
        tracked += [ROOT / 'toolchains' / name for name in
                    ('kaillera-toolchain.json', 'kaillera-packer.json')]
        tracked += [ROOT / 'toolchains/vc6rtm/bin' / name for name in
                    ('cl.exe', 'c1xx.dll', 'c2.dll', 'lib.exe', 'rc.exe', 'rcdll.dll')]
        tracked += [ROOT / 'toolchains/upx120/upx.exe']
        report['build_input_sha256'] = {str(path.relative_to(ROOT)): digest(path) for path in tracked}
        objects = []
        for unit in UNITS:
            obj = output / (unit.replace('/', '-') + '.obj')
            tool('compile-' + unit.replace('/', '-'), 'cl', [*FLAGS, '/Fo' + winpath(obj), unit + '.cpp'])
            objects.append(obj)
        tool('resources', 'rc', ['/l', '0x40c', '/fo' + winpath(output / 'kaillera.res'), 'kaillera.rc'])
        tool('component-library', 'lib', ['/nologo', '/out:' + winpath(output / 'kaillera-components.lib'),
                                         *map(winpath, objects)])
        # The reference is consumed only by the following diagnostic phase.
        if digest(ORIGINAL) != ORIGINAL_SHA256:
            raise ValueError('Original kailleraclient.dll hash changed')
        reference = output / 'reference.unpacked.dll'
        if reference.exists():
            reference.unlink()
        command('unpack-reference-for-diagnostics', ['wine', str(ROOT / 'toolchains/upx120/upx.exe'),
            '-d', '-o' + winpath(reference), winpath(ORIGINAL)])
        report.update(inspect(reference, objects, output / 'kaillera.res'))
        report.update(status='incomplete-reconstruction',
                      remaining=['server browser and lobby', 'game synchronization and protocol dispatch',
                                 'window resizing and shared UI helpers',
                                 'remaining component instruction differences',
                                 'final link layout and UPX container reproduction'])
        save()
        print(f'Component library: {output / "kaillera-components.lib"}\n'
              f'Resources exact: {report["all_resource_payloads_byte_exact"]}\n'
              f'Diagnostic unique masked-signature coverage: '
              f'{report["unique_masked_signature_code_bytes"]}/{report["original_code_virtual_bytes"]} bytes\n'
              'Complete DLL byte exact: False (reconstruction incomplete)', flush=True)
        return 1
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        report.update(status='build-failed', error=str(error))
        save()
        print(error, file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
