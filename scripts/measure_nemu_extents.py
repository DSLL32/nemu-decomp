#!/usr/bin/env python3
"""Measure linked function extents without borrowing original function lengths.

The MAP bounds each function by the next distinct function entry. Walk its own
control-flow graph, including PE-relocated inline switch tables, then require
every remaining byte in the block to decode as alignment padding. Results are
bound to the candidate image hash and can be passed to compare_nemu_functions.py
with --extents. This measures extents; it does not establish source identities
or certify the executable.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_JUMP, CS_GRP_RET
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

from compare_nemu_functions import (aliases, base_relocations, image_bytes,
    load_metadata, read_map, map_function_spans, reachable_instructions,
    seh_scope_entries, Symbols)
from pe_fingerprints import PE32


def alignment_instruction(ins):
    if ins.mnemonic in ('nop', 'int3'):
        return True
    operands = ins.operands
    if len(operands) != 2 or operands[0].type != X86_OP_REG:
        return False
    if ins.mnemonic == 'mov' and operands[1].type == X86_OP_REG:
        return operands[0].reg == operands[1].reg
    if ins.mnemonic == 'lea' and operands[1].type == X86_OP_MEM:
        memory = operands[1].mem
        return (memory.base == operands[0].reg and not memory.index
                and not memory.disp)
    return False


def measure(image, start, end, fixups, symbols):
    data = image_bytes(image, start, end - start)
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    scopes = seh_scope_entries(image, start, end - start, fixups, symbols)
    entries = {row[key] for row in scopes for key in ('filter_va', 'handler_va') if row[key]}
    decoded = reachable_instructions(data, start, fixups, entries)
    tables, errors = {}, []
    extent = start
    if not decoded:
        errors.append('No decoded entry instruction')
    for ins in decoded:
        extent = max(extent, ins.address + ins.size)
        if not ins.group(CS_GRP_JUMP) or not ins.operands:
            continue
        operand = ins.operands[0]
        if (operand.type == X86_OP_MEM and not operand.mem.base
                and operand.mem.scale == 4 and start <= operand.mem.disp < end):
            table = operand.mem.disp
            cursor, targets = table, []
            while cursor + 4 <= end and cursor in fixups:
                target = struct.unpack_from('<I', data, cursor - start)[0]
                if not start <= target < end:
                    break
                targets.append(target)
                cursor += 4
            if not targets:
                errors.append(f'No PE-relocated local switch entries at {table:#x}')
            tables[table] = dict(address=hex(table), entries=len(targets), size=len(targets) * 4)
            extent = max(extent, cursor)
    padding, address = [], extent
    while address < end:
        ins = next(decoder.disasm(data[address - start:], address, count=1), None)
        if ins is None or ins.address + ins.size > end:
            errors.append(f'Cannot decode trailing bytes at {address:#x}')
            break
        if not alignment_instruction(ins):
            errors.append(f'Nonpadding after measured extent: {address:#x} '
                          f'{ins.mnemonic} {ins.op_str}')
            break
        padding.append(f'{ins.mnemonic} {ins.op_str}'.strip())
        address += ins.size
    if any(extent <= site < end for site in fixups):
        errors.append('Unaccounted PE relocation after measured extent')
    return dict(rebuilt_size=extent - start, map_block_size=end - start,
                tables=list(tables.values()), trailing_padding=padding,
                seh_scope_entries=scopes,
                extent_verified=not errors, errors=errors)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--metadata', action='append', type=Path, required=True)
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    image = PE32(args.image)
    symbols = [s for s in read_map(args.map) if s['kind'] == 'function']
    symbol_index = Symbols(symbols)
    spans = map_function_spans(image, symbols)
    fixups = base_relocations(image)
    if not fixups:
        parser.error('candidate has no PE HIGHLOW fixups; use a verified /FIXED:NO diagnostic')
    functions, _ = load_metadata(args.metadata)
    rows = []
    for function in functions:
        candidates = [s for s in symbols
                      if aliases(s['name']).intersection(aliases(function['name']))]
        addresses = {s['va'] for s in candidates}
        if len(addresses) != 1:
            raise ValueError(f'{function["name"]}: expected one linked function address')
        start = next(iter(addresses))
        if start not in spans:
            raise ValueError(f'{function["name"]}: no following MAP function boundary')
        row = dict(name=function['name'], symbol=candidates[0]['name'],
                   original_va=function['va'], original_size=function['size'],
                   rebuilt_va=start, measurement='Contiguous extent from entry through '
                   'highest reachable instruction end or PE-relocated inline switch-table '
                   'end; includes intervening gaps, excludes separately decoded trailing '
                   'alignment padding.')
        row.update(measure(image, start, start + spans[start], fixups, symbol_index))
        rows.append(row)
    report = dict(rebuilt_image=str(args.image),
                  rebuilt_image_sha256=hashlib.sha256(image.data).hexdigest(),
                  rebuilt_map_sha256=hashlib.sha256(args.map.read_bytes()).hexdigest(),
                  measurement_script='scripts/measure_nemu_extents.py',
                  measurement_script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  comparison_tool_sha256=hashlib.sha256(Path(__file__).with_name('compare_nemu_functions.py').read_bytes()).hexdigest(),
                  metadata=[dict(path=str(p), sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in args.metadata],
                  functions=rows)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    good = sum(r['extent_verified'] for r in rows)
    print(f'Independently measured extents: {good}/{len(rows)}')
    return 0 if good == len(rows) else 1


if __name__ == '__main__':
    raise SystemExit(main())
