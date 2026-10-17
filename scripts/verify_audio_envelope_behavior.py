#!/usr/bin/env python3
"""Compare scalar envelope mixer executions in Unicorn (finite diagnostic).

Both images receive identical PCM, context, arguments and preserved registers.
This does not replace complete byte comparison or prove behavior for all inputs.
Only self-contained COFF functions without relocations are accepted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBX,
                              UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)
from pe_fingerprints import PE32
from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]
BUFFER, CONTEXT, STACK, STOP = 0x2000000, 0x2100000, 0x2200000, 0x2300000
PRESERVED = [UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI]


def run(code, address, arguments, context, pcm):
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    page = address & ~4095
    machine.mem_map(page, (address - page + len(code) + 4095) & ~4095)
    machine.mem_write(address, code)
    for base, size in [(BUFFER, 0x10000), (CONTEXT, 4096), (STACK, 0x10000), (STOP, 4096)]:
        machine.mem_map(base, size)
    machine.mem_write(BUFFER, pcm)
    machine.mem_write(CONTEXT, context)
    stack_pointer = STACK + 0x8000
    machine.mem_write(stack_pointer, struct.pack('<8I', STOP, *[x & 0xffffffff for x in arguments], CONTEXT))
    machine.reg_write(UC_X86_REG_ESP, stack_pointer)
    for i, reg in enumerate(PRESERVED):
        machine.reg_write(reg, 0x12345670 + i)
    machine.emu_start(address, STOP, count=1000000)
    if machine.reg_read(UC_X86_REG_EIP) != STOP:
        raise RuntimeError('Mixer did not return within instruction limit')
    return dict(pcm=bytes(machine.mem_read(BUFFER, len(pcm))),
                context=bytes(machine.mem_read(CONTEXT, len(context))),
                stack=machine.reg_read(UC_X86_REG_ESP) - stack_pointer,
                preserved=[machine.reg_read(reg) for reg in PRESERVED])


def cases(seed, random_count):
    rng = random.Random(seed)
    values = [-32768, -32767, -1, 0, 1, 16384, 32767, 32768, 65535]
    for index in range(random_count + 192):
        if index < 192:
            layout, length, category = index // 48, (index // 6) % 8, index % 6
        else:
            layout, length, category = rng.randrange(4), rng.randrange(8), rng.randrange(6)
        pcm = struct.pack('<2048h', *[rng.randint(-32768, 32767) for _ in range(2048)])
        context = bytearray(rng.randbytes(128))
        struct.pack_into('<I', context, 0, BUFFER)
        gains = [rng.choice(values) for _ in range(8)]
        if category == 0:
            gains[0:2] = [0, 0]
        if category == 1:
            gains[4:6] = [0, 0]
        if category == 2:
            gains[0:6] = [0, 0, 0, 0, 0, rng.choice([1, -1, 32767, -32768])]
        if category == 3:
            gains[0:2] = [0, 0]
            gains[4:6] = [0, 0]
        for i, value in enumerate(gains):
            struct.pack_into('<I', context, 0x34 + i * 4, value & 0xffffffff)
        for offset in range(0x54, 0x74, 2):
            struct.pack_into('<H', context, offset, rng.choice(values) & 65535)
        positions = [[0x100, 0x300, 0x500, 0x700, 0x900],
                     [0x100] * 5,
                     [0x100, 0x102, 0x104, 0x106, 0x108],
                     [0x100, 0x300, 0x300, 0x500, 0x500]][layout]
        count = [-16, 0, 2, 14, 16, 18, 32, 64][length]
        yield index, positions + [count], bytes(context), pcm


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('object', type=Path)
    parser.add_argument('function')
    parser.add_argument('original_va', type=lambda x: int(x, 0))
    parser.add_argument('--original-size', type=int, required=True)
    parser.add_argument('--random-cases', type=int, default=400)
    parser.add_argument('--seed', type=int, default=6410060)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    original_path = ROOT / 'master/Plugin/audio.dll'
    original = PE32(original_path)
    offset = original.offset(args.original_va - original.base)
    original_code = original.data[offset:offset + args.original_size]
    function = next(f for f in COFF(args.object).functions() if f['name'] == args.function)
    if function['relocations']:
        raise ValueError('This verifier accepts only functions without COFF relocations')
    report = dict(diagnostic_only=True, original_sha256=hashlib.sha256(original.data).hexdigest(),
                  object=str(args.object.resolve()), object_sha256=hashlib.sha256(args.object.read_bytes()).hexdigest(),
                  function=args.function, original_va=args.original_va, seed=args.seed,
                  code_bytes_equal=function['data'] == original_code,
                  coverage='192-case cross product of four buffer layouts, eight lengths, six gain/ramp categories, '
                           'plus random cases; full 128-byte context and 4096-byte PCM comparison',
                  compared_cases=0, mismatches=[])
    for index, arguments, context, pcm in cases(args.seed, args.random_cases):
        expected = run(original_code, args.original_va, arguments, context, pcm)
        candidate = run(function['data'], 0x2400000, arguments, context, pcm)
        report['compared_cases'] += 1
        if candidate != expected:
            report['mismatches'].append(dict(index=index, arguments=arguments, context_hex=context.hex(),
                pcm_sha256=hashlib.sha256(pcm).hexdigest(), fields=[k for k in expected if expected[k] != candidate[k]]))
    report['behavior_equal_in_cases'] = not report['mismatches']
    data = json.dumps(report, indent=2) + '\n'
    if args.report:
        args.report.write_text(data)
    print(data)
    return 0 if report['behavior_equal_in_cases'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
