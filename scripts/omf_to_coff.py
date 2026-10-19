#!/usr/bin/env python3
"""Convert Delphi 6's 32-bit OMF objects to Microsoft i386 COFF.

Preserves Borland virtual function/data segments as COFF COMDATs, including
relocations and initialization records. No disassembly, PE images, DLL loading,
or third-party converter is involved. Unsupported semantic records fail closed.
"""
import argparse
from dataclasses import dataclass, field
import json
from pathlib import Path
import re
import struct


class ConversionError(ValueError):
    pass


class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def take(self, size):
        if size < 0 or self.pos + size > len(self.data):
            raise ConversionError('Truncated OMF record')
        value = self.data[self.pos:self.pos + size]
        self.pos += size
        return value

    def number(self, size):
        return int.from_bytes(self.take(size), 'little')

    def byte(self):
        return self.number(1)

    def index(self):
        first = self.byte()
        return ((first & 0x7f) << 8) | self.byte() if first & 0x80 else first

    def string(self):
        return self.take(self.byte()).decode('latin1')

    def length(self):
        first = self.byte()
        if first <= 0x80:
            return first
        sizes = {0x81: 2, 0x84: 3, 0x88: 4}
        if first not in sizes:
            raise ConversionError(f'Unsupported communal length {first:#x}')
        return self.number(sizes[first])

    def __bool__(self):
        return self.pos < len(self.data)


def records(data):
    pos = 0
    while pos < len(data):
        if pos + 3 > len(data):
            raise ConversionError('Truncated OMF record header')
        kind, length = struct.unpack_from('<BH', data, pos)
        end = pos + 3 + length
        if length < 1 or end > len(data):
            raise ConversionError(f'Invalid OMF record length at {pos:#x}')
        # Delphi deliberately writes zero instead of computing record checksums.
        if data[end - 1] and sum(data[pos:end]) & 255:
            raise ConversionError(f'Invalid OMF checksum at {pos:#x}')
        yield kind, data[pos + 3:end - 1]
        pos = end


@dataclass(eq=False)
class Symbol:
    name: str
    section: object = None
    value: int = 0
    storage: int = 2
    absolute: bool = False


@dataclass(eq=False)
class Section:
    name: str
    size: int
    flags: int
    key: Symbol
    data: bytearray = field(default_factory=bytearray)
    relocations: list = field(default_factory=list)
    number: int = 0

    def write(self, offset, data):
        if offset < 0 or offset + len(data) > self.size:
            raise ConversionError(f'Data exceeds section {self.name}')
        self.data.extend(bytes(max(0, offset + len(data) - len(self.data))))
        self.data[offset:offset + len(data)] = data


class Converter:
    def __init__(self, data, aliases=None):
        self.records = list(records(data))
        if not self.records or self.records[0][0] not in (0x80, 0x82):
            raise ConversionError('Expected an OMF object, not a library or COFF image')
        if (sum(kind in (0x80, 0x82) for kind, _ in self.records) != 1 or
                sum(kind in (0x8a, 0x8b) for kind, _ in self.records) != 1 or
                self.records[-1][0] not in (0x8a, 0x8b)):
            raise ConversionError('Expected exactly one complete OMF module')
        self.module = Path(Reader(self.records[0][1]).string().replace('\\', '/')).stem
        self.tag = re.sub(r'[^a-zA-Z0-9_]', '_', self.module)
        self.aliases = aliases or {}
        self.names = ['']
        self.segments = [None]
        self.groups = [None]
        self.externals = [None]
        self.virtuals = [None]
        self.sections = []
        self.symbols = []
        self.named = {}
        self.comments = []
        self.data_imports = {}
        self.unit_initializers = []
        self.unit_finalizers = []

    def symbol(self, name, local=False):
        if local or re.match(r'^@(Finalization|initialization)(\$|$)', name):
            name = f'__delphi_{self.tag}_{name}'
        name = self.aliases.get(name, name)
        if name not in self.named:
            self.named[name] = Symbol(name)
            self.symbols.append(self.named[name])
        return self.named[name]

    def section(self, source, size, flags, key=None):
        index = len(self.sections) + 1
        key = key or self.symbol(f'__delphi_{self.tag}_segment_{index}')
        if key.section is not None:
            raise ConversionError(f'Duplicate definition of {key.name}')
        # ANY COMDAT retains Delphi's function-level selection. Each regular
        # segment also needs an independent key to support dead-code removal.
        s = Section(source, size, flags | 0x1000, key)
        s.number = index
        key.section = s
        self.sections.append(s)
        return s

    def segment(self, index):
        if index & 0xc000 == 0x4000:
            return self.virtuals[index & 0x3fff]
        return self.segments[index]

    def declarations(self):
        for kind, data in self.records:
            r = Reader(data)
            if kind in (0x96, 0xca):  # LNAMES / LLNAMES
                while r:
                    self.names.append(r.string())
            elif kind in (0x98, 0x99):  # SEGDEF
                attributes = r.byte()
                alignment = attributes >> 5
                if alignment == 0:
                    raise ConversionError('Absolute OMF segments are unsupported')
                size = r.number(4 if kind & 1 else 2)
                if attributes & 2:
                    size = 1 << (32 if kind & 1 else 16)
                name, cls, _ = (self.names[r.index()] for _ in range(3))
                alignments = {1: 1, 2: 2, 3: 5, 4: 9, 5: 3}
                if alignment not in alignments or not attributes & 1:
                    raise ConversionError('Expected a supported USE32 segment')
                name_upper, cls_upper = name.upper(), cls.upper()
                if cls_upper == 'CODE':
                    output, flags = '.text', 0x60000020
                elif cls_upper == 'BSS':
                    output, flags = '.bss', 0xc0000080
                elif name_upper == '_INIT_':
                    output, flags = '.dinit', 0x40000040
                elif name_upper == '_EXIT_':
                    output, flags = '.dfini', 0x40000040
                elif name_upper == '_TLS':
                    # Sort after MSVC's .tls start sentinel and before its
                    # .tls$ZZZ end sentinel so the loader copies these bytes.
                    output, flags = '.tls$D', 0xc0000040
                elif 'DEBUG' in cls_upper or cls_upper.startswith('DEB'):
                    # Debug records are not executable semantics; drop them.
                    self.segments.append(None)
                    continue
                else:
                    output, flags = '.data', 0xc0000040
                flags |= alignments[alignment] << 20
                s = self.section(output, size, flags)
                self.segments.append(s)
                if output == '.dinit' and size:
                    s.key.name = '__delphi_init_' + self.tag
                    self.unit_initializers.append(s.key.name)
                elif output == '.dfini' and size:
                    s.key.name = '__delphi_fini_' + self.tag
                    self.unit_finalizers.append(s.key.name)
                if r:
                    raise ConversionError('Trailing SEGDEF data')
            elif kind == 0x9a:  # GRPDEF
                name = self.names[r.index()]
                segments = []
                while r:
                    if r.byte() != 0xff:
                        raise ConversionError('Unsupported GRPDEF component')
                    segments.append(r.index())
                self.groups.append((name, segments))
            elif kind in (0x8c, 0xb4):  # EXTDEF / LEXTDEF
                while r:
                    name, _ = r.string(), r.index()
                    self.externals.append(self.symbol(name, kind == 0xb4))
            elif kind in (0xb0, 0xb8):  # COMDEF / LCOMDEF
                while r:
                    name, _ = r.string(), r.index()
                    dtype, size = r.byte(), r.length()
                    symbol = self.symbol(name, kind == 0xb8)
                    self.externals.append(symbol)
                    if 0 < dtype < 0x60:
                        template = self.segments[dtype]
                        if template is None:
                            self.virtuals.append(None)
                        else:
                            self.virtuals.append(self.section(template.name, size,
                                                               template.flags, symbol))
                    elif dtype == 0x62:  # NEAR communal variable
                        symbol.value = size
                    else:
                        raise ConversionError(f'Unsupported COMDEF type {dtype:#x}')
            elif kind == 0x88:
                self.comments.append(data)
        # Public symbols may refer to segment declarations later in the file.
        for kind, data in self.records:
            if kind not in (0x90, 0x91, 0xb6, 0xb7):
                continue
            r = Reader(data)
            group, segment = r.index(), r.index()
            frame = r.number(2) if not segment else None
            while r:
                name, value, _ = r.string(), r.number(4 if kind & 1 else 2), r.index()
                symbol = self.symbol(name, kind in (0xb6, 0xb7))
                section = self.segment(segment) if segment else None
                if symbol.section is not None and (symbol.section is not section or symbol.value != value):
                    raise ConversionError(f'Conflicting public symbol {symbol.name}')
                symbol.section, symbol.value = section, value
                if frame is not None:
                    symbol.absolute, symbol.value = True, frame * 16 + value
        # Directive comments can change link semantics. Only compiler identity,
        # Borland metadata/dependencies, and standard end-pass markers are inert.
        for data in self.comments:
            if len(data) < 2 or data[1] not in (0, 0xfb, 0xa2):
                raise ConversionError(f'Unsupported semantic COMENT {data.hex()}')
            if data[1:3] == b'\xfb\x00':
                # With Delphi's $G+ an external data reference addresses a
                # pointer cell even when the final program is statically linked.
                # Borland records these references in its external-data marker.
                r = Reader(data[3:])
                index = r.index()
                if len(r.data) - r.pos != 4:
                    raise ConversionError('Malformed Borland external-data marker')
                mode = r.number(4)
                if mode == 4:
                    # A class import already names a VMT pointer cell emitted
                    # by Delphi at the start of the class's virtual segment.
                    continue
                if mode != 1:
                    raise ConversionError(f'Unsupported Borland data import mode {mode}')
                original = self.externals[index]
                key = self.symbol(f'__delphi_{self.tag}_import_{index}')
                cell = self.section('.rdata', 4, 0x40300040, key)
                cell.write(0, bytes(4))
                cell.relocations.append((0, original, 6))
                self.data_imports[index] = key
            elif data[1] == 0xfb and (len(data) < 3 or data[2] not in (8, 10, 12)):
                raise ConversionError(f'Unsupported Borland COMENT {data.hex()}')

    def contents(self):
        last = None
        target_threads = {}
        for kind, data in self.records:
            r = Reader(data)
            if kind in (0xa0, 0xa1, 0xa2, 0xa3):
                s = self.segment(r.index())
                offset = r.number(4 if kind & 1 else 2)
                if kind in (0xa0, 0xa1):
                    payload = r.take(len(r.data) - r.pos)
                else:
                    def expand(depth=0):
                        if depth > 32:
                            raise ConversionError('LIDATA recursion limit exceeded')
                        repeat = r.number(4 if kind & 1 else 2)
                        count = r.number(2)
                        payload = b''.join(expand(depth + 1) for _ in range(count)) if count else r.take(r.byte())
                        if len(payload) * repeat > 128 * 1024 * 1024:
                            raise ConversionError('LIDATA expansion limit exceeded')
                        return payload * repeat
                    chunks = []
                    while r:
                        chunks.append(expand())
                    payload = b''.join(chunks)
                if s is not None:
                    s.write(offset, payload)
                last = (s, offset, len(payload), kind in (0xa2, 0xa3))
            elif kind in (0x9c, 0x9d):
                while r:
                    first = r.byte()
                    if not first & 0x80:  # FRAME / TARGET THREAD
                        frame, method, thread = bool(first & 0x40), (first >> 2) & 7, first & 3
                        datum = r.index() if method < 4 else None
                        if not frame:
                            target_threads[thread] = (method & 3, datum)
                        continue
                    second, fix = r.byte(), r.byte()
                    location = (first >> 2) & 15
                    position = ((first & 3) << 8) | second
                    if not fix & 0x80 and (fix >> 4) & 7 < 4:
                        r.index()  # Frame datum; flat 32-bit addresses need no selector.
                    if fix & 8:
                        method, target = target_threads[fix & 3]
                    else:
                        method, target = fix & 3, r.index()
                    displacement = r.number(4 if kind & 1 else 2) if not fix & 4 else 0
                    if last is None:
                        raise ConversionError('FIXUPP has no preceding data record')
                    s, offset, size, iterated = last
                    if s is None:
                        continue
                    if iterated:
                        raise ConversionError('Relocations in LIDATA are unsupported')
                    if location not in (9, 13) or position + 4 > size:
                        raise ConversionError(f'Unsupported or out-of-range FIXUPP location {location}')
                    address = offset + position
                    inline = struct.unpack_from('<I', s.data, address)[0]
                    value = (inline + displacement) & 0xffffffff
                    if method == 0:
                        dest = self.segment(target)
                        if dest is None:
                            raise ConversionError('Executable relocation refers to discarded debug data')
                        symbol = dest.key
                    elif method == 2:
                        symbol = self.data_imports.get(target, self.externals[target])
                    else:
                        raise ConversionError(f'Unsupported FIXUPP target method {method}')
                    struct.pack_into('<I', s.data, address, value)
                    relocation = 6 if first & 0x40 else 0x14
                    if symbol.section is not None and symbol.section.name == '.tls$D':
                        if relocation != 6:
                            raise ConversionError('PC-relative TLS relocation is unsupported')
                        # Delphi uses threadvar addresses as offsets into the
                        # TLS block, not as image virtual addresses.
                        relocation = 0x0b  # IMAGE_REL_I386_SECREL
                    s.relocations.append((address, symbol, relocation))
            elif kind in (0x8a, 0x8b):
                if r.byte() & 0x40:
                    raise ConversionError('OMF start-address records are unsupported')
            elif kind not in (0x80, 0x82, 0x88, 0x8c, 0x90, 0x91, 0x96,
                              0x98, 0x99, 0x9a, 0xb0, 0xb4, 0xb6, 0xb7, 0xb8, 0xca):
                raise ConversionError(f'Unsupported OMF record {kind:#x}')

    def coff(self):
        self.declarations()
        self.contents()
        strings = bytearray(4)
        string_offsets = {}

        def string(name):
            if name not in string_offsets:
                string_offsets[name] = len(strings)
                strings.extend(name.encode('latin1') + b'\0')
            return string_offsets[name]

        def symbol_name(name):
            encoded = name.encode('latin1')
            return encoded.ljust(8, b'\0') if len(encoded) <= 8 else struct.pack('<II', 0, string(name))

        symbols = bytearray()
        symbol_indices = {}
        for s in self.sections:
            # A COMDAT section definition followed by its external key is the
            # layout understood by Microsoft's linker and librarian.
            symbols.extend(struct.pack('<8sIhHBB', symbol_name(s.name), 0, s.number, 0, 3, 1))
            symbols.extend(struct.pack('<IHHIhBBH', s.size, len(s.relocations), 0, 0, 0, 2, 0, 0))
            symbol_indices[s.key] = len(symbols) // 18
            symbols.extend(struct.pack('<8sIhHBB', symbol_name(s.key.name), 0, s.number, 0, 2, 0))
        for sym in self.symbols:
            if sym in symbol_indices:
                continue
            symbol_indices[sym] = len(symbols) // 18
            section = -1 if sym.absolute else sym.section.number if sym.section else 0
            symbols.extend(struct.pack('<8sIhHBB', symbol_name(sym.name), sym.value,
                                       section, 0, sym.storage, 0))
        if len(self.sections) > 32767:
            raise ConversionError('COFF section count requires bigobj support')
        output = bytearray(20 + 40 * len(self.sections))
        for i, s in enumerate(self.sections):
            if len(s.relocations) > 65535:
                raise ConversionError('COFF relocation overflow is unsupported')
            while len(output) % 4:
                output.append(0)
            # BSS has a virtual size in the COFF SizeOfRawData field, no bytes.
            bss = bool(s.flags & 0x80)
            if bss and any(s.data):
                raise ConversionError('Nonzero data in an uninitialized section')
            raw = 0 if bss or not s.size else len(output)
            if raw:
                output.extend(s.data)
                output.extend(bytes(s.size - len(s.data)))
            relocs = len(output) if s.relocations else 0
            for address, sym, kind in s.relocations:
                output.extend(struct.pack('<IIH', address, symbol_indices[sym], kind))
            name = s.name.encode('latin1')
            if len(name) > 8:
                name = ('/' + str(string(s.name))).encode('ascii')
            struct.pack_into('<8sIIIIIIHHI', output, 20 + 40 * i, name, 0, 0,
                             s.size, raw, relocs, 0, len(s.relocations), 0, s.flags)
        symbol_offset = len(output)
        output.extend(symbols)
        struct.pack_into('<I', strings, 0, len(strings))
        output.extend(strings)
        struct.pack_into('<HHIIIHH', output, 0, 0x14c, len(self.sections), 0,
                         symbol_offset, len(symbols) // 18, 0, 0)
        return bytes(output)


def convert(source, destination, aliases=None):
    converter = Converter(source.read_bytes(), aliases)
    result = converter.coff()
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(result)
    return dict(source=str(source), output=str(destination), module=converter.module,
                sections=len(converter.sections), symbols=len(converter.symbols),
                relocations=sum(len(s.relocations) for s in converter.sections),
                initialization_symbols=converter.unit_initializers,
                finalization_symbols=converter.unit_finalizers)


def import_library_aliases(paths):
    """Read Microsoft's archive index to spell foreign C/stdcall symbols.

    This changes only symbol spelling, using the actual import library's
    decoration. It does not translate C++ ABIs or infer calling conventions.
    """
    aliases = {}
    for path in paths:
        data = Path(path).read_bytes()
        if data[:8] != b'!<arch>\n' or data[8:24].strip() != b'/':
            raise ConversionError(f'{path}: expected a Microsoft import library index')
        size = int(data[56:66])
        member = data[68:68 + size]
        if len(member) < 4 or len(member) != size:
            raise ConversionError(f'{path}: truncated archive index')
        count = int.from_bytes(member[:4], 'big')
        if 4 + count * 4 > len(member):
            raise ConversionError(f'{path}: invalid archive index')
        names = member[4 + count * 4:].split(b'\0')
        if len(names) < count:
            raise ConversionError(f'{path}: truncated archive symbols')
        for encoded in names[:count]:
            symbol = encoded.decode('ascii')
            match = re.fullmatch(r'_([A-Za-z][A-Za-z_0-9]*)(?:@(\d+))?', symbol)
            if not match:
                continue
            plain = match[1]
            if plain in aliases and aliases[plain] != symbol:
                raise ConversionError(f'Ambiguous foreign symbol {plain}: {aliases[plain]}, {symbol}')
            aliases[plain] = symbol
    return aliases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--aliases', type=Path, help='JSON mapping of OMF names to COFF names')
    parser.add_argument('--import-lib', type=Path, action='append', default=[],
                        help='Microsoft import library for foreign symbol spelling; repeatable')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        aliases = import_library_aliases(args.import_lib)
        if args.aliases:
            aliases.update(json.loads(args.aliases.read_text()))
        report = convert(args.input, args.output, aliases)
    except (ConversionError, IndexError, KeyError, struct.error) as error:
        parser.exit(1, f'{args.input}: {error}\n')
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
