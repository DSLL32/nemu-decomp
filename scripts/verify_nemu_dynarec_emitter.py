#!/usr/bin/env python3
"""Compare generated x86 output from the original and rebuilt IR assemblers.

Both complete PE images execute in Unicorn. No application functions are
replaced. Cases cover every decoded IR opcode, both operand widths and code
buffers, short-branch widening, label fixups, and guest-address recording.
This is behavioral evidence; it does not establish host-image byte equality.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from compare_nemu_functions import aliases, read_map
from pe_fingerprints import PE32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

ROOT = Path(__file__).resolve().parents[1]
STACK = 0x10000000
RETURN = 0x20000000


def digest(data):
    return hashlib.sha256(data).hexdigest()


def instruction(opcode, size=1, **fields):
    row = dict(code=0, opcode=opcode, destination=3, destination2=-1,
        source=2, source2=1, source3=-1, immediate=0x87654321,
        address=0x12345678, condition=6, mode=5, target=1, label=-1,
        guest_pc=0x80001234, size=size, long_jump=1, reads_flags=0,
        writes_flags=0, reserved=0)
    row.update(fields)
    return row


def cases():
    result = []
    for fast in (0, 1):
        for size in (0, 1):
            for opcode in (*range(1, 71), *range(1000, 1038)):
                rows = [instruction(opcode, size)]
                if opcode in (21, 23):
                    rows.append(instruction(0, label=1))
                result.append(dict(kind='opcode', fast=fast, rows=rows))
        for opcode in (21, 23):
            for gap in (0, 1, 125, 126, 127, 128, 129, 300):
                for record in (0, 1):
                    rows = [instruction(opcode, long_jump=0)]
                    rows.extend(instruction(43, guest_pc=0x80001238)
                                for _ in range(gap))
                    rows.append(instruction(0, label=1, guest_pc=0x8000123c))
                    result.append(dict(kind='branch', fast=fast, rows=rows,
                                       record=record, start=16))
        for register in range(8):
            result.append(dict(kind='same_register_sign_extend', fast=fast,
                rows=[instruction(12, source=register, destination=register)]))
        result.append(dict(kind='recorded_sequence', fast=fast, record=1,
            rows=[instruction(1), instruction(2),
                  instruction(14, guest_pc=0x80001238),
                  instruction(65, guest_pc=0x8000123c, address=0x80004000)]))
    return result


class Image:
    def __init__(self, path, symbols):
        self.pe = PE32(path)
        self.symbols = symbols
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        end = max(s['rva'] + max(s['raw_size'], s['virtual_size'])
                  for s in self.pe.sections)
        self.uc.mem_map(self.pe.base, (end + 0xfff) & ~0xfff)
        for section in self.pe.sections:
            offset = section['offset']
            self.uc.mem_write(self.pe.base + section['rva'],
                self.pe.data[offset:offset + section['raw_size']])
        self.uc.mem_map(STACK, 0x10000)
        self.uc.mem_map(RETURN, 0x1000)

    def put(self, name, value):
        self.uc.mem_write(self.symbols[name], struct.pack('<I', value))

    def get(self, name):
        return struct.unpack('<I', self.uc.mem_read(self.symbols[name], 4))[0]

    def run(self, case):
        fast, start = case['fast'], case.get('start', 0)
        buffer = 'Dynarec_FastCode' if fast else 'Dynarec_MainCode'
        used = 'Dynarec_FastCodeUsed' if fast else 'Dynarec_MainCodeUsed'
        base = self.symbols[buffer]
        self.uc.mem_write(base, b'\xcc' * 8192)
        self.put('Dynarec_MainCodeUsed', start)
        self.put('Dynarec_FastCodeUsed', start)
        self.put('Dynarec_JumpCount', 0)
        self.put('Dynarec_ConversionCount', 0)
        self.put('Dynarec_InstructionCount', len(case['rows']))
        self.uc.mem_write(self.symbols['Dynarec_RecordAddresses'],
                          bytes([case.get('record', 0)]))
        self.uc.mem_write(self.symbols['Dynarec_LabelInstructions'], b'\0' * 32)
        encoded = bytearray()
        for source in case['rows']:
            row = dict(source)
            if row['opcode'] == 17:
                # Equivalent buffer-relative call targets in the two images.
                row['address'] = base + 0x1234
            values = [value & 0xffffffff for value in row.values()]
            encoded.extend(struct.pack('<14I4BI', *values))
        ir = self.symbols['Dynarec_Instructions']
        self.uc.mem_write(ir, bytes(encoded))
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                         UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI,
                         UC_X86_REG_EBP):
            self.uc.reg_write(register, 0x5a5a5a5a)
        self.uc.reg_write(UC_X86_REG_ESP, STACK + 0xfff0)
        self.uc.mem_write(STACK + 0xfff0, struct.pack('<II', RETURN, fast))
        self.uc.reg_write(UC_X86_REG_EFLAGS, 2)
        self.uc.emu_start(self.symbols['AssembleInstructions'], RETURN,
                          count=1000000)
        if self.uc.reg_read(UC_X86_REG_EIP) != RETURN:
            raise RuntimeError('instruction limit reached')
        length = self.get(used)
        if not start <= length < 8192:
            raise RuntimeError(f'invalid generated-code length {length}')
        final_ir = self.uc.mem_read(ir, len(encoded))
        jump_count = self.get('Dynarec_JumpCount')
        conversion_count = self.get('Dynarec_ConversionCount')
        return dict(code=bytes(self.uc.mem_read(base + start, length - start)).hex(),
            used=length, cursor=self.get('Dynarec_CodePointer') - base,
            guest_jump=self.get('Dynarec_LastWasGuestJump'),
            record_offsets=[struct.unpack_from('<I', final_ir, i * 64)[0] - base
                            for i in range(len(case['rows']))],
            long_jumps=[final_ir[i * 64 + 57] for i in range(len(case['rows']))],
            jump_count=jump_count, conversion_count=conversion_count,
            jump_fixups=bytes(self.uc.mem_read(self.symbols['Dynarec_JumpFixups'],
                                               jump_count * 8)).hex(),
            address_map=bytes(self.uc.mem_read(self.symbols['Dynarec_AddressMap'],
                                               conversion_count * 8)).hex())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifact', type=Path, required=True,
                        help='directory containing nemu64.exe and nemu64.map')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    metadata_path = ROOT / 'analysis/nemu-dynarec.json'
    metadata = json.loads(metadata_path.read_text())
    original = {row['name']: int(row['va'], 0)
                for kind in ('functions', 'globals') for row in metadata[kind]}
    rebuilt = {}
    map_path = args.artifact / 'nemu64.map'
    for row in read_map(map_path):
        for name in aliases(row['name']):
            rebuilt[name] = row['va']
    a = Image(ROOT / metadata['target'], original)
    b = Image(args.artifact / 'nemu64.exe', rebuilt)
    if digest(a.pe.data) != metadata['sha256']:
        raise RuntimeError('reference image does not match the recorded target hash')
    tests, failures = cases(), []
    results = [hashlib.sha256(), hashlib.sha256()]
    for index, case in enumerate(tests):
        try:
            one, two = a.run(case), b.run(case)
            for value, aggregate in zip((one, two), results):
                aggregate.update(json.dumps(value, sort_keys=True).encode())
            if one != two:
                failures.append(dict(index=index, case=case, original=one, rebuilt=two))
        except Exception as error:
            failures.append(dict(index=index, case=case, error=str(error)))
    report = dict(status='passed' if not failures else 'different',
        evidence_type='behavioral_differential_only_not_byte_equality',
        original_sha256=digest(a.pe.data), rebuilt_sha256=digest(b.pe.data),
        rebuilt_map_sha256=digest(map_path.read_bytes()),
        script_sha256=digest(Path(__file__).read_bytes()),
        metadata_sha256=digest(metadata_path.read_bytes()),
        tests=len(tests), original_results_sha256=results[0].hexdigest(),
        rebuilt_results_sha256=results[1].hexdigest(), failures=failures)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(tests) - len(failures)}/{len(tests)} assembler cases passed')
    for failure in failures[:3]:
        print(json.dumps(failure, indent=2))
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
