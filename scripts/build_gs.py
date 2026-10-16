#!/usr/bin/env python3
"""Build the GameShark DLL reconstruction using the tracked VC6 toolchain.

All generated files and the disposable Wine prefix stay under ~/.ai-tmp.
The historical G: project and E: compiler paths preserve __FILE__ strings.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from restore_gs_metadata import restore
from verify_gs import compare_dll


ROOT = Path(__file__).resolve().parents[1]
UNITS = ['Addsubcode', 'gs_dll', 'gs_exeDlg', 'GS_interpret',
         'NewCodeDlg', 'PropertiesDlg', 'RW_INI_files', 'StdAfx']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path)
    parser.add_argument('--no-map', action='store_true')
    parser.add_argument('--report', type=Path,
                        help='also write the full comparison and metadata-restoration report here')
    args = parser.parse_args()
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = args.work.resolve() if args.work else Path(tempfile.mkdtemp(prefix='nemu-gs-build.', dir=parent))
    if not scratch.is_relative_to(parent.resolve()):
        parser.error('--work must be under ~/.ai-tmp')
    project = scratch / 'G/nemu/gs_dll'
    output = scratch / 'G/nemu/new37'
    project.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    (project / 'Debug').mkdir(exist_ok=True)
    prefix = scratch / 'wine'
    env = dict(os.environ, WINEPREFIX=str(prefix), WINEARCH='win32', WINEDEBUG='-all')
    print(f'Build directory: {project}\nWine prefix: {prefix}', flush=True)

    def run(label, command, extra_env=None):
        log = scratch / (label + '.log')
        with log.open('w') as stream:
            result = subprocess.run(command, cwd=project, env=dict(env, **(extra_env or {})), stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=180)
        if result.returncode:
            raise RuntimeError(f'{log}: exit {result.returncode}\n{log.read_text(errors="replace")}')

    if not (prefix / 'system.reg').exists():
        run('wineboot', ['wineboot', '-u'])
    for letter in ['G', 'E']:
        directory = scratch / letter
        directory.mkdir(exist_ok=True)
        link = prefix / 'dosdevices' / (letter.lower() + ':')
        if not link.exists():
            link.symlink_to(directory)
    installed = scratch / 'E/Microsoft Visual Studio/VC98'
    installed.parent.mkdir(parents=True, exist_ok=True)
    if not installed.exists():
        installed.symlink_to(ROOT / 'toolchains/vc6')
    vc = r'E:\Microsoft Visual Studio\VC98'
    env.update(WINEPATH=vc + r'\bin',
               INCLUDE=';'.join(vc + '\\' + x for x in ['INCLUDE', 'MFC\\INCLUDE', 'ATL\\INCLUDE']),
               LIB=';'.join(vc + '\\' + x for x in ['LIB', 'MFC\\LIB']))
    for source in (ROOT / 'src/gs_dll').rglob('*'):
        if source.is_file() and source.suffix.lower() in {'.cpp', '.h', '.rc', '.rc2', '.def', '.ico', '.bmp'}:
            destination = project / source.relative_to(ROOT / 'src/gs_dll')
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
    flags = ['/nologo', '/MTd', '/W3', '/Gm', '/GX', '/GZ', '/ZI', '/Od',
             '/DWIN32', '/D_DEBUG', '/D_WINDOWS', '/D_USRDLL',
             '/FpDebug/gs_dll.pch', '/FoDebug/', '/FdDebug/', '/c']
    run('compile-StdAfx', ['wine', vc + r'\bin\cl.exe', *flags, '/Ycstdafx.h', 'StdAfx.cpp'])
    for unit in UNITS[:-1]:
        run('compile-' + unit, ['wine', vc + r'\bin\cl.exe', *flags, '/Yustdafx.h', unit + '.cpp'])
    run('resources', ['wine', vc + r'\bin\rc.exe', '/l', '0x409', '/d_DEBUG',
                      '/foDebug/gs_dll.res', 'gs_dll.rc'])
    options = ['/nologo', '/subsystem:windows', '/dll', '/incremental:yes',
               '/pdb:Debug/gs_dll.pdb', '/debug', '/machine:I386', '/def:.\\gs_dll.def',
               '/out:g:\\nemu\\new37\\gs_dll.dll', '/implib:g:\\nemu\\new37\\gs_dll.lib',
               '/pdbtype:sept', *['.\\Debug\\' + unit + '.obj' for unit in UNITS],
               '.\\Debug\\gs_dll.res', '/incremental:yes', '/nologo', '/fullbuild']
    if not args.no_map:
        options.append('/map:Debug/gs_dll.map')
    # The original incremental linker used the Windows 9x mapped-file layout.
    # Wine's default NT layout uses a different base for its ILK heap.
    for program in ['link.exe', 'cvtres.exe']:
        run('platform-' + program, ['wine', 'reg', 'add',
            r'HKCU\Software\Wine\AppDefaults' + '\\' + program,
            '/v', 'Version', '/t', 'REG_SZ', '/d', 'win98', '/f'])
    runtime = prefix / 'drive_c/windows/system32/msvcrt.dll'
    if runtime.is_symlink():
        runtime.unlink()
    shutil.copyfile(ROOT / 'toolchains/vc6/runtime/msvcrt.dll', runtime)
    run('link', ['wine', vc + r'\bin\link.exe', *options],
        {'WINEDLLOVERRIDES': 'msvcrt=n'})
    dll = output / 'gs_dll.dll'
    linked = output / 'gs_dll.linked.dll'
    shutil.copyfile(dll, linked)
    metadata = restore(dll)
    report = compare_dll(dll)
    report['build'] = dict(project=str(project), wine_prefix=str(prefix),
                           toolchain='toolchains/vc6', raw_linked_dll=str(linked))
    report['metadata_restoration'] = metadata
    serialized = json.dumps(report, indent=2) + '\n'
    (scratch / 'comparison.json').write_text(serialized)
    if args.report:
        args.report.write_text(serialized)
    print(f'Raw linker output: {linked}\nDLL: {dll}\n'
          f'Complete DLL byte exact: {report["full_dll_byte_exact"]}\n'
          f'Differing file bytes: {report["differing_file_bytes"]}\n'
          f'SHA-256: {report["rebuilt_sha256"]}', flush=True)
    return 0 if report['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
