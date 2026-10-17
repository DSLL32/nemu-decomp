#!/usr/bin/env python3
"""Build the reconstructed Nemu64 sources with the tracked historical tools.

The current reconstruction may produce ordinary compiler/linker errors while
units are still being recovered. Missing implementations are never supplied by
stubs, and no original executable bytes are copied into the output. Successful
linking is followed by an unmasked, complete-file comparison.
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

from verify_nemu import compare
from inspect_gs_symbols import imported_symbols
from pe_fingerprints import PE32
from inspect_nemu_link_layout import inspect as inspect_link_layout


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/nemu64'

# Inferred application ordering. Include every additional real source unit
# after these; this list is not a claim that the original filenames survived.
APPLICATION_ORDER = [
    'RSP.cpp', 'RSPReciprocal.cpp', 'DynarecTracking.cpp', 'DynarecEmitter.cpp',
    'Dynarec.cpp', 'CPU.cpp', 'CPUFPU.cpp', 'CPUExecution.cpp', 'DynarecOpcodes.cpp',
    'DynarecMemoryOps.cpp', 'DynarecFPU.cpp', 'CpuControl.cpp', 'Memory.cpp',
    'Plugins.cpp', 'State.cpp', 'WinMain.cpp',
    'FrontendIni.cpp', 'Rom.cpp', 'RomBrowser.cpp', 'RuntimeSignatures.cpp', 'CIC.cpp', 'PIF.cpp', 'SaveDevices.cpp',
    'Globals.cpp', 'LegacyData.cpp', 'ByteSwap.c',
]

# These 14 VC6 RTM contributions are identified in the original code. gzio.c
# is part of upstream zlib but none of its functions occurs in this executable.
ZLIB_ORDER = [
    'uncompr.c', 'compress.c', 'contrib/minizip/unzip.c', 'inflate.c',
    'deflate.c', 'crc32.c', 'infblock.c', 'adler32.c', 'zutil.c', 'trees.c',
    'infcodes.c', 'inftrees.c', 'infutil.c', 'inffast.c',
]
APPLICATION_FLAGS = [
    '/nologo', '/c', '/O2', '/Ob1', '/ML', '/GL', '/W3',
    '/DWIN32', '/DNDEBUG', '/D_WINDOWS', '/DWINVER=0x0500',
    '/D_WIN32_WINNT=0x0400', '/Ivendor/zlib-1.1.3',
]
# CopyAndSwapWords retains a native C/cdecl boundary in the original. Its
# independent 36-byte match and sole residual VC7 C Rich contribution support
# this inferred unit boundary; compiling it with /GL changes its callers.
NATIVE_C_FLAGS = [flag for flag in APPLICATION_FLAGS if flag not in ('/GL', '/Ob1')]
ZLIB_FLAGS = [
    '/nologo', '/c', '/O2', '/ML', '/DWIN32', '/DNDEBUG',
    '/Ivendor/zlib-1.1.3',
]
RECIPE_FILES = [
    'build_nemu.py', 'build_nemu_imports.py', 'run_toolchain.py',
    'verify_nemu.py', 'verify_gs.py', 'pe_fingerprints.py',
    'build_nemu_link_inputs.py', 'map_nemu_libraries.py', 'inspect_gs_symbols.py',
    'stage_nemu_storage.py', 'nemu_storage_source.py', 'map_nemu_globals.py',
    'inspect_nemu_link_layout.py',
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, help='reusable build directory under ~/.ai-tmp')
    parser.add_argument('--no-map', action='store_true')
    parser.add_argument('--function-order', type=Path,
                        help='VC7 /ORDER file for reconstructed application functions')
    parser.add_argument('--ordered-libraries', action='store_true',
                        help='use the inferred unchanged CRT/import-member input order')
    parser.add_argument('--chronological-libraries', action='store_true',
                        help='optional native/alias/import chronology (implies --ordered-libraries)')
    parser.add_argument('--stage-storage', action='store_true',
                        help='stage typed initialized and zero storage in observed address order')
    parser.add_argument('--report', type=Path, help='also write the build/comparison report here')
    args = parser.parse_args()
    if args.chronological_libraries:
        args.ordered_libraries = True
        if args.no_map:
            parser.error('--chronological-libraries requires a MAP for layout verification')
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = (args.work.resolve() if args.work else
               Path(tempfile.mkdtemp(prefix='nemu-build.', dir=parent)))
    if not scratch.is_relative_to(parent.resolve()):
        parser.error('--work must be under ~/.ai-tmp')
    project, output, logs = scratch / 'src/nemu64', scratch / 'output', scratch / 'logs'
    for directory in [project, output, logs, project / 'obj/app', project / 'obj/zlib']:
        directory.mkdir(parents=True, exist_ok=True)
    prefix = scratch / 'wine'
    env = dict(os.environ, WINEPREFIX=str(prefix), WINEARCH='win32',
               WINEDEBUG='-all', PYTHONDONTWRITEBYTECODE='1')
    report = dict(project=str(project), wine_prefix=str(prefix),
                  output_dir=str(output), status='building',
                  build_constraints='analysis/nemu-build.json',
                  source_files=[], commands=[], metadata_restoration=None)
    print(f'Build directory: {project}\nWine prefix: {prefix}', flush=True)

    def write_report():
        serialized = json.dumps(report, indent=2) + '\n'
        (scratch / 'build.json').write_text(serialized)
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(serialized)

    def run(label, command, cwd=project, timeout=600):
        print(label, flush=True)
        log = logs / (label + '.log')
        row = dict(stage=label, command=command, cwd=str(cwd), log=str(log))
        report['commands'].append(row)
        with log.open('w') as stream:
            process = subprocess.run(command, cwd=cwd, env=env, stdout=stream,
                                     stderr=subprocess.STDOUT, timeout=timeout)
        row['returncode'] = process.returncode
        if process.returncode:
            raise RuntimeError(f'{label} failed ({process.returncode}); {log}\n'
                               + log.read_text(errors='replace'))

    def tool(label, toolchain, name, *arguments):
        run(label, [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                    toolchain, name, *arguments])

    try:
        # Capture the recipe before building: other reconstruction work may
        # continue in the shared checkout while the compiler runs.
        recipe = {name: (ROOT / 'scripts' / name).read_bytes()
                  for name in RECIPE_FILES}
        toolchain_manifest = (ROOT / 'toolchains/manifest.json').read_bytes()
        function_order = args.function_order.read_bytes() if args.function_order else None
        if function_order is not None:
            (project / 'function-order.txt').write_bytes(function_order)
            report['function_order'] = dict(source=str(args.function_order.resolve()),
                copied_file='function-order.txt', sha256=hashlib.sha256(function_order).hexdigest())
        # Preserve source bytes, including the Windows-1252 resource script.
        # Build from this snapshot; no generated files are written into src/.
        source_inputs = {}
        for source in sorted(SOURCE.rglob('*')):
            if not source.is_file() or '__pycache__' in source.parts:
                continue
            relative = source.relative_to(SOURCE)
            data = source.read_bytes()
            source_inputs[relative] = data
            report['source_files'].append(dict(path=str(relative),
                sha256=hashlib.sha256(data).hexdigest()))
        compiler_inputs = source_inputs
        storage_inputs = {}
        if args.ordered_libraries and not args.no_map:
            storage_inputs['nemu-library-addresses.json'] = (
                ROOT / 'analysis/nemu-library-addresses.json').read_bytes()
        if args.stage_storage:
            stage_root = Path(tempfile.mkdtemp(prefix='storage.', dir=scratch))
            stage_source = stage_root / 'original-source'
            for relative, data in source_inputs.items():
                destination = stage_source / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(data)
            for name in ['nemu-globals.json', 'nemu-storage-layout.json']:
                data = (ROOT / 'analysis' / name).read_bytes()
                storage_inputs[name] = data
                (stage_root / name).write_bytes(data)
            staged = stage_root / 'staged-source'
            run('stage-storage', [sys.executable, str(ROOT / 'scripts/stage_nemu_storage.py'),
                '--source', str(stage_source), '--output', str(staged),
                '--inventory', str(stage_root / 'nemu-globals.json'),
                '--layout', str(stage_root / 'nemu-storage-layout.json')])
            report['storage_staging'] = json.loads((staged / 'storage-stage.json').read_text())
            compiler_inputs = {path.relative_to(staged): path.read_bytes()
                               for path in sorted(staged.rglob('*'))
                               if path.is_file() and '__pycache__' not in path.parts}
        report['compiler_source_files'] = []
        application = []
        for relative, data in compiler_inputs.items():
            destination = project / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            report['compiler_source_files'].append(dict(path=str(relative),
                sha256=hashlib.sha256(data).hexdigest()))
            if relative.parts[0] not in ('vendor', 'imports', 'res') and relative.suffix in ('.cpp', '.c'):
                application.append(relative)
        rank = {name: i for i, name in enumerate(APPLICATION_ORDER)}
        storage_rank = {'NemuZeroStorage.cpp': 1, 'NemuInitializedStorage.cpp': 2}
        application.sort(key=lambda path: (storage_rank.get(path.as_posix(), 0),
            rank.get(path.as_posix(), len(rank)), path.as_posix()))
        initialized_storage = Path('NemuInitializedStorage.cpp')
        if initialized_storage in application:
            # This input position preserves the original signature-name pool
            # beside RuntimeSignatures while .data retains its observed order.
            application.remove(initialized_storage)
            application.insert(application.index(Path('RuntimeSignatures.cpp')), initialized_storage)
        report['application_units'] = [str(path) for path in application]
        report['application_flags'] = APPLICATION_FLAGS
        report['native_c_flags'] = NATIVE_C_FLAGS
        report['zlib_units'] = ZLIB_ORDER
        report['zlib_flags'] = ZLIB_FLAGS
        if not (prefix / 'system.reg').exists():
            run('wineboot', ['wineboot', '-u'])
        run('generate-cic-resource', [sys.executable, 'res/generate_pif_table.py',
                                     'res/pif-table.bin'])
        run('generate-cic-data', [sys.executable, 'res/generate_pif_tables.py',
                                 'PifTables.inc', '--report', str(scratch / 'cic-data.json')])
        run('import-libraries', [sys.executable, str(ROOT / 'scripts/build_nemu_imports.py'),
                                '--build-dir', str(project / 'imports'),
                                '--source-dir', str(project / 'imports')])
        if args.ordered_libraries:
            run('ordered-library-inputs', [sys.executable,
                str(ROOT / 'scripts/build_nemu_link_inputs.py'),
                '--order', 'imports/link-order.json', '--import-dir', 'imports',
                '--output-dir', 'obj/link-inputs',
                *(['--chronology', 'imports/link-chronology.json']
                  if args.chronological_libraries else [])])
            report['ordered_library_inputs'] = json.loads(
                (project / 'obj/link-inputs/inputs.json').read_text())
        zlib_objects = []
        for source in ZLIB_ORDER:
            name = Path(source).stem
            obj = 'obj/zlib/' + name + '.obj'
            tool('compile-zlib-' + name, 'vc6rtm', 'cl', *ZLIB_FLAGS,
                 '/Fo' + obj, 'vendor/zlib-1.1.3/' + source)
            zlib_objects.append(obj)
        application_objects = []
        for source in application:
            obj = Path('obj/app') / source.with_suffix('.obj')
            (project / obj).parent.mkdir(parents=True, exist_ok=True)
            flags = NATIVE_C_FLAGS if source.suffix == '.c' else APPLICATION_FLAGS
            tool('compile-' + source.as_posix().replace('/', '-'), 'vc7', 'cl',
                 *flags, '/Fo' + obj.as_posix(), source.as_posix())
            application_objects.append(obj.as_posix())
        tool('resources', 'vc7', 'rc', '/foobj/nemu64.res', 'nemu64.rc')
        # Absolute Wine paths avoid depending on the source tree's depth.
        winoutput = 'Z:' + str(output).replace('/', '\\')
        options = [
            '/nologo', '/LTCG', '/SUBSYSTEM:WINDOWS', '/OPT:WIN98',
            '/INCREMENTAL:NO', '/MACHINE:I386', '/BASE:0x400000', '/FIXED',
            '/OUT:' + winoutput + r'\nemu64.exe',
            *(['@obj/link-inputs/early.rsp']
              if args.ordered_libraries and not args.chronological_libraries else []),
            *application_objects, *zlib_objects, 'obj/nemu64.res',
            *(['@obj/link-inputs/link.rsp'] if args.ordered_libraries else []),
            'comctl32.lib', 'winmm.lib', 'imports/kailleraclient.lib',
            'imports/gs_dll.lib', 'kernel32.lib', 'user32.lib', 'gdi32.lib',
            'comdlg32.lib', 'shell32.lib',
        ]
        if not args.no_map:
            options.append('/MAP:' + winoutput + r'\nemu64.map')
        if function_order is not None:
            options.append('/ORDER:@function-order.txt')
        # Previous successful outputs have immutable copies under artifacts/.
        # Remove only this build's known outputs so failure cannot leave a stale
        # executable looking like a newly successful link.
        for name in ['nemu64.exe', 'nemu64.linked.exe', 'nemu64.map']:
            (output / name).unlink(missing_ok=True)
        tool('link', 'vc7', 'link', *options)
        executable = output / 'nemu64.exe'
        raw = output / 'nemu64.linked.exe'
        shutil.copyfile(executable, raw)
        if function_order is not None:
            report['function_order']['ignored_entries'] = re.findall(
                r'[^\r\n]*LNK4037[^\r\n]*', (logs / 'link.log').read_text(errors='replace'))
        if args.ordered_libraries:
            expected = json.loads((project / 'imports/link-order.json').read_text())['imports']['expected_layout']
            actual = imported_symbols(PE32(executable))
            identity = lambda row: (row['dll'].lower(), row['name'])
            wanted = {identity(row): row['iat_va'] for row in expected}
            observed = {identity(row): row['iat_va'] for row in actual}
            report['import_layout'] = dict(expected_count=len(expected), actual_count=len(actual),
                exact_iat_slots=sum(observed.get(key) == address for key, address in wanted.items()),
                names_and_slots_equal=wanted == observed)
            if not args.no_map:
                report['native_and_import_layout'] = inspect_link_layout(
                    executable, output / 'nemu64.map',
                    json.loads(storage_inputs['nemu-library-addresses.json']),
                    json.loads(source_inputs[Path('imports/link-chronology.json')]))
            report['library_input_mode'] = ('chronological' if args.chronological_libraries
                                            else 'ordered')
        comparison = compare(executable)
        (scratch / 'comparison.json').write_text(json.dumps(comparison, indent=2) + '\n')
        artifact = scratch / 'artifacts' / comparison['rebuilt_sha256']
        artifact.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(raw, artifact / raw.name)
        shutil.copyfile(executable, artifact / executable.name)
        if not args.no_map:
            shutil.copyfile(output / 'nemu64.map', artifact / 'nemu64.map')
        report.update(status='byte_exact' if comparison['full_exe_byte_exact'] else 'different',
                      raw_linked_exe=str(raw), executable=str(executable),
                      immutable_artifact_dir=str(artifact), comparison=comparison)
        # Keep the exact input source and generated object tree alongside every
        # successful link. This permits isolated relinks and reproducing earlier
        # checkpoints after the shared sources have changed.
        snapshot = artifact / 'source-snapshot'
        records = []
        for row in report['compiler_source_files']:
            data = (project / row['path']).read_bytes()
            if hashlib.sha256(data).hexdigest() != row['sha256']:
                raise RuntimeError('Build source changed during compilation: ' + row['path'])
        for row in report['source_files']:
            data = source_inputs[Path(row['path'])]
            destination = snapshot / 'src/nemu64' / row['path']
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            records.append(dict(path=str(destination.relative_to(snapshot)), sha256=row['sha256']))
        for name, data in storage_inputs.items():
            destination = snapshot / 'analysis' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            records.append(dict(path=str(destination.relative_to(snapshot)),
                                sha256=hashlib.sha256(data).hexdigest()))
        for name, data in recipe.items():
            destination = snapshot / 'scripts' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            records.append(dict(path=str(destination.relative_to(snapshot)),
                                sha256=hashlib.sha256(data).hexdigest()))
        if function_order is not None:
            destination = snapshot / 'build-inputs/function-order.txt'
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(function_order)
            records.append(dict(path=str(destination.relative_to(snapshot)),
                                sha256=hashlib.sha256(function_order).hexdigest()))
        for name in ['toolchains', 'master']:
            link = snapshot / name
            if not link.exists():
                link.symlink_to(ROOT / name, target_is_directory=True)
        shutil.copytree(project, artifact / 'build-tree', dirs_exist_ok=True)
        (artifact / 'toolchain-manifest.json').write_bytes(toolchain_manifest)
        (artifact / 'snapshot-manifest.json').write_text(json.dumps(dict(
            parent_artifact=str(artifact), source_file_hashes_verified=True,
            source_files_from_build_report=len(report['source_files']),
            source_snapshot_kind='original_reconstruction_inputs',
            compiled_input_tree='build-tree',
            scripts_captured_before_build=RECIPE_FILES, files=records), indent=2) + '\n')
        write_report()
        shutil.copyfile(scratch / 'build.json', artifact / 'build.json')
        print(f'Raw linker output: {raw}\n'
              f'Immutable artifact: {artifact}\n'
              f'Complete executable byte exact: {comparison["full_exe_byte_exact"]}\n'
              f'Differing file bytes: {comparison["differing_file_bytes"]}', flush=True)
        return 0 if comparison['full_exe_byte_exact'] else 1
    except (RuntimeError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        report.update(status='build_failed', error=str(error))
        write_report()
        print(str(error), file=sys.stderr, flush=True)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
