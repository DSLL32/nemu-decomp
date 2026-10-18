#!/usr/bin/env python3
"""Reproduce Delphi's original ANSI representation of three empty list items.

ObjectTextToResource serializes the textual '' items as empty WideStrings.
The original Delphi form writer used empty ShortStrings. Both represent the
same source strings. This restores their resource-stream types before linking;
it does not read or patch an executable or use any original resource bytes.
"""

from pathlib import Path
import struct


EMPTY_LIST_PREDECESSORS = (
    'TEXOPCAPS_SUBTRACT', 'SRC: BLENDCAPS_ZERO', 'DST: BLENDCAPS_ZERO',
)


def legacy_empty_list_strings(path: Path):
    data = path.read_bytes()
    # ObjectTextToResource writes a 16-bit Delphi resource header.
    header = b'\xff\x0a\x00TFORMEDITOR\x00\x30\x10'
    if not data.startswith(header) or data[len(header) + 4:len(header) + 8] != b'TPF0':
        raise ValueError('Expected the TFORMEDITOR Delphi form resource')
    size = struct.unpack_from('<I', data, len(header))[0]
    payload = data[len(header) + 4:]
    if size != len(payload):
        raise ValueError('Unexpected Delphi resource-stream size')
    for previous in EMPTY_LIST_PREDECESSORS:
        encoded = previous.encode('ascii')
        prefix = bytes([6, len(encoded)]) + encoded  # vaString, length, text
        wide = prefix + b'\x12\0\0\0\0'         # vaWString, zero characters
        if payload.count(wide) != 1:
            raise ValueError(f'Expected one empty list item after {previous!r}')
        payload = payload.replace(wide, prefix + b'\x06\0')
    path.write_bytes(header + struct.pack('<I', len(payload)) + payload)
    return dict(empty_items=3, bytes_saved=size - len(payload),
                original_converter_payload_size=size, legacy_payload_size=len(payload))
