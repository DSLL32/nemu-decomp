#!/usr/bin/env python3
"""Select pinned SDK import members in a static native LINK chronology.

Only ordinary archive headers and indexes are generated. Kernel DLL-name case
metadata is supplied by the separately guarded spelling adapter. Optional CRT
inputs come from the separately guarded native LIB/COFF placement adapter.
No reference executable is read.
"""

import hashlib
import json
from pathlib import Path
import struct

from build_kaillera_link_inputs import build as build_kernel32
from build_nemu_link_inputs import archive_for
from map_nemu_libraries import archive_members
from pe_fingerprints import PE32, cstring, u32

ROOT = Path(__file__).resolve().parents[1]
RECIPE = ROOT / 'analysis/kaillera-import-chronology.json'


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


def build(output, input_paths=None, conversion_report=None):
    recipe_bytes = RECIPE.read_bytes()
    recipe = json.loads(recipe_bytes)
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    input_paths = input_paths or {}
    if set(input_paths) - {'msvcrt', 'ws2_32'}:
        raise ValueError('Only the guarded CRT/WinSock adapter may replace SDK inputs')
    kernel = output / 'kernel32.lib'
    report = dict(recipe_sha256=digest(recipe_bytes), libraries={}, archives=[],
                  kernel32_spelling=build_kernel32(kernel), reference_bytes_copied=False)
    catalogs = {}
    for library, spec in recipe['libraries'].items():
        path = ROOT / spec['path']
        if path != ROOT / 'toolchains/vc6rtm/lib' / (library + '.lib'):
            raise ValueError('SDK source must be a pinned toolchain archive')
        original = path.read_bytes()
        if digest(original) != spec['sha256']:
            raise ValueError('Pinned SDK archive changed: ' + library)
        linked = kernel if spec['dll_name_case_adjustment'] else path
        data = linked.read_bytes()
        if digest(data) != spec['linked_archive_sha256']:
            raise ValueError('SDK linking-view metadata changed: ' + library)
        index = indexes(data)
        members = list(archive_members(linked))
        derived = members
        if library in input_paths:
            if conversion_report is None:
                raise ValueError('Derived SDK inputs require the guarded converter report')
            proof = conversion_report['libraries'][library]
            derived_path = Path(input_paths[library])
            if (proof['source_sha256'] != spec['sha256'] or
                    digest(derived_path.read_bytes()) != proof['output_sha256']):
                raise ValueError('Derived SDK archive provenance changed: ' + library)
            derived = list(archive_members(derived_path))
            if len(derived) != len(members):
                raise ValueError('Derived SDK member inventory changed')
            changes = {row['short_record_sha256']: row['full_coff_sha256']
                       for row in proof['converted_imports']}
            changes.update({row['original_sha256']: row['normalized_sha256']
                            for row in proof['crt_code_group_metadata']})
            observed = set()
            derived_index = indexes(derived_path.read_bytes())
            for (name, offset, body), (new_name, new_offset, new_body) in zip(members, derived):
                if name != new_name or index[offset - 60] != derived_index[new_offset - 60]:
                    raise ValueError('Derived SDK names or archive definitions changed')
                if body != new_body:
                    old_hash = digest(body)
                    if changes.get(old_hash) != digest(new_body):
                        raise ValueError('Unreviewed derived SDK member mutation')
                    observed.add(old_hash)
            if observed != set(changes):
                raise ValueError('Derived SDK conversion record coverage changed')
        catalogs[library] = {offset: (name, body, index[offset - 60], new_body)
                             for (name, offset, body), (_, _, new_body) in zip(members, derived)}
        report['libraries'][library] = dict(source_sha256=spec['sha256'],
            linking_view_sha256=digest(data),
            derived_sha256=digest(Path(input_paths[library]).read_bytes())
                          if library in input_paths else None)

    def member(row):
        name, body, symbols, linked_body = catalogs[row['library']][row['body_offset']]
        if (name != row['member'] or digest(body) != row['sha256'] or
                symbols != row['index_symbols']):
            raise ValueError('SDK import member or archive declarations changed')
        if row.get('root') and row['root'] not in symbols:
            raise ValueError('Import root is not defined by its unchanged SDK index')
        return name, linked_body, symbols

    generated = set()

    def archive(name, rows):
        if name in generated or Path(name).name != name:
            raise ValueError('Generated native archive names must be unique basenames')
        generated.add(name)
        selected = [member(row) for row in rows]
        path = output / name
        path.write_bytes(archive_for(selected))
        actual = list(archive_members(path))
        if [(n, b) for n, _, b in actual] != [(n, b) for n, b, _ in selected]:
            raise ValueError('Native import archive wrapping changed member bodies')
        index = indexes(path.read_bytes())
        if any(index[offset - 60] != sorted(symbols)
               for (_, offset, _), (_, _, symbols) in zip(actual, selected)):
            raise ValueError('Native import archive definitions changed')
        report['archives'].append(dict(path=str(path), sha256=digest(path.read_bytes()),
            members=[dict(source_library=row['library'], member=row['member'],
                          original_member_sha256=row['sha256'], linked_member_sha256=digest(body))
                     for row, (_, body, _) in zip(rows, selected)]))
        return path

    early, late, roots = [], [], []
    descriptor_libraries = []
    for row in recipe['descriptors']:
        descriptor_libraries.append(row['library'])
        selected = [member(item) for item in row['members']]
        defined = {symbol for _, _, symbols in selected for symbol in symbols}
        if set(row['roots']) != {s for s in defined if s.startswith('__IMPORT_DESCRIPTOR_')}:
            raise ValueError('Import descriptor roots changed')
        early += [archive('descriptor-' + row['library'] + '.lib', row['members'])]
        early += ['/include:' + name for name in row['roots']]
    if len(set(descriptor_libraries)) != len(descriptor_libraries):
        raise ValueError('Duplicate import descriptor library')
    keys = []
    native_placements = set()

    def place_native(side, imported):
        for index, native in enumerate(recipe.get('native_archives', [])):
            if native.get(side) != dict(dll=imported['dll'], name=imported['name']):
                continue
            if index in native_placements or ('before_import' in native) == ('after_import' in native):
                raise ValueError('A native CRT archive must have one unique chronology placement')
            if Path(native['archive']).name != native['archive']:
                raise ValueError('Native CRT archive name must be a basename')
            defined = {symbol for row in native['members'] for symbol in member(row)[2]}
            if native['root'] not in defined:
                raise ValueError('Native CRT chronology root is not defined by its member')
            early.extend([archive(native['archive'], native['members']),
                          '/include:' + native['root']])
            native_placements.add(index)

    for i, row in enumerate(recipe['imports']):
        keys.append((row['dll'], row['name']))
        place_native('before_import', row)
        early.append(archive('chronology-%03d.lib' % i, [row]))
        place_native('after_import', row)
        roots.append('/include:' + row['root'])
    if native_placements != set(range(len(recipe.get('native_archives', [])))):
        raise ValueError('Native CRT archive placement does not select exactly one import')
    expected = {(row['dll'], name) for row in recipe['expected_imports'] for name in row['names']}
    if len(set(keys)) != len(keys) or set(keys) != expected:
        raise ValueError('Import chronology does not cover the final inventory exactly once')
    if set(descriptor_libraries) != set(recipe['libraries']):
        raise ValueError('Import descriptors do not cover all pinned DLL libraries')
    early += roots
    for i, row in enumerate(recipe['discarded_imports']):
        if row.get('root') or (row['dll'], row['name']) in expected:
            raise ValueError('Discarded import must remain unrooted and outside the final inventory')
        late.append(archive('discarded-%03d.lib' % i, [row]))
    report.update(import_count=len(keys), descriptor_count=len(descriptor_libraries),
                  member_bodies_unchanged=not input_paths)
    return early, late, report


def check_inventory(path):
    """Check the linked import declarations against the static recipe, not a PE reference."""
    recipe = json.loads(RECIPE.read_bytes())
    image = PE32(Path(path))
    at = image.offset(image.directory(1)[0])
    rows = []
    while u32(image.data, at + 12):
        dll = cstring(image.data, image.offset(u32(image.data, at + 12)))
        first = u32(image.data, at + 16)
        thunk = image.offset(u32(image.data, at) or first)
        names = []
        while u32(image.data, thunk):
            value = u32(image.data, thunk)
            names.append('ordinal:' + str(value & 0xffff) if value & 0x80000000
                         else cstring(image.data, image.offset(value) + 2))
            thunk += 4
        rows.append(dict(dll=dll.lower(), names=names, iat_rva=first))
        at += 20
    if rows != recipe['expected_imports'] or image.directory(12) != (
            recipe['iat_rva'], recipe['iat_bytes']):
        raise ValueError('Native import inventory or chronology changed')
    return dict(dll_count=len(rows), import_count=sum(len(row['names']) for row in rows),
                iat_rva=recipe['iat_rva'], iat_bytes=recipe['iat_bytes'],
                symbolic_inventory_and_iat_addresses_exact=True)
