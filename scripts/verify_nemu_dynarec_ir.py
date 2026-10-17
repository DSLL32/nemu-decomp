#!/usr/bin/env python3
"""Execute original/rebuilt dynarec translators and compare their emitted IR.

Requires Unicorn and Capstone. This is behavioral differential evidence, not
byte equality. Both PE images execute their real emitter, register helpers and
delay-slot memory reads. No application functions are mocked. Only IR address
fields are normalized using the recovered symbol map; integer immediates stay
literal. Native runtime helpers referenced by emitted calls are not executed.

Example:
  python3 scripts/verify_nemu_dynarec_ir.py --artifact /path/to/build \
      --report /path/to/report.json
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import random
import struct

from compare_nemu_functions import aliases, read_map
from pe_fingerprints import PE32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

ROOT = Path(__file__).resolve().parents[1]
METADATA = ('cpu', 'cpu-control', 'dynarec', 'dynarec-opcodes', 'dynarec-fpu',
            'dynarec-memory', 'memory', 'plugins', 'runtime', 'globals')
TRANSLATORS = '''ADDI ADDIU SLTI SLTIU ANDI ORI XORI LUI DADDI
SLL SRL SRA SLLV SRLV SRAV MFHI MTHI MFLO MTLO DSLLV DSRLV DSRAV
MULT MULTU DIV DIVU DMULT DMULTU DDIV DDIVU ADD SUB AND OR XOR NOR
SLT SLTU DADD DSUB DSLL DSRL DSRA DSLL32 DSRL32 DSRA32 MFC0 MTC0
Unknown BREAK TEQ SYSCALL TLBR TLBWI TLBWR TLBP ERET J JAL BEQ BNE
BLEZ BGTZ BEQL BNEL BLEZL BGTZL JR JALR BLTZ BGEZ BLTZL BGEZL
BLTZAL BGEZAL BLTZALL BGEZALL'''.split()
IMMEDIATES = set('ADDI ADDIU SLTI SLTIU ANDI ORI XORI LUI DADDI'.split())


def digest(data):
    return hashlib.sha256(data).hexdigest()


def metadata():
    addresses, sizes, evidence = {}, {}, []
    for suffix in METADATA:
        path = ROOT / f'analysis/nemu-{suffix}.json'
        document = json.loads(path.read_text())
        evidence.append(dict(path=str(path.relative_to(ROOT)), sha256=digest(path.read_bytes())))
        for key in ('globals', 'functions'):
            rows = document.get(key, [])
            if isinstance(rows, dict):
                rows = [dict(v, name=k) if isinstance(v, dict) else dict(name=k, va=v)
                        for k, v in rows.items()]
            for row in rows:
                name = row.get('symbol', row.get('name'))
                address = row.get('original_va', row.get('va', row.get('address')))
                if not name or address is None:
                    continue
                address = int(address, 0) if isinstance(address, str) else address
                addresses[name] = address
                size = row.get('extent_bytes', row.get('extent', row.get('size', 4)))
                if isinstance(size, str):
                    size = int(size, 0)
                sizes[name] = (size or 4) if key == 'globals' else 1
    sizes.update(CPU_GPR=256, CPU_COP0=128, OpcodeStats=0x880, RegisterStats=128)
    return addresses, sizes, evidence


class Image:
    def __init__(self, path, symbols, sizes, common_names):
        self.pe = PE32(path)
        self.symbols = symbols
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        end = max(s['rva'] + max(s['raw_size'], s['virtual_size']) for s in self.pe.sections)
        self.uc.mem_map(self.pe.base, (end + 0xfff) & ~0xfff)
        self.uc.mem_write(self.pe.base, self.pe.data[:0x400])
        for section in self.pe.sections:
            start = section['offset']
            self.uc.mem_write(self.pe.base + section['rva'],
                              self.pe.data[start:start + section['raw_size']])
        self.uc.mem_map(0x10000000, 0x10000)   # Host stack.
        self.uc.mem_map(0x20000000, 0x1000)    # Return sentinel.
        self.uc.mem_map(0x30000000, 0x800000)  # Guest RDRAM, initially MIPS NOPs.
        self.ranges = sorted((address, sizes[name], name)
            for name, address in symbols.items()
            if name in common_names and name in sizes and sizes[name])
        # Prefer the smallest named object over an enclosing allocation.
        self.ranges.sort(key=lambda row: row[1])

    def put(self, name, value):
        self.uc.mem_write(self.symbols[name], struct.pack('<I', value & 0xffffffff))

    def get(self, name):
        return struct.unpack('<I', self.uc.mem_read(self.symbols[name], 4))[0]

    def normalize_address(self, value):
        if value:
            for address, size, name in self.ranges:
                if address <= value < address + size:
                    return f'{name}+{value - address:x}'
        return value

    def run(self, case):
        for name in ('Dynarec_InstructionCount', 'Dynarec_LabelCount',
                     'Dynarec_BlockFinished', 'Dynarec_BlockInstructionCount',
                     'Dynarec_BranchPending', 'Dynarec_PendingCount',
                     'Dynarec_NextPC', 'Dynarec_InlineAddressCount',
                     'Dynarec_ResetRequired', 'CPU_ActionFlags', 'CPUBreakpointCount'):
            self.put(name, 0)
        self.put('RDRAM', 0x30000000)
        self.put('RDRAMSize', 0x800000)
        self.put('CPU_Instruction', case['instruction'])
        self.put('CPU_LastPC', case.get('pc', 0x80001000))
        self.put('BootCicSeed', case.get('cic_seed', 0))
        # CRC fields are at offsets 0x29c/0x2a0 in the recovered RomInformation.
        self.uc.mem_write(self.symbols['CurrentRom'] + 0x29c,
                          struct.pack('<II', *case.get('rom_crc', (0, 0))))
        self.put('Dynarec_DelaySlot', case.get('delay_slot', 0))
        self.put('OpcodeStatsEnabled', case.get('profile', 0))
        self.put('RegisterStatsEnabled', case.get('profile', 0))
        if case.get('breakpoint_hit'):
            self.put('CPUBreakpointCount', 1)
            self.uc.mem_write(self.symbols['CPUBreakpoints'],
                              struct.pack('<III', 0x24220001, 0x1000, 0))
        self.uc.mem_write(self.symbols['Dynarec_Instructions'], b'\0' * 16384)
        self.uc.mem_write(self.symbols['Dynarec_LabelInstructions'], b'\0' * 2048)
        for register in (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                         UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.uc.reg_write(register, 0x5a5a5a5a)
        self.uc.reg_write(UC_X86_REG_ESP, 0x1000fff0)
        self.uc.mem_write(0x1000fff0, struct.pack('<I', 0x20000000))
        self.uc.reg_write(UC_X86_REG_EFLAGS, 2)
        try:
            self.uc.emu_start(self.symbols[case['name']], 0x20000000, count=100000)
        except Exception as error:
            pc = self.uc.reg_read(UC_X86_REG_EIP)
            raise RuntimeError(f"{case['name']} at {pc:08x}: {error}") from error
        if self.uc.reg_read(UC_X86_REG_EIP) != 0x20000000:
            raise RuntimeError('instruction limit reached')
        count = self.get('Dynarec_InstructionCount')
        if count > 200:
            raise RuntimeError(f'invalid IR count {count}')
        data = self.uc.mem_read(self.symbols['Dynarec_Instructions'], count * 64)
        instructions = []
        for index in range(count):
            row = list(struct.unpack('<14I4BI', data[index * 64:(index + 1) * 64]))
            row[8] = self.normalize_address(row[8])
            instructions.append(row)
        labels = self.get('Dynarec_LabelCount')
        return dict(ir=instructions, label_count=labels,
            label_instructions=list(struct.unpack('<' + 'I' * labels,
                self.uc.mem_read(self.symbols['Dynarec_LabelInstructions'], labels * 4))),
            finished=self.get('Dynarec_BlockFinished'),
            block_count=self.get('Dynarec_BlockInstructionCount'),
            next_pc=self.get('Dynarec_NextPC'), instruction=self.get('CPU_Instruction'),
            pc=self.get('CPU_LastPC'))


def cases():
    rng = random.Random(0x64)
    result = []
    for name in TRANSLATORS:
        registers = [(0, 0, 0, 0, 0), (29, 29, 29, 31, 0xffff),
                     (31, 31, 31, 31, 0x8000), (1, 2, 3, 7, 0x7fff)]
        registers += [(rng.randrange(32), rng.randrange(32), rng.randrange(32),
                       rng.randrange(32), rng.randrange(65536)) for _ in range(10)]
        for rs, rt, rd, sa, immediate in registers:
            instruction = (rs << 21) | (rt << 16) | (rd << 11) | (sa << 6)
            if name in IMMEDIATES:
                instruction = (rs << 21) | (rt << 16) | immediate
            result.append(dict(name='Recompile_' + name, instruction=instruction))
    for rd in range(32):
        for name in ('MFC0', 'MTC0'):
            for delay in (0, 1):
                result.append(dict(name='Recompile_' + name,
                    instruction=(7 << 16) | (rd << 11), delay_slot=delay))
    for name in TRANSLATORS:
        result.append(dict(name='Recompile_' + name, instruction=0x03bdfdef,
                           profile=1, delay_slot=1))
    for name, instruction in [('SPECIAL', 0x00221821), ('REGIMM', 0x04200004),
                              ('COP0', 0x40026000), ('COP1', 0x44620000),
                              ('TLB', 0x42000001)]:
        for profile in (0, 1):
            result.append(dict(name='Recompile_' + name, instruction=instruction, profile=profile))
    for hit in (0, 1):
        result.append(dict(name='Recompile_Breakpoint', instruction=0xec000000,
                           breakpoint_hit=hit))
    for name in ('BEQ', 'BNE', 'BLEZ', 'BGTZ', 'BEQL', 'BNEL', 'BLEZL', 'BGTZL',
                 'BLTZ', 'BGEZ', 'BLTZL', 'BGEZL', 'BLTZAL', 'BGEZAL', 'BLTZALL', 'BGEZALL'):
        for immediate in (0x7fff, 0x8000, 0xffff):
            result.append(dict(name='Recompile_' + name,
                               instruction=(1 << 21) | (2 << 16) | immediate))
    result.append(dict(name='Recompile_BEQ', instruction=0x1000ffff))
    # Original game/boot-specific branches are deliberately preserved.
    for crc in ((0, 0), (0x7c3829d9, 0x6e8247ce), (0x0b0ab4cd, 0x7b158937)):
        result.append(dict(name='Recompile_J', instruction=0x080030b0,
                           pc=0x8000c2c0, rom_crc=crc))
    for pc, target, crc in ((0x8007a128, 0x8007ad60, (0x82dc04fd, 0xcf2d82f4)),
                            (0x8008d448, 0x8008e080, (0xa150743e, 0xcf2522cd))):
        result.append(dict(name='Recompile_JAL', instruction=0x0c000000 | ((target & 0x0fffffff) >> 2),
                           pc=pc, rom_crc=crc))
    for seed, pc in ((0x3f, 0x800001c0), (0x78, 0x80000190),
                     (0x85, 0x800001a0), (0x91, 0x80000240)):
        for offset in (0, 8, 16):
            result.append(dict(name='Recompile_BGEZAL', instruction=0x04310002,
                               pc=pc + offset, cic_seed=seed))
    return result


def dispatch_tables(original, rebuilt):
    document = json.loads((ROOT / 'analysis/nemu-dynarec-opcodes.json').read_text())
    checked, failures = 0, []
    for table in document['dispatch_tables']:
        original_base = int(table['va'], 0)
        rebuilt_base = rebuilt.symbols[table['name']]
        for entry in table['entries']:
            index = entry['index']
            actual_original = struct.unpack('<I', original.uc.mem_read(original_base + index * 4, 4))[0]
            actual_rebuilt = struct.unpack('<I', rebuilt.uc.mem_read(rebuilt_base + index * 4, 4))[0]
            expected_original = int(entry['va'], 0)
            expected_rebuilt = rebuilt.symbols[entry['name']]
            checked += 1
            if actual_original != expected_original or actual_rebuilt != expected_rebuilt:
                failures.append(dict(table=table['name'], index=index, target=entry['name'],
                    original=actual_original, expected_original=expected_original,
                    rebuilt=actual_rebuilt, expected_rebuilt=expected_rebuilt))
    return dict(entries_checked=checked, failures=failures)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifact', type=Path, required=True,
                        help='directory containing nemu64.exe and nemu64.map')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    original, sizes, evidence = metadata()
    rebuilt = {}
    for row in read_map(args.artifact / 'nemu64.map'):
        for name in aliases(row['name']):
            rebuilt[name] = row['va']
    common_names = original.keys() & rebuilt.keys()
    a = Image(ROOT / 'master/nemu64.exe', original, sizes, common_names)
    b = Image(args.artifact / 'nemu64.exe', rebuilt, sizes, common_names)
    dispatch = dispatch_tables(a, b)
    tests, failures, counts = cases(), [], Counter()
    aggregates = [hashlib.sha256(), hashlib.sha256()]
    for case in tests:
        counts[case['name']] += 1
        try:
            one, two = a.run(case), b.run(case)
            for result, aggregate in zip((one, two), aggregates):
                aggregate.update(json.dumps([case, result], sort_keys=True).encode())
            if one != two:
                failures.append(dict(case, original=one, rebuilt=two))
        except Exception as error:
            failures.append(dict(case, error=str(error)))
    report = dict(status='passed' if not failures and not dispatch['failures'] else 'different',
        evidence_type='behavioral_differential_only_not_byte_equality',
        original_sha256=digest(a.pe.data), rebuilt_sha256=digest(b.pe.data),
        rebuilt_map_sha256=digest((args.artifact / 'nemu64.map').read_bytes()),
        script_sha256=digest(Path(__file__).read_bytes()), metadata=evidence,
        tests=len(tests), functions=len(counts), cases_per_function=dict(sorted(counts.items())),
        original_results_sha256=aggregates[0].hexdigest(),
        rebuilt_results_sha256=aggregates[1].hexdigest(), dispatch_tables=dispatch,
        failures=failures)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f"{len(tests) - len(failures)}/{len(tests)} cases passed across {len(counts)} translators")
    print(f"{dispatch['entries_checked'] - len(dispatch['failures'])}/{dispatch['entries_checked']} dispatch targets match")
    for failure in failures[:8]:
        print(json.dumps(failure, indent=2))
    return bool(failures or dispatch['failures'])


if __name__ == '__main__':
    raise SystemExit(main())
