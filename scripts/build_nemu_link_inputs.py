#!/usr/bin/env python3
"""Generate ordinary linker inputs with the inferred native-library ordering.

CRT objects and import members are copied unchanged from the tracked historical
libraries or freshly generated import archives. Small standard COFF archives
make import-member selection order explicit. Only archive indexes and headers
are generated; application instructions, data and PE output are never edited.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct

from map_nemu_libraries import archive_members

ROOT = Path(__file__).resolve().parents[1]


def defined_symbols(data):
    machine, sections, timestamp, table, count, optional, flags = struct.unpack_from('<HHIIIHH', data)
    if machine != 0x14c:
        raise ValueError('Expected an i386 import-directory object')
    strings, result, index = table + 18 * count, [], 0
    while index < count:
        position = table + 18 * index
        name = data[position:position + 8]
        name = (data[strings + struct.unpack_from('<I', name, 4)[0]:].split(b'\0', 1)[0]
                if name[:4] == bytes(4) else name.split(b'\0', 1)[0]).decode('cp1252')
        value, section, kind, storage, auxiliary = struct.unpack_from('<IhHBB', data, position + 8)
        if storage == 2 and section > 0:
            result.append(name)
        index += auxiliary + 1
    return result


def archive_for(members):
    """Emit both Microsoft archive indexes, preserving every member body."""
    symbols = sorted((symbol, index) for index, (_, _, names) in enumerate(members)
                     for symbol in names)
    symbol_names = b''.join(symbol.encode('cp1252') + b'\0' for symbol, _ in symbols)
    long_names, name_offsets = b'', {}
    for name, _, _ in members:
        if name not in name_offsets:
            name_offsets[name] = len(long_names)
            long_names += name.encode('cp1252') + b'\0'

    def header(name, size):
        return (name.ljust(16) + '0'.ljust(12) + '0'.ljust(6) + '0'.ljust(6)
                + '0'.ljust(8) + str(size).ljust(10) + '`\n').encode('ascii')

    first_size = 4 + 4 * len(symbols) + len(symbol_names)
    second_size = 4 + 4 * len(members) + 4 + 2 * len(symbols) + len(symbol_names)
    position = (8 + 60 + first_size + (first_size & 1) + 60 + second_size
                + (second_size & 1) + 60 + len(long_names) + (len(long_names) & 1))
    offsets = []
    for _, body, _ in members:
        offsets.append(position)
        position += 60 + len(body) + (len(body) & 1)
    first = (struct.pack('>I', len(symbols))
             + struct.pack('>' + 'I' * len(symbols), *[offsets[i] for _, i in symbols])
             + symbol_names)
    second = (struct.pack('<I', len(members))
              + struct.pack('<' + 'I' * len(members), *offsets)
              + struct.pack('<I', len(symbols))
              + struct.pack('<' + 'H' * len(symbols), *[i + 1 for _, i in symbols])
              + symbol_names)
    result = b'!<arch>\n'
    for name, body in [('/', first), ('/', second), ('//', long_names)] + [
            ('/' + str(name_offsets[name]), body) for name, body, _ in members]:
        result += header(name, len(body)) + body + (b'\n' if len(body) & 1 else b'')
    return result


def import_name(data):
    sig1, sig2, version, machine, stamp, size, hint, flags = struct.unpack_from('<HHHHIIHH', data)
    if (sig1, sig2, machine) != (0, 0xffff, 0x14c) or flags & 3:
        raise ValueError('Expected an i386 code-import record')
    symbol, dll = [part.decode('cp1252') for part in data[20:].split(b'\0')[:2]]
    kind = (flags >> 2) & 7
    if kind == 0:
        public = 'ordinal:' + str(hint)
    elif kind == 1:
        public = symbol
    elif kind == 2:
        public = symbol.lstrip('_?@')
    elif kind == 3:
        public = symbol.lstrip('_?@').split('@')[0]
    else:
        raise ValueError('Unsupported import name convention')
    return dll.lower(), public, symbol


def weak_aliases(data):
    """Read the Machine=0 weak aliases used by the unchanged oldnames members."""
    machine, sections, stamp, table, count, optional, flags = struct.unpack_from('<HHIIIHH', data)
    if machine != 0 or sections != 1:
        raise ValueError('Expected an oldnames weak-alias member')
    strings, symbols, aliases, index = table + 18 * count, {}, {}, 0
    while index < count:
        position = table + 18 * index
        name = data[position:position + 8]
        name = (data[strings + struct.unpack_from('<I', name, 4)[0]:].split(b'\0', 1)[0]
                if name[:4] == bytes(4) else name.split(b'\0', 1)[0]).decode('cp1252')
        value, section, kind, storage, auxiliary = struct.unpack_from('<IhHBB', data, position + 8)
        symbols[index] = name
        if storage == 105:
            if section != 0 or auxiliary != 1:
                raise ValueError('Unexpected weak alias layout')
            target, search = struct.unpack_from('<II', data, position + 18)
            if search != 3:
                raise ValueError('Expected IMAGE_WEAK_EXTERN_SEARCH_ALIAS')
            aliases[name] = target
        index += auxiliary + 1
    return {name: symbols[target] for name, target in aliases.items()}


def write_archive(file, members):
    file.write_bytes(archive_for(members))
    actual = list(archive_members(file))
    if [(name, data) for name, _, data in actual] != [(name, data) for name, data, _ in members]:
        raise ValueError('Generated archive changed member bytes: ' + str(file))


def chronological_inputs(recipe, chronology, output, imports, report):
    """Select unchanged members with stock archive indexes and /INCLUDE roots."""
    if chronology['original_sha256'] != recipe['original_sha256']:
        raise ValueError('Chronology refers to a different original executable')
    crt = [row for row in report['crt'] if not row['deferred']]
    if [row['object'] for row in crt] != [row['object'] for row in chronology['crt_roots']]:
        raise ValueError('Chronology roots do not cover the selected CRT members in order')
    members, roots = [], []
    for row, anchor in zip(crt, chronology['crt_roots']):
        data = Path(row['file']).read_bytes()
        symbols = defined_symbols(data)
        if anchor['symbol'] not in symbols:
            raise ValueError('CRT anchor is not defined by its member: ' + anchor['symbol'])
        members.append((row['object'], data, symbols))
        roots.append(anchor['symbol'])
    position = [row['object'] for row in crt].index(chronology['insert_after']) + 1
    alias_members, aliases = [], {}
    for row in report['aliases']:
        data = Path(row['file']).read_bytes()
        definitions = weak_aliases(data)
        aliases.update(definitions)
        alias_members.append((row['object'], data, list(definitions)))
    if set(aliases) != set(chronology['alias_roots']):
        raise ValueError('Chronology roots do not cover the unchanged weak aliases')
    embedded = chronology['embedded_import']
    embedded_key = embedded['dll'], embedded['name']
    import_member = imports[embedded_key][0][-1]
    if embedded['root'] not in import_member[2]:
        raise ValueError('Embedded import root is not defined by its member')
    members[position:position] = alias_members + [import_member]
    roots[position:position] = chronology['alias_roots'] + [embedded['root']]
    archive = output / 'crt-chronological.lib'
    write_archive(archive, members)

    expected = {(row['dll'], row['name']) for row in recipe['imports']['input_order']}
    timeline = chronology['input_order']
    selected = [(row['dll'], row['name']) for row in timeline if row['kind'] == 'import']
    dlls = [row['dll'] for row in timeline if row['kind'] == 'descriptor']
    if set(selected) != expected or len(selected) != len(expected):
        raise ValueError('Chronology must cover every selected import exactly once')
    if set(dlls) != {dll for dll, name in expected} or len(dlls) != len(set(dlls)):
        raise ValueError('Chronology must cover each import descriptor exactly once')
    objects, import_roots, descriptor_roots = [], [], []
    for index, row in enumerate(timeline):
        if row['kind'] == 'import':
            key = row['dll'], row['name']
            original, provenance = imports[key]
            if key == embedded_key:
                file = archive
            else:
                file = output / ('chronology-%03d.lib' % index)
                write_archive(file, [original[-1]])
                objects.append(file)
                import_roots.append(original[-1][2][1])
            report['imports'].append(dict(row, **provenance, file=str(file)))
        elif row['kind'] == 'descriptor':
            original = next(value[0] for key, value in imports.items() if key[0] == row['dll'])
            metadata = original[:-1]
            descriptor_roots.extend(symbol for _, _, symbols in metadata for symbol in symbols
                                    if symbol.startswith('__IMPORT_DESCRIPTOR_'))
            file = output / ('chronology-%03d.lib' % index)
            write_archive(file, metadata)
            objects.append(file)
        else:
            raise ValueError('Unknown chronology entry kind: ' + row['kind'])
    if sorted(descriptor_roots) != sorted(chronology['descriptor_roots']):
        raise ValueError('Import descriptor roots changed')
    objects.extend([archive, output / 'crt-late.lib'])
    report['chronology'] = dict(crt_roots=roots, alias_targets=aliases,
        descriptor_roots=chronology['descriptor_roots'], limitations=chronology['limitations'],
        combined_archive=str(archive), selected_member_count=len(members),
        note='Only archive indexes, headers and response files are generated. Member bodies are unchanged.')
    return objects, ['/INCLUDE:' + name for name in roots + import_roots + chronology['descriptor_roots']]


def generate(recipe, import_dir, output, chronology=None):
    output.mkdir(parents=True, exist_ok=True)
    objects, early, report = [], [], dict(crt=[], imports=[], aliases=[], member_bytes_unchanged=True)
    aliases = recipe.get('aliases')
    if aliases:
        library = ROOT / aliases['library']
        if hashlib.sha256(library.read_bytes()).hexdigest() != aliases['library_sha256']:
            raise ValueError('CRT alias archive hash changed')
        contents = {offset: (name, data) for name, offset, data in archive_members(library)}
        for index, row in enumerate(aliases['members']):
            name, data = contents[row['archive_offset']]
            if name != row['object'] or hashlib.sha256(data).hexdigest() != row['sha256']:
                raise ValueError('CRT alias member changed: ' + row['object'])
            file = output / ('alias-%03d.obj' % index)
            file.write_bytes(data)
            early.append(file)
            report['aliases'].append(dict(row, file=str(file)))
    crt = recipe['crt']
    deferred_names = set(crt.get('deferred_members', []))
    deferred = []
    library = ROOT / crt['library']
    if hashlib.sha256(library.read_bytes()).hexdigest() != crt['library_sha256']:
        raise ValueError('CRT archive hash changed')
    contents = {offset: (name, data) for name, offset, data in archive_members(library)}
    for index, row in enumerate(crt['members']):
        name, data = contents[row['archive_offset']]
        if name != row['object'] or hashlib.sha256(data).hexdigest() != row['sha256']:
            raise ValueError('CRT member changed: ' + row['object'])
        if name in deferred_names:
            deferred.append((name, data, defined_symbols(data)))
            file = output / 'crt-late.lib'
        else:
            file = output / ('crt-%03d.obj' % index)
            file.write_bytes(data)
            objects.append(file)
        report['crt'].append(dict(row, file=str(file), deferred=name in deferred_names))
    if deferred_names != {name for name, _, _ in deferred}:
        raise ValueError('Deferred CRT member is absent from the selected member list')
    if deferred:
        file = output / 'crt-late.lib'
        file.write_bytes(archive_for(deferred))
        actual = list(archive_members(file))
        if [(name, data) for name, _, data in actual] != [(name, data) for name, data, _ in deferred]:
            raise ValueError('Generated CRT archive changed member bytes')
        objects.append(file)
    imports = {}
    for dll, source in recipe['imports']['libraries'].items():
        library = ROOT / source['path'] if 'path' in source else import_dir / source['generated']
        if 'sha256' in source and hashlib.sha256(library.read_bytes()).hexdigest() != source['sha256']:
            raise ValueError('SDK import archive changed: ' + dll)
        metadata, functions = [], []
        for name, offset, data in archive_members(library):
            if data[:4] == b'\0\0\xff\xff':
                functions.append((name, offset, data))
            elif data[:2] == b'\x4c\x01':
                symbols = defined_symbols(data)
                if any(s.startswith(('__IMPORT_DESCRIPTOR_', '__NULL_IMPORT_DESCRIPTOR', '\x7f'))
                       for s in symbols):
                    if 'descriptor_compiler_id' in source:
                        marker = b'@comp.id' + struct.pack('<I', int(source['descriptor_compiler_id'], 16))
                        if marker not in data:
                            raise ValueError('Generated descriptor compiler provenance changed: ' + dll)
                    metadata.append((name, data, symbols))
        if len(metadata) != 3:
            raise ValueError('Expected exactly three import-directory members: ' + dll)
        for name, offset, data in functions:
            import_dll, public, symbol = import_name(data)
            if import_dll != dll:
                raise ValueError('Import DLL identity changed')
            imports[dll, public] = (metadata + [(name, data, [symbol, '__imp_' + symbol])],
                                   dict(library=str(library), archive_offset=offset,
                                        import_member_sha256=hashlib.sha256(data).hexdigest()))
    for index, row in enumerate([] if chronology else recipe['imports']['input_order']):
        members, provenance = imports[row['dll'], row['name']]
        file = output / ('import-%03d.lib' % index)
        file.write_bytes(archive_for(members))
        # Independently read the archive again to ensure wrapping preserved bodies.
        actual = list(archive_members(file))
        if [(name, data) for name, _, data in actual] != [(name, data) for name, data, _ in members]:
            raise ValueError('Generated import archive changed member bytes')
        objects.append(file)
        report['imports'].append(dict(row, **provenance, file=str(file)))
    roots = []
    if chronology:
        objects, roots = chronological_inputs(recipe, chronology, output, imports, report)
        early = []
    report['mode'] = 'chronological' if chronology else 'ordered'
    def response_path(file):
        try:
            path = str(file.relative_to(Path.cwd())).replace('/', '\\')
        except ValueError:
            path = 'Z:' + str(file).replace('/', '\\')
        if any(character in path for character in '\r\n"'):
            raise ValueError('Unsupported character in linker input path')
        return '"' + path + '"'
    response = ['/NODEFAULTLIB:libc'] + roots + [response_path(file) for file in objects]
    (output / 'link.rsp').write_text('\n'.join(response) + '\n')
    (output / 'early.rsp').write_text('\n'.join(response_path(file) for file in early) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--order', type=Path, default=ROOT / 'src/nemu64/imports/link-order.json')
    parser.add_argument('--chronology', type=Path, help='optional native and import chronology recipe')
    parser.add_argument('--import-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    output = args.output_dir.resolve()
    if not output.is_relative_to((Path.home() / '.ai-tmp').resolve()):
        parser.error('--output-dir must be under ~/.ai-tmp')
    chronology = json.loads(args.chronology.read_text()) if args.chronology else None
    report = generate(json.loads(args.order.read_text()), args.import_dir.resolve(), output, chronology)
    report.update(order_file=str(args.order), order_sha256=hashlib.sha256(args.order.read_bytes()).hexdigest())
    if args.chronology:
        report.update(chronology_file=str(args.chronology),
                      chronology_sha256=hashlib.sha256(args.chronology.read_bytes()).hexdigest())
    (output / 'inputs.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(report["crt"])} CRT objects and {len(report["imports"])} imports: {output / "link.rsp"}')


if __name__ == '__main__':
    main()
