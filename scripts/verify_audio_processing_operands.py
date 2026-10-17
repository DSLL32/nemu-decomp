#!/usr/bin/env python3
"""Check constants, SEH metadata, and CRT operands for audio processing objects.

This supplements complete-body diagnostics. Data matched uniquely by its full
contents is distinguished from addresses inferred only from an instruction.
It does not certify a rebuilt DLL or change executable bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from map_nemu_libraries import archive_members
from pe_fingerprints import PE32
from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL_SHA256 = 'f05b3f9e239041758a4aca09a891e2ebebe4136a3b326f8bbc5c160d76c73808'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--objects', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--update-addresses', action='store_true')
    args = parser.parse_args()
    original = PE32(ROOT / 'master/Plugin/audio.dll')
    assert digest(original.data) == ORIGINAL_SHA256
    libraries_path = ROOT / 'analysis/audio-libraries.json'
    libraries = json.loads(libraries_path.read_text())
    assert libraries['original_sha256'] == ORIGINAL_SHA256
    addresses_path = ROOT / 'analysis/audio-processing-addresses.json'
    addresses = json.loads(addresses_path.read_text())
    comparison_path = ROOT / 'analysis/audio-processing-comparison.json'
    comparison = json.loads(comparison_path.read_text())
    compiled = {Path(o['object']).name: o for o in comparison['objects']}
    rows, symbols = [], []

    def occurrences(data):
        matches = []
        for section in original.sections:
            raw = original.data[section['offset']:section['offset'] + section['raw_size']]
            start = 0
            while True:
                offset = raw.find(data, start)
                if offset < 0:
                    break
                matches.append(original.base + section['rva'] + offset)
                start = offset + 1
        return matches

    def record(name, va, basis, evidence, **details):
        rows.append(dict(name=name, va=va, address_basis=basis, **details))
        symbols.append(dict(name=name, va=va, address_basis=basis,
                            address_evidence=evidence))

    for name in ('_fprintf', '__aulldiv', '__alldiv', '__allmul', '__except_handler3'):
        matches = [r for r in libraries['symbols'] if r['name'] == name]
        assert len(matches) == 1
        row = matches[0]
        assert row['address_basis'] == 'independent_library_mapping'
        assert row['discovery'] == 'unique_body_scan'
        assert not row['comparison_only']
        record(name, row['va'], 'independent_library_mapping',
               'Independent complete CRT body scan in analysis/audio-libraries.json.',
               library=row['library'], member=row['object'],
               member_sha256=row['sha256'], size=row['size'])

    for object_name in ('AudioProcessing.obj', 'AudioSSE.obj'):
        coff = COFF(args.objects / object_name)
        assert digest(coff.data) == compiled[object_name]['sha256']
        if object_name == 'AudioProcessing.obj':
            function = next(r for r in compiled[object_name]['functions']
                            if r['name'] == '_DumpAudioCommand')
            assert function['relocation_masked_equal'] and function['size_equal']
            for relocation in function['relocations']:
                name = relocation['symbol']
                if not name.startswith('??_C@') or any(r['name'] == name for r in rows):
                    continue
                symbol = next(s for s in coff.symbols if s['name'] == name)
                section = coff.sections[symbol['section'] - 1]
                data = section['data'][symbol['value']:]
                data = data[:data.index(0) + 1]
                va = relocation['original_target_va']
                offset = original.offset(va - original.base)
                assert original.data[offset:offset + len(data)] == data
                matches = occurrences(data)
                unique = len(matches) == 1
                assert not unique or matches == [va]
                record(name, va, 'identified_symbol' if unique else 'observed_operand_inference',
                       ('Unique complete NUL-terminated literal match' if unique else
                        'Nonunique literal bytes verified at the decoded operand address') +
                       '; see analysis/audio-processing-operands.json.',
                       complete_data_hex=data.hex(), matches=[hex(v) for v in matches],
                       complete_data_sha256=digest(data))
        else:
            function = next(r for r in compiled[object_name]['functions']
                            if r['name'] == '_AudioOSHasSSE')
            assert function['relocation_masked_equal'] and function['size_equal']
            actual_function = next(f for f in coff.functions()
                                   if f['name'] == '_AudioOSHasSSE')
            scope_operand = next(r for r in function['relocations']
                                 if r['symbol'].startswith('$T'))
            name = scope_operand['symbol']
            symbol = next(s for s in coff.symbols if s['name'] == name)
            section = coff.sections[symbol['section'] - 1]
            assert section['size'] == 12 and symbol['value'] == 0
            data = bytearray(section['data'])
            destinations = []
            for relocation in section['relocations']:
                target = relocation['symbol']
                assert relocation['type'] == 6
                assert target['section'] == actual_function['section']
                delta = target['value'] - actual_function['value']
                assert 0 <= delta < actual_function['size']
                destination = function['original_va'] + delta
                struct.pack_into('<I', data, relocation['offset'], destination)
                destinations.append(dict(offset=relocation['offset'],
                                         function_offset=delta, va=hex(destination)))
            assert len(destinations) == 2
            matches = occurrences(bytes(data))
            assert matches == [scope_operand['original_target_va']]
            record(name, matches[0], 'identified_symbol',
                   'Unique complete SEH scope-table match after resolving both labels inside '
                   'the matched AudioOSHasSSE body; see analysis/audio-processing-operands.json.',
                   complete_data_hex=data.hex(), complete_data_sha256=digest(data),
                   internal_targets=destinations, matches=[hex(v) for v in matches])

    library_path = ROOT / 'toolchains/vc7/lib/libcmt.lib'
    absolute = []
    for name, offset, data in archive_members(library_path):
        if not name.endswith('\\exsup.obj'):
            continue
        scratch = args.objects / 'processing-exsup-proof.obj'
        scratch.write_bytes(data)
        obj = COFF(scratch)
        absolute.extend((name, offset, data, s) for s in obj.symbols
                        if s['name'] == '__except_list' and s['section'] == -1)
    assert len(absolute) == 1
    name, offset, data, symbol = absolute[0]
    assert symbol['value'] == 0
    record('__except_list', 0, 'independent_library_mapping',
           'Absolute COFF symbol with value zero in VC7 libcmt exsup.obj, used as FS:[0]; '
           'see analysis/audio-processing-operands.json.',
           library='toolchains/vc7/lib/libcmt.lib', member=name,
           archive_offset=offset, member_sha256=digest(data), section='IMAGE_SYM_ABSOLUTE')
    report = dict(original='master/Plugin/audio.dll', original_sha256=ORIGINAL_SHA256,
                  status='scoped_operand_evidence_not_whole_dll_equality',
                  library_mapping_sha256=digest(libraries_path.read_bytes()),
                  library_sha256=digest(library_path.read_bytes()),
                  comparison_sha256=digest(comparison_path.read_bytes()), symbols=rows)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    if args.update_addresses:
        names = {r['name'] for r in symbols}
        addresses['symbols'] = [r for r in addresses['symbols'] if r['name'] not in names] + symbols
        addresses_path.write_text(json.dumps(addresses, indent=2) + '\n')
    print(f'{len(rows)} processing operand identities checked')


if __name__ == '__main__':
    main()
