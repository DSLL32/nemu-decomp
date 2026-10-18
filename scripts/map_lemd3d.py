#!/usr/bin/env python3
"""Map a reconstructed LemD3D8 DLL's linked symbols onto the original.

Unique matches compare complete spans between public MAP symbols, masking only
PE relocations and decoded relative branches leaving the span. Such a span can include private
helpers; its length is not a recovered source function size. Corresponding
operands then identify additional referenced symbols. Conflicts are reported,
not resolved by voting. This is diagnostic evidence, never a DLL success test.
"""

import argparse
from bisect import bisect_left
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
from capstone.x86 import X86_OP_IMM

from compare_delphi_rtl import relocations
from inspect_gs_symbols import imported_symbols
from pe_fingerprints import PE32, u32

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'master/Plugin/LemD3D8.dll'
# End of the original executable section, excluding file-alignment padding.
ORIGINAL_CODE_END = 0x33133


def map_rows(path):
    text = path.read_text()
    rows = []
    for match in re.finditer(
            r'^ (\d{4}):([0-9A-Fa-f]{8})\s+(\S+)\s+([0-9A-Fa-f]{8})\s+(.*?)\s*$',
            text, re.M):
        section, offset, name, va, suffix = match.groups()
        rows.append(dict(name=name, rebuilt_va=int(va, 16),
                         section=int(section), is_function=suffix.startswith('f'),
                         object=re.sub(r'^(?:f\s+)?(?:i\s+)?', '', suffix)))
    return rows, None


def branch_operands(disassembler, data, va):
    for instruction in disassembler.disasm(data, va):
        if ((instruction.group(CS_GRP_CALL) or instruction.group(CS_GRP_JUMP))
                and instruction.operands and instruction.operands[0].type == X86_OP_IMM):
            yield instruction


def masked_pattern(data, mask):
    pieces = []
    for match in re.finditer(b'\x00+|\x01+', bytes(mask)):
        start, end = match.span()
        pieces.append(re.escape(data[start:end]) if not mask[start]
                      else b'.{' + str(end - start).encode() + b'}')
    return re.compile(b''.join(pieces), re.S)


def produce(dll, map_path):
    original, rebuilt = PE32(ORIGINAL), PE32(dll)
    rows, bss_offset = map_rows(map_path)
    rebuilt_text = next(s for s in rebuilt.sections if s['name'] == '.text')
    final_va = rebuilt.base + rebuilt_text['rva'] + rebuilt_text['virtual_size']
    starts = sorted({row['rebuilt_va'] for row in rows if row['section'] == 1 and row['is_function']})
    following = dict(zip(starts, starts[1:] + [final_va]))
    original_fixups = relocations(original)
    rebuilt_fixups = sorted(relocations(rebuilt))
    cs = Cs(CS_ARCH_X86, CS_MODE_32)
    cs.detail = True
    matches, unresolved = {}, []
    blocks = {}
    for va in starts:
        if va >= final_va:
            continue
        rva = va - rebuilt.base
        offset = rebuilt.offset(rva)
        data = rebuilt.data[offset:offset + following[va] - va].rstrip(b'\x90').rstrip(b'\xcc')
        if not data:
            continue
        fixups = rebuilt_fixups[bisect_left(rebuilt_fixups, rva):
                                bisect_left(rebuilt_fixups, rva + len(data))]
        branches = list(branch_operands(cs, data, va))
        mask = bytearray(len(data))
        for fixup in fixups:
            mask[fixup - rva:fixup - rva + 4] = b'\x01' * 4
        for instruction in branches:
            if va <= instruction.operands[0].imm < va + len(data):
                continue
            pos = instruction.address - va + instruction.imm_offset
            mask[pos:pos + instruction.imm_size] = b'\x01' * instruction.imm_size
        blocks[va] = (data, fixups, branches)
        if len(data) < 5:
            unresolved.append(dict(rebuilt_va=va, reason='tiny', compared_bytes=len(data)))
            continue
        candidates = []
        for match in masked_pattern(data, mask).finditer(original.data, 0x4000, ORIGINAL_CODE_END):
            candidates.append(match.start())
            if len(candidates) > 10:
                break
        if len(candidates) == 1:
            matches[va] = dict(va=original.base + candidates[0],
                               compared_bytes=len(data), masked_bytes=sum(mask),
                               match_kind='unique_complete_public_block')
        else:
            unresolved.append(dict(rebuilt_va=va, reason='ambiguous' if candidates else 'none',
                                   compared_bytes=len(data), candidate_file_offsets=candidates))
    references = defaultdict(lambda: defaultdict(list))
    original_image_end = original.base + u32(original.data, original.opt + 56)
    rebuilt_image_end = rebuilt.base + u32(rebuilt.data, rebuilt.opt + 56)
    for va, row in matches.items():
        target_va = row['va']
        references[va][target_va].append(dict(kind='unique_complete_public_block', at=va))
        rva, target_rva = va - rebuilt.base, target_va - original.base
        data, fixups, branches = blocks[va]
        for fixup in fixups:
            target_fixup = target_rva + fixup - rva
            if target_fixup not in original_fixups:
                continue
            before = u32(rebuilt.data, rebuilt.offset(fixup))
            after = u32(original.data, original.offset(target_fixup))
            if (rebuilt.base <= before < rebuilt_image_end
                    and original.base <= after < original_image_end):
                references[before][after].append(dict(kind='corresponding_DIR32',
                    at=rebuilt.base + fixup, original_at=original.base + target_fixup))
        for instruction in branches:
            pos = instruction.address - va + instruction.imm_offset
            size = instruction.imm_size
            target_pos = original.offset(target_rva) + pos
            displacement = int.from_bytes(original.data[target_pos:target_pos + size],
                                          byteorder='little', signed=True)
            before = instruction.operands[0].imm
            after = target_va + pos + size + displacement
            if (rebuilt.base <= before < rebuilt_image_end
                    and original.base <= after < original_image_end):
                references[before][after].append(dict(kind='corresponding_branch_operand',
                    at=instruction.address, original_at=target_va + instruction.address - va))
    original_imports = {(r['dll'].lower(), r['name']): r for r in imported_symbols(original)}
    for imported in imported_symbols(rebuilt):
        other = original_imports.get((imported['dll'].lower(), imported['name']))
        if other:
            references[imported['iat_va']][other['iat_va']].append(
                dict(kind='PE_import_name', name=imported['name']))
    symbols, unmapped = [], []
    for row in rows:
        va = row['rebuilt_va']
        result = {key: value for key, value in row.items() if key not in ('section', 'is_function')}
        if va in matches:
            result.update(matches[va])
            symbols.append(result)
        elif va in references and len(references[va]) == 1:
            target_va = next(iter(references[va]))
            evidence = references[va][target_va]
            result.update(va=target_va, match_kind='corresponding_relocations',
                          reference_count=len(evidence), references=evidence[:4])
            symbols.append(result)
        else:
            unmapped.append(result)
    conflicts = [dict(rebuilt_va=va, candidates={str(k): v for k, v in candidates.items()})
                 for va, candidates in references.items() if len(candidates) > 1]
    return dict(
        source_dll=str(dll), source_map=str(map_path),
        source_sha256=hashlib.sha256(rebuilt.data).hexdigest(),
        original_sha256=hashlib.sha256(original.data).hexdigest(),
        notes=[
            'Public-block spans end at the next public MAP symbol and may include private helpers; compared_bytes is not a source function size.',
            'Unique block matches mask PE DIR32 relocations and decoded immediate branches leaving the block; internal branches remain exact.',
            'Additional addresses come from corresponding operands in unique blocks or exact PE import names.',
            'Conflicting operand mappings are excluded; an independently unique block match remains available and its conflicts remain reported.',
            'Symbol names come from the reconstruction MAP, not recovered target CodeView records. Whole DLL equality requires verify_lemd3d.py.',
        ], symbols=symbols, unresolved_blocks=unresolved,
        unmapped_symbols=unmapped, reference_conflicts=conflicts)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--map', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    result = produce(args.dll.resolve(), (args.map or args.dll.with_suffix('.map')).resolve())
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(f"Mapped {len(result['symbols'])} symbols; "
          f"{len(result['reference_conflicts'])} reference conflicts; report: {args.report}")
