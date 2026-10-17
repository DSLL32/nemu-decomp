#!/usr/bin/env python3
"""Complete CPUHostException's measured extent using its bounded byte selectors.

The input is a hash-bound report from measure_nemu_extents.py. This supplements
its CFG and DWORD-table measurement with the two indexed byte selector tables,
without borrowing the original function length. It does not certify code or
source identities.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

from compare_nemu_functions import base_relocations, image_bytes
from pe_fingerprints import PE32


def require(condition, message):
    if not condition:
        raise ValueError(message)


def complete(report, image):
    require(hashlib.sha256(image.data).hexdigest() == report['rebuilt_image_sha256'],
            'Input extent report does not identify this image')
    row = next(r for r in report['functions'] if r['name'] == 'CPUHostException')
    require(all(e.startswith('Nonpadding after measured extent:') for e in row['errors']),
            'Base extent report has errors unrelated to unmeasured selectors')
    start = row['rebuilt_va']
    end = start + row['map_block_size']
    data = image_bytes(image, start, end - start)
    fixups = base_relocations(image)
    code_end = min(int(table['address'], 16) for table in row['tables'])
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(data[:code_end - start], start))
    instruction_starts = {ins.address for ins in instructions}
    selectors = []
    for index, ins in enumerate(instructions):
        if (index < 2 or ins.mnemonic != 'movzx' or len(ins.operands) != 2 or
                ins.operands[1].type != X86_OP_MEM or ins.operands[1].size != 1):
            continue
        memory = ins.operands[1].mem
        if not start <= memory.disp < end:
            continue
        selector_index = memory.base or memory.index
        require(selector_index and not (memory.base and memory.index) and memory.scale == 1,
                'Selector does not use one byte index')
        compare, branch = instructions[index - 2:index]
        require(compare.mnemonic == 'cmp' and compare.operands[0].reg == selector_index and
                compare.operands[1].type == X86_OP_IMM and branch.mnemonic == 'ja' and
                branch.operands[0].type == X86_OP_IMM,
                'Selector lacks an adjacent unsigned bound check')
        maximum = compare.operands[1].imm
        require(0 <= maximum < 4096, 'Selector bound is outside the supported range')
        require(index + 1 < len(instructions), 'Selector has no following dispatch')
        dispatch = instructions[index + 1]
        require(dispatch.mnemonic == 'jmp' and dispatch.operands[0].type == X86_OP_MEM,
                'Selector is not followed by an indirect dispatch')
        table = dispatch.operands[0].mem
        require(not table.base and table.index == ins.operands[0].reg and table.scale == 4,
                'Dispatch does not index DWORD targets with the selected byte')
        require(branch.operands[0].imm in instruction_starts and
                branch.operands[0].imm >= dispatch.address + dispatch.size,
                'Out-of-range branch does not skip the dispatch')
        require(ins.address + ins.disp_offset in fixups and
                dispatch.address + dispatch.disp_offset in fixups,
                'Selector or dispatch displacement lacks a PE HIGHLOW fixup')
        selector_end = memory.disp + maximum + 1
        require(selector_end <= end, 'Selector extends beyond the MAP block')
        values = data[memory.disp - start:selector_end - start]
        entries = max(values) + 1
        require(start <= table.disp and table.disp + entries * 4 <= end,
                'Dispatch table extends beyond the MAP block')
        targets = []
        for slot in range(entries):
            address = table.disp + slot * 4
            require(address in fixups, 'Dispatch target lacks a PE HIGHLOW fixup')
            target = struct.unpack_from('<I', data, address - start)[0]
            require(target in instruction_starts, 'Dispatch target is not an instruction start')
            targets.append(target)
        selectors.append(dict(bounds_cmp_va=hex(compare.address), instruction_va=hex(ins.address),
                              dispatch_va=hex(dispatch.address), table_va=hex(memory.disp),
                              max_index=maximum, size=len(values), end_exclusive=hex(selector_end),
                              dispatch_table_va=hex(table.disp), dispatch_entries=len(targets),
                              targets_are_instruction_starts=True))
    require(len(selectors) == 2, 'Expected both CPUHostException byte selectors')
    extent = max(row['rebuilt_size'], max(int(s['end_exclusive'], 16) - start for s in selectors))
    padding = data[extent:]
    require(padding and all(value == 0x90 for value in padding),
            'Remaining MAP tail is not exclusively NOP alignment')
    require(not any(start + extent <= site < end for site in fixups),
            'Unaccounted PE relocation remains after the completed extent')
    row.update(rebuilt_size=extent, extent_verified=True, errors=[], byte_selectors=selectors,
               trailing_padding=[dict(address=hex(start + extent), bytes=padding.hex(), size=len(padding))],
               measurement='CFG and PE-relocated DWORD tables plus independently bounded byte selectors; '
               'both selector/dispatch references and every target have HIGHLOW fixups. '
               'Selected targets are decoded instruction starts. Trailing NOP alignment is excluded.')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--extents', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = complete(json.loads(args.extents.read_text()), PE32(args.image))
    report['selector_input_report_sha256'] = hashlib.sha256(args.extents.read_bytes()).hexdigest()
    report['selector_measurement_script'] = 'scripts/complete_nemu_cpu_control_extents.py'
    report['selector_measurement_script_sha256'] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    host = next(r for r in report['functions'] if r['name'] == 'CPUHostException')
    print(f"CPUHostException body: {host['rebuilt_size']} bytes; MAP block: {host['map_block_size']} bytes")


if __name__ == '__main__':
    main()
