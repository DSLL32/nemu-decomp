#!/usr/bin/env python3
"""Compare Nemu64 startup, native menus/dialogs, and normal shutdown under Wine.

Uses original master plug-ins with both executables and an empty ROM directory.
This is a behavioral smoke test, not proof of binary or behavioral equivalence.
All application files and Wine state are disposable; master is read only.
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

ROOT = Path(__file__).resolve().parents[1]


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def stop_wine(env):
    subprocess.run(['wineserver', '-k'], env=env, timeout=10, check=False)
    subprocess.run(['wineserver', '-w'], env=env, timeout=10, check=False)


def worker(scratch, rebuilt, report):
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32',
               WINEDEBUG='-all', WINEDLLOVERRIDES='winemenubuilder.exe=d',
               LIBGL_ALWAYS_SOFTWARE='1')
    runs = []
    result = {'passed': False, 'scratch': str(scratch), 'runs': runs,
              'candidate_path': str(rebuilt), 'kind': 'behavioral smoke comparison',
              'wine_launch': 'setarch i386 -R wine (deterministic host address layout)'}
    try:
        # Freeze all inputs before starting Wine. Parent builds can then replace
        # their output without changing either run of this comparison.
        inputs = [('original', (ROOT / 'master/nemu64.exe').read_bytes()),
                  ('rebuilt', rebuilt.read_bytes())]
        dependencies = {str(p.relative_to(ROOT / 'master')): p.read_bytes()
                        for p in sorted((ROOT / 'master').rglob('*'))
                        if p.is_file() and p.name.lower() != 'nemu64.exe'}
        frozen = scratch / 'inputs'
        frozen.mkdir()
        for name, data in inputs:
            (frozen / f'{name}.exe').write_bytes(data)
        for relative, data in dependencies.items():
            destination = frozen / 'master' / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
        result['frozen_dependencies'] = {p: sha256(b) for p, b in dependencies.items()}
        result['probe_sources'] = {}
        for filename in ['nemu_probe.c', 'nemu_menu_probe.c', 'nemu_menu_probe.def']:
            probe_source = (ROOT / 'scripts' / filename).read_bytes()
            (scratch / filename).write_bytes(probe_source)
            result['probe_sources'][filename] = sha256(probe_source)
        with (scratch / 'compiler.log').open('w') as log:
            compiled = subprocess.run([sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                'vc7', 'cl', '/nologo', '/TC', '/ML', '/LD', 'nemu_menu_probe.c', '/link',
                'user32.lib', '/def:nemu_menu_probe.def', '/out:nemu_menu_probe.dll'],
                cwd=scratch, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=120)
            if compiled.returncode:
                raise RuntimeError(f'Menu probe compilation failed: {scratch / "compiler.log"}')
            compiled = subprocess.run([sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                'vc7', 'cl', '/nologo', '/TC', '/ML', 'nemu_probe.c', '/link',
                'user32.lib', '/out:nemu_probe.exe'], cwd=scratch, env=env,
                stdout=log, stderr=subprocess.STDOUT, timeout=120)
        if compiled.returncode:
            raise RuntimeError(f'Probe compilation failed: {scratch / "compiler.log"}')
        stop_wine(env)
        # Restore the complete initialized prefix before each run, including
        # registry and drive_c. Both runs consequently use the same prefix path
        # and state, rather than inheriting the original application's changes.
        baseline = scratch / 'wine-baseline'
        shutil.copytree(scratch / 'wine', baseline, symlinks=True)
        fixture = scratch / 'fixture'
        seeded = dict(dependencies)
        config = seeded['config.ini'].decode('latin1')
        config = '\r\n'.join('Folder=.\\roms\\' if line.startswith('Folder=') else line
                               for line in config.splitlines()) + '\r\n'
        seeded['config.ini'] = config.encode('latin1')
        result['fixture_configuration_sha256'] = sha256(seeded['config.ini'])
        for name, executable in inputs:
            shutil.rmtree(scratch / 'wine')
            shutil.copytree(baseline, scratch / 'wine', symlinks=True)
            if fixture.exists():
                shutil.rmtree(fixture)
            fixture.mkdir()
            (fixture / 'roms').mkdir()
            for relative, data in seeded.items():
                destination = fixture / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(data)
            (fixture / 'nemu64.exe').write_bytes(executable)
            output = scratch / name
            output.mkdir()
            timed_out = False
            with (output / 'stdout.txt').open('w') as out, (output / 'stderr.txt').open('w') as err:
                try:
                    # The application reserves a contiguous 512 MiB region in
                    # its 32-bit address space. Host ASLR can fragment Wine's
                    # address space differently across otherwise identical runs.
                    process = subprocess.run(['setarch', 'i386', '-R', 'wine',
                        str(scratch / 'nemu_probe.exe'), 'nemu64.exe'],
                        cwd=fixture, env=env, stdout=out, stderr=err, timeout=90)
                    code = process.returncode
                except subprocess.TimeoutExpired:
                    timed_out = True
                    code = None
                finally:
                    stop_wine(env)
            files, changed = {}, {}
            for item in sorted(fixture.rglob('*')):
                if item.is_file() and item.name.lower() != 'nemu64.exe':
                    relative = str(item.relative_to(fixture))
                    data = item.read_bytes()
                    files[relative] = sha256(data)
                    if relative not in seeded or data != seeded[relative]:
                        changed[relative] = files[relative]
                        destination = output / 'files' / relative
                        destination.parent.mkdir(parents=True, exist_ok=True)
                        destination.write_bytes(data)
            stdout = (output / 'stdout.txt').read_bytes()
            completed = code == 0 and b'Probe completed' in stdout
            runs.append({'name': name, 'exe_sha256': sha256(executable), 'exit_code': code,
                'timed_out': timed_out, 'completed': completed,
                'stdout_sha256': sha256(stdout), 'fixture_files': files, 'changed_files': changed})
            print(f'{name}: completed={completed}, exit={code}, logs={output}', flush=True)
        result.update(
            stdout_equal=(scratch / 'original/stdout.txt').read_bytes() ==
                         (scratch / 'rebuilt/stdout.txt').read_bytes(),
            fixture_files_equal=runs[0]['fixture_files'] == runs[1]['fixture_files'],
            scenarios=[
                'Launch with frozen original plug-ins and an empty ROM directory',
                'Reach an enabled main window and inspect native controls and every menu item',
                'Open About, open its Disclaimer, inspect both dialogs, and close with OK',
                'Open Settings through the main menu, inspect controls and combo choices, cancel',
                'Inspect the main window after returning from dialogs',
                'Request WM_CLOSE and require the application to exit with status zero',
                'Compare control/menu transcripts and every file left in the disposable fixture'],
            limitations=[
                'Exercises Windows UI under Wine/Xvfb using software graphics; no real Windows run.',
                'Uses original plug-in binaries with both executables; does not test rebuilt plug-ins.',
                'No ROM, emulation, save-state, netplay, audio output, or rendered-pixel coverage.',
                'Settings are inspected and canceled; no settings are applied.',
                'A read-only SetWindowsHookEx helper inspects actual menu state inside each process, because Wine menu handles are process-local.',
                'Control IDs and window handles are excluded because VCL can assign handles as IDs.',
                'The test does not establish full behavioral equivalence or byte equality.'])
        result['passed'] = all(r['completed'] for r in runs) and result['stdout_equal'] and result['fixture_files_equal']
    except Exception as error:
        result['error'] = str(error)
        print(str(error), file=sys.stderr, flush=True)
    finally:
        stop_wine(env)
    encoded = json.dumps(result, indent=2) + '\n'
    (scratch / 'comparison.json').write_text(encoded)
    if report:
        report.write_text(encoded)
    print(f'Behavioral comparison passed: {result["passed"]}', flush=True)
    print(f'Report: {scratch / "comparison.json"}', flush=True)
    return 0 if result['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--worker', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    rebuilt = args.exe.resolve(strict=True)
    report = args.report.resolve() if args.report else None
    if args.worker:
        return worker(args.worker, rebuilt, report)
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-startup-probe.', dir=parent))
    print(f'Probe scratch: {scratch}', flush=True)
    command = ['xvfb-run', '-a', '-s', '-screen 0 1280x1024x24', '-e', str(scratch / 'xvfb.log'),
               sys.executable, str(Path(__file__).resolve()), str(rebuilt), '--worker', str(scratch)]
    if report:
        command.extend(['--report', str(report)])
    return subprocess.run(command, env=dict(os.environ, TMPDIR=str(scratch))).returncode


if __name__ == '__main__':
    raise SystemExit(main())
