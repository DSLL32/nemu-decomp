#!/usr/bin/env python3
"""Exercise metadata-only BSS layout guards with synthetic COFF objects."""

import copy
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import build_kaillera_layout_inputs as layout_inputs
from verify_gs import COFF


class LayoutInputsTests(unittest.TestCase):
    def setUp(self):
        scratch = Path.home() / '.ai-tmp'
        scratch.mkdir(exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix='kaillera-layout-test.', dir=scratch)
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.object = self.directory / 'example.obj'
        self.layout = self.directory / 'layout.json'
        self.output = self.directory / 'output'
        self.spec = dict(size=20, source_bss_size=16, source_characteristics=0xc0300080,
                        symbols=[dict(name='_first', size=4, offset=16, source_offset=0,
                                      source_type=0, source_storage_class=2),
                                 dict(name='_second', size=8, offset=0, source_offset=8,
                                      source_type=0, source_storage_class=3)])
        self.document = dict(objects={self.object.name: self.spec})
        self.layout.write_text(json.dumps(self.document))
        self.write_object()
        self.patcher = patch.object(layout_inputs, 'LAYOUT', self.layout)
        self.patcher.start()
        self.addCleanup(self.patcher.stop)

    def write_object(self, *, first_value=0, first_type=0, first_storage=2,
                     section=1, flags=0xc0300080, size=16, auxiliary_size=16,
                     duplicate=False, extra=False, raw=0):
        def symbol(name, value, section_id, kind, storage, auxiliary=0):
            return struct.pack('<8sIhHBB', name.encode(), value, section_id, kind,
                               storage, auxiliary)
        payload = b'UNCHANGED DATA!!!'
        symbols = [symbol('_first', first_value, section, first_type, first_storage),
                   symbol('_second', 8, section, 0, 3),
                   symbol('.bss', 0, 1, 0, 3, 1),
                   struct.pack('<I', auxiliary_size) + bytes(14)]
        if duplicate:
            symbols.append(symbol('_first', 4, 1, 0, 2))
        if extra:
            symbols.append(symbol('.extra', 4, 1, 0, 3))
        header = struct.pack('<HHIIIHH', 0x14c, 2, 0, 100 + len(payload), len(symbols), 0, 0)
        bss = struct.pack('<8sIIIIIIHHI', b'.bss', 0, 0, size, raw, 0, 0, 0, 0, flags)
        data = struct.pack('<8sIIIIIIHHI', b'.data', 0, 0, len(payload), 100, 0, 0, 0, 0,
                           0xc0300040)
        self.object.write_bytes(header + bss + data + payload + b''.join(symbols)
                                + struct.pack('<I', 4))

    def test_only_allocation_metadata_changes(self):
        original = self.object.read_bytes()
        paths, report = layout_inputs.build([self.object], self.output)
        rebuilt = paths[0].read_bytes()
        table = struct.unpack_from('<I', original, 8)[0]
        allowed = set(range(36, 40))
        for offset in (table + 8, table + 26, table + 54):
            allowed.update(range(offset, offset + 4))
        self.assertEqual(len(original), len(rebuilt))
        self.assertTrue(all(a == b or i in allowed for i, (a, b) in
                            enumerate(zip(original, rebuilt))))
        before, after = COFF(self.object), COFF(paths[0])
        self.assertEqual(before.sections[1]['data'], after.sections[1]['data'])
        self.assertEqual(after.sections[0]['size'], 20)
        self.assertEqual([s['value'] for s in after.symbols[:2]], [16, 0])
        self.assertFalse(report['reference_bytes_copied'])

    def test_source_metadata_changes_are_rejected(self):
        variants = [dict(first_value=4), dict(first_type=0x20), dict(first_storage=3),
                    dict(section=0), dict(section=-1), dict(section=3),
                    dict(flags=0xc03000c0), dict(flags=0xe0300080),
                    dict(flags=0xc0301080), dict(size=20), dict(auxiliary_size=20),
                    dict(duplicate=True), dict(extra=True), dict(raw=100)]
        for variant in variants:
            with self.subTest(variant=variant):
                self.write_object(**variant)
                with self.assertRaises(ValueError):
                    layout_inputs.build([self.object], self.output)

    def test_missing_and_duplicate_objects_are_rejected(self):
        for objects in ([], [self.object, self.object]):
            with self.subTest(objects=objects), self.assertRaises(ValueError):
                layout_inputs.build(objects, self.output)

    def test_bad_layout_declarations_are_rejected(self):
        for mode in ('duplicate', 'overlap', 'outside', 'source-extent'):
            document = copy.deepcopy(self.document)
            spec = document['objects'][self.object.name]
            if mode == 'duplicate':
                spec['symbols'].append(dict(spec['symbols'][0], offset=12))
            elif mode == 'overlap':
                spec['symbols'][0]['offset'] = 4
            elif mode == 'outside':
                spec['symbols'][0]['offset'] = 20
            else:
                spec['symbols'][0]['size'] = 17
            self.layout.write_text(json.dumps(document))
            with self.subTest(mode=mode), self.assertRaises(ValueError):
                layout_inputs.build([self.object], self.output)

    def test_source_object_cannot_be_overwritten(self):
        original = self.object.read_bytes()
        with self.assertRaises(ValueError):
            layout_inputs.build([self.object], self.directory)
        self.assertEqual(self.object.read_bytes(), original)


if __name__ == '__main__':
    unittest.main()
