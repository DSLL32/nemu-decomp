#!/usr/bin/env python3
"""Smoke-test the static user build without loose bundled DLLs, under Wine/Xvfb.

Also tests an empty Plugin directory, an external DLL, and duplicate bundled
filenames. All fixture files, probe binaries, and Wine state are disposable.
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

from pe_fingerprints import PE32
from build_static import COMPONENTS
from static_objects import Glue
from check_gs import seed_ini

ROOT = Path(__file__).resolve().parents[1]
EXTERNAL = r'''
#include <windows.h>
static void mark(const char *name) {
    HANDLE h = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
}
__declspec(dllexport) unsigned int __cdecl ToolInit(void *host) {
    mark("external-init.txt"); return 1;
}
__declspec(dllexport) void __cdecl ToolFree(void) { mark("external-free.txt"); }
__declspec(dllexport) void __cdecl ToolFunc(unsigned int reason, unsigned int index) {}
__declspec(dllexport) void __cdecl ToolGetMenuText(char *text) {
    lstrcpyA(text, "External static build probe");
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) mark("external-attach.txt");
    if (reason == DLL_PROCESS_DETACH) mark("external-detach.txt");
    return TRUE;
}
'''


def worker(scratch, executable, report, components=False):
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32',
               WINEDEBUG='-all', WINEDLLOVERRIDES='winemenubuilder.exe=d',
               LIBGL_ALWAYS_SOFTWARE='1')
    results = dict(passed=False, scratch=str(scratch), scenarios=[])

    def run(label, command, cwd=scratch, timeout=120):
        with (scratch / (label + '.log')).open('w') as log:
            p = subprocess.run(command, cwd=cwd, env=env, stdout=log,
                               stderr=subprocess.STDOUT, timeout=timeout)
        if p.returncode:
            raise RuntimeError(f'{label}: exit {p.returncode}; see {scratch / (label + ".log")}')
        return (scratch / (label + '.log')).read_text(errors='replace')

    def compile_source(filename, flags):
        run('compile-' + filename, [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
            'vc7', 'cl', '/nologo', '/ML', *flags, filename, '/link', 'user32.lib'])

    try:
        frozen = scratch / 'input.exe'
        frozen.write_bytes(executable.read_bytes())
        results['exe_sha256'] = hashlib.sha256(frozen.read_bytes()).hexdigest()
        imports = [g['dll'].lower() for g in PE32(frozen).imports()]
        bundled = {Path(name).name.lower() for name, _ in COMPONENTS} | {'nemu_static.dll'}
        assert not bundled.intersection(imports), 'Loose component import remains'
        results['windows_imports'] = sorted(set(imports))
        for name in ('nemu_probe.c', 'nemu_menu_probe.c', 'nemu_menu_probe.def'):
            shutil.copy2(ROOT / 'scripts' / name, scratch)
        (scratch / 'external.c').write_text(EXTERNAL)
        compile_source('nemu_probe.c', ['/DNEMU_STATIC_PROBE'])
        # The helper's explicit .def preserves the hook's stdcall export name.
        run('compile-menu-hook', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
            'vc7', 'cl', '/nologo', '/ML', '/LD', 'nemu_menu_probe.c', '/link',
            'user32.lib', '/def:nemu_menu_probe.def'])
        compile_source('external.c', ['/LD'])
        for mode in ('no-plugin-directory', 'empty-plugin-directory', 'external-and-duplicates'):
            fixture = scratch / mode
            fixture.mkdir()
            (fixture / 'roms').mkdir()
            shutil.copy2(frozen, fixture / 'nemu64-static.exe')
            for name in ('config.ini', 'nemu64.ini', 'LemD3DCombine.dat', 'LemD3DCombine.new'):
                shutil.copy2(ROOT / 'bin' / name, fixture / name)
            config = (fixture / 'config.ini').read_text(encoding='latin1')
            config = '\n'.join('Folder=.\\roms\\' if line.startswith('Folder=') else line
                               for line in config.splitlines()) + '\n'
            (fixture / 'config.ini').write_text(config, encoding='latin1')
            if mode != 'no-plugin-directory':
                (fixture / 'Plugin').mkdir()
            if mode == 'external-and-duplicates':
                shutil.copy2(scratch / 'external.dll', fixture / 'Plugin' / 'external.dll')
                # Invalid duplicates prove these paths are skipped, not loaded.
                for name, plugin in COMPONENTS:
                    if plugin:
                        (fixture / 'Plugin' / Path(name).name).write_bytes(b'not a DLL')
            output = run(mode, ['setarch', 'i386', '-R', 'wine',
                str(scratch / 'nemu_probe.exe'), 'nemu64-static.exe'], cwd=fixture)
            assert 'Probe completed' in output
            assert 'input-settings' in output
            assert 'audio-settings' in output
            assert 'video-settings' in output
            assert 'Show Profiler...' in output
            assert 'Show Combine modes...' in output
            if mode == 'external-and-duplicates':
                assert 'External static build probe' in output
                for marker in ('attach', 'init', 'free', 'detach'):
                    assert (fixture / f'external-{marker}.txt').exists(), marker
            else:
                assert 'External static build probe' not in output
            results['scenarios'].append(dict(name=mode, passed=True))
            print(f'{mode}: passed', flush=True)
        if components:
            # Link the existing GameShark behavioral fixture against the same
            # static archive/resources, exercising MFC without needing a ROM.
            fixture = scratch / 'gameshark'
            fixture.mkdir()
            seed_ini(fixture / 'probe.ini')
            source = (ROOT / 'scripts/gs_probe.c').read_text()
            source = source.replace('int main(int argc, char **argv) {',
                'extern unsigned char __cdecl NemuStaticInitialize(HINSTANCE);\n'
                'extern void __cdecl NemuStaticShutdown(void);\n'
                'extern void __cdecl StaticGSInitialize(char *, char *, char *);\n'
                'extern void __cdecl StaticGSConfigure(void), StaticGSUpdate(void);\n'
                'int main(int argc, char **argv) {')
            start = source.index('    library = LoadLibraryA(argv[1]);')
            end = source.index('    puts("Exports resolved");', start)
            source = source[:start] + (
                '    require(NemuStaticInitialize(GetModuleHandleA(NULL)), "static runtime initialization");\n'
                '    initialize = StaticGSInitialize; configure = StaticGSConfigure; update = StaticGSUpdate;\n') + source[end:]
            source = source.replace('require(FreeLibrary(library), "DLL unload");', 'NemuStaticShutdown();')
            (fixture / 'gs_probe.c').write_text(source)
            glue = Glue()
            for name, target in [('GSInitialize', '?GSInitialize@@YAXPAD00@Z'),
                                 ('GSConfigure', '?GSConfigure@@YAXXZ'), ('GSUpdate', '?GSUpdate@@YAXXZ')]:
                glue.jump('_Static' + name, target)
            (fixture / 'glue.obj').write_bytes(glue.emit_coff())
            run('compile-static-gs', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                'vc7', 'cl', '/nologo', '/c', '/MT', 'gs_probe.c'], cwd=fixture)
            options = []
            for line in (ROOT / 'obj/static/link.rsp').read_text().splitlines():
                if line.upper().startswith(('/OUT:', '/MAP:', '/SUBSYSTEM:', '/LTCG')): continue
                if line.lower().endswith('.obj"'): continue
                options.append(line)
            options += ['/SUBSYSTEM:CONSOLE', '/OUT:gs_probe.exe', 'gs_probe.obj', 'glue.obj']
            (fixture / 'link.rsp').write_text('\n'.join(options) + '\n')
            run('link-static-gs', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                'vc7', 'link', '@link.rsp'], cwd=fixture)
            output = run('static-gameshark', ['setarch', 'i386', '-R', 'wine', 'gs_probe.exe', 'static'], cwd=fixture)
            assert 'Probe completed' in output
            assert 'RAM=all-code-types' in output
            results['scenarios'].append(dict(name='static-gameshark-archive', passed=True))
            print('static-gameshark-archive: passed', flush=True)
        results['passed'] = True
    except Exception as error:
        results['error'] = str(error)
        print(str(error), file=sys.stderr, flush=True)
    finally:
        subprocess.run(['wineserver', '-k'], env=env, timeout=15, check=False)
        results['limitations'] = [
            'Wine/Xvfb only; not yet checked on native Windows.',
            'Covers startup, built-in menus, About/Disclaimer, Delphi Settings/input and native audio/video dialogs, '
            'external tool initialization/unload, and normal process shutdown.',
            'No ROM execution, audio output, netplay session, or rendering comparison.']
        encoded = json.dumps(results, indent=2) + '\n'
        (scratch / 'report.json').write_text(encoded)
        if report:
            report.parent.mkdir(parents=True, exist_ok=True)
            report.write_text(encoded)
    return 0 if results['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path, nargs='?', default=ROOT / 'bin/nemu64-static.exe')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--components', action='store_true', help='also link/run the GameShark fixture against obj/static/builtins.lib')
    parser.add_argument('--worker', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    executable = args.exe.resolve(strict=True)
    report = args.report.resolve() if args.report else None
    if args.worker:
        return worker(args.worker, executable, report, args.components)
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-static-probe.', dir=parent))
    print(f'Probe scratch: {scratch}', flush=True)
    command = ['xvfb-run', '-a', '-s', '-screen 0 1280x1024x24',
               '-e', str(scratch / 'xvfb.log'), sys.executable, str(Path(__file__).resolve()),
               str(executable), '--worker', str(scratch)]
    if args.components:
        command.append('--components')
    if report:
        command.extend(['--report', str(report)])
    return subprocess.run(command, env=dict(os.environ, TMPDIR=str(scratch))).returncode


if __name__ == '__main__':
    raise SystemExit(main())
