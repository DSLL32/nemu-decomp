#!/usr/bin/env python3
"""Audit native audio readonly layout without claiming complete DLL equality.

Only actual PE HIGHLOW fields and export function-RVA entries are classified as
addresses. No bytes are rewritten. Success means every other readonly byte and
all repeated source-constant positions agree; absolute pointer values and the
whole DLL must still be checked separately.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from pe_fingerprints import PE32
from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def highlow_sites(pe):
    rva, size = pe.directory(5)
    cursor, end = pe.offset(rva), pe.offset(rva) + size
    sites = []
    while cursor < end:
        page, length = struct.unpack_from('<II', pe.data, cursor)
        if length < 8 or cursor + length > end or length % 2:
            raise ValueError('Invalid PE relocation block')
        for offset in range(cursor + 8, cursor + length, 2):
            word, = struct.unpack_from('<H', pe.data, offset)
            if word >> 12 == 3:
                sites.append(page + (word & 4095))
            elif word >> 12:
                raise ValueError('Unexpected relocation kind')
        cursor += length
    return sites


def export_fields(pe):
    rva, _ = pe.directory(0)
    offset = pe.offset(rva)
    base, count, _, table = struct.unpack_from('<IIII', pe.data, offset + 16)
    names = {row['ordinal']: row['name'] for row in pe.exports()}
    return [dict(rva=table + 4 * i, ordinal=base + i,
                 name=names.get(base + i),
                 function_rva=struct.unpack_from('<I', pe.data,
                                                pe.offset(table + 4 * i))[0])
            for i in range(count)]


def constant_bytes(obj):
    coff = COFF(obj)
    symbols = [next(s for s in coff.symbols if s['name'] == '_b%d' % i)
               for i in range(1, 32)]
    first = symbols[0]
    section = coff.sections[first['section'] - 1]
    assert all(s['storage_class'] == 3 and s['section'] == first['section']
               and s['value'] == first['value'] + 4 * i
               for i, s in enumerate(symbols))
    assert section['name'] == '.rdata'
    assert not section['characteristics'] & 0x1000  # ordinary, not COMDAT
    raw = section['data'][first['value']:first['value'] + 124]
    assert len(raw) == 124
    return raw


def audit(original, rebuilt, object_path):
    left, right = PE32(original), PE32(rebuilt)
    ls = next(s for s in left.sections if s['name'] == '.rdata')
    rs = next(s for s in right.sections if s['name'] == '.rdata')
    a = left.data[ls['offset']:ls['offset'] + ls['raw_size']]
    b = right.data[rs['offset']:rs['offset'] + rs['raw_size']]
    differences = [i for i in range(max(len(a), len(b)))
                   if a[i:i + 1] != b[i:i + 1]]
    lh = [r for r in highlow_sites(left)
          if ls['rva'] <= r < ls['rva'] + ls['raw_size']]
    rh = [r for r in highlow_sites(right)
          if rs['rva'] <= r < rs['rva'] + rs['raw_size']]
    le, re = export_fields(left), export_fields(right)
    export_layout_equal = [(r['rva'], r['ordinal'], r['name']) for r in le] == [
        (r['rva'], r['ordinal'], r['name']) for r in re]
    fields = set(lh) | {r['rva'] for r in le}
    covered = {r + i - ls['rva'] for r in fields for i in range(4)}
    unexpected = [ls['rva'] + i for i in differences if i not in covered]
    constants = constant_bytes(object_path)

    def occurrences(raw, section):
        found, start = [], 0
        while (start := raw.find(constants, start)) >= 0:
            found.append(section['rva'] + start)
            start += 1
        return found

    lc, rc = occurrences(a, ls), occurrences(b, rs)
    address_fields = []
    for rva in sorted(fields):
        av, = struct.unpack_from('<I', left.data, left.offset(rva))
        bv, = struct.unpack_from('<I', right.data, right.offset(rva))
        address_fields.append(dict(rva=rva,
            kind='HIGHLOW' if rva in lh else 'export_function_rva',
            original=av, rebuilt=bv, equal=av == bv))
    passed = (ls['rva'] == rs['rva'] and
              ls['virtual_size'] == rs['virtual_size'] and len(a) == len(b) and
              lh == rh and export_layout_equal and not unexpected and
              len(lc) == 13 and lc == rc)
    return dict(scope='Readonly layout diagnostic only; address values remain unmodified.',
        original=str(original), original_sha256=sha(left.data),
        rebuilt=str(rebuilt), rebuilt_sha256=sha(right.data),
        full_dll_byte_exact=left.data == right.data,
        readonly_raw_byte_exact=a == b, readonly_layout_diagnostic_passed=passed,
        original_section=ls, rebuilt_section=rs, differing_bytes=len(differences),
        highlow_sites_exact=lh == rh, highlow_fields=len(lh),
        export_layout_exact=export_layout_equal, export_function_fields=len(le),
        all_non_address_bytes_equal=not unexpected,
        unclassified_difference_rvas=unexpected, address_fields=address_fields,
        repeated_constants=dict(source_object=str(object_path),
            object_sha256=sha(object_path.read_bytes()), bytes=len(constants),
            sha256=sha(constants), original_rvas=lc, rebuilt_rvas=rc,
            positions_exact=lc == rc),
        limitation='Matching non-address bytes does not establish the identities or values '
                   'of differing address fields, nor full DLL equality.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--object', type=Path, required=True,
                        help='Native C object containing b1..b31')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    result = audit(ROOT / 'master/Plugin/audio.dll', args.dll, args.object)
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print('Readonly layout diagnostic:', result['readonly_layout_diagnostic_passed'])
    print('Readonly differing bytes:', result['differing_bytes'])
    print('Full DLL byte exact:', result['full_dll_byte_exact'])
    return 0 if result['readonly_layout_diagnostic_passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
