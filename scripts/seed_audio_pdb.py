#!/usr/bin/env python3
"""Restore audio.dll's historical PDB identity in a freshly generated VC7 PDB.

This is build-metadata reconstruction, not source recovery. The original DLL
records a GUID and revision 8. A native LINK run given a PDB with that GUID and
revision 7 emits revision 8 itself. Only the PDB Info and DBI identity fields
are changed; no executable is read or modified. Keep the input PDB as evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import uuid


MAGIC = b'Microsoft C/C++ MSF 7.00\r\n\x1aDS\x00\x00\x00'
GUID = uuid.UUID('53a2b525-689f-4912-82cd-8a849f0d4dcb')
PRIOR_AGE = 7


def seed_pdb(original):
    """Return seeded bytes and an audit of the three changed metadata fields."""
    if len(original) < 56 or original[:32] != MAGIC:
        raise ValueError('Expected a native MSF 7.00 PDB')
    block_size, _, block_count, directory_size, _, block_map = struct.unpack_from('<6I', original, 32)
    if block_size not in (512, 1024, 2048, 4096) or block_count * block_size != len(original):
        raise ValueError('Invalid MSF block geometry')

    def block(index):
        if index >= block_count:
            raise ValueError('MSF block outside file')
        return original[index * block_size:(index + 1) * block_size]

    directory_count = (directory_size + block_size - 1) // block_size
    if directory_count * 4 > block_size:
        raise ValueError('Unsupported multi-block directory map')
    directory_ids = struct.unpack_from('<%dI' % directory_count, block(block_map))
    directory = b''.join(block(i) for i in directory_ids)[:directory_size]
    if len(directory) < 4:
        raise ValueError('Missing stream directory')
    stream_count = struct.unpack_from('<I', directory)[0]
    if stream_count < 4 or 4 + 4 * stream_count > len(directory):
        raise ValueError('Invalid stream count')
    sizes = struct.unpack_from('<%dI' % stream_count, directory, 4)
    cursor = 4 + 4 * stream_count
    streams = []
    for size in sizes:
        count = 0 if size == 0xffffffff else (size + block_size - 1) // block_size
        if cursor + count * 4 > len(directory):
            raise ValueError('Truncated stream block list')
        ids = struct.unpack_from('<%dI' % count, directory, cursor)
        if any(i >= block_count for i in ids):
            raise ValueError('Stream block outside file')
        streams.append((size, ids))
        cursor += count * 4

    def stream(index):
        size, ids = streams[index]
        if size == 0xffffffff:
            raise ValueError('Required metadata stream is absent')
        return b''.join(block(i) for i in ids)[:size]

    info, dbi = stream(1), stream(3)
    if len(info) < 28 or struct.unpack_from('<I', info)[0] != 20000404:
        raise ValueError('Expected VC7 PDB Info stream version 20000404')
    if len(dbi) < 12 or struct.unpack_from('<2I', dbi) != (0xffffffff, 19990903):
        raise ValueError('Expected VC7 DBI stream version 19990903')
    data = bytearray(original)
    fields = []

    def replace(index, offset, value, name):
        if offset + len(value) > streams[index][0]:
            raise ValueError('Metadata field exceeds its stream')
        offsets = []
        for i, byte in enumerate(value):
            page, within = divmod(offset + i, block_size)
            physical = streams[index][1][page] * block_size + within
            data[physical] = byte
            offsets.append(physical)
        fields.append(dict(name=name, stream=index, stream_offset=offset,
                           file_offsets=offsets, before=bytes(original[i] for i in offsets).hex(),
                           after=value.hex()))

    replace(1, 8, struct.pack('<I', PRIOR_AGE), 'Info.Age')
    replace(1, 12, GUID.bytes_le, 'Info.Guid')
    replace(3, 8, struct.pack('<I', PRIOR_AGE), 'DBI.Age')
    allowed = {i for field in fields for i in field['file_offsets']}
    changed = [i for i, (a, b) in enumerate(zip(original, data)) if a != b]
    if not set(changed) <= allowed:
        raise AssertionError('Non-identity metadata changed')
    return bytes(data), dict(
        scope='Historical PDB identity restored in toolchain-generated metadata; no executable edits.',
        original_sha256=hashlib.sha256(original).hexdigest(),
        seeded_sha256=hashlib.sha256(data).hexdigest(), size=len(data),
        guid=str(GUID), prior_age=PRIOR_AGE, expected_link_age=PRIOR_AGE + 1,
        fields=fields, changed_file_offsets=changed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path, help='freshly generated VC7 PDB; preserved unchanged')
    parser.add_argument('output', type=Path, help='separate PDB seed for the final native link')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if len({p.resolve() for p in (args.input, args.output, args.report)}) != 3:
        parser.error('Input, output, and report must be separate paths')
    try:
        seeded, report = seed_pdb(args.input.read_bytes())
    except ValueError as error:
        parser.error(str(error))
    args.output.write_bytes(seeded)
    report.update(input=str(args.input.resolve()), output=str(args.output.resolve()))
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('Seeded PDB GUID %s, prior age %d; %d metadata bytes differ' %
          (GUID, PRIOR_AGE, len(report['changed_file_offsets'])))


if __name__ == '__main__':
    main()
