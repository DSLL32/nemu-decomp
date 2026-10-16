#!/usr/bin/env python3
"""Compare public GameShark exports and native dialogs in disposable Wine hosts.

The fixture seeds an INI database, activates codes through GSConfigure, checks
all supported interpreter cases against expected RAM, edits and saves a code,
and compares native controls and generated files. This is a behavioral smoke
test; whole-DLL equality is independently checked by verify_gs.py.
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

GROUPS = [
    ('A Writes', ['80000100 12AB', 'A0000101 34CD',
                  '81000104 1234', 'A1000106 BEEF']),
    ('B Conditions', ['D0000200 1222', '80000300 00A1',
                      'D0000201 0033', '80000301 00A2',
                      'D1000204 6677', '81000304 5566',
                      'D1000206 8899', '81000306 7788',
                      'D2000208 1255', '80000308 00B1',
                      'D2000209 0066', '80000309 00B2',
                      'D300020C 1234', '8100030C 7654',
                      'D300020E 4321', '8100030E ABCD',
                      'D0000201 0033', 'D0000201 0033', '80000310 00C1']),
    ('C Unsupported', ['F0000314 4444', '00000315 FFFF', '88000316 DDDD']),
]


def seed_ini(path):
    lines = ['[GS PROBE.Cheats]', f'NumCheats={len(GROUPS)}']
    for index, (name, codes) in enumerate(GROUPS):
        lines.extend([f'CheatName{index}={name}', f'CheatName{index}Count={len(codes)}'])
        lines.extend(f'CheatName{index}Code{number}={code}' for number, code in enumerate(codes))
    path.write_bytes(('\r\n'.join(lines) + '\r\n').encode('ascii'))


def worker(scratch, rebuilt, report):
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    shutil.copyfile(ROOT / 'scripts/gs_probe.c', scratch / 'gs_probe.c')
    fixture = scratch / 'fixture'
    fixture.mkdir()
    runs = []
    result = {'passed': False, 'scratch': str(scratch), 'runs': runs}
    try:
        # Freeze both candidates before compiler/Wine startup. A separate build
        # may then replace its output while this comparison is running.
        inputs = [('original', (ROOT / 'master/gs_dll.dll').read_bytes()),
                  ('rebuilt', rebuilt.read_bytes())]
        with (scratch / 'compiler.log').open('w') as log:
            compilation = subprocess.run([sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                'vc6', 'cl', '/nologo', '/TC', '/MT', 'gs_probe.c', '/link',
                'user32.lib', '/out:gs_probe.exe'], cwd=scratch, env=env,
                stdout=log, stderr=subprocess.STDOUT, timeout=120)
        if compilation.returncode:
            raise RuntimeError(f'Probe compilation failed: {scratch / "compiler.log"}')
        for name, dll_bytes in inputs:
            for item in fixture.iterdir():
                if item.is_dir():
                    shutil.rmtree(item)
                else:
                    item.unlink()
            (fixture / 'plugin.dll').write_bytes(dll_bytes)
            seed_ini(fixture / 'probe.ini')
            output = scratch / name
            output.mkdir()
            timed_out = False
            with (output / 'stdout.txt').open('w') as out, (output / 'stderr.txt').open('w') as err:
                try:
                    process = subprocess.run(['wine', str(scratch / 'gs_probe.exe'), 'plugin.dll'],
                        cwd=fixture, env=env, stdout=out, stderr=err, timeout=45)
                    code = process.returncode
                except subprocess.TimeoutExpired:
                    timed_out = True
                    code = None
                    subprocess.run(['wineserver', '-k'], env=env, timeout=10, check=False)
            files = {}
            for item in sorted(fixture.rglob('*')):
                if item.is_file() and item.name != 'plugin.dll':
                    relative = item.relative_to(fixture)
                    files[str(relative)] = hashlib.sha256(item.read_bytes()).hexdigest()
                    destination = output / 'files' / relative
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(item, destination)
            stdout = (output / 'stdout.txt').read_text(errors='replace')
            completed = code == 0 and 'Probe completed' in stdout
            runs.append({'name': name, 'dll_sha256': hashlib.sha256(dll_bytes).hexdigest(),
                'exit_code': code, 'timed_out': timed_out, 'completed': completed,
                'stdout_sha256': hashlib.sha256((output / 'stdout.txt').read_bytes()).hexdigest(),
                'generated_files': files})
            print(f'{name}: completed={completed}, exit={code}, logs={output}', flush=True)
        result.update(
            stdout_equal=(scratch / 'original/stdout.txt').read_bytes() ==
                         (scratch / 'rebuilt/stdout.txt').read_bytes(),
            generated_files_equal=runs[0]['generated_files'] == runs[1]['generated_files'],
            scenarios=[
                'Resolve GSInitialize, GSConfigure, GSUpdate and call before initialization',
                'Byte-swapped and space-padded ROM title selects the seeded INI section',
                'Activate, deactivate, and reactivate cheat groups using native controls',
                'New Code, Add, and Properties dialogs create and edit a fourth cheat',
                'Native control snapshots and saved INI contents compare identically',
                '80/A0 byte and 81/A1 halfword writes with N64 byte swapping and byte truncation',
                'D0/D1/D2/D3 conditions each exercise both execute and skip outcomes',
                'A failed condition skips exactly one subsequent conditional code',
                'Unsupported code types leave RAM unchanged',
                'Repeated GSUpdate reapplies active writes',
                'Reopen saved code database and retain active names within the process',
                'Deactivate all codes, reinitialize a different ROM, and unload DLL'],
            limitations=[
                'Uses synthetic RAM and deterministic valid INI input, not a running emulator.',
                'Does not test malformed files, invalid addresses, every dialog operation, or rendered pixels.',
                'Active names are tested across dialog reopen; the DLL does not persist them to INI.',
                'This behavioral smoke comparison does not establish full behavioral or byte equivalence.'])
        result['passed'] = all(run['completed'] for run in runs) and result['stdout_equal'] and result['generated_files_equal']
    except Exception as error:
        result['error'] = str(error)
        print(str(error), file=sys.stderr, flush=True)
    finally:
        subprocess.run(['wineserver', '-k'], env=env, timeout=10, check=False)
    encoded = json.dumps(result, indent=2) + '\n'
    (scratch / 'comparison.json').write_text(encoded)
    if report:
        report.write_text(encoded)
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
    scratch = Path(tempfile.mkdtemp(prefix='nemu-gs-probe.', dir=parent))
    print(f'Probe scratch: {scratch}', flush=True)
    command = ['xvfb-run', '-a', '-e', str(scratch / 'xvfb.log'), sys.executable,
               str(Path(__file__).resolve()), str(rebuilt), '--worker', str(scratch)]
    if report:
        command.extend(['--report', str(report)])
    return subprocess.run(command, env=dict(os.environ, TMPDIR=str(scratch))).returncode


if __name__ == '__main__':
    raise SystemExit(main())
