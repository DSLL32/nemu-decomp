#!/usr/bin/env python3
"""Compare LemD3D8 initialization, metadata and shutdown in isolated Wine hosts.

No Direct3D device is created. This checks the host ABI, not rendered graphics.
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='lemd3d-runtime.', dir=parent))
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    report = dict(passed=False, scratch=str(scratch), runs=[],
                  coverage='13 export names; plugin info; initialization; combiner header; '
                           'resolution/window state before device creation; idle update; repeated shutdown; unload',
                  limitations='No Direct3D device, display lists, rendering, dialogs, or malformed inputs.')
    print(f'Runtime scratch: {scratch}', flush=True)
    try:
        shutil.copyfile(ROOT / 'scripts/lemd3d_probe.c', scratch / 'lemd3d_probe.c')
        inputs = [('original', (ROOT / 'master/Plugin/LemD3D8.dll').read_bytes()),
                  ('rebuilt', args.dll.read_bytes())]
        with (scratch / 'compile.log').open('w') as log:
            subprocess.run([sys.executable, str(ROOT / 'scripts/run_toolchain.py'), 'vc7', 'cl',
                            '/nologo', '/MT', 'lemd3d_probe.c', '/link',
                            '/out:lemd3d_probe.exe', 'user32.lib'], cwd=scratch, env=env,
                           stdout=log, stderr=subprocess.STDOUT, check=True, timeout=120)
        for name, data in inputs:
            run_dir = scratch / name
            run_dir.mkdir()
            (run_dir / 'plugin.dll').write_bytes(data)
            (run_dir / 'config.ini').write_text('[GFX]\nFullscreen=0\n')
            with (run_dir / 'stdout.txt').open('w') as out, (run_dir / 'stderr.txt').open('w') as err:
                result = subprocess.run(['wine', str(scratch / 'lemd3d_probe.exe'), 'plugin.dll'],
                                        cwd=run_dir, env=env, stdout=out, stderr=err, timeout=45)
            stdout = (run_dir / 'stdout.txt').read_text()
            report['runs'].append(dict(name=name, dll_sha256=hashlib.sha256(data).hexdigest(),
                exit_code=result.returncode, completed=result.returncode == 0 and 'Probe completed' in stdout,
                stdout=stdout))
            print(f'{name}: exit {result.returncode}', flush=True)
        report['stdout_equal'] = report['runs'][0]['stdout'] == report['runs'][1]['stdout']
        report['passed'] = report['stdout_equal'] and all(r['completed'] for r in report['runs'])
    except (OSError, subprocess.SubprocessError) as error:
        report['error'] = str(error)
        subprocess.run(['wineserver', '-k'], env=env, check=False, timeout=10)
    data = json.dumps(report, indent=2) + '\n'
    (scratch / 'runtime.json').write_text(data)
    if args.report:
        args.report.write_text(data)
    print(f'Runtime comparison passed: {report["passed"]}')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
