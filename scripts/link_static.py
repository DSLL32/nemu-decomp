#!/usr/bin/env python3
"""Statically link the source-built PE32 components into the user executable.

This is an offline image linker, not a runtime DLL extractor or memory loader.
It preserves each compiler's runtime and resource namespace, applies HIGHLOW
relocations, resolves bundled imports, and combines Windows import descriptors.
The output has ordinary PE sections mapped directly by the Windows loader.
Only the fixed-address x86 images built by this repository are supported.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from pe_fingerprints import PE32, u16, u32, cstring

# Initialization order: imports before their users, then the plugin catalogue.
COMPONENTS = [
    ('kailleraclient.dll', False), ('gs_dll.dll', False),
    ('Plugin/audio.dll', True), ('Plugin/Input.dll', True),
    ('Plugin/LemD3D8.dll', True), ('Plugin/debugger.dll', True),
    ('Plugin/dialogs.dll', True), ('Plugin/profile.dll', True),
    ('Plugin/LemCombineDebugger.dll', True),
]
HOOKS = {
    'GetProcAddress': '_StaticGetProcAddress@8',
    'GetModuleHandleA': '_StaticGetModuleHandleA@4',
    'GetModuleHandleW': '_StaticGetModuleHandleW@4',
    'GetModuleFileNameA': '_StaticGetModuleFileNameA@12',
    'GetModuleFileNameW': '_StaticGetModuleFileNameW@12',
    'LoadLibraryA': '_StaticLoadLibraryA@4',
    'LoadLibraryW': '_StaticLoadLibraryW@4',
    'LoadLibraryExA': '_StaticLoadLibraryExA@12',
    'LoadLibraryExW': '_StaticLoadLibraryExW@12',
    'FreeLibrary': '_StaticFreeLibrary@4',
    'DisableThreadLibraryCalls': '_StaticDisableThreadLibraryCalls@4',
    'VirtualQuery': '_StaticVirtualQuery@12',
}
DELAYED_LIBRARIES = {'d3d8.dll', 'dsound.dll'}


def align(n, boundary=0x1000):
    return (n + boundary - 1) & -boundary


def put32(data, offset, value):
    struct.pack_into('<I', data, offset, value & 0xffffffff)


class Image(PE32):
    def __init__(self, path, shift=0):
        super().__init__(path)
        if u16(self.data, self.pe + 4) != 0x14c:
            raise ValueError(f'{path}: expected x86')
        for directory in (9, 13):
            if self.directory(directory)[0]:
                raise ValueError(f'{path}: input TLS/delay imports are unsupported')
        self.shift = shift
        self.size = u32(self.data, self.opt + 56)
        self.memory = bytearray(self.size)
        self.header_size = u32(self.data, self.opt + 60)
        self.memory[:self.header_size] = self.data[:self.header_size]
        for s in self.sections:
            rva, raw, size = s['rva'], s['offset'], s['raw_size']
            self.memory[rva:rva + size] = self.data[raw:raw + size]
        self.export_symbols = {}
        export_rva, export_size = self.directory(0)
        if export_rva:
            off = self.offset(export_rva)
            ordinal_base, count, names_count, funcs, names, ords = struct.unpack_from(
                '<IIIIII', self.data, off + 16)
            for i in range(count):
                rva = u32(self.memory, funcs + i * 4)
                if export_rva <= rva < export_rva + export_size:
                    raise ValueError(f'{path}: forwarded exports are unsupported')
                if rva:
                    self.export_symbols[ordinal_base + i] = rva
            for i in range(names_count):
                name = cstring(self.memory, u32(self.memory, names + i * 4))
                ordinal = ordinal_base + u16(self.memory, ords + i * 2)
                self.export_symbols[name] = self.export_symbols[ordinal]

    def address(self, name, base):
        try:
            return base + self.shift + self.export_symbols[name]
        except KeyError:
            raise ValueError(f'{self.path}: missing export {name!r}') from None

    def relocate(self, base):
        delta = base + self.shift - self.base
        reloc, size = self.directory(5)
        if delta and not reloc:
            raise ValueError(f'{self.path}: relocations required')
        pos = reloc
        while pos < reloc + size:
            page, length = struct.unpack_from('<II', self.memory, pos)
            if length < 8 or length % 2 or pos + length > reloc + size:
                raise ValueError(f'{self.path}: malformed relocation block')
            for off in range(pos + 8, pos + length, 2):
                entry = u16(self.memory, off)
                kind, target = entry >> 12, page + (entry & 0xfff)
                if kind == 3:
                    put32(self.memory, target, u32(self.memory, target) + delta)
                elif kind:
                    raise ValueError(f'{self.path}: unsupported relocation type {kind}')
            pos += length
        put32(self.memory, self.opt + 28, base + self.shift)

    def import_groups(self):
        pos, size = self.directory(1)
        if not pos:
            return
        limit = pos + size if size else self.size
        while pos + 20 <= limit and u32(self.memory, pos + 12):
            lookup, _, _, name, iat = struct.unpack_from('<IIIII', self.memory, pos)
            dll = cstring(self.memory, name)
            lookup = lookup or iat
            symbols = []
            while u32(self.memory, lookup):
                thunk = u32(self.memory, lookup)
                symbol = (thunk & 0xffff) if thunk & 0x80000000 else cstring(self.memory, thunk + 2)
                symbols.append((iat, symbol))
                lookup += 4
                iat += 4
            yield dll, symbols
            pos += 20


def link(executable, directory, runtime, output, report):
    root = Image(executable)
    base = root.base
    images = [root]
    cursor = align(root.size, 0x10000)
    for name, _ in COMPONENTS:
        image = Image(directory / name, cursor)
        images.append(image)
        cursor = align(cursor + image.size, 0x10000)
    helper = Image(runtime, cursor)
    images.append(helper)
    cursor = align(cursor + helper.size)
    if cursor + base >= 0x80000000:
        raise ValueError('Static image exceeds the supported address space')
    for image in images[1:]:
        image.relocate(base)
    builtins = {image.path.name.lower(): image for image in images[1:]}
    metadata_rva = cursor
    metadata = bytearray()

    def add(data, boundary=4):
        metadata.extend(bytes(align(len(metadata), boundary) - len(metadata)))
        rva = metadata_rva + len(metadata)
        metadata.extend(data)
        return rva

    imports = []
    static_imports = 0
    hook_imports = 0
    delayed = []
    for image in images:
        for dll, symbols in image.import_groups():
            run = []

            def flush():
                if not run:
                    return
                dll_name = add(dll.encode('ascii') + b'\0', 1)
                thunks = []
                for _, symbol in run:
                    thunks.append(0x80000000 | symbol if isinstance(symbol, int) else
                                  add(b'\0\0' + symbol.encode('ascii') + b'\0', 2))
                lookup = add(struct.pack('<' + 'I' * (len(thunks) + 1), *thunks, 0))
                imports.append((lookup, 0, 0, dll_name, image.shift + run[0][0]))
                run.clear()

            for iat, symbol in symbols:
                target = builtins.get(dll.lower())
                if target:
                    flush()
                    put32(image.memory, iat, target.address(symbol, base))
                    static_imports += 1
                elif image is not helper and dll.lower() == 'kernel32.dll' and symbol in HOOKS:
                    flush()
                    put32(image.memory, iat, helper.address(HOOKS[symbol], base))
                    hook_imports += 1
                elif dll.lower() in DELAYED_LIBRARIES:
                    flush()
                    delayed.append((dll, symbol, base + image.shift + iat))
                    put32(image.memory, iat, 0)
                else:
                    run.append((iat, symbol))
            flush()
    descriptors = b''.join(struct.pack('<IIIII', *entry) for entry in imports) + bytes(20)
    import_rva = add(descriptors)

    # Private component table consumed by the small statically linked runtime.
    module_table = helper.export_symbols['StaticModules']
    for i, ((name, plugin), image) in enumerate(zip(COMPONENTS, images[1:])):
        entry = u32(image.data, image.opt + 16)
        record = struct.pack('<64sIIIIII', Path(name).name.encode('ascii'),
            base + image.shift, image.size,
            base + image.shift + entry if entry else 0, int(plugin), 0, 0)
        helper.memory[module_table + 88 * i:module_table + 88 * (i + 1)] = record
    put32(helper.memory, helper.export_symbols['StaticModuleCount'], len(COMPONENTS))
    if len(delayed) > 32:
        raise ValueError('Too many delayed system imports')
    delayed_table = helper.export_symbols['StaticDelayedImports']
    for i, (dll, symbol, slot) in enumerate(delayed):
        record = struct.pack('<64s128sII', dll.encode('ascii'),
            b'' if isinstance(symbol, int) else symbol.encode('ascii'),
            symbol if isinstance(symbol, int) else 0, slot)
        helper.memory[delayed_table + 200 * i:delayed_table + 200 * (i + 1)] = record
    put32(helper.memory, helper.export_symbols['StaticDelayedImportCount'], len(delayed))

    # One TLS callback fans out native loader notifications to the components.
    tls_index = add(bytes(4))
    callbacks = add(struct.pack('<II', helper.address('_StaticTlsCallback@12', base), 0))
    tls_rva = add(struct.pack('<IIIIII', base + tls_index, base + tls_index,
                             base + tls_index, base + callbacks, 0, 0))
    code_rva = align(metadata_rva + len(metadata))
    initialize = helper.address('StaticInitialize', base)
    original_entry = base + u32(root.data, root.opt + 16)
    code = b'\xe8' + struct.pack('<i', initialize - (base + code_rva + 5))
    code += b'\xe9' + struct.pack('<i', original_entry - (base + code_rva + 10))

    sections = []
    for index, image in enumerate(images):
        if image is not root:
            sections.append((f'h{index}', image.shift, align(image.header_size),
                             bytes(image.memory[:image.header_size]), 0x40000040))
        for s in image.sections:
            section_index = image.sections.index(s)
            characteristics = u32(image.data, image.opt + u16(image.data, image.pe + 20) +
                                  section_index * 40 + 36)
            size = max(s['virtual_size'], s['raw_size'])
            data = bytes(image.memory[s['rva']:s['rva'] + size]).rstrip(b'\0')
            # Private resources/headers/export metadata must remain available.
            characteristics &= ~0x02000000  # IMAGE_SCN_MEM_DISCARDABLE
            sections.append((f'm{index}{s["name"]}'[:8], image.shift + s['rva'],
                             size, data, characteristics))
    sections.append(('.stdata', metadata_rva, len(metadata), bytes(metadata), 0xc0000040))
    sections.append(('.stcode', code_rva, len(code), code, 0x60000020))
    if len(sections) > 96:
        raise ValueError('Too many sections for the Windows image loader')
    table = root.opt + u16(root.data, root.pe + 20)
    header_size = align(table + 40 * len(sections), 0x200)
    if header_size > min(s[1] for s in sections):
        raise ValueError('Combined section headers overlap code')
    headers = bytearray(header_size)
    headers[:table] = root.memory[:table]
    struct.pack_into('<H', headers, root.pe + 6, len(sections))
    # The main executable has no relocations; retain its fixed preferred base.
    struct.pack_into('<H', headers, root.pe + 22, u16(headers, root.pe + 22) | 1)
    struct.pack_into('<H', headers, root.opt + 70, u16(headers, root.opt + 70) & ~0x40)
    put32(headers, root.opt + 16, code_rva)
    put32(headers, root.opt + 56, align(code_rva + len(code)))
    put32(headers, root.opt + 60, header_size)
    put32(headers, root.opt + 36, 0x200)  # repacked file alignment
    put32(headers, root.opt + 64, 0)  # checksum
    for index in (4, 5, 6, 11, 12):  # certificates, reloc/debug/bound imports, IAT span
        struct.pack_into('<II', headers, root.opt + 96 + 8 * index, 0, 0)
    struct.pack_into('<II', headers, root.opt + 104, import_rva, len(descriptors))
    struct.pack_into('<II', headers, root.opt + 96 + 8 * 9, tls_rva, 24)
    result = headers
    code_size = initialized = uninitialized = 0
    for i, (name, rva, size, data, flags) in enumerate(sections):
        raw_size = align(len(data), 0x200) if data else 0
        offset = len(result) if data else 0
        struct.pack_into('<8sIIIIIIHHI', result, table + i * 40,
                         name.encode('ascii'), size, rva, raw_size, offset, 0, 0, 0, 0, flags)
        result.extend(data)
        result.extend(bytes(raw_size - len(data)))
        if flags & 0x20:
            code_size += raw_size
        elif flags & 0x40:
            initialized += raw_size
        if flags & 0x80:
            uninitialized += size
    struct.pack_into('<III', result, root.opt + 4, code_size, initialized, uninitialized)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(result)
    summary = dict(
        kind='offline static PE32 link', output=str(output),
        sha256=hashlib.sha256(result).hexdigest(), sections=len(sections),
        image_base=hex(base), image_size=hex(align(code_rva + len(code))),
        resolved_component_imports=static_imports, runtime_adapters=hook_imports,
        delayed_system_imports=[dict(dll=dll, symbol=symbol) for dll, symbol, _ in delayed],
        inputs=[dict(path=str(image.path), sha256=hashlib.sha256(image.data).hexdigest(),
                     rva=hex(image.shift)) for image in images],
        windows_imports=sorted({group['dll'].lower() for group in PE32(output).imports()}))
    if any(name in builtins for name in summary['windows_imports']):
        output.unlink()
        raise ValueError('Unresolved bundled import remains')
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(summary, indent=2) + '\n')
    print(f'{output}: {len(COMPONENTS)} static components, {len(sections)} sections, '
          f'{len(result):,} bytes; report: {report}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=Path('obj/static/nemu64.exe'))
    parser.add_argument('--bin', type=Path, default=Path('bin'))
    parser.add_argument('--runtime', type=Path, default=Path('obj/static/nemu_static.dll'))
    parser.add_argument('--output', type=Path, default=Path('bin/nemu64-static.exe'))
    parser.add_argument('--report', type=Path, default=Path('obj/static/link.json'))
    args = parser.parse_args()
    link(args.exe, args.bin, args.runtime, args.output, args.report)


if __name__ == '__main__':
    main()
