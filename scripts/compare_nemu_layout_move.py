#!/usr/bin/env python3
"""Audit a source/layout-only relink without modifying either executable.

Code and writable-data public addresses must stay fixed. The only normalized
operands are PE HIGHLOW relocations to complete equal compiler literals, float
constants, or explicitly supplied equal readonly objects. This is a comparison
between two reconstructed builds, not a claim that either matches the original.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from inspect_nemu_rdata import LITERAL, publics
from pe_fingerprints import PE32


def relocation_rvas(pe):
    rva, size = pe.directory(5)
    if not size:
        raise ValueError('Both inputs need relocation directories')
    offset = pe.offset(rva)
    end = offset + size
    result = set()
    while offset < end:
        page, length = struct.unpack_from('<II', pe.data, offset)
        if length < 8 or offset + length > end:
            raise ValueError('Invalid relocation block')
        for pos in range(offset + 8, offset + length, 2):
            value = struct.unpack_from('<H', pe.data, pos)[0]
            if value >> 12 == 3:
                result.add(page + (value & 4095))
            elif value >> 12:
                raise ValueError('Unexpected non-HIGHLOW relocation')
        offset += length
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--baseline-map', type=Path)
    parser.add_argument('--candidate-map', type=Path)
    parser.add_argument('--anchor', nargs=4, action='append', default=[],
                        metavar=('NAME', 'BASELINE_VA', 'CANDIDATE_VA', 'SIZE'))
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    a, b = PE32(args.baseline), PE32(args.candidate)
    sa = publics(args.baseline_map or args.baseline.with_suffix('.map'))
    sb = publics(args.candidate_map or args.candidate.with_suffix('.map'))
    mapping, anchors = {}, []

    def anchor(name, old, new, size, basis):
        assert size > 0
        ao, bo = a.offset(old - a.base), b.offset(new - b.base)
        av, bv = a.data[ao:ao + size], b.data[bo:bo + size]
        assert len(av) == size and av == bv, name
        for delta in range(size):
            assert mapping.get(old + delta, new + delta) == new + delta, name
            mapping[old + delta] = new + delta
        anchors.append(dict(name=name, baseline_va=hex(old), candidate_va=hex(new),
                            size=size, sha256=hashlib.sha256(av).hexdigest(), basis=basis))

    fixed_symbols, unmapped_publics = [], []
    for name, row in sa.items():
        if row['section'] in (1, 3):
            fixed_symbols.append(name)
            assert name in sb and sb[name]['va'] == row['va'], name
        if name not in sb or row['va'] == sb[name]['va']:
            continue
        match = LITERAL.match(name)
        if match:
            length = match[2]
            size = int(length) + 1 if length.isdigit() else int(
                ''.join(format(ord(c) - 65, 'x') for c in length[:-1]), 16)
            anchor(name, row['va'], sb[name]['va'], size, 'compiler_named_literal')
        elif name.startswith('__real@'):
            anchor(name, row['va'], sb[name]['va'], len(name.split('@')[1]) // 2,
                   'compiler_named_floating_constant')
        elif name in ('___rtc_iaa', '___rtc_izz', '___rtc_taa', '___rtc_tzz'):
            anchor(name, row['va'], sb[name]['va'], 4, 'vc7_runtime_check_array_marker')
        else:
            # Zero-length linker boundary symbols can move with their group.
            # Do not use an unclassified symbol to normalize any operand.
            unmapped_publics.append(dict(name=name, baseline_va=hex(row['va']),
                                         candidate_va=hex(sb[name]['va'])))
    for name, old, new, size in args.anchor:
        anchor(name, int(old, 0), int(new, 0), int(size, 0), 'explicit_complete_readonly_object')

    ar, br = relocation_rvas(a), relocation_rvas(b)
    sections = []
    for section in a.sections:
        if section['name'] not in ('.text', '.data'):
            continue
        other = next(s for s in b.sections if s['name'] == section['name'])
        assert section == other, section['name']
        original = a.data[section['offset']:section['offset'] + section['raw_size']]
        adjusted = bytearray(original)
        candidate = b.data[other['offset']:other['offset'] + other['raw_size']]
        references = []
        for rva in sorted(ar & br):
            pos = rva - section['rva']
            if not 0 <= pos <= len(adjusted) - 4:
                continue
            old = struct.unpack_from('<I', original, pos)[0]
            new = struct.unpack_from('<I', candidate, pos)[0]
            if old != new and mapping.get(old) == new:
                references.append(dict(rva=hex(rva), baseline_target=hex(old), candidate_target=hex(new)))
                struct.pack_into('<I', adjusted, pos, new)
        differences = [i for i, (x, y) in enumerate(zip(adjusted, candidate)) if x != y]
        sections.append(dict(name=section['name'], compared_raw_bytes=len(original),
                             relocated_readonly_references=references,
                             residual_different_bytes=len(differences),
                             first_difference_rvas=[hex(section['rva'] + i) for i in differences[:20]]))
    report = dict(scope='Reconstructed-build layout invariance; no original-image equality claim.',
                  baseline=str(args.baseline), candidate=str(args.candidate),
                  baseline_sha256=hashlib.sha256(a.data).hexdigest(),
                  candidate_sha256=hashlib.sha256(b.data).hexdigest(),
                  fixed_code_and_data_publics=len(fixed_symbols), readonly_anchors=anchors,
                  moved_publics_not_used_for_normalization=unmapped_publics,
                  sections=sections, passed=all(r['residual_different_bytes'] == 0 for r in sections))
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], fixed_publics=len(fixed_symbols),
                         residual_bytes={r['name']: r['residual_different_bytes'] for r in sections})))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
