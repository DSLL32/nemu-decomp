#!/usr/bin/env python3
"""Compare literal placement without treating content matches as TU identities.

The original has no linker MAP. Candidate MAP literal names encode their byte
lengths, allowing complete-string matching, including embedded null bytes.
Ambiguous occurrences remain ambiguous. This tool never changes source or PE data.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]
PUBLIC = re.compile(r'^\s+([0-9a-f]+):[0-9a-f]+\s+(\S+)\s+([0-9a-f]+)\s+(.*?)\s*$', re.M)
LITERAL = re.compile(r'\?\?_C@_([01])([0-9]|[A-P]+@)')


def publics(path):
    return {m[2]: dict(va=int(m[3], 16), owner=m[4], section=int(m[1], 16))
            for m in PUBLIC.finditer(path.read_text(errors='replace'))}


def occurrences(data, value):
    cursor = 0
    while True:
        pos = data.find(value, cursor)
        if pos < 0:
            break
        yield pos
        cursor = pos + 1


def owner_runs(rows, address):
    result = []
    for row in sorted(rows, key=lambda r: r[address]):
        if result and result[-1]['candidate_owner'] == row['owner']:
            result[-1]['end_exclusive'] = hex(row[address] + row['size'])
            result[-1]['literals'] += 1
        else:
            result.append(dict(candidate_owner=row['owner'], start=hex(row[address]),
                               end_exclusive=hex(row[address] + row['size']), literals=1))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('map_file', type=Path)
    parser.add_argument('--variant', nargs=3, action='append', default=[],
                        metavar=('NAME', 'EXE', 'MAP'))
    parser.add_argument('--output', type=Path,
                        default=ROOT / 'analysis/nemu-rdata-order.json')
    args = parser.parse_args()
    original = PE32(ROOT / 'master/nemu64.exe')
    candidate = PE32(args.candidate)
    sec = next(s for s in original.sections if s['name'] == '.rdata')
    start = original.base + sec['rva']
    data = original.data[sec['offset']:sec['offset'] + sec['virtual_size']]
    vendor = json.loads((ROOT / 'analysis/nemu-vendor-data.json').read_text())
    vendor_names = {r['object'] for r in vendor['data_sections']}
    rows = []
    for symbol, entry in publics(args.map_file).items():
        match = LITERAL.match(symbol)
        if not match or entry['section'] != 2:
            continue
        length = match[2]
        size = int(length) + 1 if length.isdigit() else int(
            ''.join(format(ord(c) - ord('A'), 'x') for c in length[:-1]), 16)
        off = candidate.offset(entry['va'] - candidate.base)
        value = candidate.data[off:off + size]
        positions = [start + p for p in occurrences(data, value)]
        row = dict(symbol=symbol, owner=entry['owner'], candidate_va=entry['va'], size=size,
                   value_sha256=hashlib.sha256(value).hexdigest(),
                   text_preview=value[:120].decode('latin1'),
                   original_occurrence_count=len(positions),
                   original_occurrences=[hex(p) for p in positions[:32]],
                   application=not entry['owner'].startswith('LIBC:') and entry['owner'] not in vendor_names)
        if len(positions) == 1:
            row['original_va'] = positions[0]
        rows.append(row)
    unique = [r for r in rows if 'original_va' in r]
    app = [r for r in unique if r['application']]
    fixed_objects = [('BootRspImem', 0x4348E0, 4096), ('DefaultMemPack', 0x4358E0, 32768)]
    floating_objects = [(0x43E01C, 4), (0x43E020, 8), (0x43E028, 8),
                        (0x43E030, 8), (0x43E038, 4), (0x43E03C, 4),
                        (0x43E040, 4), (0x43E044, 4), (0x43E048, 4),
                        (0x43E084, 4), (0x43E088, 4)]
    variants = []
    for name, exe_path, map_path in [('baseline', args.candidate, args.map_file), *args.variant]:
        exe_path, map_path = Path(exe_path), Path(map_path)
        pe, symbols = PE32(exe_path), publics(map_path)
        readonly = next(s for s in pe.sections if s['name'] == '.rdata')
        contents = pe.data[readonly['offset']:readonly['offset']+readonly['virtual_size']]
        other_objects = []
        for label, va, size in fixed_objects:
            value = data[va-start:va-start+size]
            positions = [pe.base+readonly['rva']+p for p in occurrences(contents, value)]
            other_objects.append(dict(name=label, original_va=hex(va), size=size,
                                      complete_byte_matches=[hex(p) for p in positions]))
        for va, size in floating_objects:
            value = data[va-start:va-start+size]
            symbol = '__real@' + value[::-1].hex()
            cv = symbols[symbol]['va']
            off = pe.offset(cv-pe.base)
            assert pe.data[off:off+size] == value
            other_objects.append(dict(name=symbol, original_va=hex(va), size=size,
                                      candidate_va=hex(cv), delta=cv-va, complete_bytes_equal=True))
        comparisons = []
        for row in unique:
            if row['symbol'] not in symbols:
                continue
            va = symbols[row['symbol']]['va']
            off = pe.offset(va - pe.base)
            assert hashlib.sha256(pe.data[off:off + row['size']]).hexdigest() == row['value_sha256']
            comparisons.append(dict(symbol=row['symbol'], owner=row['owner'], size=row['size'],
                                    application=row['application'], original_va=row['original_va'],
                                    candidate_va=va, delta=va-row['original_va']))
        owners = []
        for owner in dict.fromkeys(r['owner'] for r in comparisons):
            group = sorted([r for r in comparisons if r['owner'] == owner], key=lambda r: r['original_va'])
            ranks = [r['candidate_va'] for r in group]
            owners.append(dict(owner=owner, unique_literals=len(group),
                               exact_positions=sum(r['delta'] == 0 for r in group),
                               order_inversions=sum(x > y for i, x in enumerate(ranks) for y in ranks[i+1:]),
                               deltas=[dict(delta=d, literals=n) for d, n in Counter(r['delta'] for r in group).most_common()]))
        variants.append(dict(name=name, exe=str(exe_path), map=str(map_path),
            exe_sha256=hashlib.sha256(pe.data).hexdigest(), map_sha256=hashlib.sha256(map_path.read_bytes()).hexdigest(),
            exact_literal_positions=sum(r['delta'] == 0 for r in comparisons),
            exact_application_literal_positions=sum(r['delta'] == 0 and r['application'] for r in comparisons),
            compared_unique_literals=len(comparisons), owner_summary=owners,
            application_literal_runs=owner_runs([r for r in comparisons if r['application']], 'candidate_va'),
            rich_cpp_ltcg_records=[r for r in pe.rich()['records'] if r['comp_id'] == '002c24fa'],
            other_readonly_objects=other_objects,
            rdata=next(s for s in pe.sections if s['name'] == '.rdata')))
    rtti = []
    for section in original.sections:
        if section['name'] not in ('.rdata', '.data'):
            continue
        content = original.data[section['offset']:section['offset']+min(section['virtual_size'], section['raw_size'])]
        for match in re.finditer(rb'\.\?A[UV][ -~]{1,100}?@@\x00', content):
            rtti.append(dict(va=hex(original.base + section['rva'] + match.start()), value=match[0].decode('ascii')))
    report = dict(original='master/nemu64.exe', original_sha256=hashlib.sha256(original.data).hexdigest(),
        scope='Complete literal contents and relative placement; neither source equality nor inferred TU identity',
        summary=dict(literal_symbols=len(rows), uniquely_located=len(unique),
                     ambiguous=sum(r['original_occurrence_count'] > 1 for r in rows),
                     absent=sum(r['original_occurrence_count'] == 0 for r in rows)),
        original_rich_cpp_ltcg_records=[r for r in original.rich()['records'] if r['comp_id'] == '002c24fa'],
        original_application_literal_runs=owner_runs(app, 'original_va'),
        observed_original_rtti_type_name_strings=rtti, variants=variants, literals=rows,
        limitations=['Candidate object ownership does not establish original source filenames or unit boundaries.',
                     'Twenty-seven original C++ LTCG contributions do not identify their filenames, contents or ordering.',
                     'Shared strings can acquire a different owner when input order changes.',
                     'RTTI string absence is an observation, not proof that all original C++ type metadata was disabled.',
                     'Boot/mempak arrays, floating pools, compiler exception tables and native library data require separate placement evidence.'])
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(summary=report['summary'], variants=[{k:r[k] for k in
        ('name','exact_literal_positions','exact_application_literal_positions','compared_unique_literals')} for r in variants]), indent=2))


if __name__ == '__main__':
    main()
