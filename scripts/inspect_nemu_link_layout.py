#!/usr/bin/env python3
"""Diagnose native/import placement without masking or modifying an executable.

Function address agreement is a layout observation, not a substitute for the
independent library-body evidence or the unmasked complete-file comparison.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

from inspect_gs_symbols import imported_symbols
from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]


def inspect(executable, map_file, inventory, chronology, original=ROOT / 'master/nemu64.exe'):
    target, candidate = PE32(original), PE32(executable)
    if hashlib.sha256(target.data).hexdigest() != chronology['original_sha256']:
        raise ValueError('Chronology refers to a different original executable')
    addresses = {}
    for match in re.finditer(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s',
                             map_file.read_text(), re.M):
        addresses.setdefault(match[1], set()).add(int(match[2], 16))
    symbols = {name: next(iter(values)) for name, values in addresses.items() if len(values) == 1}
    functions = inventory['symbols']
    missing = [row['name'] for row in functions if row['name'] not in symbols]
    deltas = Counter(symbols[row['name']] - row['va'] for row in functions if row['name'] in symbols)
    present = sorted((row for row in functions if row['name'] in symbols), key=lambda row: row['va'])
    inversions = sum(symbols[a['name']] > symbols[b['name']]
                     for a, b in zip(present, present[1:]))
    observed = [row for row in inventory['observed_data_symbols'] if row['va'] >= 0x479950]
    available = [row for row in observed if row['name'] in symbols]
    storage_mismatches = [dict(name=row['name'], original_va=row['va'], rebuilt_va=symbols[row['name']])
                          for row in available if row['va'] != symbols[row['name']]]
    wanted = {(row['dll'].lower(), row['name']): row['iat_va'] for row in imported_symbols(target)}
    actual = {(row['dll'].lower(), row['name']): row['iat_va'] for row in imported_symbols(candidate)}
    excluded = {(row['dll'], row['name']) for row in chronology['remaining_insertion_positions']}
    relative = lambda rows: [key for key in sorted(rows, key=rows.get) if key not in excluded]
    ranges = []
    for row in chronology['comparison_ranges']:
        start, end = row['rva_start'], row['rva_end']
        def read(pe):
            offset = pe.offset(start)
            if pe.offset(end - 1) != offset + end - start - 1:
                raise ValueError('Comparison range crosses a noncontiguous file mapping')
            return pe.data[offset:offset + end - start]
        a, b = read(target), read(candidate)
        ranges.append(dict(row, bytes=end - start, equal=a == b,
                           differing_bytes=sum(x != y for x, y in zip(a, b))))
    # setmode's selected native section is 119 bytes, with alignment 1. The
    # following two original empty alias sections create the intervening gap.
    setmode = next(row for row in functions if row['name'] == '__setmode')
    gap = dict(original_start=0x42dc95, original_end=0x42dca0)
    if '__setmode' in symbols and '_RtlUnwind@16' in symbols:
        gap.update(rebuilt_start=symbols['__setmode'] + setmode['size'],
                   rebuilt_end=symbols['_RtlUnwind@16'])
        def contents(pe, start, end):
            if end < start or end - start > 64:
                return None
            offset = pe.offset(start - pe.base)
            return pe.data[offset:offset + end - start]
        a = contents(target, gap['original_start'], gap['original_end'])
        b = contents(candidate, gap['rebuilt_start'], gap['rebuilt_end'])
        gap.update(bytes_equal=a == b, original_bytes=len(a),
                   rebuilt_bytes=len(b) if b is not None else None)
    rich = lambda pe: {row['comp_id']: row['count'] for row in pe.rich()['records']}
    return dict(executable_sha256=hashlib.sha256(candidate.data).hexdigest(),
        native_functions=dict(expected=len(functions), present=len(functions) - len(missing),
            missing_or_ambiguous=missing, address_delta_counts=dict(sorted(deltas.items())),
            uniform_displacement=not missing and len(deltas) == 1, order_inversions=inversions,
            qualification='Placement only; independent library-body evidence is in the inventory.'),
        observed_native_storage=dict(observations=len(observed), map_symbols=len(available),
            exact=len(available) - len(storage_mismatches), mismatches=storage_mismatches,
            unavailable=[row['name'] for row in observed if row['name'] not in symbols],
            qualification='Library-operand observations, not independently proved original identities; includes initialized storage and BSS.'),
        imports=dict(expected_count=len(wanted), actual_count=len(actual),
            identity_sets_equal=set(wanted) == set(actual),
            exact_iat_slots=sum(actual.get(key) == value for key, value in wanted.items()),
            names_and_slots_equal=wanted == actual,
            relative_order_excluding_recorded_insertions_equal=relative(wanted) == relative(actual),
            relative_order_count=len(relative(wanted)),
            excluded_insertion_positions=chronology['remaining_insertion_positions'],
            ranges=ranges), alias_alignment=gap,
        rich_counts=dict(original=rich(target), rebuilt=rich(candidate)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('map', type=Path)
    parser.add_argument('--inventory', type=Path, default=ROOT / 'analysis/nemu-library-addresses.json')
    parser.add_argument('--chronology', type=Path, default=ROOT / 'src/nemu64/imports/link-chronology.json')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = inspect(args.exe, args.map, json.loads(args.inventory.read_text()),
                     json.loads(args.chronology.read_text()))
    serialized = json.dumps(result, indent=2) + '\n'
    if args.report:
        args.report.write_text(serialized)
    else:
        print(serialized, end='')


if __name__ == '__main__':
    main()
