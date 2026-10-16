#!/usr/bin/env python3
"""Set only PE resource-directory timestamps for reproducible Delphi builds.

Delphi 6 stores local DOS date/time values in these fields. No resource payload,
machine code, PE layout, checksum, or other timestamp is changed. This utility
does not read a reference binary. The build script preserves the unstamped DLL.
"""

import argparse
from datetime import datetime
from pathlib import Path
import struct

from pe_fingerprints import PE32, u16, u32


def resource_time_offsets(pe):
    rva, size = pe.directory(2)
    if not rva:
        return []
    base = pe.offset(rva)
    seen, offsets = set(), []

    def visit(relative):
        if relative in seen or not 0 <= relative <= size - 16:
            raise ValueError('Invalid or repeated resource directory')
        seen.add(relative)
        at = base + relative
        count = u16(pe.data, at + 12) + u16(pe.data, at + 14)
        if relative + 16 + count * 8 > size:
            raise ValueError('Resource directory exceeds the resource section')
        offsets.append(at + 4)
        for index in range(count):
            child = u32(pe.data, at + 16 + index * 8 + 4)
            if child & 0x80000000:
                visit(child & 0x7fffffff)

    visit(0)
    return sorted(offsets)


def stamp_resource_times(path, timestamp):
    time = datetime.fromisoformat(timestamp)
    if time.tzinfo or not 1980 <= time.year <= 2107 or time.second % 2 or time.microsecond:
        raise ValueError('Expected a local DOS timestamp (1980–2107, even seconds)')
    date = (time.year - 1980) << 9 | time.month << 5 | time.day
    clock = time.hour << 11 | time.minute << 5 | time.second // 2
    value = date << 16 | clock
    pe = PE32(path)
    offsets = resource_time_offsets(pe)
    data = bytearray(pe.data)
    for offset in offsets:
        struct.pack_into('<I', data, offset, value)
    path.write_bytes(data)
    return offsets


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('local_time', help='YYYY-MM-DDTHH:MM:SS, no timezone')
    args = parser.parse_args()
    offsets = stamp_resource_times(args.dll, args.local_time)
    print(f'Stamped {len(offsets)} resource-directory time fields')
