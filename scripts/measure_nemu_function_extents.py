#!/usr/bin/env python3
"""Measure contiguous x86 function extents, excluding decoded linker padding.

Function entries come from an MSVC MAP file, or from original-image metadata
with --original. A control-flow walk finds the highest instruction or inline
switch-table end. The measured size includes all intervening bytes, rather
than summing reachable instructions. Every remaining byte before the next
known function must decode as alignment padding for the result to be verified.

This does not prove reachability of exception-handler fragments or recognize
arbitrary indirect dispatch. Such functions need separately established
boundaries; unrecognized nonpadding tails are reported instead of discarded.
"""

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_JUMP, CS_GRP_RET
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

from pe_fingerprints import PE32


METHOD = (
    'contiguous extent from entry through highest reachable instruction end or '
    'referenced inline switch-table end; includes intervening gaps, excludes '
    'separately decoded alignment padding'
)


def integer(value):
    return int(value, 0) if isinstance(value, str) else value


def map_symbols(path):
    symbols = {}
    for line in path.read_text().splitlines():
        match = re.match(r'\s+[0-9a-f]+:[0-9a-f]+\s+(\S+)\s+([0-9a-f]+)\s+f(?:\s|$)',
                         line, re.IGNORECASE)
        if match:
            symbols[match[1]] = int(match[2], 16)
    return symbols


def is_padding(instruction):
    if instruction.mnemonic in ('nop', 'int3'):
        return True
    operands = instruction.operands
    if len(operands) != 2 or operands[0].type != X86_OP_REG:
        return False
    if instruction.mnemonic == 'mov':
        return operands[1].type == X86_OP_REG and operands[0].reg == operands[1].reg
    if instruction.mnemonic == 'lea' and operands[1].type == X86_OP_MEM:
        memory = operands[1].mem
        return (memory.base == operands[0].reg and not memory.index and
                not memory.disp and not memory.segment)
    return False


def measure(pe, start, bound):
    data = pe.data[pe.offset(start - pe.base):pe.offset(bound - pe.base)]
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    pending, seen, tables, errors = [start], set(), {}, []
    extent = start

    def decode(address):
        return next(iter(decoder.disasm(data[address - start:], address, count=1)), None)

    while pending:
        address = pending.pop()
        while start <= address < bound and address not in seen:
            seen.add(address)
            instruction = decode(address)
            if not instruction or address + instruction.size > bound:
                errors.append('decode failed or crossed bound at ' + hex(address))
                break
            extent = max(extent, address + instruction.size)
            if instruction.group(CS_GRP_RET):
                break
            if instruction.group(CS_GRP_JUMP):
                operand = instruction.operands[0]
                if operand.type == X86_OP_IMM:
                    if start <= operand.imm < bound:
                        pending.append(operand.imm)
                    if instruction.mnemonic == 'jmp':
                        break
                elif (operand.type == X86_OP_MEM and not operand.mem.base and
                      operand.mem.index and operand.mem.scale == 4 and
                      start <= operand.mem.disp < bound):
                    table_start = cursor = operand.mem.disp
                    targets = []
                    while cursor + 4 <= bound:
                        target = struct.unpack_from('<I', data, cursor - start)[0]
                        if not start <= target < bound:
                            break
                        targets.append(target)
                        pending.append(target)
                        cursor += 4
                    if not targets:
                        errors.append('empty switch table at ' + hex(table_start))
                    tables[table_start] = dict(address=hex(table_start), entries=len(targets),
                                              size=cursor - table_start)
                    extent = max(extent, cursor)
                    break
                else:
                    # An indirect transfer is terminal. A later nonpadding tail
                    # prevents this conservative result from being certified.
                    break
            address += instruction.size

    padding = []
    cursor = extent
    while cursor < bound:
        instruction = decode(cursor)
        if not instruction or cursor + instruction.size > bound:
            errors.append('undecoded trailing bytes at ' + hex(cursor))
            break
        if not is_padding(instruction):
            errors.append('nonpadding after extent at %s: %s %s' %
                          (hex(cursor), instruction.mnemonic, instruction.op_str))
            break
        padding.append((instruction.mnemonic + ' ' + instruction.op_str).rstrip())
        cursor += instruction.size
    return dict(rebuilt_va=hex(start), rebuilt_size=extent - start,
                map_block_size=bound - start, measurement=METHOD,
                tables=list(tables.values()), trailing_padding=padding,
                extent_verified=not errors, errors=errors)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--map', type=Path, help='MSVC MAP file for the rebuilt image')
    parser.add_argument('--metadata', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--original', action='store_true',
                        help='use original addresses from metadata instead of a MAP')
    parser.add_argument('--code-end', type=lambda value: int(value, 0),
                        help='independently known bound after the final original function')
    args = parser.parse_args()
    if args.original:
        if args.map or args.code_end is None:
            parser.error('--original requires --code-end and forbids --map')
    elif not args.map:
        parser.error('rebuilt-image measurement requires --map')

    metadata = json.loads(args.metadata.read_text())
    functions = metadata['functions']
    image_hash = hashlib.sha256(args.image.read_bytes()).hexdigest()
    pe = PE32(args.image)
    if args.original:
        symbols = {row.get('name', row.get('symbol')):
                   integer(row.get('address', row.get('original_va')))
                   for row in functions}
        bounds = sorted(set(symbols.values()) | {args.code_end})
    else:
        symbols = map_symbols(args.map)
        bounds = sorted(set(symbols.values()))

    rows = []
    for entry in functions:
        name = entry.get('name', entry.get('symbol'))
        if args.original:
            symbol = name
        else:
            candidates = [symbol for symbol in symbols if
                          symbol == entry.get('symbol') or
                          symbol.startswith('?' + name + '@@') or
                          symbol in (name, '_' + name) or
                          symbol.startswith('_' + name + '@')]
            symbol = candidates[0] if len(candidates) == 1 else None
        row = dict(name=name, address=entry.get('address', entry.get('original_va')),
                   original_size=entry.get('size'), rebuilt_image_sha256=image_hash)
        if symbol is None:
            row.update(extent_verified=False, errors=['missing or ambiguous MAP symbol'])
        else:
            start = symbols[symbol]
            bound = next((value for value in bounds if value > start), None)
            row['symbol'] = symbol
            if bound is None:
                row.update(extent_verified=False, errors=['missing next function bound'])
            else:
                row.update(measure(pe, start, bound))
        rows.append(row)

    report = dict(rebuilt_image_sha256=image_hash, rebuilt_image=str(args.image.resolve()),
                  original_image_measurement=args.original,
                  metadata=str(args.metadata),
                  metadata_sha256=hashlib.sha256(args.metadata.read_bytes()).hexdigest(),
                  functions=rows)
    if args.map:
        report['map_sha256'] = hashlib.sha256(args.map.read_bytes()).hexdigest()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    verified = sum(row['extent_verified'] for row in rows)
    print('%s: %d/%d verified contiguous extents' % (args.output, verified, len(rows)))
    for row in rows:
        if not row['extent_verified']:
            print('  %s: %s' % (row['name'], '; '.join(row['errors'])))
    return 0 if verified == len(rows) else 1


if __name__ == '__main__':
    raise SystemExit(main())
