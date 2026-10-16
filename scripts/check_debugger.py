#!/usr/bin/env python3
"""Run the same disposable Win32 host against original and rebuilt debugger DLLs.

This is a behavioral smoke comparison, not proof of complete equivalence. It
checks all four exports, all ten menu windows, notification paths, and native
register/opcode breakpoint controls. Requires Wine, Xvfb, and tracked VC7 tools.
All generated files, application configuration, and Wine registry writes belong
under a fresh ~/.ai-tmp directory. The input DLLs are copied before execution.
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


def worker(scratch, rebuilt, report):
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    shutil.copyfile(ROOT / 'scripts/debugger_probe.c', scratch / 'debugger_probe.c')
    work = scratch / 'fixture'
    work.mkdir()
    scenarios = []
    result = {'passed': False, 'scratch': str(scratch), 'runs': scenarios}
    try:
        with (scratch / 'compiler.log').open('w') as log:
            compiled = subprocess.run([sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                'vc7', 'cl', '/nologo', '/TC', '/MT', 'debugger_probe.c', '/link',
                'user32.lib', '/out:debugger_probe.exe'], cwd=scratch, env=env,
                stdout=log, stderr=subprocess.STDOUT, timeout=120)
        if compiled.returncode:
            raise RuntimeError(f'Probe compiler failed: {scratch / "compiler.log"}')
        for name, dll in [('original', ROOT / 'master/Plugin/debugger.dll'), ('rebuilt', rebuilt)]:
            # Reuse the identical Windows path and current directory. Both runs
            # begin with empty fixture files and reset plugin-specific registry.
            for item in work.iterdir():
                if item.is_dir(): shutil.rmtree(item)
                else: item.unlink()
            dll_bytes = dll.read_bytes()
            (work / 'plugin.dll').write_bytes(dll_bytes)
            with (scratch / f'{name}-registry.log').open('w') as log:
                subprocess.run(['wine', 'reg', 'delete', r'HKCU\Software\Nemu\0.8\Debugger', '/f'],
                    cwd=work, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=30)
            output = scratch / name
            output.mkdir()
            timed_out = False
            with (output / 'stdout.txt').open('w') as out, (output / 'stderr.txt').open('w') as err:
                try:
                    process = subprocess.run(['wine', str(scratch / 'debugger_probe.exe'),
                        'plugin.dll'], cwd=work, env=env, stdout=out, stderr=err, timeout=45)
                    code = process.returncode
                except subprocess.TimeoutExpired:
                    timed_out = True
                    code = None
                    subprocess.run(['wineserver', '-k'], env=env, timeout=10, check=False)
            files = {}
            for item in sorted(work.rglob('*')):
                if item.is_file() and item.name != 'plugin.dll':
                    files[str(item.relative_to(work))] = hashlib.sha256(item.read_bytes()).hexdigest()
            stdout = (output / 'stdout.txt').read_text(errors='replace')
            passed = code == 0 and 'Probe completed' in stdout
            scenarios.append({'name': name, 'dll_sha256': hashlib.sha256(dll_bytes).hexdigest(),
                'exit_code': code, 'timed_out': timed_out, 'completed': passed,
                'stdout_sha256': hashlib.sha256((output / 'stdout.txt').read_bytes()).hexdigest(),
                'generated_files': files})
            print(f'{name}: completed={passed}, exit={code}, logs={output}', flush=True)
        result.update(
            stdout_equal=(scratch / 'original/stdout.txt').read_bytes() ==
                         (scratch / 'rebuilt/stdout.txt').read_bytes(),
            generated_files_equal=scenarios[0]['generated_files'] == scenarios[1]['generated_files'],
            scenarios=['Public export loading, ToolInit return value, menu text',
                       'All ten ToolFunc(1, index) windows and native control snapshots',
                       'CPU/RSP debugger notification and bookmark reload notifications',
                       'Register-access breakpoint enable, trigger, and clear',
                       'Opcode breakpoint enable and clear with host ABI table observation',
                       'Window close, ToolFree, and DLL unload'],
            limitations=['Synthetic RAM, registers, and peripheral reads replace real emulation.',
                         'Native control text/state is hashed; non-windowed labels, rendered pixels, '
                         'and every interactive operation are not exercised.',
                         'This smoke check does not establish byte equality or complete behavior equivalence.'])
        result['passed'] = all(s['completed'] for s in scenarios) and result['stdout_equal'] and result['generated_files_equal']
    except Exception as error:
        result['error'] = str(error)
        print(str(error), file=sys.stderr, flush=True)
    finally:
        subprocess.run(['wineserver', '-k'], env=env, timeout=10, check=False)
    text = json.dumps(result, indent=2) + '\n'
    (scratch / 'comparison.json').write_text(text)
    if report:
        report.write_text(text)
    print(f'Behavioral comparison passed: {result["passed"]}', flush=True)
    print(f'Report: {scratch / "comparison.json"}', flush=True)
    return 0 if result['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--worker', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    rebuilt = args.dll.resolve(strict=True)
    report = args.report.resolve() if args.report else None
    if args.worker:
        return worker(args.worker, rebuilt, report)
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-debugger-probe.', dir=parent))
    print(f'Probe scratch: {scratch}', flush=True)
    command = ['xvfb-run', '-a', '-e', str(scratch / 'xvfb.log'), sys.executable,
               str(Path(__file__).resolve()), str(rebuilt), '--worker', str(scratch)]
    if report:
        command.extend(['--report', str(report)])
    return subprocess.run(command, env=dict(os.environ, TMPDIR=str(scratch))).returncode


if __name__ == '__main__':
    raise SystemExit(main())
