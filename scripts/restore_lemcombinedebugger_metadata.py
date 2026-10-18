#!/usr/bin/env python3
"""Restore only guarded archival resource-directory clock DWORDs.

The candidate must already match the complete original file except these
identified directory timestamps. A refusal leaves the candidate untouched.
No instruction, data, resource payload, layout or other header field is changed.
"""

import hashlib
from pathlib import Path
import struct

from pe_fingerprints import PE32
from stamp_pe_resources import resource_time_offsets
from verify_lemcombinedebugger import MASTER, MASTER_SHA256

RESOURCE_DOS_TIME = 0x2C480395  # 2002-02-08 00:28:42, local DOS time


def restore(path: Path):
    target, candidate = PE32(MASTER), PE32(path)
    if hashlib.sha256(target.data).hexdigest() != MASTER_SHA256:
        raise ValueError('Unexpected master LemCombineDebugger.dll SHA-256')
    offsets = resource_time_offsets(candidate)
    if offsets != resource_time_offsets(target):
        return dict(status='refused', reason='Resource directory layout differs', fields=[])
    updated = bytearray(candidate.data)
    fields = []
    for offset in offsets:
        expected = struct.unpack_from('<I', target.data, offset)[0]
        if expected != RESOURCE_DOS_TIME:
            raise ValueError('Unexpected original resource-directory timestamp')
        old = struct.unpack_from('<I', updated, offset)[0]
        struct.pack_into('<I', updated, offset, RESOURCE_DOS_TIME)
        fields.append(dict(offset=offset, old=old, restored=RESOURCE_DOS_TIME))
    if updated != target.data:
        return dict(status='refused', reason='Non-timestamp bytes differ', fields=[])
    path.write_bytes(updated)
    return dict(status='restored', scope='PE resource-directory timestamp DWORDs only',
                local_dos_time='2002-02-08T00:28:42', fields=fields)
