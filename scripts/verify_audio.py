#!/usr/bin/env python3
"""Strict whole-file audio.dll comparison and native COFF source diagnostics.

COFF relocation masking is diagnostic only. A complete DLL succeeds only when
all input file bytes, including metadata and alignment, are exactly reproduced.
"""
import argparse
import hashlib
import json
from pathlib import Path

from pe_fingerprints import PE32
from verify_gs import COFF, TargetSymbols, compare_function, compare_dll

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'master/Plugin/audio.dll'


def load_symbols(directory=None):
    directory = Path(directory) if directory is not None else ROOT / 'analysis'
    result = []
    for path in sorted(directory.glob('audio*addresses.json')):
        document = json.loads(path.read_text())
        result.extend(document if isinstance(document, list) else document.get('symbols', []))
    library = directory / 'audio-libraries.json'
    if library.exists():
        result.extend(json.loads(library.read_text())['symbols'])
    return result


def compare_objects(paths, records=None):
    target = PE32(ORIGINAL)
    records = load_symbols() if records is None else records
    results = []
    for path in paths:
        obj = COFF(path)
        symbols = TargetSymbols(target, rows=records)
        literals = []
        # A source string has independent content evidence. Admit its address
        # only when the complete compiler-emitted literal matches there.
        # Repeated strings retain their observed address; no data is modified.
        for function in obj.functions():
            candidates = symbols.candidates(function['name'], path.name)
            if len(candidates) != 1:
                continue
            start = target.offset(candidates[0]['va'] - target.base)
            for relocation in function['relocations']:
                symbol = relocation['symbol']
                if relocation['type'] != 6 or not symbol['name'].startswith('??_C@'):
                    continue
                section = obj.sections[symbol['section'] - 1]
                raw = section['data'][symbol['value']:]
                offset = relocation['offset']
                address = int.from_bytes(target.data[start+offset:start+offset+4], 'little')
                addend = int.from_bytes(function['data'][offset:offset+4], 'little')
                address -= addend
                try:
                    original_offset = target.offset(address - target.base)
                except ValueError:
                    continue
                if target.data[original_offset:original_offset+len(raw)] != raw:
                    continue
                row = dict(name=symbol['name'], va=address, object=path.name,
                           address_basis='identified_symbol',
                           address_evidence='Complete compiler-emitted source string equals original data',
                           size=len(raw), sha256=hashlib.sha256(raw).hexdigest())
                symbols.add(row)
                if row not in literals:
                    literals.append(row)
        functions = [compare_function(f, target, symbols, path.name) for f in obj.functions()]
        results.append(dict(object=str(path), sha256=hashlib.sha256(obj.data).hexdigest(),
                            literal_data_checks=literals, functions=functions,
                            functions_compared=sum(f['status']=='compared' for f in functions),
                            relocation_masked_matches=sum(f.get('relocation_masked_equal',False) for f in functions),
                            relocated_exact_matches=sum(f.get('relocated_bytes_equal',False) for f in functions)))
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', nargs='?', type=Path)
    parser.add_argument('--object', action='append', type=Path, default=[])
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = compare_dll(args.dll, ORIGINAL) if args.dll else dict(
        original=str(ORIGINAL), original_sha256=hashlib.sha256(ORIGINAL.read_bytes()).hexdigest(),
        diagnostic_only=True, full_dll_byte_exact=False)
    if args.object:
        result['objects'] = compare_objects(args.object)
    serialized = json.dumps(result, indent=2)+'\n'
    if args.report:
        args.report.write_text(serialized)
    for obj in result.get('objects', []):
        print('%s: %d/%d bodies match outside recorded COFF relocations; %d with checked targets' % (
            obj['object'],obj['relocation_masked_matches'],len(obj['functions']),obj['relocated_exact_matches']))
        for function in obj['functions']:
            if not function.get('relocation_masked_equal',False) or function.get('relocation_target_mismatch_count',0):
                print(' ',function['name'],function['status'],function.get('relocation_masked_differing_bytes',''),
                      'target mismatches:',function.get('relocation_target_mismatch_count',0))
    if args.dll:
        print('Complete DLL byte exact:',result['full_dll_byte_exact'])
        print('Differing bytes:',result['differing_file_bytes'])
        return 0 if result['full_dll_byte_exact'] else 1
    return 0 if all(f.get('relocation_masked_equal',False) and not f.get('relocation_target_mismatch_count',0)
                    for obj in result.get('objects',[]) for f in obj['functions']) else 1


if __name__ == '__main__':
    raise SystemExit(main())
