#!/usr/bin/env python3
"""Compare a rebuilt dialogs.dll with master/Plugin/dialogs.dll.

Requires python3-capstone. Exit 0 means the complete DLL is byte exact; exit 1
means it is not. Address-masked code matches are diagnostic evidence only.
No binaries are executed and no bytes in either DLL are modified.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re

from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
from capstone.x86 import X86_OP_IMM

from compare_delphi_rtl import relocations
from inspect_delphi_forms import inspect
from pe_fingerprints import PE32, u32


ROOT = Path(__file__).resolve().parents[1]
PRIVATE_SYMBOLS = {
    'ReadWord': 0x218C8,
    'ReadHeaderByte': 0x83CCC,
    'TFormHeader.UpdateInfo': 0x83D0C,
    'ReadSection': 0x84648,
    'WriteSection': 0x84968,
}


def normalized(pe, rva, size, fixups):
    offset = pe.offset(rva)
    data = pe.data[offset:offset + size]
    if len(data) != size:
        raise ValueError('Code block extends beyond file')
    result = bytearray(data)
    masked = set()
    for address in fixups:
        if rva <= address <= rva + size - 4:
            masked.update(range(address - rva, address - rva + 4))
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    decoder.skipdata = True  # Delphi places string literals between routines.
    for ins in decoder.disasm(data, pe.base + rva):
        if not ins.id:
            continue
        if ((ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP)) and
                ins.operands and ins.operands[0].type == X86_OP_IMM and
                ins.imm_size == 4 and
                not pe.base + rva <= ins.operands[0].imm < pe.base + rva + size):
            at = ins.address - pe.base - rva + ins.imm_offset
            masked.update(range(at, at + 4))
    for at in masked:
        result[at] = 0
    return bytes(result), masked


def compare(rebuilt, map_path):
    original = ROOT / 'master/Plugin/dialogs.dll'
    target, candidate = PE32(original), PE32(rebuilt)
    before, after = inspect(original), inspect(rebuilt)
    symbols = {f['name'] + '.' + m['name']: m['va'] - target.base
               for f in before['forms'] for m in f['methods']}
    symbols.update({e['name']: e['rva'] for e in target.exports()})
    symbols.update(PRIVATE_SYMBOLS)
    map_text = map_path.read_text()
    mapped = {name: int(offset, 16) + 0x1000 for offset, name in re.findall(
        r'^ 0001:([0-9A-F]{8})\s+(\S+)\s*$', map_text, re.M)}
    data_rva = next(s['rva'] for s in candidate.sections if s['name'] == 'DATA')
    data_symbols = {name: candidate.base + data_rva + int(offset, 16)
                    for offset, name in re.findall(
                        r'^ 0002:([0-9A-F]{8})\s+(\S+)\s*$', map_text, re.M)}
    starts = sorted(set(mapped.values()))
    target_fixups, candidate_fixups = relocations(target), relocations(candidate)
    blocks = []
    for name, target_rva in sorted(symbols.items(), key=lambda row: row[1]):
        candidate_rva = mapped.get(name)
        if candidate_rva is None:
            blocks.append(dict(name=name, status='missing'))
            continue
        following = next((at for at in starts if at > candidate_rva), None)
        if following is None:
            blocks.append(dict(name=name, status='no mapped end'))
            continue
        size = following - candidate_rva
        a, am = normalized(target, target_rva, size, target_fixups)
        b, bm = normalized(candidate, candidate_rva, size, candidate_fixups)
        differences = [i for i, (x, y) in enumerate(zip(a, b)) if x != y]
        blocks.append(dict(name=name, original_rva=target_rva,
                           rebuilt_rva=candidate_rva, compared_bytes=size,
                           differing_bytes=len(differences),
                           first_differences=differences[:12],
                           mask_positions_equal=am == bm,
                           status='address-masked match' if a == b and am == bm
                           else 'different'))
    forms = []
    after_forms = {f['name']: f for f in after['forms']}
    for form in before['forms']:
        other = after_forms.get(form['name'])
        forms.append(dict(name=form['name'], unit=form['unit'],
                          resource_bytes=form['size'],
                          resource_exact=bool(other and form['sha256'] == other['sha256']),
                          fields_exact=bool(other and form['fields'] == other['fields']),
                          instance_size_exact=bool(other and form['instance_size'] == other['instance_size']),
                          method_names_exact=bool(other and
                              [m['name'] for m in form['methods']] ==
                              [m['name'] for m in other['methods']])))
    export_signature = lambda pe: sorted((e['name'], e['ordinal']) for e in pe.exports())
    units_before = {u['name'] for u in before['units']}
    units_after = {u['name'] for u in after['units']}

    def host_fields(pe, start, fixups, base):
        return [(at - start, u32(pe.data, pe.offset(at)) - base)
                for at in sorted(fixups) if start <= at < start + 108 and
                base <= u32(pe.data, pe.offset(at)) < base + 244]

    # Absolute field addresses were masked above. Validate their offsets within
    # the host record separately, so an incorrect ABI cannot pass this check.
    original_fields = host_fields(target, PRIVATE_SYMBOLS['ReadWord'],
                                  target_fixups, 0x488A30)
    rebuilt_fields = host_fields(candidate, mapped['ReadWord'],
                                 candidate_fixups, data_symbols['Nemu'])

    def string_table(pe, va):
        entries = []
        for i in range(32):
            address = u32(pe.data, pe.offset(va - pe.base) + 4 * i)
            at = pe.offset(address - pe.base)
            entries.append(pe.data[at:at + u32(pe.data, at - 4)].decode('ascii'))
        return entries

    tables = []
    for name, va in [('CPURegisterNames', 0x4865D8), ('RSPRegisterNames', 0x486658),
                     ('VectorControlNames', 0x4866D8), ('GeneralRegisterNames', 0x486758)]:
        a = string_table(target, va)
        b = string_table(candidate, data_symbols[name])
        tables.append(dict(name=name, entries_exact=a == b, entry_count=len(a)))

    def manifest(pe):
        resource = next((r for r in pe.resources() if r['path'] == [24, 1, 1033]), None)
        return (pe.data[resource['offset']:resource['offset'] + resource['size']]
                if resource else None)

    return dict(
        original_sha256=hashlib.sha256(target.data).hexdigest(),
        rebuilt_sha256=hashlib.sha256(candidate.data).hexdigest(),
        original_bytes=len(target.data), rebuilt_bytes=len(candidate.data),
        full_dll_byte_exact=target.data == candidate.data,
        export_names_and_ordinals_exact=export_signature(target) == export_signature(candidate),
        manifest_byte_exact=manifest(target) == manifest(candidate),
        register_name_tables=tables,
        read_word_host_field_offsets=dict(original=original_fields, rebuilt=rebuilt_fields,
                                         exact=original_fields == rebuilt_fields),
        forms=forms, application_blocks=blocks,
        address_masked_matches=sum(b['status'] == 'address-masked match' for b in blocks),
        missing_units=sorted(units_before - units_after),
        extra_units=sorted(units_after - units_before),
        limitations=[
            'Code block lengths come from consecutive symbols in the rebuilt MAP; '
            'blocks can include literals, alignment, and unit finalization code.',
            'HIGHLOW relocation operands and external rel32 call/jump operands are masked. '
            'Their destinations are not validated by a masked match.',
            'Only complete-file equality meets the byte-exact reconstruction criterion.'
        ])


def summarize(result):
    exact_forms = sum(f['resource_exact'] for f in result['forms'])
    print(f"Form resources: {exact_forms}/{len(result['forms'])} byte exact")
    print(f"Application blocks: {result['address_masked_matches']}/"
          f"{len(result['application_blocks'])} address-masked matches")
    for block in result['application_blocks']:
        if block['status'] != 'address-masked match':
            print(f"  {block['name']}: {block['status']}")
    print(f"Complete DLL byte exact: {result['full_dll_byte_exact']}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--map', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = compare(args.dll, args.map or args.dll.with_suffix('.map'))
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + '\n')
    summarize(result)
    return 0 if result['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
