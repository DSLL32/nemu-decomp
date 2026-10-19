"""Aggregate compiler .res inputs, assigning private numeric resource ranges.

This operates before LINK on standard Win32 resource records, never on PEs.
Dialog control/command IDs remain local to their windows and are preserved.
"""
import struct


def align(n):
    return (n + 3) & ~3


def identifier(data, pos):
    value = struct.unpack_from('<H', data, pos)[0]
    if value == 0xffff:
        return struct.unpack_from('<H', data, pos + 2)[0], pos + 4
    end = pos
    while struct.unpack_from('<H', data, end)[0]: end += 2
    return data[pos:end].decode('utf-16le'), end + 2


def encode(value):
    return struct.pack('<HH', 0xffff, value) if isinstance(value, int) else value.encode('utf-16le') + b'\0\0'


def records(data):
    if data.startswith(b'\xff\x0a\x00'):
        # ObjectTextToResource writes the compact Win16 resource envelope
        # consumed by DCC. Its TPF0 form payload is identical in Win32 RCDATA.
        end = data.index(b'\0', 3)
        name = data[3:end].decode('latin1')
        flags, size = struct.unpack_from('<HI', data, end + 1)
        payload = data[end + 7:]
        if len(payload) != size or not payload.startswith(b'TPF0'):
            raise ValueError('Invalid Delphi form resource')
        yield 10, name, struct.pack('<IHHII', 0, flags, 0, 0, 0), bytearray(payload)
        return
    pos = 0
    while pos < len(data):
        size, header = struct.unpack_from('<II', data, pos)
        if header < 8 or pos + header + size > len(data):
            raise ValueError('Invalid Win32 resource record')
        typ, start = identifier(data, pos + 8)
        name, start = identifier(data, start)
        fixed = data[align(start):pos + header]
        if len(fixed) != 16: raise ValueError('Invalid resource header')
        yield typ, name, fixed, bytearray(data[pos + header:pos + header + size])
        pos = align(pos + header + size)


def dialog(data, offset):
    extended = data[:4] == b'\x01\0\xff\xff'
    if extended:
        style, count, pos = struct.unpack_from('<I', data, 12)[0], struct.unpack_from('<H', data, 16)[0], 26
    else:
        style, count, pos = struct.unpack_from('<I', data)[0], struct.unpack_from('<H', data, 8)[0], 18
    def resource(pos, remap=False):
        name, end = identifier(data, pos)
        if remap and isinstance(name, int) and 0 < name < 0x8000:
            struct.pack_into('<H', data, pos + 2, name + offset)
        return name, end
    _, pos = resource(pos, True)  # dialog menu
    _, pos = resource(pos)       # class
    _, pos = resource(pos)       # title
    if style & 0x40:
        pos += 6 if extended else 2
        _, pos = resource(pos)
    for _ in range(count):
        pos = align(pos)
        control_style = struct.unpack_from('<I', data, pos + (8 if extended else 0))[0]
        pos += 24 if extended else 18
        cls, pos = resource(pos)
        # Static icon/bitmap controls refer to resources in their title field.
        image = (cls == 0x82 or str(cls).upper() == 'STATIC') and control_style & 0x1f in (3, 14)
        _, pos = resource(pos, image)
        extra = struct.unpack_from('<H', data, pos)[0]
        pos += 2 + extra
    if pos > len(data): raise ValueError('Dialog exceeds resource length')
    return data


def aggregate(inputs, destination):
    output, seen = bytearray(), {}
    # Standard null header.
    output.extend(struct.pack('<IIHHHHIHHII', 0, 32, 0xffff, 0, 0xffff, 0, 0, 0, 0, 0, 0))
    for path, offset in inputs:
        for typ, name, fixed, data in records(path.read_bytes()):
            if typ == 0: continue
            if offset:
                if typ in (16, 24): continue  # executable owns version/manifest
                if isinstance(name, int):
                    low = (name - 1) * 16 if typ == 6 else name
                    if low < 0x8000:
                        name += offset // 16 if typ == 6 else offset
                if typ == 14:
                    for i in range(struct.unpack_from('<H', data, 4)[0]):
                        pos = 6 + 14 * i + 12
                        struct.pack_into('<H', data, pos, struct.unpack_from('<H', data, pos)[0] + offset)
                elif typ == 5: data = dialog(data, offset)
            key = typ, name, struct.unpack_from('<H', fixed, 6)[0]
            value = bytes(data)
            if key in seen:
                if seen[key] != value: raise ValueError(f'Conflicting resource {key} in {path}')
                continue
            seen[key] = value
            ids = encode(typ) + encode(name)
            ids += bytes(align(len(ids)) - len(ids))
            output.extend(struct.pack('<II', len(data), 8 + len(ids) + 16) + ids + fixed + data)
            output.extend(bytes(align(len(output)) - len(output)))
    destination.write_bytes(output)
    return len(seen)
