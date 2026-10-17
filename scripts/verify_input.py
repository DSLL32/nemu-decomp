#!/usr/bin/env python3
"""Compare an input rebuild with the complete original DLL, without masking.

Per-routine address-masked diagnostics help locate source differences while
addresses are still moving. Only complete-file equality is success.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re

from compare_delphi_rtl import relocations
from inspect_delphi_forms import inspect
from pe_fingerprints import PE32
from verify_dialogs import normalized

ROOT = Path(__file__).resolve().parents[1]
UNIT_MAP = ROOT / 'analysis/input-unit-map.json'


def unit_ranges():
    """Original unit boundaries inferred from VMTs and initialization entries."""
    if not UNIT_MAP.exists():
        return {}
    return {u['unit']: (u['code_start_rva'], u['code_length'])
            for u in json.loads(UNIT_MAP.read_text())['units']}



def differences(a, b):
    return sum(x != y for x, y in zip(a, b)) + abs(len(a) - len(b))


def compare(rebuilt, map_path=None):
    original = ROOT / 'master/Plugin/Input.dll'
    target, candidate = PE32(original), PE32(rebuilt)
    result = dict(original_sha256=hashlib.sha256(target.data).hexdigest(),
                  rebuilt_sha256=hashlib.sha256(candidate.data).hexdigest(),
                  original_bytes=len(target.data), rebuilt_bytes=len(candidate.data),
                  full_dll_byte_exact=target.data == candidate.data,
                  differing_file_bytes=differences(target.data, candidate.data))
    candidate_sections = {s['name']: s for s in candidate.sections}
    result['sections'] = []
    for section in target.sections:
        other = candidate_sections.get(section['name'])
        a = target.data[section['offset']:section['offset'] + section['raw_size']]
        b = (candidate.data[other['offset']:other['offset'] + other['raw_size']]
             if other else b'')
        result['sections'].append(dict(name=section['name'], original=section,
                                       rebuilt=other, raw_bytes_equal=bool(other and a == b),
                                       differing_bytes=differences(a, b)))
    candidate_resources = {tuple(r['path']): r for r in candidate.resources()}
    result['resources'] = []
    for resource in target.resources():
        other = candidate_resources.get(tuple(resource['path']))
        a = target.data[resource['offset']:resource['offset'] + resource['size']]
        b = (candidate.data[other['offset']:other['offset'] + other['size']]
             if other else b'')
        result['resources'].append(dict(path=resource['path'], original_bytes=len(a),
                                        rebuilt_bytes=len(b), byte_exact=bool(other and a == b),
                                        differing_bytes=differences(a, b)))
    result['exports_exact'] = target.exports() == candidate.exports()
    result['forms'] = []
    before, after = inspect(original), inspect(rebuilt)
    after_forms = {f['name']: f for f in after['forms']}
    for form in before['forms']:
        other = after_forms.get(form['name'])
        result['forms'].append(dict(name=form['name'],
            resource_exact=bool(other and form['sha256'] == other['sha256']),
            fields_exact=bool(other and form['fields'] == other['fields']),
            instance_size_exact=bool(other and form['instance_size'] == other['instance_size']),
            method_names_exact=bool(other and [m['name'] for m in form['methods']] ==
                                   [m['name'] for m in other['methods']])))
    result['application_blocks'], result['units'] = [], []
    if map_path and map_path.exists():
        text = map_path.read_text()
        mapped = {name: int(offset, 16) + 0x1000 for offset, name in re.findall(
            r'^ 0001:([0-9A-F]{8})\s+(\S+)\s*$', text, re.M)}
        starts = sorted(set(mapped.values()))
        tf, cf = relocations(target), relocations(candidate)
        symbols = {f['name'] + '.' + m['name']: m['va'] - target.base
                   for f in before['forms'] for m in f['methods']}
        symbols.update({e['name']: e['rva'] for e in target.exports()})
        for name, start in sorted(symbols.items(), key=lambda item: item[1]):
            rebuilt_start = mapped.get(name)
            row = dict(name=name, original_rva=start, rebuilt_rva=rebuilt_start)
            following = next((x for x in starts if rebuilt_start is not None and
                              x > rebuilt_start), None)
            if following:
                size = following - rebuilt_start
                a = target.data[target.offset(start):target.offset(start) + size]
                b = candidate.data[candidate.offset(rebuilt_start):candidate.offset(rebuilt_start) + size]
                na, am = normalized(target, start, size, tf)
                nb, bm = normalized(candidate, rebuilt_start, size, cf)
                row.update(compared_bytes=size, raw_bytes_equal=a == b,
                           address_masked_equal=na == nb and am == bm,
                           masked_differing_bytes=differences(na, nb))
            result['application_blocks'].append(row)
        segments = {name: (int(offset, 16) + 0x1000, int(size, 16))
                    for offset, size, name in re.findall(
                        r'^ 0001:([0-9A-F]+) ([0-9A-F]+) C=CODE.*?M=(\w+)', text, re.M)}
        for name, (start, size) in unit_ranges().items():
            other = segments.get(name)
            a = target.data[target.offset(start):target.offset(start) + size]
            b = (candidate.data[candidate.offset(other[0]):candidate.offset(other[0]) + other[1]]
                 if other else b'')
            result['units'].append(dict(name=name, original_rva=start, original_bytes=size,
                                        rebuilt_rva=other[0] if other else None,
                                        rebuilt_bytes=other[1] if other else None,
                                        layout_equal=bool(other and other == (start, size)),
                                        raw_bytes_equal=bool(other and a == b),
                                        differing_bytes=differences(a, b)))
    return result


def summarize(result):
    blocks = result['application_blocks']
    print(f"Forms: {sum(f['resource_exact'] for f in result['forms'])}/{len(result['forms'])} exact resources")
    print(f"Application blocks: {sum(b.get('raw_bytes_equal', False) for b in blocks)}/{len(blocks)} raw, "
          f"{sum(b.get('address_masked_equal', False) for b in blocks)}/{len(blocks)} address-masked matches")
    units = result['units']
    print(f"Units: {sum(u['layout_equal'] for u in units)}/{len(units)} exact layouts, "
          f"{sum(u['raw_bytes_equal'] for u in units)}/{len(units)} exact bytes")
    print(f"Differing file bytes: {result['differing_file_bytes']}")
    print(f"Complete DLL byte exact: {result['full_dll_byte_exact']}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--map', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = compare(args.dll, args.map or args.dll.with_suffix('.map'))
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + '\n')
    summarize(result)
    raise SystemExit(0 if result['full_dll_byte_exact'] else 1)
