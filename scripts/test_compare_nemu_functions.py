#!/usr/bin/env python3
"""Regression checks for diagnostic normalization and evidence boundaries."""

import json
from pathlib import Path
import struct
import tempfile
import unittest

import compare_nemu_functions as compare


class Image:
    base = 0x400000
    path = Path('<test-image>')

    def __init__(self, code, table=b''):
        self.data = code + table
        self.sections = [dict(rva=0x1000, raw_size=len(code), offset=0)]
        if table:
            self.sections.append(dict(rva=0x2000, raw_size=len(table), offset=len(code)))

    def offset(self, rva):
        for section in self.sections:
            if section['rva'] <= rva < section['rva'] + section['raw_size']:
                return section['offset'] + rva - section['rva']
        raise ValueError('unmapped test address')


class ComparisonEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.original = compare.Symbols([
            dict(name='Array', va=0x420000, size=16, kind='data', address_basis='application_observation'),
            dict(name='Callee', va=0x402000, kind='function', address_basis='independent_library_mapping')])
        self.candidate = compare.Symbols([
            dict(name='Array', va=0x520000, size=16, kind='data', address_basis='linked_map'),
            dict(name='Callee', va=0x502000, kind='function', address_basis='linked_map')])

    def compare_code(self, before, after, fixups=()):
        function = dict(name='Function', va=0x401000, size=len(before))
        fields = compare.linked_fields(after, 0x501000, len(after), set(fixups), self.candidate)
        return compare.compare_bytes(function, after, len(after), 0x501000, fields,
                                     Image(before), self.original, self.candidate)

    def test_map_like_integer_requires_real_relocation(self):
        code = bytes.fromhex('b800005200c3')
        self.assertEqual(compare.linked_fields(code, 0x501000, len(code), set(), self.candidate), [])
        fields = compare.linked_fields(code, 0x501000, len(code), {0x501001}, self.candidate)
        self.assertEqual([(r['offset'], r['authority']) for r in fields], [(1, 'PE_HIGHLOW')])

    def test_internal_branch_distance_is_never_masked(self):
        result = self.compare_code(bytes.fromhex('750190c3'), bytes.fromhex('750090c3'))
        self.assertFalse(result['instruction_and_known_target_match'])
        self.assertEqual(result['masked_bytes'], 0)

    def test_register_difference_survives_address_normalization(self):
        result = self.compare_code(bytes.fromhex('b800004200c3'), bytes.fromhex('b900005200c3'), [0x501001])
        self.assertFalse(result['instruction_and_known_target_match'])
        self.assertEqual(result['normalized_differing_bytes'], 1)

    def test_wrong_callee_fails_despite_equal_instruction_shape(self):
        before = b'\xe8' + struct.pack('<i', 0x403000 - 0x401005) + b'\xc3'
        after = b'\xe8' + struct.pack('<i', 0x502000 - 0x501005) + b'\xc3'
        result = self.compare_code(before, after)
        self.assertTrue(result['normalized_prefix_equal'])
        self.assertFalse(result['instruction_and_known_target_match'])
        self.assertEqual(result['target_mismatches'], 1)

    def test_unreachable_data_does_not_invent_branch_masks(self):
        code = bytes.fromhex('c3e87f7f7f7f')
        self.assertEqual(compare.linked_fields(code, 0x501000, len(code), set(), self.candidate), [])

    def test_map_data_aliases_remain_observations(self):
        function = dict(name='Function', va=0x401000, size=16)
        for target in (0x520000, 0x520001):
            field = dict(offset=0, width=4, kind='DIR32', authority='PE_HIGHLOW',
                         candidate_target=target, original_target=0x430000)
            result = compare.resolve_field(field, function, self.original, self.candidate)
            self.assertEqual(result['status'], 'unresolved')
            self.assertFalse(result['identity_verified'])

    def test_forced_unresolved_header_address_is_invalid(self):
        self.candidate.invalid_link_fixture = True
        self.candidate.invalid_names = set()
        self.candidate.image_ranges = [(0x501000, 0x530000)]
        field = dict(offset=0, width=4, kind='DIR32', authority='PE_HIGHLOW',
                     candidate_target=0x500000, original_target=0x402000)
        result = compare.resolve_field(field, dict(name='Function', va=0x401000, size=16),
                                       self.original, self.candidate)
        self.assertEqual(result['status'], 'invalid_link_operand')

    def test_missing_metadata_remains_in_inventory(self):
        scratch = Path.home() / '.ai-tmp'
        scratch.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='nemu-comparison-test.', dir=scratch) as directory:
            path = Path(directory) / 'metadata.json'
            path.write_text(json.dumps({'functions': [
                dict(name='NoExtent', va=0x401000, ghidra_body_bytes=32),
                dict(name=None, va=0x401010, size=4)]}))
            functions, _ = compare.load_metadata([path])
            result = compare.coverage([], functions)
            self.assertEqual(result['inventory_entries'], 2)
            self.assertEqual(result['incomplete_metadata_entries'], 2)

    def test_seh_funclets_require_prolog_and_relocation_evidence(self):
        code = bytearray(b'\x6a\xff\x68' + struct.pack('<I', 0x402000) +
                         b'\x68' + struct.pack('<I', 0x403000) + b'\xc3')
        code.extend(b'\x90' * (32 - len(code)))
        code[16] = code[24] = 0xc3
        image = Image(bytes(code), struct.pack('<iII', -1, 0x401010, 0x401018))
        symbols = compare.Symbols([dict(name='__except_handler3', va=0x403000)])
        fixups = {0x401003, 0x401008, 0x402004, 0x402008}
        entries = compare.seh_scope_entries(image, 0x401000, 32, fixups, symbols)
        self.assertEqual([(r['filter_va'], r['handler_va']) for r in entries], [(0x401010, 0x401018)])
        self.assertEqual(compare.seh_scope_entries(image, 0x401000, 32, fixups - {0x401008}, symbols), [])


if __name__ == '__main__':
    unittest.main()
