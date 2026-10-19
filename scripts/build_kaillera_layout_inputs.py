#!/usr/bin/env python3
"""Apply recovered uninitialized-data layout through COFF metadata only.

VC6 orders globals using private identifiers that have not all survived.
This supplies a declarative linker layout for the reconstructed declarations.
No initialized data, instruction bytes, or relocation operands are changed.
"""

import hashlib
import json
from pathlib import Path
import struct

from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]
LAYOUT = ROOT / 'analysis/kaillera-bss-layout.json'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def build(objects, output):
    layout = json.loads(LAYOUT.read_text())
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    paths, changes = [], []
    expected = set(layout['objects'])
    for path in map(Path, objects):
        if path.name not in expected:
            paths.append(path)
            continue
        expected.remove(path.name)
        spec = layout['objects'][path.name]
        before = COFF(path)
        names = {s['name']: s for s in before.symbols}
        declared = {s['name'] for s in spec['symbols']}
        if not declared <= names.keys():
            raise ValueError(f'{path}: recovered BSS declarations changed')
        section_ids = {names[name]['section'] for name in declared}
        if len(section_ids) != 1:
            raise ValueError(f'{path}: expected one uninitialized allocation')
        section_id = section_ids.pop()
        section = before.sections[section_id - 1]
        if (section['name'] != '.bss' or section['offset'] != 0 or
                not section['characteristics'] & 0x80 or
                section['characteristics'] & (0x20 | 0x20000000 | 0x1000) or
                section['relocations']):
            raise ValueError(f'{path}: expected plain BSS, without file contents')
        if section['size'] != spec['source_bss_size']:
            raise ValueError(f'{path}: source BSS allocation changed; review declarations')
        actual = {s['name'] for s in before.symbols
                  if s['section'] == section_id and not s['auxiliary'] and
                  not s['name'].startswith('.')}
        if actual != declared:
            raise ValueError(f'{path}: unaccounted BSS declarations: {actual ^ declared}')
        data = bytearray(before.data)
        table = struct.unpack_from('<I', data, 8)[0]
        optional = struct.unpack_from('<H', data, 16)[0]
        allowed, occupied = set(), set()

        def update(offset, value):
            struct.pack_into('<I', data, offset, value)
            allowed.update(range(offset, offset + 4))

        update(20 + optional + 40 * (section_id - 1) + 16, spec['size'])
        allocations = []
        for item in spec['symbols']:
            symbol = names[item['name']]
            start, size = item['offset'], item['size']
            extent = set(range(start, start + size))
            if start < 0 or size <= 0 or start + size > spec['size'] or occupied & extent:
                raise ValueError(f'{path}: invalid overlapping BSS declarations')
            occupied.update(extent)
            update(table + 18 * symbol['index'] + 8, start)
            allocations.append(dict(symbol=item['name'], old_offset=symbol['value'],
                                    new_offset=start, size=size))
        for symbol in before.symbols:
            if symbol['section'] == section_id and symbol['auxiliary']:
                if symbol['name'] != '.bss' or len(symbol['auxiliary']) != 1:
                    raise ValueError(f'{path}: unexpected BSS auxiliary metadata')
                update(table + 18 * (symbol['index'] + 1), spec['size'])
        if not all(a == b or i in allowed for i, (a, b) in
                   enumerate(zip(before.data, data))):
            raise ValueError(f'{path}: changes extend beyond COFF allocation metadata')
        destination = output / path.name
        destination.write_bytes(data)
        after = COFF(destination)
        for old, new in zip(before.sections, after.sections):
            if old['index'] != section_id and old['data'] != new['data']:
                raise ValueError(f'{path}: section contents changed')
            if old['relocations'] != new['relocations']:
                # Symbol values legitimately change; relocation records do not.
                old_records = [(r['offset'], r['symbol_index'], r['type'])
                               for r in old['relocations']]
                new_records = [(r['offset'], r['symbol_index'], r['type'])
                               for r in new['relocations']]
                if old_records != new_records:
                    raise ValueError(f'{path}: relocation metadata changed')
        changes.append(dict(object=path.name, source_sha256=digest(before.data),
                            output_sha256=digest(data), old_bss_size=section['size'],
                            new_bss_size=spec['size'], allocations=allocations))
        paths.append(destination)
    if expected:
        raise ValueError(f'Missing reconstructed BSS objects: {expected}')
    return paths, dict(layout_sha256=digest(LAYOUT.read_bytes()), objects=changes,
                       initialized_and_executable_section_contents_unchanged=True,
                       relocation_records_unchanged=True,
                       reference_bytes_copied=False)
