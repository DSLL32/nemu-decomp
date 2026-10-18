#!/usr/bin/env python3
"""Diagnostic COFF and resource coverage; never certify a complete DLL.

Only COFF-declared four-byte relocation operands are masked for instruction
searches. Their names, kinds and original values remain in the report. A match
does not establish the unresolved operand identities or original source names.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

from pe_fingerprints import PE32
from verify_gs import COFF

UNPACKED_SHA256 = '0c225584d66eaf9f5620eb86e070c44e7177f034a39de50f071bb1e759393a07'
UNPACKED_120_SHA256 = 'f3082e3b5c7ecbd368abd996f7ecfef9a3faa409729d182e8beff8d7baaac6dc'


def resources(path):
    data = Path(path).read_bytes()
    position = 0
    while position < len(data):
        size, header = struct.unpack_from('<II', data, position)
        cursor = position + 8

        def identifier():
            nonlocal cursor
            value = struct.unpack_from('<H', data, cursor)[0]
            cursor += 2
            if value == 0xffff:
                value = struct.unpack_from('<H', data, cursor)[0]
                cursor += 2
                return value
            text = ''
            while value:
                text += chr(value)
                value = struct.unpack_from('<H', data, cursor)[0]
                cursor += 2
            return text

        kind, name = identifier(), identifier()
        cursor = (cursor + 3) & ~3
        language = struct.unpack_from('<H', data, cursor + 6)[0]
        payload = data[position + header:position + header + size]
        position = (position + header + size + 3) & ~3
        if size:
            yield (kind, name, language), payload


def inspect(reference, objects, resource=None):
    pe = PE32(Path(reference))
    if hashlib.sha256(pe.data).hexdigest() not in (UNPACKED_SHA256, UNPACKED_120_SHA256):
        raise ValueError('Unexpected unpacked reference hash')
    section = next(s for s in pe.sections if s['name'] == '.text')
    code = pe.data[section['offset']:section['offset'] + section['virtual_size']]
    rows = []
    covered = set()
    for path in objects:
        obj = COFF(path)
        for function in obj.functions():
            mask = set()
            for fixup in function['relocations']:
                if fixup['type'] not in (6, 7, 20):
                    raise ValueError(f'{path}: unsupported relocation type {fixup["type"]}')
                mask.update(range(fixup['offset'], fixup['offset'] + 4))
            # The final function's COFF alignment may extend into the PE file's
            # section padding, so retain that padding in the search region.
            search = pe.data[section['offset']:section['offset'] + section['raw_size']]
            expression = b''.join(b'.' if i in mask else re.escape(bytes([byte]))
                                  for i, byte in enumerate(function['data']))
            hits = [match.start() for match in re.finditer(expression, search, re.S)]
            row = dict(object=Path(path).name, name=function['name'],
                       coff_bytes=function['size'], fixed_bytes=function['size'] - len(mask),
                       matching_original_vas=[pe.base + section['rva'] + offset for offset in hits],
                       instruction_bytes_equal_outside_coff_relocations=bool(hits),
                       relocation_identities_verified=False, relocations=[])
            for fixup in function['relocations']:
                offset = fixup['offset']
                addend = struct.unpack_from('<I', function['data'], offset)[0]
                original_values = []
                for hit in hits:
                    value = struct.unpack_from('<I', search, hit + offset)[0]
                    if fixup['type'] == 20:
                        value = (pe.base + section['rva'] + hit + offset + 4 + value) & 0xffffffff
                    original_values.append(value)
                row['relocations'].append(dict(offset=offset, type=fixup['type'],
                    symbol=fixup['symbol']['name'], coff_addend=addend,
                    original_operand_or_destination=original_values))
            # Unique sufficiently substantive whole-COFF-function signatures
            # are a diagnostic inventory, never an acceptance criterion.
            if len(hits) == 1 and row['fixed_bytes'] >= 16:
                covered.update(range(hits[0], min(hits[0] + function['size'], len(code))))
            rows.append(row)
    result = dict(diagnostic_only=True, complete_dll_available=False,
                  full_dll_byte_exact=False, original_code_virtual_bytes=len(code),
                  unique_masked_signature_code_bytes=len(covered), functions=rows)
    if resource:
        original_resources = {tuple(r['path']): pe.data[r['offset']:r['offset'] + r['size']]
                              for r in pe.resources()}
        rebuilt = dict(resources(resource))
        result['resources'] = [dict(path=list(key), original_bytes=len(value),
            rebuilt_bytes=len(rebuilt.get(key, b'')), byte_exact=rebuilt.get(key) == value)
            for key, value in original_resources.items()]
        result['all_resource_payloads_byte_exact'] = rebuilt == original_resources
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--objects', type=Path, nargs='+', required=True)
    parser.add_argument('--resources', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = inspect(args.reference, args.objects, args.resources)
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(f'Diagnostic unique masked-signature coverage: '
          f'{result["unique_masked_signature_code_bytes"]}/{result["original_code_virtual_bytes"]} bytes')
    print('Complete DLL byte exact: False (application reconstruction incomplete)')
    return 1


if __name__ == '__main__':
    raise SystemExit(main())
