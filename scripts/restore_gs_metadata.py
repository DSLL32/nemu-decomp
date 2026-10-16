#!/usr/bin/env python3
"""Restore historical noncode GameShark linker metadata after a strict guard.

The source must first reproduce every byte outside the documented metadata
regions. The archival residue is uninitialized incremental-linker memory from
the original build, not reconstructed program code. Keep the raw linked DLL.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / 'analysis/gs-build-metadata.json'


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def restore(path):
    spec = json.loads(SPEC.read_text())
    raw = path.read_bytes()
    if len(raw) != spec['original_size']:
        raise ValueError('Linked DLL size differs; refusing metadata restoration')
    canonical = bytearray(raw)
    end = 0
    for region in sorted(spec['regions'], key=lambda r: r['offset']):
        start, size = region['offset'], region['size']
        if start < end or size <= 0 or start + size > len(raw):
            raise ValueError('Invalid or overlapping metadata regions')
        canonical[start:start + size] = bytes(size)
        end = start + size
    actual = sha256(canonical)
    if actual != spec['canonical_sha256']:
        raise ValueError('Linked code, data, layout or other nonmetadata bytes differ; '
                         f'refusing restoration (canonical SHA-256 {actual})')
    residue = (SPEC.parent / spec['residue_asset']).read_bytes()
    if len(residue) != spec['residue_size'] or sha256(residue) != spec['residue_sha256']:
        raise ValueError('Archival linker-residue asset does not match its recorded hash')
    result = bytearray(raw)
    changes = []
    for region in spec['regions']:
        start, size = region['offset'], region['size']
        if region['kind'] == 'archival_residue':
            offset = region['asset_offset']
            replacement = residue[offset:offset + size]
        elif region['kind'] == 'uint32' and size == 4:
            replacement = struct.pack('<I', region['value'])
        else:
            raise ValueError('Unrecognized metadata field')
        if len(replacement) != size:
            raise ValueError('Truncated metadata asset')
        changes.append(dict(name=region['name'], offset=start, size=size,
                            kind=region['kind'], changed_bytes=sum(
                                a != b for a, b in zip(raw[start:start + size], replacement))))
        result[start:start + size] = replacement
    if sha256(result) != spec['original_sha256']:
        raise ValueError('Restored image does not match the recorded original hash')
    path.write_bytes(result)
    return dict(raw_linked_sha256=sha256(raw), restored_sha256=sha256(result),
                all_nonmetadata_bytes_exact=True, canonical_sha256=actual,
                archival_residue_sha256=sha256(residue), regions=changes)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    report = restore(args.dll)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('Every nonmetadata byte matched before restoration.')
    print(f'Complete DLL SHA-256: {report["restored_sha256"]}')


if __name__ == '__main__':
    main()
