#!/usr/bin/env python3
"""Measure the unmatched native Mp3DCT source contribution.

The exit status requires complete-byte agreement. Finite x86 execution probes
are additional diagnostics and cannot satisfy that requirement on their own.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile

from pe_fingerprints import PE32
from verify_gs import COFF, TargetSymbols, compare_function

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/audio/ahle_ds/Mp3.c'
ORIGINAL = ROOT / 'master/Plugin/audio.dll'
INPUT, TABLE, STATE = 0x31010000, 0x31020000, 0x31030000
STACK, STOP = 0x40080000, 0x60000000


def number(value):
    return int(value, 0) if isinstance(value, str) else value


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def winpath(path):
    return 'Z:' + str(path.resolve()).replace('/', '\\')


def machine(image, addresses, obj=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(image.base, 0x200000)
    for section in image.sections:
        cpu.mem_write(image.base + section['rva'], image.data[section['offset']:
                      section['offset'] + section['raw_size']])
    for address, size in [(0x31000000, 0x100000), (0x40000000, 0x100000),
                          (STOP, 4096)]:
        cpu.mem_map(address, size)
    symbols = dict(addresses)
    if obj:
        cpu.mem_map(0x20000000, (len(obj.sections) + 1) * 0x100000)
        for index, section in enumerate(obj.sections):
            cpu.mem_write(0x20000000 + index * 0x100000, section['data'])
        for symbol in obj.symbols:
            if symbol['section'] > 0:
                symbols[symbol['name']] = (0x20000000 +
                    (symbol['section'] - 1) * 0x100000 + symbol['value'])
        for index, section in enumerate(obj.sections):
            for relocation in section['relocations']:
                address = 0x20000000 + index * 0x100000 + relocation['offset']
                addend = struct.unpack('<I', cpu.mem_read(address, 4))[0]
                value = symbols[relocation['symbol']['name']]
                if relocation['type'] == 20:
                    value -= address + 4
                elif relocation['type'] == 7:
                    value -= 0x20000000
                elif relocation['type'] != 6:
                    raise ValueError('Unsupported COFF relocation: ' + str(relocation))
                cpu.mem_write(address, struct.pack('<I', (value + addend) & 0xffffffff))
    return cpu, symbols


def behavior(image, addresses, obj, seed, phase):
    from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP
    executions = []
    for candidate in (None, obj):
        cpu, symbols = machine(image, addresses, candidate)
        rng = random.Random(seed)
        for address, size in [(INPUT, 0x1000), (TABLE, 0x890), (STATE, 0x440)]:
            cpu.mem_write(address, bytes(rng.randrange(256) for _ in range(size)))
        cpu.mem_write(STACK, struct.pack('<5I', STOP, INPUT, TABLE, STATE, phase))
        cpu.reg_write(UC_X86_REG_ESP, STACK)
        cpu.emu_start(symbols['_Mp3DCT'], STOP, count=2000000)
        if cpu.reg_read(UC_X86_REG_EIP) != STOP:
            raise ValueError('Instruction limit reached before return')
        if cpu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise ValueError('cdecl stack was not preserved')
        executions.append((cpu, symbols))
    (original, osymbols), (rebuilt, rsymbols) = executions
    regions = [('input_and_output', INPUT, INPUT, 0x1000),
               ('coefficient_input', TABLE, TABLE, 0x890),
               ('state_input', STATE, STATE, 0x440),
               ('local_dmem', STACK - 0x3248, STACK - 0x3248, 0x1000)]
    regions += [(name, osymbols[name], rsymbols[name], 2)
                for name in (['_d' + str(i) for i in range(32)] +
                             ['_c' + str(i) for i in range(16)])]
    differences = []
    for region, left_address, right_address, size in regions:
        left = original.mem_read(left_address, size)
        right = rebuilt.mem_read(right_address, size)
        changed = [i for i, (a, b) in enumerate(zip(left, right)) if a != b]
        if changed:
            differences.append(dict(region=region, differing_bytes=len(changed),
                                    first_offsets=changed[:16]))
    return dict(seed=seed, phase=phase, matching=not differences,
                differences=differences)


def run(output):
    flags = ['/nologo', '/c', '/O2', '/Ob1', '/MT']
    objpath = output / 'audio-mp3-main-candidate.obj'
    command = [sys.executable, str(ROOT / 'scripts/run_toolchain.py'), 'vc7', 'cl',
               *flags, '/Fo' + winpath(objpath), winpath(SOURCE)]
    env = dict(os.environ, WINEPREFIX=str(output / 'wine'), WINEDEBUG='-all')
    subprocess.run(command, cwd=ROOT, env=env, check=True)
    image = PE32(ORIGINAL)
    map_path = ROOT / 'analysis/audio-mp3-addresses.json'
    rows = json.loads(map_path.read_text())['symbols']
    library_path = ROOT / 'analysis/audio-libraries.json'
    library = json.loads(library_path.read_text())
    if library['original_sha256'] != digest(ORIGINAL):
        raise ValueError('CRT identity map does not describe this original DLL')
    addresses = {row['name']: number(row['va']) for row in library['symbols']}
    addresses.update({row['name']: number(row['va']) for row in rows})
    obj = COFF(objpath)
    function = next(f for f in obj.functions() if f['name'] == '_Mp3DCT')
    comparison = compare_function(function, image, TargetSymbols(image, rows=rows))
    # The exact frame-size check is a prerequisite for comparing the observed
    # local-DMEM storage partition after return. It is never patched or masked.
    frame_exact = function['data'][6:11] == bytes.fromhex('b8 04 36 00 00')
    cases = []
    if frame_exact:
        for seed in (1, 7, 0x1234):
            for phase in range(0, 32, 2):
                cases.append(behavior(image, addresses, obj, seed, phase))
    sources = [SOURCE, ROOT / 'src/audio/ahle_ds/transform.h', Path(__file__).resolve()]
    return dict(status='unmatched_native_source_candidate',
        original='master/Plugin/audio.dll', original_sha256=digest(ORIGINAL),
        compiler_flags=flags, build_directory=str(output),
        compiler_sha256={str(p.relative_to(ROOT)): digest(p)
                         for p in [ROOT / 'toolchains/vc7/bin' / name
                                   for name in ('cl.exe', 'c1.dll', 'c2.dll')]},
        source_sha256={str(p.relative_to(ROOT)): digest(p) for p in sources},
        address_map_sha256=digest(map_path), crt_map_sha256=digest(library_path),
        object_sha256=digest(objpath), comparison=comparison,
        frame_exact=frame_exact, behavioral_cases=cases,
        behavioral_matching=sum(c['matching'] for c in cases),
        byte_exact=bool(comparison.get('inferred_relocated_bytes_equal')),
        whole_dll_byte_exact=False,
        limitations=[
            'Mp3DCT is included in diagnostic DLL builds, but its complete function bytes do not match.',
            'Finite behavioral matches do not establish byte equality or universal equivalence.',
            'The emulator runs the original independently identified CRT and memory-copy callees.',
            'Only ordinary COFF relocations are applied inside disposable emulator memory.',
            'Helper declarations, local value widths and original source layout remain unresolved.',
        ])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path)
    parser.add_argument('--report', type=Path,
                        default=ROOT / 'analysis/audio-mp3-main-comparison.json')
    args = parser.parse_args()
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    output = (args.output_dir.resolve() if args.output_dir else
              Path(tempfile.mkdtemp(prefix='audio-mp3-main.', dir=parent)))
    if not output.is_relative_to(parent.resolve()) or output == parent.resolve():
        parser.error('--output-dir must be a folder below ~/.ai-tmp')
    output.mkdir(parents=True, exist_ok=True)
    result = run(output)
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(f"Mp3DCT: {result['comparison']['rebuilt_bytes']}/5104 bytes; "
          f"byte exact: {result['byte_exact']}; "
          f"behavior: {result['behavioral_matching']}/{len(result['behavioral_cases'])}")
    return 0 if result['byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
