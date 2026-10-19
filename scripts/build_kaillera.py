#!/usr/bin/env python3
"""Build the recovered Kaillera candidate and report remaining differences.

The candidate is not yet byte exact. Instruction coverage is diagnostic;
verify_kaillera.py alone accepts full packed equality.
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
from build_kaillera_crt_inputs import build as build_crt_inputs, NAMED_DEF, ORDINAL_DEF
from build_kaillera_import_inputs import build as build_import_inputs, check_inventory
from build_kaillera_layout_inputs import build as build_layout_inputs
from verify_kaillera import ORIGINAL, ORIGINAL_SHA256, compare

ROOT = Path(__file__).resolve().parents[1]
UNITS = ['reliableconnection', 'config', 'message', 'udpconnection',
         'jnetlib/asyncdns', 'jnetlib/connection', 'jnetlib/httpget', 'jnetlib/util',
         'listview', 'resizer', 'connect', 'kailleraclient', 'session', 'serverbrowser']
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
                   ('build_kaillera.py', 'build_kaillera_link_inputs.py',
                    'build_kaillera_import_inputs.py', 'build_kaillera_crt_inputs.py',
                    'build_kaillera_layout_inputs.py', 'check_kaillera.py',
                    'verify_kaillera.py', 'run_toolchain.py')]
        tracked += [ROOT / 'toolchains' / name for name in
                    ('kaillera-toolchain.json', 'kaillera-packer.json')]
        tracked += [ROOT / 'toolchains/vc6rtm/bin' / name for name in
                    ('cl.exe', 'c1xx.dll', 'c2.dll', 'lib.exe', 'rc.exe', 'rcdll.dll')]
        tracked += [ROOT / 'toolchains/upx120/upx.exe']
        tracked += [ROOT / 'analysis' / name for name in
                    ('kaillera-bss-layout.json', 'kaillera-import-chronology.json')]
        report['build_input_sha256'] = {str(path.relative_to(ROOT)): digest(path) for path in tracked}
        objects = []
        for unit in UNITS:
            obj = output / (unit.replace('/', '-') + '.obj')
            tool('compile-' + unit.replace('/', '-'), 'cl', [*FLAGS, '/Fo' + winpath(obj), unit + '.cpp'])
            objects.append(obj)
        tool('resources', 'rc', ['/l', '0x40c', '/fo' + winpath(output / 'kaillera.res'), 'kaillera.rc'])
        tool('component-library', 'lib', ['/nologo', '/out:' + winpath(output / 'kaillera-components.lib'),
                                         *map(winpath, objects)])
        native = output / 'link-inputs' / 'native-templates'
        native.mkdir(parents=True, exist_ok=True)
        for label, text in [('named', NAMED_DEF), ('ordinal', ORDINAL_DEF)]:
            definition = native / (label + '.def')
            definition.write_text(text)
            tool('import-template-' + label, 'lib', ['/nologo', '/link50compat',
                '/machine:i386', '/def:' + winpath(definition),
                '/out:' + winpath(native / (label + '.lib'))])
        crt_libraries, thunk_order, report['crt_link_inputs'] = build_crt_inputs(
            output / 'link-inputs' / 'crt', native / 'named.lib', native / 'ordinal.lib')
        early, late, report['import_chronology'] = build_import_inputs(
            output / 'link-inputs' / 'imports', crt_libraries, report['crt_link_inputs'])
        report['link_inputs'] = report['import_chronology']['kernel32_spelling']
        arguments = lambda rows: [winpath(row) if isinstance(row, Path) else row for row in rows]
        link_objects, report['bss_layout_inputs'] = build_layout_inputs(
            objects, output / 'link-inputs' / 'layout')
        candidate = output / 'linked' / 'kailleraclient.dll'
        candidate.parent.mkdir(exist_ok=True)
        tool('link-candidate', 'link', ['/nologo', *arguments(early),
            '/dll', '/subsystem:windows', '/machine:i386',
            '/incremental:no', '/opt:ref', '/opt:nowin98',
            '/order:@' + winpath(thunk_order),
            '/out:' + winpath(candidate), '/map:' + winpath(output / 'kailleraclient.map'),
            '/implib:' + winpath(output / 'kailleraclient.lib'),
            *map(winpath, link_objects), winpath(output / 'kaillera.res'),
            winpath(crt_libraries['msvcrt']), *arguments(late)])
        report['native_import_inventory'] = check_inventory(candidate)
        # Keep a descriptive alias without changing the export directory's DLL name.
        shutil.copyfile(candidate, output / 'kailleraclient.unpacked.dll')
        packed = output / 'kailleraclient.dll'
        if packed.exists():
            packed.unlink()
        command('pack-candidate', ['wine', str(ROOT / 'toolchains/upx120/upx.exe'),
            '-9', '-o' + winpath(packed), winpath(candidate)])
        report['candidate'] = dict(unpacked=str(candidate), unpacked_sha256=digest(candidate),
                                   packed=str(packed), packed_sha256=digest(packed))
        # The reference is consumed only by the following diagnostic phase.
        if digest(ORIGINAL) != ORIGINAL_SHA256:
            raise ValueError('Original kailleraclient.dll hash changed')
        reference = output / 'reference.unpacked.dll'
        if reference.exists():
            reference.unlink()
        command('unpack-reference-for-diagnostics', ['wine', str(ROOT / 'toolchains/upx120/upx.exe'),
            '-d', '-o' + winpath(reference), winpath(ORIGINAL)])
        report.update(inspect(reference, objects, output / 'kaillera.res'))
        report.update(compare(packed))
        report.update(status='byte-exact' if report['full_dll_byte_exact'] else 'nonmatching-reconstruction', complete_dll_available=True,
                      remaining=['remaining component instruction differences',
                                 'final link layout and UPX container reproduction'])
        save()
        print(f'Component library: {output / "kaillera-components.lib"}\n'
              f'Resources exact: {report["all_resource_payloads_byte_exact"]}\n'
              f'Diagnostic unique masked-signature coverage: '
              f'{report["unique_masked_signature_code_bytes"]}/{report["original_code_virtual_bytes"]} bytes\n'
              f'Complete DLL byte exact: {report["full_dll_byte_exact"]}\n'
              f'Candidate: {packed}', flush=True)
        return 0 if report['full_dll_byte_exact'] else 1
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        report.update(status='build-failed', error=str(error))
        save()
        print(error, file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
