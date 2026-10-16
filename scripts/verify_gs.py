#!/usr/bin/env python3
"""Verify gs_dll.dll, or diagnose VC6 COFF application objects against it.

Whole-file equality is the only complete-DLL success criterion. Object
comparisons are diagnostics: recorded COFF relocation fields may be masked,
but the report separately checks their target identities where symbols permit.
No image slack, debug metadata, or executable bytes are omitted from the
whole-file comparison.
"""

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import struct

from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'master/gs_dll.dll'
SYMBOLS = ROOT / 'analysis/gs-addresses.json'
RELOCATION_TYPES = {0: ('ABSOLUTE', 0), 1: ('DIR16', 2),
                    2: ('REL16', 2), 6: ('DIR32', 4),
                    7: ('DIR32NB', 4), 10: ('SECTION', 2),
                    11: ('SECREL', 4), 20: ('REL32', 4)}


def differences(a, b):
    return sum(x != y for x, y in zip(a, b)) + abs(len(a) - len(b))


def number(value):
    return int(value, 0) if isinstance(value, str) else value


def basename(value):
    return str(value).replace('\\', '/').rsplit('/', 1)[-1].lower()


class COFF:
    """Read ordinary i386 COFF objects, retaining auxiliary symbol records."""

    def __init__(self, path):
        self.path = Path(path)
        self.data = data = self.path.read_bytes()
        (self.machine, count, self.timestamp, table, symbol_count,
         optional_size, self.characteristics) = struct.unpack_from('<HHIIIHH', data)
        if self.machine != 0x14c or optional_size:
            raise ValueError(f'{path}: expected an i386 COFF object')
        strings = table + 18 * symbol_count
        string_size = struct.unpack_from('<I', data, strings)[0]

        def string(offset):
            if not 4 <= offset < string_size:
                raise ValueError(f'{path}: invalid COFF string offset {offset}')
            return data[strings + offset:data.index(0, strings + offset)].decode('cp1252')

        self.sections = []
        for index in range(count):
            fields = struct.unpack_from('<8sIIIIIIHHI', data, 20 + 40 * index)
            raw_name, virtual_size, address, size, raw, rel, line, nr, nl, flags = fields
            name = raw_name.rstrip(b'\0').decode('ascii')
            if name.startswith('/'):
                name = string(int(name[1:]))
            section = dict(index=index + 1, name=name, virtual_size=virtual_size,
                           address=address, size=size, offset=raw,
                           characteristics=flags, relocations=[])
            section['data'] = data[raw:raw + size] if raw else bytes(size)
            if nr == 0xffff and flags & 0x01000000:
                nr = struct.unpack_from('<I', data, rel)[0] - 1
                rel += 10
            for index2 in range(nr):
                offset, symbol, kind = struct.unpack_from('<IIH', data, rel + index2 * 10)
                section['relocations'].append(dict(offset=offset, symbol_index=symbol,
                                                   type=kind))
            self.sections.append(section)
        self.symbols = []
        self.symbol_by_index = {}
        index = 0
        while index < symbol_count:
            pos = table + 18 * index
            raw_name = data[pos:pos + 8]
            name = (string(struct.unpack_from('<I', raw_name, 4)[0])
                    if raw_name[:4] == bytes(4)
                    else raw_name.rstrip(b'\0').decode('cp1252'))
            value, section, kind, storage, aux_count = struct.unpack_from('<IhHBB', data, pos + 8)
            auxiliary = [data[pos + 18 * (i + 1):pos + 18 * (i + 2)]
                         for i in range(aux_count)]
            symbol = dict(index=index, name=name, value=value, section=section,
                          type=kind, storage_class=storage, auxiliary=auxiliary)
            if kind == 0x20 and storage == 2 and section > 0 and auxiliary:
                symbol['size'] = struct.unpack_from('<I', auxiliary[0], 4)[0]
            self.symbols.append(symbol)
            self.symbol_by_index[index] = symbol
            index += 1 + aux_count
        for section in self.sections:
            for relocation in section['relocations']:
                symbol = self.symbol_by_index.get(relocation['symbol_index'])
                if symbol is None:
                    raise ValueError(f'{path}: relocation references an auxiliary/missing symbol')
                relocation['symbol'] = symbol

    def functions(self):
        functions = [s for s in self.symbols if s['type'] == 0x20 and s['section'] > 0]
        for symbol in functions:
            section = self.sections[symbol['section'] - 1]
            following = [s['value'] for s in functions
                         if s['section'] == symbol['section'] and s['value'] > symbol['value']]
            end = min(following + [section['size']])
            size = symbol.get('size') or end - symbol['value']
            yield dict(symbol, size=size, data=section['data'][symbol['value']:symbol['value'] + size],
                       relocations=[dict(r, offset=r['offset'] - symbol['value'])
                                    for r in section['relocations']
                                    if symbol['value'] <= r['offset'] < symbol['value'] + size])


class TargetSymbols:
    def __init__(self, image, path=None, rows=None, library_path=None):
        self.image = image
        self.by_name = defaultdict(list)
        self.by_va = defaultdict(list)
        self.thunks = {}
        # VC6's incremental jump table starts after five INT3 bytes. Limit
        # equivalence to this recorded table; an arbitrary function beginning
        # with JMP must not silently acquire its destination's identity.
        text = next((s for s in image.sections if s['name'] == '.text'), None)
        if text:
            pos, va = text['offset'], image.base + text['rva']
            if image.data[pos:pos + 5] == b'\xcc' * 5:
                pos, va = pos + 5, va + 5
                while image.data[pos:pos + 1] == b'\xe9':
                    self.thunks[va] = va + 5 + struct.unpack_from('<i', image.data, pos + 1)[0]
                    pos, va = pos + 5, va + 5
        if rows is None:
            document = json.loads(Path(path or SYMBOLS).read_text())
            if isinstance(document, list):
                rows = document
            elif 'symbols' in document:
                rows = document['symbols']
            else:
                rows = [dict(name=name, va=address) for name, address in document.items()]
            supplementary = Path(library_path or SYMBOLS.with_name('gs-library-addresses.json'))
            if Path(path or SYMBOLS).resolve() == SYMBOLS and supplementary.exists():
                library = json.loads(supplementary.read_text())
                if library.get('original_sha256') == hashlib.sha256(image.data).hexdigest():
                    for r in library['symbols']:
                        kind = r.get('match_kind', '')
                        independent = kind.startswith('unique_complete') or ':' in r.get('object', '')
                        rows.append(dict(r,
                            address_basis=('independent_library_mapping' if independent else
                                           'observed_operand_inference'),
                            address_evidence=('Address corroborated by ' if independent else
                                'Inferred reconstructed application name at an address mapped by ') +
                                kind + '; see analysis/gs-library-addresses.json.'))
        for row in rows:
            row = dict(row)
            if 'name' not in row:
                row['name'] = row.get('decorated_name', row.get('symbol'))
            if not row.get('name'):
                continue
            va = row.get('va', row.get('address'))
            if va is None and row.get('rva') is not None:
                va = image.base + number(row['rva'])
            if va is None:
                continue
            row['va'] = number(va)
            if row.get('size') is not None:
                row['size'] = number(row['size'])
            self.add(row)
        # IAT symbols are independently recoverable even when absent from the
        # debug symbols. The decorated import name includes __imp_ in COFF.
        rva, size = image.directory(1)
        if rva:
            pos = image.offset(rva)
            while struct.unpack_from('<I', image.data, pos + 12)[0]:
                lookup, _, _, _, iat = struct.unpack_from('<IIIII', image.data, pos)
                entries = image.offset(lookup or iat)
                index = 0
                while True:
                    value = struct.unpack_from('<I', image.data, entries + 4 * index)[0]
                    if not value:
                        break
                    if not value & 0x80000000:
                        off = image.offset(value) + 2
                        name = image.data[off:image.data.index(0, off)].decode('ascii')
                        for decorated in ('__imp_' + name, '__imp__' + name):
                            self.add(dict(name=decorated, va=image.base + iat + 4 * index,
                                          source='import_table'))
                    index += 1
                pos += 20

    def add(self, row):
        if 'address_basis' not in row:
            row['address_basis'] = ('import_table' if row.get('source') == 'import_table' else
                                    'observed_operand_inference' if row.get('address_evidence', '').startswith('Inferred')
                                    else 'identified_symbol')
        self.by_name[row['name']].append(row)
        self.by_va[row['va']].append(row)

    def candidates(self, name, object_name=None):
        rows = self.by_name.get(name, [])
        if not rows and name.startswith('__imp_') and '@' in name:
            undecorated, suffix = name.rsplit('@', 1)
            if suffix.isdigit():
                rows = self.by_name.get(undecorated, [])
        sizes = {r['va']: r['size'] for r in rows if r.get('size') is not None}
        if object_name:
            local = [r for r in rows if basename(r.get('object', r.get('module', ''))) == basename(object_name)]
            if local:
                rows = local
        # A reconstruction's observed call operand cannot establish the callee
        # name used in that reconstruction. Independently matched library
        # bodies override such circular observations, even when they disagree.
        ranks = {'observed_operand_inference': 0, 'identified_symbol': 1,
                 'import_table': 2, 'independent_library_mapping': 3}
        if rows:
            highest = max(ranks[r['address_basis']] for r in rows)
            discarded = [dict(va=r['va'], address_basis=r['address_basis']) for r in rows
                         if ranks[r['address_basis']] < highest]
            rows = [dict(r, lower_priority_observations=discarded) for r in rows
                    if ranks[r['address_basis']] == highest]
            rows = [dict(r, size=sizes[r['va']]) if r['va'] in sizes else r for r in rows]
        # Multiple identical debug records are not ambiguous.
        unique = {}
        for row in rows:
            if row['va'] not in unique:
                unique[row['va']] = dict(row)
            elif row.get('address_evidence'):
                entry = unique[row['va']]
                entry['address_evidence'] = '; '.join(filter(None, [
                    entry.get('address_evidence'), row['address_evidence']]))
        return list(unique.values())

    def follow_thunk(self, va):
        visited = set()
        while va in self.thunks and va not in visited:
            visited.add(va)
            va = self.thunks[va]
        return va

    def describe(self, va):
        names = [r['name'] for r in self.by_va.get(va, [])]
        final = self.follow_thunk(va)
        if final != va:
            names += [r['name'] for r in self.by_va.get(final, [])]
        return sorted(set(names))


def compare_function(function, target, symbols, object_name=None, original_va=None):
    candidates = symbols.candidates(function['name'], object_name)
    row = dict(name=function['name'], object=object_name, rebuilt_bytes=function['size'])
    if original_va is None:
        if len(candidates) != 1:
            row.update(status='missing_symbol' if not candidates else 'ambiguous_symbol',
                       original_candidates=[r['va'] for r in candidates])
            return row
        original_va = candidates[0]['va']
    original_va = symbols.follow_thunk(original_va)
    original_offset = target.offset(original_va - target.base)
    raw = function['data']
    original_size = candidates[0].get('size') if len(candidates) == 1 else None
    original = target.data[original_offset:original_offset + (original_size or len(raw))]
    masked_original, masked_rebuilt = bytearray(original), bytearray(raw)
    relocated = bytearray(raw)
    row.update(status='compared', original_va=original_va, raw_bytes_equal=original == raw,
               raw_differing_bytes=differences(original, raw), compared_bytes=len(raw),
               original_size_verified=False)
    if len(candidates) == 1 and candidates[0].get('size') is not None:
        row['original_bytes'] = candidates[0]['size']
        row['size_equal'] = row['original_bytes'] == len(raw)
        row['original_size_verified'] = True
    details = []
    all_resolved = True
    for relocation in function['relocations']:
        offset, kind = relocation['offset'], relocation['type']
        type_name, width = RELOCATION_TYPES.get(kind, (f'UNKNOWN_{kind}', 0))
        symbol = relocation['symbol']
        detail = dict(offset=offset, type=type_name, symbol=symbol['name'])
        details.append(detail)
        if not width or offset + width > min(len(raw), len(original)):
            detail['status'] = 'ignored_absolute' if kind == 0 else 'unsupported_relocation'
            all_resolved &= kind == 0
            continue
        # Only fields explicitly present in the COFF relocation table are
        # masked. Local branch displacements remain ordinary compared bytes.
        masked_original[offset:offset + width] = bytes(width)
        masked_rebuilt[offset:offset + width] = bytes(width)
        addend = int.from_bytes(raw[offset:offset + width], 'little')
        observed = int.from_bytes(original[offset:offset + width], 'little')
        expected = None
        resolution_basis = None
        targets = symbols.candidates(symbol['name'], object_name)
        detail['addend'] = addend
        if kind == 20:
            observed = (original_va + offset + 4 + observed) & 0xffffffff
        elif kind == 7:
            observed += target.base
        detail['original_target_va'] = observed if kind in (6, 7, 20) else None
        detail['original_target_symbols'] = symbols.describe(observed) if kind in (6, 7, 20) else []
        if kind in (6, 7, 20):
            if symbol['section'] == -1:
                expected = (symbol['value'] + addend) & 0xffffffff
                resolution_basis = 'absolute_coff_symbol'
            elif (symbol['section'] == function['section'] and
                  function['value'] <= symbol['value'] < function['value'] + function['size']):
                expected = (original_va + symbol['value'] - function['value'] + addend) & 0xffffffff
                resolution_basis = 'same_function_label'
            elif len(targets) == 1:
                expected = (targets[0]['va'] + addend) & 0xffffffff
                resolution_basis = targets[0]['address_basis']
        if expected is None:
            detail['status'] = 'unresolved'
            all_resolved = False
            continue
        detail['expected_target_va'] = expected
        detail['resolution_basis'] = resolution_basis
        if len(targets) == 1 and targets[0].get('address_evidence'):
            detail['target_address_evidence'] = targets[0]['address_evidence']
        # Incremental-link call thunks may legitimately differ from the body
        # symbol. Preserve the original thunk only after checking its endpoint.
        equal = observed == expected or (kind == 20 and
                symbols.follow_thunk(observed) == symbols.follow_thunk(expected))
        detail.update(status='resolved', target_equal=equal)
        detail['target_identity_verified'] = equal and resolution_basis != 'observed_operand_inference'
        if equal:
            relocated[offset:offset + width] = original[offset:offset + width]
        else:
            linked_value = expected
            if kind == 20:
                linked_value -= original_va + offset + 4
            elif kind == 7:
                linked_value -= target.base
            relocated[offset:offset + width] = (linked_value & 0xffffffff).to_bytes(4, 'little')
    row['relocations'] = details
    row['relocation_target_mismatch_count'] = sum(r.get('target_equal') is False for r in details)
    row['inferred_target_count'] = sum(r.get('resolution_basis') == 'observed_operand_inference' for r in details)
    row['unresolved_target_count'] = sum(r['status'] in ('unresolved', 'unsupported_relocation') for r in details)
    row['relocation_masked_equal'] = masked_original == masked_rebuilt and row.get('size_equal', True)
    row['relocation_masked_differing_bytes'] = differences(masked_original, masked_rebuilt)
    row['relocations_all_resolved'] = all_resolved
    row['relocation_values_consistent'] = all_resolved and all(
        r.get('target_equal', r['status'] == 'ignored_absolute') for r in details)
    row['relocation_targets_verified'] = all_resolved and all(
        r.get('target_identity_verified', r['status'] == 'ignored_absolute') for r in details)
    row['relocation_targets_equal'] = row['relocation_targets_verified']
    row['inferred_relocated_bytes_equal'] = all_resolved and original == relocated and row.get('size_equal', True)
    row['relocated_bytes_equal'] = row['inferred_relocated_bytes_equal'] and row['relocation_targets_verified']
    row['first_unmasked_differences'] = [i for i, (a, b) in enumerate(zip(masked_original, masked_rebuilt)) if a != b][:32]
    return row


def compare_object(path, original=ORIGINAL, symbols_path=SYMBOLS, object_name=None,
                   symbol_filter=None, original_va=None, library_path=None):
    obj = COFF(path)
    target = PE32(Path(original))
    symbols = TargetSymbols(target, symbols_path, library_path=library_path)
    functions = [f for f in obj.functions() if not symbol_filter or symbol_filter in f['name']]
    if original_va is not None and len(functions) != 1:
        raise ValueError('--va requires selecting exactly one function with --symbol')
    rows = [compare_function(f, target, symbols, object_name or Path(path).name, original_va)
            for f in functions]
    return dict(object=str(path), sha256=hashlib.sha256(obj.data).hexdigest(),
                original=str(original), functions=rows,
                functions_compared=sum(r['status'] == 'compared' for r in rows),
                relocation_masked_matches=sum(r.get('relocation_masked_equal', False) for r in rows),
                relocation_consistent_matches=sum(r.get('inferred_relocated_bytes_equal', False) for r in rows),
                relocation_checked_matches=sum(r.get('relocated_bytes_equal', False) for r in rows),
                relocation_target_mismatches=sum(r.get('relocation_target_mismatch_count', 0) for r in rows),
                inferred_targets=sum(r.get('inferred_target_count', 0) for r in rows),
                unresolved_targets=sum(r.get('unresolved_target_count', 0) for r in rows))


def compare_dll(path, original=ORIGINAL):
    a, b = PE32(Path(original)), PE32(Path(path))
    result = dict(original=str(original), rebuilt=str(path),
                  original_sha256=hashlib.sha256(a.data).hexdigest(),
                  rebuilt_sha256=hashlib.sha256(b.data).hexdigest(),
                  original_bytes=len(a.data), rebuilt_bytes=len(b.data),
                  full_dll_byte_exact=a.data == b.data,
                  differing_file_bytes=differences(a.data, b.data))
    sections = {s['name']: s for s in b.sections}
    result['sections'] = []
    for section in a.sections:
        other = sections.get(section['name'])
        x = a.data[section['offset']:section['offset'] + section['raw_size']]
        y = b.data[other['offset']:other['offset'] + other['raw_size']] if other else b''
        result['sections'].append(dict(name=section['name'], original=section, rebuilt=other,
                                       raw_bytes_equal=bool(other and x == y),
                                       differing_bytes=differences(x, y)))
    result['exports_exact'] = a.exports() == b.exports()
    return result


def summarize_objects(objects):
    functions = [f for obj in objects for f in obj['functions']]
    unique = {(f['name'], f['original_va']): f for f in functions if f['status'] == 'compared'}
    return dict(function_instances=len(functions), unique_functions=len(unique),
                unique_function_bytes=sum(f.get('original_bytes', f['compared_bytes']) for f in unique.values()),
                original_sizes_verified=all(f['original_size_verified'] for f in unique.values()),
                relocation_masked_matches=sum(obj['relocation_masked_matches'] for obj in objects),
                relocation_target_mismatches=sum(obj['relocation_target_mismatches'] for obj in objects),
                inferred_targets=sum(obj['inferred_targets'] for obj in objects),
                unresolved_targets=sum(obj['unresolved_targets'] for obj in objects))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', nargs='?', type=Path)
    parser.add_argument('--object', action='append', type=Path, default=[])
    parser.add_argument('--object-name', help='original module/object name for scoped symbols')
    parser.add_argument('--original', type=Path, default=ORIGINAL)
    parser.add_argument('--symbols', type=Path, default=SYMBOLS)
    parser.add_argument('--library-addresses', type=Path,
                        help='independently matched library address report (takes precedence over inferences)')
    parser.add_argument('--symbol', help='substring selecting object function names')
    parser.add_argument('--va', type=lambda s: int(s, 0), help='explicit original VA for one function')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if not args.dll and not args.object:
        parser.error('provide a rebuilt DLL and/or --object')
    result = compare_dll(args.dll, args.original) if args.dll else {}
    if args.object:
        result['objects'] = [compare_object(p, args.original, args.symbols, args.object_name,
                                            args.symbol, args.va, args.library_addresses) for p in args.object]
        result['object_summary'] = summarize_objects(result['objects'])
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + '\n')
    if args.dll:
        print(f"Complete DLL byte exact: {result['full_dll_byte_exact']}")
        print(f"Differing file bytes: {result['differing_file_bytes']}")
    for obj in result.get('objects', []):
        print(f"{obj['object']}: {obj['functions_compared']}/{len(obj['functions'])} functions located; "
              f"{obj['relocation_masked_matches']} relocation-masked matches; "
              f"{obj['relocation_checked_matches']} with independently identified matching relocation targets")
        print(f"  Targets: {obj['relocation_target_mismatches']} mismatches, "
              f"{obj['inferred_targets']} inferred only, {obj['unresolved_targets']} unresolved")
        for function in obj['functions']:
            if function.get('relocation_masked_equal') and not function.get('relocation_target_mismatch_count'):
                continue
            print(' ', function['name'], function['status'],
                  function.get('relocation_masked_differing_bytes', ''))
            for relocation in function.get('relocations', []):
                if relocation.get('target_equal') is False:
                    print(f"    +0x{relocation['offset']:x}: {relocation['symbol']} "
                          f"original 0x{relocation['original_target_va']:x}, "
                          f"expected 0x{relocation['expected_target_va']:x}")
    if args.dll:
        return 0 if result['full_dll_byte_exact'] else 1
    # Object diagnostics can succeed independently of full-DLL completion.
    return 0 if all(r.get('relocation_masked_equal', False) and not r.get('relocation_target_mismatch_count')
                    for obj in result['objects'] for r in obj['functions']) else 1


if __name__ == '__main__':
    raise SystemExit(main())
