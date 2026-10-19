#!/usr/bin/env python3
"""Reproduce the historical import-library DLL spelling before native linking."""

import argparse
import hashlib
import json
from pathlib import Path
import struct

from map_nemu_libraries import archive_members

ROOT = Path(__file__).resolve().parents[1]
KERNEL32 = ROOT / 'toolchains/vc6rtm/lib/kernel32.lib'
KERNEL32_SHA256 = 'cd4a546f71481319aeee455aa0fb51698a094f9d3e6d8339acf445489a263c68'


def build(output):
    source = KERNEL32.read_bytes()
    if hashlib.sha256(source).hexdigest() != KERNEL32_SHA256:
        raise ValueError('Pinned VC6 RTM kernel32.lib changed')
    old, new = b'KERNEL32.dll', b'KERNEL32.DLL'
    count = source.count(old)
    if count != 1537 or new in source:
        raise ValueError('Unexpected import-library name metadata')
    rebuilt = source.replace(old, new)
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(rebuilt)
    original_members = list(archive_members(KERNEL32))
    rebuilt_members = list(archive_members(output))
    if len(original_members) != len(rebuilt_members):
        raise ValueError('Import archive structure changed')
    short_imports = native_objects = executable_sections = 0
    for (_, offset, before), (_, position, after) in zip(original_members, rebuilt_members):
        if offset != position or len(before) != len(after):
            raise ValueError('Import archive member boundaries changed')
        if before[:4] == b'\0\0\xff\xff':
            # Short import records contain symbolic import metadata, not instructions.
            size = struct.unpack_from('<I', before, 12)[0]
            if before[20:20 + size].replace(old, new) != after[20:20 + size]:
                raise ValueError('Short import symbol metadata changed')
            if before[:20] != after[:20] or before[20:].split(b'\0')[0] != after[20:].split(b'\0')[0]:
                raise ValueError('Short import header or function name changed')
            short_imports += 1
            continue
        native_objects += 1
        sections, optional_size = struct.unpack_from('<H', before, 2)[0], struct.unpack_from('<H', before, 16)[0]
        for index in range(sections):
            header = 20 + optional_size + 40 * index
            size, start = struct.unpack_from('<II', before, header + 16)
            flags = struct.unpack_from('<I', before, header + 36)[0]
            if flags & (0x20 | 0x20000000):
                if before[start:start + size] != after[start:start + size]:
                    raise ValueError('Import library executable contents changed')
                executable_sections += 1
    return dict(source_sha256=KERNEL32_SHA256,
                output_sha256=hashlib.sha256(rebuilt).hexdigest(),
                changed_dll_name_occurrences=count, short_import_records=short_imports,
                native_objects=native_objects, unchanged_executable_sections=executable_sections,
                output=str(output), executable_bytes_copied_from_reference=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    print(json.dumps(build(args.output), indent=2))


if __name__ == '__main__':
    main()
