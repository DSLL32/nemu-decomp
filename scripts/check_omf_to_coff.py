#!/usr/bin/env python3
"""Compile Delphi probes, convert OMF to COFF, archive/link with MSVC, and run.

Exercises real compiler output and the real Windows runtime; no reference DLLs,
PE merger, third-party converter, or mocked Delphi runtime are involved.
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

from omf_to_coff import Converter, ConversionError, convert, import_library_aliases
from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / 'scripts/fixtures/omf_to_coff'


def winpath(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')


def worker(work, report):
    env = dict(os.environ, WINEPREFIX=str(work / 'wine'), WINEARCH='win32',
               WINEDEBUG='-all', WINEDLLOVERRIDES='winemenubuilder.exe,winedbg.exe=d')
    result = dict(passed=False, scratch=str(work), converted=[])

    def run(label, command, cwd=work, timeout=120):
        log = work / (label + '.log')
        with log.open('w') as stream:
            p = subprocess.run(command, cwd=cwd, env=env, stdout=stream,
                               stderr=subprocess.STDOUT, timeout=timeout)
        output = log.read_text(errors='replace')
        if p.returncode:
            raise RuntimeError(f'{label}: exit {p.returncode}; see {log}\n{output[-2500:]}')
        return output

    def tool(label, chain, name, arguments, cwd=work):
        return run(label, [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                          chain, name, *arguments], cwd)

    try:
        rtl = work / 'rtl'
        rtl.mkdir()
        # Object output follows the Pascal source path. Use disposable copies
        # so the tracked toolchain is never modified by this experiment.
        for name in ('system.pas', 'sysinit.pas', 'getmem.inc'):
            shutil.copy2(ROOT / 'toolchains/d6u2/source/rtl/sys' / name, rtl / name)
        for name in ('RuntimeProbe.pas', 'HelperProbe.pas', 'runtime_probe.c'):
            shutil.copy2(FIXTURES / name, work / name)
        # -y permits compiling the reserved System unit; -JP gives overloaded
        # routines distinct internal names. C entry points are explicit aliases.
        tool('compile-rtl', 'd6u2', 'dcc32-personal',
             ['-y', '-JP', '-$D-', '-$Y-', '-N.', '-E.', 'system.pas'], rtl)
        tool('compile-helper', 'd6u2', 'dcc32-personal',
             ['-JP', '-$D-', '-$L-', '-$Y-', '-N.', '-E.', 'HelperProbe.pas'])
        tool('compile-probe', 'd6u2', 'dcc32-personal',
             ['-JP', '-$D-', '-$L-', '-$Y-', '-N.', '-E.', 'RuntimeProbe.pas'])
        libraries = []
        for name in ('kernel32', 'user32', 'advapi32', 'oleaut32'):
            libraries.append(next(path / (name + '.lib') for path in
                (ROOT / 'toolchains/vc7/lib', ROOT / 'toolchains/vc7/platformsdk/lib')
                if (path / (name + '.lib')).exists()))
        aliases = import_library_aliases(libraries)
        inputs = [rtl / 'system.obj', rtl / 'sysinit.obj',
                  work / 'HelperProbe.obj', work / 'RuntimeProbe.obj']
        c_exports = ('Values', 'Strings', 'Callback', 'FinallyTest', 'ThreadValue',
                     'CrossUnit', 'ExceptionTest', 'VclInit', 'VclExit')
        for path in inputs:
            image = Converter(path.read_bytes())
            image.declarations()
            for sym in image.symbols:
                for export in c_exports:
                    if sym.name.startswith('@' + export + '$'):
                        aliases[sym.name] = '_' + export
        (work / 'aliases.json').write_text(json.dumps(aliases, indent=2) + '\n')
        objects = []
        for path in inputs:
            output = work / (path.stem + '.coff.obj')
            item = convert(path, output, aliases)
            item['input_sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()
            item['output_sha256'] = hashlib.sha256(output.read_bytes()).hexdigest()
            result['converted'].append(item)
            objects.append(output.name)
        # Refuse semantic records we do not implement, instead of dropping code.
        sample = inputs[-1].read_bytes()
        payload = b'\x80\xa0\x01'
        record = bytes([0x88]) + (len(payload) + 1).to_bytes(2, 'little') + payload + b'\0'
        # Insert before MODEND, preserving the enclosing module's structure.
        pos, last_record = 0, 0
        while pos < len(sample):
            last_record = pos
            pos += 3 + int.from_bytes(sample[pos + 1:pos + 3], 'little')
        try:
            Converter(sample[:last_record] + record + sample[last_record:]).coff()
        except ConversionError as error:
            if 'Unsupported semantic COMENT' not in str(error):
                raise
        else:
            raise AssertionError('Unknown import record was accepted')
        corrupt = bytearray(sample)
        length = int.from_bytes(corrupt[1:3], 'little')
        end = length + 3
        checksum = (-sum(corrupt[:end - 1])) & 255
        corrupt[end - 1] = 1 if checksum != 1 else 2
        try:
            Converter(corrupt).coff()
        except ConversionError as error:
            if 'checksum' not in str(error):
                raise
        else:
            raise AssertionError('Corrupt checksum was accepted')
        result['negative_checks'] = ['unsupported import record rejected', 'bad checksum rejected']
        tool('archive', 'vc7', 'lib', ['/nologo', '/OUT:delphi.lib', *objects])
        tool('compile-c', 'vc7', 'cl', ['/nologo', '/ML', '/c', 'runtime_probe.c'])
        result['runs'] = []
        # Two image bases independently exercise the COFF address relocations.
        for tag, base in (('low', 0x400000), ('high', 0x10000000)):
            exe = work / (tag + '.exe')
            tool('link-' + tag, 'vc7', 'link', ['/nologo', '/SUBSYSTEM:CONSOLE',
                 '/OPT:REF', '/INCREMENTAL:NO', '/BASE:' + hex(base),
                 '/OUT:' + exe.name, '/MAP:' + tag + '.map', 'runtime_probe.obj',
                 'delphi.lib', 'kernel32.lib', 'user32.lib', 'advapi32.lib', 'oleaut32.lib'])
            image = PE32(exe)
            assert image.base == base
            imports = [group['dll'].lower() for group in image.imports()]
            assert all(name in ('kernel32.dll', 'user32.dll', 'advapi32.dll', 'oleaut32.dll')
                       for name in imports), imports
            output = run('run-' + tag, ['wine', winpath(exe)])
            assert 'PASS:' in output and 'FAIL' not in output
            result['runs'].append(dict(image_base=hex(base),
                sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                imports=imports, output=output.strip(), passed=True))
            print(f'MSVC archive/link/run at {base:#x}: passed', flush=True)
        result['passed'] = True
    except Exception as error:
        result['error'] = str(error)
        print(str(error), file=sys.stderr, flush=True)
    finally:
        subprocess.run(['wineserver', '-k'], env=env, timeout=15, check=False)
        result['limitations'] = [
            'Tested under Wine; no native Windows run.',
            'System/SysInit and focused probes, not a full Nemu64/VCL plugin build.',
            'Initializers are called explicitly by the host in dependency order.',
            'VCL forms and resource integration are not established by this probe.']
        encoded = json.dumps(result, indent=2) + '\n'
        (work / 'report.json').write_text(encoded)
        if report:
            report.parent.mkdir(parents=True, exist_ok=True)
            report.write_text(encoded)
    return 0 if result['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--worker', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    report = args.report.resolve() if args.report else None
    if args.worker:
        return worker(args.worker, report)
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='nemu-omf-coff-check.', dir=parent))
    print(f'Probe scratch: {work}', flush=True)
    command = ['xvfb-run', '-a', '-e', str(work / 'xvfb.log'),
               sys.executable, str(Path(__file__).resolve()), '--worker', str(work)]
    if report:
        command.extend(['--report', str(report)])
    return subprocess.run(command, env=dict(os.environ, TMPDIR=str(work))).returncode


if __name__ == '__main__':
    raise SystemExit(main())
