"""Small i386 COFF helpers for the ordinary static user build.

Only relocatable objects are accepted. Symbol renaming keeps relocation indexes
and auxiliary records intact; it never reads or modifies executable images.
"""
import re
import struct
from omf_to_coff import Converter


def symbols(data):
    machine, count, _, offset, total, optional, _ = struct.unpack_from('<HHIIIHH', data)
    if machine != 0x14c or optional:
        raise ValueError('Expected ordinary i386 COFF (compile without /GL)')
    strings = offset + total * 18
    length = struct.unpack_from('<I', data, strings)[0]
    if strings + length != len(data):
        raise ValueError('Invalid COFF string table')
    index = 0
    while index < total:
        pos = offset + 18 * index
        raw, value, section, typ, storage, aux = struct.unpack_from('<8sIhHBB', data, pos)
        if raw[:4] == bytes(4):
            start = strings + struct.unpack_from('<I', raw, 4)[0]
            name = data[start:data.index(b'\0', start)].decode('latin1')
        else:
            name = raw.split(b'\0')[0].decode('latin1')
        yield pos, name, section, value, storage
        index += 1 + aux


def namespace(data, mapping):
    entries = list(symbols(data))
    output = bytearray(data)
    _, count, _, offset, total, _, _ = struct.unpack_from('<HHIIIHH', data)
    strings = offset + total * 18
    for pos, name, _, _, _ in entries:
        if name in mapping:
            encoded = mapping[name].encode('latin1')
            struct.pack_into('<II', output, pos, 0, len(output) - strings)
            output.extend(encoded + b'\0')
    struct.pack_into('<I', output, strings, len(output) - strings)
    # dllexport directives belong to the original DLL build, not the executable.
    for i in range(count):
        pos = 20 + i * 40
        name, _, _, size, raw = struct.unpack_from('<8sIIII', output, pos)
        if name.rstrip(b'\0') == b'.drectve':
            contents = bytes(output[raw:raw + size])
            contents = re.sub(rb'(?i)[/-]export:(?:"[^"]*"|\S+)',
                              lambda m: b' ' * len(m[0]), contents)
            output[raw:raw + size] = contents
    return bytes(output)


class Glue(Converter):
    def __init__(self):
        self.tag = 'static_glue'
        self.aliases = {}
        self.named, self.symbols, self.sections = {}, [], []

    def jump(self, name, target):
        section = self.section('.text', 5, 0x60100020, self.symbol(name))
        section.write(0, b'\xe9\0\0\0\0')
        section.relocations.append((1, self.symbol(target), 0x14))

    def pointer(self, name, target):
        section = self.section('.rdata', 4, 0x40300040, self.symbol(name))
        section.write(0, bytes(4))
        section.relocations.append((0, self.symbol(target), 6))
