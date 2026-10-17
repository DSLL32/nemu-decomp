#!/usr/bin/env python3
"""Compile and diagnose the reconstructed older audio ABI handlers.

This is a complete native function comparison, not whole-DLL acceptance.
The source includes the inline clear helper visible to the original wrapper;
the original source filename and header partition remain inferred.
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

from verify_gs import compare_object

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def win(path):
    return 'Z:' + str(path.resolve()).replace('/', '\\')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if args.work:
        work = args.work.resolve()
        work.mkdir(parents=True, exist_ok=True)
    else:
        parent = Path.home() / '.ai-tmp'
        parent.mkdir(exist_ok=True)
        work = Path(tempfile.mkdtemp(prefix='audio-legacy-check.', dir=parent))
    source = work / 'source'
    source.mkdir(exist_ok=True)
    records = []
    for name in ('AudioPlugin.h', 'AudioHLE.h', 'AudioLegacy.h',
                 'AudioLegacy.c', 'transform.h'):
        original = ROOT / 'src/audio/ahle_ds' / name
        destination = source / name
        shutil.copyfile(original, destination)
        records.append(dict(path=str(original.relative_to(ROOT)),
                            snapshot=str(destination), sha256=digest(destination)))
    metadata = work / 'addresses.json'
    shutil.copyfile(ROOT / 'analysis/audio-legacy-addresses.json', metadata)
    unit = source / 'LegacyUnit.c'
    unit.write_text('#include "AudioLegacy.c"\n')
    output = work / 'LegacyUnit.obj'
    command = [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
               'vc7', 'cl', '/nologo', '/c', '/O2', '/Ob1', '/MT',
               '/Fo' + win(output), win(unit)]
    environment = os.environ.copy()
    environment.update(WINEPREFIX=str(work / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    print('Build directory:', work, flush=True)
    result = subprocess.run(command, env=environment, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    (work / 'compile.log').write_text(result.stdout)
    if result.returncode:
        print(result.stdout)
        return result.returncode
    comparison = compare_object(output, ROOT / 'master/Plugin/audio.dll', metadata,
                                symbol_filter='Legacy')
    functions = comparison['functions']
    exact = sum(f.get('relocated_bytes_equal', False) for f in functions)
    report = dict(scope='Complete native function spans, including compiler alignment; not a complete DLL.',
                  original_sha256=digest(ROOT / 'master/Plugin/audio.dll'),
                  source_snapshot=records, metadata_sha256=digest(metadata),
                  generated_unit_sha256=digest(unit), command=command,
                  wine_prefix=environment['WINEPREFIX'],
                  compiler_inputs={n: digest(ROOT / 'toolchains/vc7/bin' / n)
                                   for n in ('cl.exe', 'c1.dll', 'c2.dll')},
                  helper_inputs={n: digest(ROOT / 'scripts' / n)
                                 for n in ('check_audio_legacy.py', 'run_toolchain.py',
                                           'verify_gs.py', 'pe_fingerprints.py')},
                  exact_functions=exact, total_functions=len(functions),
                  all_functions_exact=exact == len(functions) == 17,
                  comparison=comparison)
    destination = work / 'comparison.json'
    destination.write_text(json.dumps(report, indent=2) + '\n')
    if args.report:
        shutil.copyfile(destination, args.report)
    print(f'Complete aligned functions with verified targets: {exact}/{len(functions)}')
    print('Report:', destination)
    return 0 if report['all_functions_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
