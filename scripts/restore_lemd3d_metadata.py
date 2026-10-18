#!/usr/bin/env python3
"""Restore archival LemD3D8 linker metadata after every program byte matches.

No instruction, program datum, resource, import, export entry, or relocation
entry is restored. Rich provenance and unreachable header heap residue cannot
be regenerated from source. The untouched linked DLL must be retained.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct

from pe_fingerprints import PE32, rol32, u16, u32

ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / 'analysis/lemd3d-build-metadata.json'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def normalized(path, spec):
    pe = PE32(path)
    raw = pe.data
    if len(raw) != spec['original_size']:
        raise ValueError('Linked DLL size differs; refusing metadata restoration')
    count, optional = u16(raw, pe.pe + 6), u16(raw, pe.pe + 20)
    header_end = pe.opt + optional + count * 40
    if (count != 6 or optional != 224 or not 128 <= pe.pe <= 0x128
            or header_end > spec['header_residue_offset']
            or u32(raw, pe.opt + 60) != 0x4000
            or any(s['offset'] < 0x4000 for s in pe.sections)):
        raise ValueError('Unexpected PE header layout')
    header = bytearray(raw[pe.pe:header_end])
    reloc = next(i for i, s in enumerate(pe.sections) if s['name'] == '.reloc')
    size_at = 24 + optional + reloc * 40 + 8
    if not pe.directory(5)[1] <= u32(header, size_at) <= pe.sections[reloc]['raw_size']:
        raise ValueError('Relocation virtual size is not unused in-section allocation')
    header[8:12] = bytes(4)  # IMAGE_FILE_HEADER.TimeDateStamp
    header[size_at:size_at + 4] = bytes(4)
    result = bytearray(raw)
    result[128:0x4000] = bytes(0x4000 - 128)
    start = spec['pe_offset']
    result[start:start + len(header)] = header
    struct.pack_into('<I', result, 60, start)
    export_time = pe.offset(pe.directory(0)[0]) + 4
    result[export_time:export_time + 4] = bytes(4)
    return result, pe, size_at, export_time


def restore(path):
    spec = json.loads(SPEC.read_text())
    result, pe, reloc_size_at, export_time = normalized(path, spec)
    raw_hash = digest(pe.data)
    actual = digest(result)
    if actual != spec['canonical_sha256']:
        raise ValueError('Linked code, data, resources, imports, exports, relocations or layout '
                         'differ; refusing metadata restoration (canonical SHA-256 ' + actual + ')')
    residue = (SPEC.parent / spec['residue_asset']).read_bytes()
    if len(residue) != spec['residue_size'] or digest(residue) != spec['residue_sha256']:
        raise ValueError('Archival linker-residue asset changed')
    start = spec['pe_offset']
    struct.pack_into('<I', result, start + 8, spec['timestamp'])
    struct.pack_into('<I', result, start + reloc_size_at, spec['reloc_virtual_size'])
    struct.pack_into('<I', result, export_time, spec['export_timestamp'])
    rich_start = 128
    key = rich_start + sum(rol32(result[i], i) for i in range(rich_start)
                           if not 0x3c <= i < 0x40)
    key += sum(rol32(int(r['comp_id'], 16), r['count']) for r in spec['rich_records'])
    key &= 0xffffffff
    values = [0x536e6144, 0, 0, 0]
    for row in spec['rich_records']:
        values.extend([int(row['comp_id'], 16), row['count']])
    rich = b''.join(struct.pack('<I', value ^ key) for value in values)
    rich += b'Rich' + struct.pack('<I', key)
    if rich_start + len(rich) > start:
        raise ValueError('Rich provenance does not fit before the PE header')
    result[rich_start:rich_start + len(rich)] = rich
    offset = spec['header_residue_offset']
    result[offset:offset + len(residue)] = residue
    if digest(result) != spec['original_sha256']:
        raise ValueError('Metadata restoration did not reproduce the recorded DLL hash')
    path.write_bytes(result)
    return dict(raw_linked_sha256=raw_hash, restored_sha256=digest(result),
                all_nonmetadata_bytes_exact=True, canonical_sha256=actual,
                original_pe_offset=pe.pe, restored_pe_offset=start,
                restored_rich_records=spec['rich_records'],
                archival_residue_sha256=digest(residue), specification=str(SPEC))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    report = restore(args.dll)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('Every program byte matched before metadata restoration.')
    print('Complete DLL SHA-256: ' + report['restored_sha256'])
