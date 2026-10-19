#!/usr/bin/env python3
"""Recover native import-thunk placement using ordinary COFF link inputs.

LIB /LINK50COMPAT supplies full COFF import records. Expanding the selected
SDK short records into that form lets LINK order their contributions without
changing application instructions. Only import declarations and section names
are reconstructed; all executable payloads originate in the native tools/SDK.
"""

import hashlib
from pathlib import Path
import struct

from build_lemd3d_link_inputs import change_symbols
from build_nemu_link_inputs import archive_for
from map_nemu_libraries import archive_members
from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]
LIBRARIES = {
    'msvcrt': '66cde1a18119df60e3770299c41c1150b9bc5efa5125edf6c8e432836211ff13',
    'ws2_32': 'dbe9de991aea6662d8857da13e572729b5379377be8e13bdd86e57a51354988e',
}
CRT_IMPORTS = {'??3@YAXPAX@Z', '??2@YAPAXI@Z', '___CxxFrameHandler',
               '___dllonexit', '__initterm'}
THUNK_ORDER = [
    'closesocket@4', 'shutdown@8', 'ntohs@4', 'getsockname@12',
    'ioctlsocket@12', 'bind@12', 'htonl@4', 'socket@12', 'htons@4',
    'gethostbyname@4', 'inet_addr@4', 'sendto@24', 'recvfrom@24',
    '__WSAFDIsSet@8', 'select@20', 'gethostbyaddr@12', 'recv@16',
    'send@16', 'WSAGetLastError@0', 'connect@12', 'WSAStartup@8',
    'WSACleanup@0', '??3@YAXPAX@Z',
]
NAMED_DEF = 'LIBRARY MSVCRT.dll\nEXPORTS\n    sample\n'
ORDINAL_DEF = 'LIBRARY WS2_32.dll\nEXPORTS\n    sample @1 NONAME\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def indexes(data):
    size = int(data[56:66])
    first = data[68:68 + size]
    count = struct.unpack_from('>I', first)[0]
    offsets = struct.unpack_from('>' + 'I' * count, first, 4)
    names = first[4 + 4 * count:].split(b'\0')[:count]
    result = {}
    for offset, name in zip(offsets, names):
        result.setdefault(offset, []).append(name.decode('cp1252'))
    return result


def read_template(path, inspection, ordinal):
    candidates = []
    for _, _, body in archive_members(path):
        if body[:2] != b'L\1' or struct.unpack_from('<H', body, 16)[0]:
            continue
        inspection.write_bytes(body)
        coff = COFF(inspection)
        if any(s['name'] == '.text' for s in coff.sections):
            candidates.append((body, coff))
    if len(candidates) != 1:
        raise ValueError('Expected one native LIB import template')
    body, coff = candidates[0]
    expected = ['.text', '.idata$5', '.idata$4'] + ([] if ordinal else ['.idata$6'])
    if [s['name'] for s in coff.sections] != expected:
        raise ValueError('Native import template section structure changed')
    text = coff.sections[0]
    if (text['data'] != bytes.fromhex('ff2500000000') or
            [(r['offset'], r['type'], r['symbol']['name'])
             for r in text['relocations']] != [(2, 6, '__imp__sample')]):
        raise ValueError('Native LIB did not emit the standard SDK import thunk')
    return body, coff


def rebuild(body, coff, names=None, payloads=None):
    """Rewrap COFF metadata and import declarations; retain relocation records."""
    names, payloads = names or {}, payloads or {}
    table, count = struct.unpack_from('<II', body, 8)
    symbols = bytearray(body[table:table + 18 * count])
    strings = body[table + 18 * count:]
    headers, tail = [], bytearray()
    position = 20 + 40 * len(coff.sections)
    for section in coff.sections:
        at = 20 + 40 * (section['index'] - 1)
        header = bytearray(body[at:at + 40])
        name = names.get(section['name'], section['name']).encode('ascii')
        if len(name) > 8:
            raise ValueError('Import section name exceeds the standard COFF field')
        header[:8] = name.ljust(8, b'\0')
        payload = payloads.get(section['name'], section['data'])
        if section['characteristics'] & (0x20 | 0x20000000):
            if payload != section['data']:
                raise ValueError('Executable import contents changed')
        stored = payload if section['offset'] else b''
        struct.pack_into('<II', header, 16, len(payload), position if stored else 0)
        tail.extend(stored)
        position += len(stored)
        relocation_count = struct.unpack_from('<H', header, 32)[0]
        if relocation_count == 0xffff or struct.unpack_from('<H', header, 34)[0]:
            raise ValueError('Unexpected import relocation overflow or line records')
        relocation_start = struct.unpack_from('<I', header, 24)[0]
        relocations = body[relocation_start:relocation_start + 10 * relocation_count]
        struct.pack_into('<I', header, 24, position if relocation_count else 0)
        tail.extend(relocations)
        position += len(relocations)
        headers.append(header)
        for symbol in coff.symbols:
            if (symbol['section'] == section['index'] and symbol['storage_class'] == 3
                    and symbol['name'] == section['name'] and symbol['auxiliary']):
                struct.pack_into('<I', symbols, 18 * (symbol['index'] + 1), len(payload))
    header = bytearray(body[:20])
    struct.pack_into('<I', header, 8, position)
    return bytes(header + b''.join(headers) + tail + symbols + strings)


def expand(short, template, inspection):
    _, _, _, machine, _, size, hint, flags = struct.unpack_from('<HHHHIIHH', short)
    symbol, dll = [s.decode('cp1252') for s in short[20:].split(b'\0')[:2]]
    kind = (flags >> 2) & 7
    if machine != 0x14c or flags & 3 or size != len(short) - 20 or kind not in (0, 1, 2, 3):
        raise ValueError('Expected an ordinary SDK i386 code-import record')
    export = (symbol if kind == 1 else symbol[1:] if kind == 2
              else symbol.lstrip('_').split('@')[0] if kind == 3 else None)
    base, coff = template
    descriptor = next(s['name'] for s in coff.symbols
                      if s['name'].startswith('__IMPORT_DESCRIPTOR_'))
    data = change_symbols(base, coff, renames={
        '_sample': symbol, '__imp__sample': '__imp_' + symbol,
        descriptor: '__IMPORT_DESCRIPTOR_' + dll[:-4].upper(),
    })
    inspection.write_bytes(data)
    parsed = COFF(inspection)
    if kind == 0:
        payloads = {name: struct.pack('<I', hint | 0x80000000)
                    for name in ('.idata$4', '.idata$5')}
    else:
        payload = struct.pack('<H', hint) + export.encode('cp1252') + b'\0'
        payloads = {'.idata$6': payload + b'\0' * (len(payload) & 1)}
    group = '.text$v' if dll.lower() == 'ws2_32.dll' or symbol == '??3@YAXPAX@Z' else '.text$w'
    data = rebuild(data, parsed, {'.text': group}, payloads)
    inspection.write_bytes(data)
    after = COFF(inspection)
    if [s['data'] for s in parsed.sections if s['name'] == '.text'] != [
            s['data'] for s in after.sections if s['name'] == group]:
        raise ValueError('Native import instruction bytes changed')
    return data, symbol


def common_declarations(source, output, inspection):
    """Supply only the SDK's COMMON declarations in recovered native order.

    LINK allocates COMMON definitions in reverse discovery order.  Their first
    discovery need not also select the CRT code contributions that use them.
    This sectionless object provides no initialized or executable allocation.
    """
    if digest(Path(source).read_bytes()) != LIBRARIES['msvcrt']:
        raise ValueError('Pinned SDK COMMON source archive changed')
    declarations = [
        ('atonexit.obj', '___onexitbegin'),
        ('atonexit.obj', '___onexitend'),
        ('crtdll.obj', '__pRawDllMain'),
        ('adjustfd.obj', '__adjust_fdiv'),
    ]
    members = {}
    for name, offset, body in archive_members(source):
        if name.rsplit('\\', 1)[-1] not in {item[0] for item in declarations}:
            continue
        short_name = name.rsplit('\\', 1)[-1]
        if short_name in members:
            raise ValueError('Duplicate native CRT COMMON source member')
        inspection.write_bytes(body)
        members[short_name] = (name, offset, body, COFF(inspection))
    if set(members) != {member for member, _ in declarations}:
        raise ValueError('Missing native CRT COMMON source member')
    symbols, strings, rows = bytearray(), bytearray(4), []
    for member, name in declarations:
        source_name, offset, body, coff = members[member]
        found = [symbol for symbol in coff.symbols if symbol['name'] == name]
        if len(found) != 1:
            raise ValueError('Native CRT COMMON declaration is not unique: ' + name)
        symbol = found[0]
        if (symbol['section'] != 0 or symbol['value'] != 4 or symbol['type'] != 0 or
                symbol['storage_class'] != 2 or symbol['auxiliary']):
            raise ValueError('Native CRT COMMON declaration changed: ' + name)
        string_offset = len(strings)
        strings.extend(name.encode('ascii') + b'\0')
        symbols.extend(struct.pack('<IIIhHBB', 0, string_offset, symbol['value'],
                                   symbol['section'], symbol['type'],
                                   symbol['storage_class'], 0))
        rows.append(dict(symbol=name, member=source_name, member_body_offset=offset,
                         source_member_sha256=digest(body), bytes=symbol['value']))
    struct.pack_into('<I', strings, 0, len(strings))
    header = struct.pack('<HHIIIHH', 0x14c, 0, 0, 20, len(rows), 0, 0)
    data = header + symbols + strings
    path = output / 'crt-common-declarations.obj'
    path.write_bytes(data)
    checked = COFF(path)
    if checked.sections or [symbol['name'] for symbol in checked.symbols] != [
            name for _, name in declarations]:
        raise ValueError('Native CRT COMMON metadata serialization changed')
    return dict(path=str(path), sha256=digest(data), declarations=rows,
                section_count=0, initialized_or_executable_bytes=0,
                original_sdk_declaration_types_and_sizes_unchanged=True)


def build(output, named_template, ordinal_template):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    inspection = output / 'import-inspection.obj'
    templates = {False: read_template(named_template, inspection, False),
                 True: read_template(ordinal_template, inspection, True)}
    report = dict(libraries={}, executable_bytes_copied_from_reference=False,
                  application_objects_modified=False,
                  native_import_instruction_payloads_unchanged=True)
    paths, all_converted = {}, []
    for library, expected in LIBRARIES.items():
        source = ROOT / 'toolchains/vc6rtm/lib' / (library + '.lib')
        original = source.read_bytes()
        if digest(original) != expected:
            raise ValueError('Pinned SDK archive changed: ' + library)
        if library == 'msvcrt':
            report['common_declarations'] = common_declarations(source, output, inspection)
        index, members, converted, renamed = indexes(original), [], [], []
        for name, offset, body in archive_members(source):
            symbols = index.get(offset - 60, [])
            if body[:4] == b'\0\0\xff\xff':
                symbol = body[20:].split(b'\0')[0].decode('cp1252')
                if ((library == 'ws2_32' and symbol[1:] in THUNK_ORDER)
                        or symbol in CRT_IMPORTS):
                    ordinal = ((struct.unpack_from('<H', body, 18)[0] >> 2) & 7) == 0
                    before = digest(body)
                    body, symbol = expand(body, templates[ordinal], inspection)
                    converted.append(dict(symbol=symbol, short_record_sha256=before,
                                          full_coff_sha256=digest(body)))
            elif (library == 'msvcrt' and body[:2] == b'L\1'
                  and struct.unpack_from('<H', body, 16)[0] == 0):
                inspection.write_bytes(body)
                coff = COFF(inspection)
                if any(s['name'] == '.text' for s in coff.sections):
                    before = body
                    body = rebuild(body, coff, {'.text': '.text$w'})
                    alignment = []
                    for symbol in coff.symbols:
                        if symbol['name'] != '__CRT_INIT@12' or symbol['section'] <= 0:
                            continue
                        section = coff.sections[symbol['section'] - 1]
                        flags = section['characteristics']
                        if section['name'] != '.text' or flags & 0x00f00000 != 0x00100000:
                            raise ValueError('Unexpected native CRT initialization alignment')
                        data = bytearray(body)
                        new_flags = (flags & ~0x00f00000) | 0x00500000
                        struct.pack_into('<I', data, 20 + 40 * (symbol['section'] - 1) + 36,
                                         new_flags)
                        body = bytes(data)
                        alignment.append(dict(symbol=symbol['name'], original=1, recovered=16))
                    inspection.write_bytes(body)
                    after = COFF(inspection)
                    if [s['data'] for s in coff.sections] != [s['data'] for s in after.sections]:
                        raise ValueError('Native CRT section contents changed')
                    renamed.append(dict(object=name, original_sha256=digest(before),
                                        normalized_sha256=digest(body), alignment=alignment))
            members.append((name, body, symbols))
        if len(converted) != (22 if library == 'ws2_32' else 5):
            raise ValueError('SDK import inventory changed')
        if library == 'msvcrt' and sum(len(row['alignment']) for row in renamed) != 1:
            raise ValueError('Native CRT initialization contribution changed')
        path = output / (library + '.lib')
        path.write_bytes(archive_for(members))
        if [(n, b) for n, _, b in archive_members(path)] != [(n, b) for n, b, _ in members]:
            raise ValueError('Import archive wrapping changed member contents')
        paths[library] = path
        all_converted.extend(row['symbol'] for row in converted)
        report['libraries'][library] = dict(source_sha256=expected,
            output_sha256=digest(path.read_bytes()), converted_imports=converted,
            crt_code_group_metadata=renamed)
    expected = {name if name.startswith('?') else '_' + name for name in THUNK_ORDER} | CRT_IMPORTS
    if set(all_converted) != expected:
        raise ValueError('Import thunk declaration set changed')
    order = output / 'import-code-order.txt'
    order.write_text('\n'.join(THUNK_ORDER) + '\n')
    report['code_order'] = dict(path=str(order), sha256=digest(order.read_bytes()))
    inspection.unlink()
    return paths, order, report
