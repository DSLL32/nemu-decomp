#!/usr/bin/env python3
"""Run the same Win32 host against original and reconstructed dialogs DLLs.

Requires Wine, Xvfb/xvfb-run, and the tracked VC7 compiler. Exercises five
dialog scenarios and compares native-control observations and resulting INI
files. This is a behavioral smoke check, not proof of complete equivalence.
All outputs, configuration changes, and the Wine prefix are kept in scratch.
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
CONFIG = '''[Configuration]
RDRAMSize=8
MEMPACK_EEPROM_PATH=C:\\mem\\
CART_EEPROM_PATH=C:\\eep\\
SRAM_PATH=C:\\sram\\
SAVESTATE_PATH=C:\\state\\
GFXDLL= PLUGIN2.DLL\x20
'''
ROM_CONFIG = '''[Default]
Count=3
[Probe ROM]
Option=1
[12345678-ABCDEF01]
Specific=7
'''


def worker(scratch, rebuilt, report):
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    shutil.copyfile(ROOT / 'scripts/dialogs_probe.c', scratch / 'dialogs_probe.c')

    def run(command, cwd, label, timeout=120):
        with (cwd / (label + '.log')).open('w') as output:
            process = subprocess.run(command, cwd=cwd, env=env, stdout=output,
                                     stderr=subprocess.STDOUT, timeout=timeout)
        if process.returncode:
            raise RuntimeError(f'{cwd / (label + ".log")}: exit {process.returncode}')

    try:
        run([sys.executable, str(ROOT / 'scripts/run_toolchain.py'), 'vc7', 'cl',
             '/nologo', '/TC', '/MT', 'dialogs_probe.c', '/link', 'user32.lib',
             '/out:dialogs_probe.exe'], scratch, 'compiler')
        for name, dll in [('original', ROOT / 'master/Plugin/dialogs.dll'), ('rebuilt', rebuilt)]:
            work = scratch / name
            work.mkdir()
            (work / 'config.ini').write_text(CONFIG)
            (work / 'Nemu64.ini').write_text(ROM_CONFIG)
            # Both fixture directories are disposable. ToolInit takes the
            # current directory as NemuFileDir; Apply writes only to these files.
            with (work / 'stdout.txt').open('w') as out, (work / 'stderr.txt').open('w') as err:
                result = subprocess.run(['wine', str(scratch / 'dialogs_probe.exe'),
                                         'Z:' + str(dll).replace('/', '\\')],
                                        cwd=work, env=env, stdout=out, stderr=err, timeout=60)
            if result.returncode or 'Probe completed' not in (work / 'stdout.txt').read_text():
                raise RuntimeError(f'{name} probe failed: see {work}')
            print(f'{name}: five dialog scenarios completed', flush=True)
        files = []
        for name in ('stdout.txt', 'config.ini', 'Nemu64.ini'):
            a = (scratch / 'original' / name).read_bytes()
            b = (scratch / 'rebuilt' / name).read_bytes()
            files.append(dict(file=name, equal=a == b,
                              original_sha256=hashlib.sha256(a).hexdigest(),
                              rebuilt_sha256=hashlib.sha256(b).hexdigest()))
        config = (scratch / 'rebuilt/config.ini').read_text()
        rom = (scratch / 'rebuilt/Nemu64.ini').read_text()
        applied = ('RDRAMSize=4' in config and 'GFXDLL=plugin1.dll' in config and
                   config.count('C:\\probe\\folder') == 4 and
                   all(f'Probe{i}=changed' in rom for i in range(1, 4)) and
                   'ignored line' not in rom)
        result = dict(
            original_sha256=hashlib.sha256((ROOT / 'master/Plugin/dialogs.dll').read_bytes()).hexdigest(),
            rebuilt_sha256=hashlib.sha256(rebuilt.read_bytes()).hexdigest(),
            scenarios=['ROM info without a ROM', 'ROM info with synthetic ROM data',
                       'Settings while stopped, including saved plugin selection',
                       'Settings while running; apply RDRAM, folders, graphics plugin',
                       'ROM settings; replace all three INI sections and skip lines without equals'],
            files=files, expected_apply_results=applied,
            passed=all(f['equal'] for f in files) and applied,
            limitations=['Native child controls and INI output are compared; non-windowed labels, '
                         'pixels, browse dialogs, error paths, and full host integration are not covered.'])
        text = json.dumps(result, indent=2) + '\n'
        (scratch / 'comparison.json').write_text(text)
        if report:
            report.write_text(text)
        print(f'Behavioral comparison passed: {result["passed"]}', flush=True)
        return 0 if result['passed'] else 1
    finally:
        # This prefix belongs exclusively to this probe, including on timeout.
        subprocess.run(['wineserver', '-k'], env=env, timeout=10, check=False)


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
    scratch = Path(tempfile.mkdtemp(prefix='nemu-dialogs-probe.', dir=parent))
    print(f'Probe scratch: {scratch}', flush=True)
    command = ['xvfb-run', '-a', '-e', str(scratch / 'xvfb.log'), sys.executable,
               str(Path(__file__).resolve()), str(rebuilt), '--worker', str(scratch)]
    if report:
        command.extend(['--report', str(report)])
    return subprocess.run(command, env=dict(os.environ, TMPDIR=str(scratch))).returncode


if __name__ == '__main__':
    raise SystemExit(main())
