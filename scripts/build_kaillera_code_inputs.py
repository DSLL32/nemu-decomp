#!/usr/bin/env python3
"""Expose recovered application COMDAT ordering to the native linker.

Only executable section names change. LINK /ORDER selects the observed order
within the queue-helper group; browser contributions follow that group. Every
instruction, relocation record, symbol, and non-executable allocation is kept.
"""

import hashlib
from pathlib import Path
import struct

from verify_gs import COFF


def digest(data):
    return hashlib.sha256(data).hexdigest()


def build(objects, output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    results, changes, helper_names = [], [], set()
    for path in objects:
        path = Path(path)
        coff = COFF(path)
        groups = {}
        if path.name == 'serverbrowser.obj':
            for section in coff.sections:
                if section['characteristics'] & 0x20 and section['name'] == '.text':
                    groups[section['index']] = '.text$b'
        if path.name in ('session.obj', 'resizer.obj', 'connect.obj'):
            for function in coff.functions():
                name = function['name']
                message_destructor = (name.startswith('??1') and 'Message@@' in name)
                list_destructor = name.startswith('??1?$C_ItemList@')
                queue_helper = (path.name == 'session.obj' and any(
                    marker in name for marker in ('ByteQueue@@', 'ByteQueueList@@',
                                                  'ByteQueue@@@@')))
                if message_destructor or list_destructor or queue_helper:
                    section = coff.sections[function['section'] - 1]
                    if (not section['characteristics'] & 0x20 or
                            not section['characteristics'] & 0x1000 or
                            function['value'] != 0 or function['size'] != section['size']):
                        raise ValueError('Expected a complete native helper COMDAT: ' + name)
                    if function['storage_class'] != 2:
                        raise ValueError('LINK /ORDER requires a public helper definition')
                    groups[function['section']] = '.text$a'
                    helper_names.add(name)
        if not groups:
            results.append(path)
            continue
        data = bytearray(coff.data)
        allowed, rows = set(), []
        for index, group in groups.items():
            section = coff.sections[index - 1]
            if section['name'] != '.text':
                raise ValueError('Unexpected source code grouping: ' + str(path))
            start = 20 + 40 * (index - 1)
            data[start:start + 8] = group.encode('ascii').ljust(8, b'\0')
            allowed.update(range(start, start + 8))
            rows.append(dict(section=index, group=group,
                             instruction_sha256=digest(section['data'])))
        if any(a != b and i not in allowed for i, (a, b) in enumerate(zip(coff.data, data))):
            raise ValueError('Non-name COFF bytes changed')
        target = output / path.name
        if target.resolve() == path.resolve():
            raise ValueError('Cannot overwrite a source object')
        target.write_bytes(data)
        after = COFF(target)
        for before_section, after_section in zip(coff.sections, after.sections):
            if (before_section['data'] != after_section['data'] or
                    before_section['relocations'] != after_section['relocations']):
                raise ValueError('Native section contents or relocation identities changed')
        if coff.symbols != after.symbols:
            raise ValueError('COFF symbols changed')
        results.append(target)
        changes.append(dict(object=path.name, source_sha256=digest(coff.data),
                            output_sha256=digest(data), sections=rows))

    def priority(name):
        if name.startswith('??1') and 'Message@@' in name:
            return 0, name
        if name.startswith('?Add@?$C_ItemList@'):
            return 1, name
        if name.startswith('??0CachedByteQueue@@') or name.startswith('??0ByteQueue@@'):
            return 2, name
        if name.startswith('?append@ByteQueue@@'):
            return 3, name
        if name.startswith('?Del@?$C_ItemList@'):
            return 4, name
        if name.startswith('?peek@ByteQueue@@'):
            return 5, name
        if name.startswith('??0ByteQueueList@@'):
            return 6, name
        if name.startswith('??1ByteQueueList@@'):
            return 7, name
        if name.startswith('??1?$C_ItemList@'):
            return 8, name
        if name.startswith('??1CachedByteQueue@@'):
            return 8, name
        if name.startswith(('??_G', '??_E')):
            return 9, name
        raise ValueError('Unclassified native helper: ' + name)

    names = sorted(helper_names, key=priority)
    if {priority(name)[0] for name in names} != set(range(10)):
        raise ValueError('The recovered native helper inventory changed')
    order = output / 'application-code-order.txt'
    order.write_text('\n'.join(names) + '\n')
    return results, order, dict(objects=changes, helper_order=names,
        order_sha256=digest(order.read_bytes()),
        executable_and_initialized_section_contents_unchanged=True,
        relocation_records_and_symbols_unchanged=True,
        reference_bytes_copied=False)
