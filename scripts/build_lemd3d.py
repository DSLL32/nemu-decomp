#!/usr/bin/env python3
"""Build LemD3D8 with VC7 and compare every output byte with the master DLL.

Exit zero requires complete file equality. No executable bytes are copied from
the original. Sources, objects, logs and an isolated Wine prefix live in scratch.
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

from verify_lemd3d import compare


ROOT = Path(__file__).resolve().parents[1]
UNITS = ['3DFunctions', 'blender', 'combiner', 'combtree', 'Direct3d',
         'DListParser', 'INIFile', 'LemNemu', 'prerender', 'StdAfx']
FLAGS = ['/nologo', '/c', '/O2', '/Ob2', '/MT', '/GL', '/GX', '/W3',
         '/DWIN32', '/DNDEBUG', '/D_WINDOWS', '/D_MBCS', '/D_USRDLL', '/DLEMNEMU_EXPORTS']


def winpath(path):
    return 'Z:' + str(path.resolve()).replace('/', '\\')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, help='reuse a directory under ~/.ai-tmp')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--no-ltcg', action='store_true', help='diagnostic native objects')
    parser.add_argument('--no-pch', action='store_true', help='diagnostic build without the upstream PCH')
    args = parser.parse_args()
    parent = (Path.home() / '.ai-tmp').resolve()
    parent.mkdir(exist_ok=True)
    scratch = (args.work.resolve() if args.work else
               Path(tempfile.mkdtemp(prefix='lemd3d-build.', dir=parent)))
    if not scratch.is_relative_to(parent) or scratch == parent:
        parser.error('--work must be a subdirectory of ~/.ai-tmp')
    project, output, logs = scratch / 'source', scratch / 'output', scratch / 'logs'
    # A reused scratch directory must not retain removed headers or old inputs.
    if project.exists():
        shutil.rmtree(project)
    for path in [project, output, logs]:
        path.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    report = dict(status='building', work=str(scratch), wine_prefix=env['WINEPREFIX'],
                  sources={}, commands=[], metadata_restoration=None)
    print(f'Build directory: {scratch}\nWine prefix: {env["WINEPREFIX"]}', flush=True)

    def save():
        data = json.dumps(report, indent=2) + '\n'
        (scratch / 'comparison.json').write_text(data)
        if args.report:
            args.report.write_text(data)

    def tool(label, name, options):
        command = [sys.executable, str(ROOT / 'scripts/run_toolchain.py'), 'vc7', name, *options]
        log = logs / (label + '.log')
        row = dict(label=label, argv=command, log=str(log))
        report['commands'].append(row)
        with log.open('w') as stream:
            result = subprocess.run(command, cwd=project, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=240)
        row['returncode'] = result.returncode
        if result.returncode:
            raise RuntimeError(f'{label}: exit {result.returncode}; {log}\n'
                               + log.read_text(errors='replace'))

    try:
        tracked_inputs = [ROOT / 'scripts' / name for name in
                          ['build_lemd3d.py', 'verify_lemd3d.py', 'run_toolchain.py', 'pe_fingerprints.py']]
        tracked_inputs += [ROOT / 'toolchains/vc7/bin' / name for name in
                           ['cl.exe', 'c1xx.dll', 'c2.dll', 'link.exe', 'rc.exe', 'cvtres.exe']]
        tracked_inputs += [ROOT / 'toolchains/vc7' / name for name in
                           ['lib/libcmt.lib', 'lib/libcpmt.lib',
                            'platformsdk/lib/prerelease/d3dx8.lib',
                            'platformsdk/lib/prerelease/d3d8.lib',
                            'platformsdk/lib/prerelease/dxerr8.lib']]
        report['build_input_sha256'] = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                                        for path in tracked_inputs}
        for source in sorted((ROOT / 'src/lemd3d').rglob('*')):
            if not source.is_file():
                continue
            relative = source.relative_to(ROOT / 'src/lemd3d')
            target = project / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
            report['sources'][str(relative)] = hashlib.sha256(target.read_bytes()).hexdigest()
        sdk = ROOT / 'toolchains/vc7/platformsdk'
        flags = [flag for flag in FLAGS if not (args.no_ltcg and flag == '/GL')]
        report['compiler_flags'] = flags
        objects = [winpath(output / (unit + '.obj')) for unit in UNITS]
        compilation_order = UNITS if args.no_pch else ['StdAfx', *[u for u in UNITS if u != 'StdAfx']]
        report['precompiled_headers'] = not args.no_pch
        for unit in compilation_order:
            pch = [] if args.no_pch else [('/Yc' if unit == 'StdAfx' else '/Yu') + 'stdafx.h',
                                        '/Fp' + winpath(output / 'LemNemu.pch')]
            tool('compile-' + unit, 'cl', [*flags, '/Ivendor/amd3d', '/I' + winpath(sdk / 'include/prerelease'),
                 *pch, '/Fo' + winpath(output / (unit + '.obj')), unit + '.cpp'])
        tool('compile-amd-detect', 'cl', ['/nologo', '/c', '/O2', '/Oy-', '/MT',
             '/Ivendor/amd3d', '/Fo' + winpath(output / 'detect.obj'), 'vendor/amd3d/detect.c'])
        objects.append(winpath(output / 'detect.obj'))
        tool('resources', 'rc', ['/l', '0x407', '/d', 'NDEBUG',
             '/fo' + winpath(output / 'LemNemu.res'), 'LemNemu.rc'])
        dll = output / 'LemD3D8.dll'
        tool('link', 'link', ['/nologo', '/dll', '/incremental:no', '/machine:I386',
             *([] if args.no_ltcg else ['/LTCG']), '/filealign:4096', '/opt:ref', '/opt:icf',
             '/out:' + winpath(dll), '/implib:' + winpath(output / 'LemD3D8.lib'),
             '/map:' + winpath(output / 'LemD3D8.map'),
             '/libpath:' + winpath(sdk / 'lib/prerelease'),
             *objects, winpath(output / 'LemNemu.res'),
             'd3dx8.lib', 'd3d8.lib', 'dxerr8.lib', 'winmm.lib',
             'kernel32.lib', 'user32.lib', 'gdi32.lib', 'advapi32.lib'])
        report['output_map'] = str(output / 'LemD3D8.map')
        report.update(compare(dll))
        report['status'] = 'matched' if report['full_dll_byte_exact'] else 'different'
        save()
        print(f'DLL: {dll}\nComplete DLL byte exact: {report["full_dll_byte_exact"]}\n'
              f'Differing file bytes: {report["differing_file_bytes"]}', flush=True)
        return 0 if report['full_dll_byte_exact'] else 1
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        report.update(status='build-failed', error=str(error), full_dll_byte_exact=False)
        save()
        print(error, file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
