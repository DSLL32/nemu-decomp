#!/usr/bin/env python3
"""Recover a VC7 linker order file from named original functions and a MAP.

This selects complete compiler-generated functions; it does not edit their
bytes or assign absolute addresses. Original private names are inferred in the
owner inventories. Missing names and ambiguous folded functions remain visible
in the report. The order file is a reconstruction of observed image order, not
evidence that the original project itself used /ORDER.
"""

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path

from compare_nemu_functions import aliases, integer, read_map

ROOT = Path(__file__).resolve().parents[1]
MODULES = (
    'rsp', 'dynarec', 'cpu', 'cpu-control', 'dynarec-opcodes',
    'dynarec-memory', 'dynarec-fpu', 'memory', 'plugins', 'frontend',
    'browser', 'pif',
)


def original_functions(paths, libraries):
    result = []
    for path in paths:
        document = json.loads(path.read_text())
        keys = ('symbols', 'folded_helpers') if path in libraries else ('functions',)
        for key in keys:
            entries = document.get(key, [])
            if isinstance(entries, dict):
                entries = [dict(value, name=name) for name, value in entries.items()]
            for entry in entries:
                name = entry.get('name', entry.get('symbol'))
                va = entry.get('original_va', entry.get('va', entry.get('address')))
                if name and va is not None and not entry.get('comparison_only', False):
                    result.append(dict(name=name, va=integer(va), metadata=str(path)))
    return result


def make_order(map_path, metadata, libraries):
    candidates = [row for row in read_map(map_path) if row['kind'] == 'function']
    names = defaultdict(list)
    for candidate in candidates:
        for alias in aliases(candidate['name']):
            names[alias].append(candidate)
    selected, missing, ambiguous = {}, [], []
    for original in original_functions(metadata, libraries):
        matches = names.get(original['name'], [])
        if not matches:
            matches = [row for alias in aliases(original['name']) for row in names[alias]]
        matches = {row['name']: row for row in matches}.values()
        if not matches:
            missing.append(original)
            continue
        addresses = {row['va'] for row in matches}
        if len(addresses) != 1:
            ambiguous.append(dict(original, candidates=list(matches)))
            continue
        for match in matches:
            row = dict(original, symbol=match['name'], candidate_va=match['va'],
                       object=match['object'])
            old = selected.get(match['name'])
            if old and old['va'] != row['va']:
                ambiguous.append(dict(symbol=match['name'], originals=[old, row]))
            else:
                selected[match['name']] = row
    ordered = sorted(selected.values(), key=lambda row: (row['va'], row['symbol']))
    # VC7 adds the leading underscore to ordinary C/stdcall names. Decorated
    # C++ names beginning with '?' are accepted verbatim.
    lines = [row['symbol'][1:] if row['symbol'].startswith('_') else row['symbol']
             for row in ordered]
    selected_names = set(selected)
    return '\n'.join(lines) + '\n', dict(
        kind='observed_function_order', map=str(map_path),
        map_sha256=hashlib.sha256(map_path.read_bytes()).hexdigest(),
        metadata_sha256={str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                         for path in metadata},
        scope='Linker function ordering only; no instruction or full-image equality claim.',
        functions=ordered, missing=missing, ambiguous=ambiguous,
        unassigned_candidates=[row for row in candidates if row['name'] not in selected_names],
    )


def verify_order(report, map_path):
    """Check the result of /ORDER independently of input-file generation."""
    symbols = defaultdict(set)
    for row in read_map(map_path):
        if row['kind'] == 'function':
            symbols[row['name']].add(row['va'])
    missing, ambiguous, observed = [], [], []
    for row in report['functions']:
        addresses = symbols[row['symbol']]
        if not addresses:
            missing.append(row['symbol'])
        elif len(addresses) != 1:
            ambiguous.append(dict(symbol=row['symbol'], addresses=sorted(addresses)))
        else:
            observed.append(dict(original_va=row['va'], symbol=row['symbol'],
                                 candidate_va=next(iter(addresses))))
    groups = defaultdict(list)
    for row in observed:
        groups[row['original_va']].append(row)
    folded_disagreements, inversions = [], []
    previous = None
    for original_va, rows in sorted(groups.items()):
        addresses = {row['candidate_va'] for row in rows}
        if len(addresses) != 1:
            folded_disagreements.append(rows)
        low, high = min(addresses), max(addresses)
        if previous is not None and low <= previous['highest_candidate_va']:
            inversions.append(dict(previous=previous, current=rows))
        previous = dict(original_va=original_va, highest_candidate_va=high,
                        symbols=[row['symbol'] for row in rows])
    return dict(map=str(map_path),
                map_sha256=hashlib.sha256(map_path.read_bytes()).hexdigest(),
                observed_symbols=len(observed), original_entries=len(groups),
                missing=missing, ambiguous=ambiguous,
                folded_disagreements=folded_disagreements, inversions=inversions,
                passed=not (missing or ambiguous or folded_disagreements or inversions))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('map', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--metadata', type=Path, action='append')
    parser.add_argument('--include-libraries', action='store_true')
    parser.add_argument('--verify-map', type=Path,
                        help='Check function ordering in a subsequent linked MAP.')
    args = parser.parse_args()
    paths = args.metadata or [ROOT / 'analysis' / ('nemu-' + name + '.json')
                              for name in MODULES]
    libraries = ([ROOT / 'analysis/nemu-zlib-addresses.json',
                  ROOT / 'analysis/nemu-library-addresses.json']
                 if args.include_libraries else [])
    order, report = make_order(args.map, paths + libraries, libraries)
    invalid = bool(report['missing'] or report['ambiguous'])
    if not invalid:
        args.output.write_text(order)
    if args.verify_map:
        report['verification'] = verify_order(report, args.verify_map)
        invalid |= not report['verification']['passed']
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(report["functions"])} ordered symbols; '
          f'{len(report["missing"])} missing; {len(report["ambiguous"])} ambiguous; '
          f'{len(report["unassigned_candidates"])} candidate symbols unassigned')
    if args.verify_map:
        print('Linked order: ' + ('PASS' if report['verification']['passed'] else 'FAIL'))
    return invalid


if __name__ == '__main__':
    raise SystemExit(main())
