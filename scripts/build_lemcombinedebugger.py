#!/usr/bin/env python3
"""Build LemCombineDebugger with Delphi 6 Update 1 and require whole-file equality.

The compiler produces every program and resource byte. Only resource-directory
clock DWORDs may be restored, after strict equality of every other file byte.
All generated files and the isolated Wine prefix live under ~/.ai-tmp.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from lemcombinedebugger_forms import legacy_empty_list_strings
from restore_lemcombinedebugger_metadata import restore
from verify_lemcombinedebugger import compare

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, help='reuse a subdirectory of ~/.ai-tmp')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    parent = (Path.home() / '.ai-tmp').resolve()
    parent.mkdir(exist_ok=True)
    scratch = args.work.resolve() if args.work else Path(tempfile.mkdtemp(prefix='lemcombinedebugger-build.', dir=parent))
    if not scratch.is_relative_to(parent) or scratch == parent:
        parser.error('--work must be a subdirectory of ~/.ai-tmp')
    source, output, logs, helper = [scratch / name for name in ('source', 'output', 'logs', 'helper')]
    # Never retain stale application DCUs between clean matching builds.
    for directory in (source, helper):
        if directory.exists():
            shutil.rmtree(directory)
    for directory in (source, output, logs, helper):
        directory.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    report = dict(status='building', work=str(scratch), wine_prefix=env['WINEPREFIX'], sources={}, commands=[])
    print(f'Build directory: {scratch}\nWine prefix: {env["WINEPREFIX"]}', flush=True)

    def save():
        text = json.dumps(report, indent=2) + '\n'
        (scratch / 'comparison.json').write_text(text)
        if args.report:
            args.report.write_text(text)

    def run(label, command, cwd=source):
        log = logs / (label + '.log')
        row = dict(label=label, argv=[str(x) for x in command], cwd=str(cwd), log=str(log))
        report['commands'].append(row)
        with log.open('w') as stream:
            result = subprocess.run(command, cwd=cwd, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=120)
        row['returncode'] = result.returncode
        if result.returncode:
            raise RuntimeError(f'{label}: exit {result.returncode}; {log}\n' + log.read_text(errors='replace'))

    def compiler(label, *flags, cwd=source):
        run(label, [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                    'd6u1', 'dcc32-personal', '-Q', *flags], cwd)

    try:
        for path in sorted((ROOT / 'src/lemcombinedebugger').glob('*')):
            if path.suffix.lower() not in ('.pas', '.dpr', '.dfm'):
                continue
            target = source / (path.name + '.txt' if path.suffix == '.dfm' else path.name)
            shutil.copyfile(path, target)
            report['sources'][path.name] = digest(path)
        tracked = [ROOT / 'scripts' / name for name in
                   ('build_lemcombinedebugger.py', 'verify_lemcombinedebugger.py',
                    'restore_lemcombinedebugger_metadata.py', 'lemcombinedebugger_forms.py',
                    'delphi_dfm.dpr', 'run_toolchain.py', 'pe_fingerprints.py', 'stamp_pe_resources.py')]
        tracked += [ROOT / 'toolchains/d6u1/bin' / name for name in ('dcc32-personal.exe', 'rlink32.dll')]
        report['build_input_sha256'] = {str(path.relative_to(ROOT)): digest(path) for path in tracked}
        shutil.copyfile(ROOT / 'scripts/delphi_dfm.dpr', helper / 'delphi_dfm.dpr')
        compiler('form-converter', 'delphi_dfm.dpr', cwd=helper)
        for name in ('UnitCamera', 'UnitCombineEditor', 'UnitCombiners'):
            run('form-' + name, ['wine', str(helper / 'delphi_dfm.exe'), 'resource',
                                str(source / (name + '.dfm.txt')), str(source / (name + '.dfm'))])
        report['form_serialization'] = legacy_empty_list_strings(source / 'UnitCombineEditor.dfm')
        # Priming the camera DCU preserves the original traversal through the
        # circular ComCtrls/ListActns/Buttons interfaces during the project link.
        compiler('camera-unit', '-GD', 'UnitCamera.pas')
        compiler('project', '-GD', 'LemCombineDebugger.dpr')
        dll = output / 'LemCombineDebugger.dll'
        shutil.copyfile(source / dll.name, dll)
        shutil.copyfile(source / 'LemCombineDebugger.map', output / 'LemCombineDebugger.map')
        shutil.copyfile(dll, output / 'LemCombineDebugger.unstamped.dll')
        map_text = (output / 'LemCombineDebugger.map').read_text()
        units = sorted(set(re.findall(r'C=(?:CODE|DATA|BSS).*?M=(\w+)', map_text)))
        native_files = {path.stem.lower(): path for path in (ROOT / 'toolchains/d6u1/lib').glob('*.dcu')}
        report['linked_library_sha256'] = {str(native_files[name.lower()].relative_to(ROOT)): digest(native_files[name.lower()])
                                          for name in units if name.lower() in native_files}
        resources = sorted(set(re.findall(r'[^\\\r\n]+\.res(?=\r?$)', map_text, re.M)))
        report['linked_resource_sha256'] = {
            str((ROOT / 'toolchains/d6u1/lib' / name).relative_to(ROOT)):
            digest(ROOT / 'toolchains/d6u1/lib' / name) for name in resources}
        report['compiler_output_sha256'] = digest(dll)
        report['metadata_restoration'] = restore(dll)
        report.update(compare(dll))
        report['output_map'] = str(output / 'LemCombineDebugger.map')
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
