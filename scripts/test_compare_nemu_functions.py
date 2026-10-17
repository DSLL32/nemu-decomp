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

    def test_relocated_internal_switch_target_preserves_relative_destination(self):
        before = b'\xc3\x90\x90\x90' + struct.pack('<I', 0x401001)
        after = b'\xc3\x90\x90\x90' + struct.pack('<I', 0x501002)
        result = self.compare_code(before, after, [0x501004])
        self.assertTrue(result['normalized_prefix_equal'])
        self.assertEqual(result['target_mismatches'], 1)
        self.assertFalse(result['instruction_and_known_target_match'])

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

    def test_relocated_table_after_code_body_can_reach_body_cases(self):
        code = bytes.fromhex('ff2485') + struct.pack('<I', 0x501014)
        code += b'\xe8' + struct.pack('<i', 0x502000 - 0x50100c) + b'\xc3'
        block = code + b'\x90' * (20 - len(code)) + struct.pack('<I', 0x501007)
        fixups = {0x501003, 0x501014}
        reads = []
        fields = compare.linked_fields(code, 0x501000, len(code), fixups, self.candidate,
                                       table_data=block, table_reads=reads)
        self.assertEqual([(r['offset'], r['kind']) for r in fields], [(3, 'DIR32'), (8, 'REL')])
        self.assertTrue(reads[0]['outside_compared_body'])
        self.assertEqual(reads[0]['code_successor_bound'], [0x501000, 0x50100d])
        fields = compare.linked_fields(code, 0x501000, len(code), fixups - {0x501014}, self.candidate,
                                       table_data=block)
        self.assertEqual([(r['offset'], r['kind']) for r in fields], [(3, 'DIR32')])

    def test_map_data_aliases_remain_observations(self):
        function = dict(name='Function', va=0x401000, size=16)
        for target in (0x520000, 0x520001):
            field = dict(offset=0, width=4, kind='DIR32', authority='PE_HIGHLOW',
                         candidate_target=target, original_target=0x430000)
            result = compare.resolve_field(field, function, self.original, self.candidate)
            self.assertEqual(result['status'], 'unresolved')
            self.assertFalse(result['identity_verified'])

    def test_authoritative_memory_target_rejects_wrong_global(self):
        self.original.rows[0]['address_basis'] = 'authoritative_application_storage'
        before = b'\xa1' + struct.pack('<I', 0x430000) + b'\xc3'
        after = b'\xa1' + struct.pack('<I', 0x520000) + b'\xc3'
        result = self.compare_code(before, after, [0x501001])
        self.assertTrue(result['normalized_prefix_equal'])
        self.assertFalse(result['instruction_and_known_target_match'])
        self.assertEqual(result['target_mismatches'], 1)
        self.assertEqual(result['relocations'][0]['address_use'], 'memory_access')

    def test_authoritative_matching_storage_offsets_are_verified(self):
        self.original.rows[0]['address_basis'] = 'authoritative_application_storage'
        for offset in (0, 4):
            before = b'\xa1' + struct.pack('<I', 0x420000 + offset) + b'\xc3'
            after = b'\xa1' + struct.pack('<I', 0x520000 + offset) + b'\xc3'
            result = self.compare_code(before, after, [0x501001])
            self.assertTrue(result['fully_verified_relocations'])
        field = dict(offset=1, kind='DIR32', width=4, authority='MAP_address_inference',
                     candidate_target=0x520004, original_target=0x420004)
        result = compare.resolve_field(field, dict(name='Function', va=0x401000, size=6),
                                       self.original, self.candidate)
        self.assertFalse(result['identity_verified'])

    def test_authoritative_pointer_alias_conflict_stays_unresolved_and_blocks_match(self):
        self.original.rows[0]['address_basis'] = 'authoritative_application_storage'
        before = b'\xb8' + struct.pack('<I', 0x430000) + b'\xc3'
        after = b'\xb8' + struct.pack('<I', 0x520000) + b'\xc3'
        result = self.compare_code(before, after, [0x501001])
        self.assertEqual(result['relocations'][0]['status'], 'unresolved')
        self.assertEqual(result['possible_data_target_mismatches'], 1)
        self.assertFalse(result['instruction_and_known_target_match'])

    def test_storage_inventory_requires_unique_conflict_free_observation(self):
        scratch = Path.home() / '.ai-tmp'
        scratch.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='nemu-comparison-test.', dir=scratch) as directory:
            path = Path(directory) / 'globals.json'
            row = dict(symbol='Setting', va='0x420000', extent_bytes=4,
                       observations=[dict(va='0x420000')], original_storage=dict(section='.data'))
            document = dict(generator='scripts/map_nemu_globals.py', original_sha256='original-hash',
                            globals=[row], conflicts=[])
            path.write_text(json.dumps(document))
            _, symbols = compare.load_metadata([path])
            self.assertEqual(symbols[0]['address_basis'], 'authoritative_application_storage')
            self.assertEqual(symbols[0]['size'], 4)
            document['conflicts'] = [dict(kind='address', symbol='Setting')]
            path.write_text(json.dumps(document))
            _, symbols = compare.load_metadata([path])
            self.assertEqual(symbols[0]['address_basis'], 'application_observation')

    def test_import_thunk_identity_comes_from_decoded_thunk_and_iat(self):
        before = Image(b'\xff\x25' + struct.pack('<I', 0x430000))
        after = Image(b'\xff\x25' + struct.pack('<I', 0x530000))
        after.base = 0x500000
        original = compare.Symbols([dict(name='__imp_Function', va=0x430000,
            kind='import', dll='Plugin.dll', import_name='Function', address_basis='import_table')])
        candidate = compare.Symbols([dict(name='__imp_Function', va=0x530000,
            kind='import', dll='PLUGIN.dll', import_name='Function', address_basis='import_table')])
        original.image, candidate.image = before, after
        field = dict(offset=1, width=4, kind='REL', authority='decoded_external_branch',
                     original_target=0x401000, candidate_target=0x501000)
        result = compare.resolve_field(field, dict(name='Caller', va=0x402000, size=6), original, candidate)
        self.assertTrue(result['identity_verified'])
        candidate.rows[0]['import_name'] = 'WrongFunction'
        result = compare.resolve_field(field, dict(name='Caller', va=0x402000, size=6), original, candidate)
        self.assertEqual(result['status'], 'mismatch')

    def test_source_operand_proof_requires_both_hashes_and_real_symbol_addend(self):
        self.original.rows[0]['address_basis'] = 'authoritative_application_storage'
        scratch = Path.home() / '.ai-tmp'
        with tempfile.TemporaryDirectory(prefix='nemu-comparison-test.', dir=scratch) as directory:
            source = Path(directory) / 'source.cpp'
            source.write_text('int *end = Array + 4;\n')
            proof = dict(offset=1, symbol='Array', addend=16,
                         candidate_image_sha256='candidate-hash', source_file=str(source),
                         source_sha256=compare.digest(source.read_bytes()), evidence='Reviewed array-end expression.')
            function = dict(name='Function', va=0x401000, size=6, linked_operand_provenance=[proof])
            field = dict(offset=1, kind='DIR32', width=4, authority='PE_HIGHLOW',
                         candidate_target=0x520010, original_target=0x420010)
            self.assertEqual(compare.apply_operand_provenance(function, [field], self.candidate,
                             self.original, 'other-image'), [])
            self.assertEqual(len(compare.apply_operand_provenance(function, [field], self.candidate,
                                 self.original, 'candidate-hash')), 1)
            result = compare.resolve_field(field, function, self.original, self.candidate)
            self.assertTrue(result['identity_verified'])
            field['candidate_target'] += 1
            with self.assertRaises(ValueError):
                compare.apply_operand_provenance(function, [field], self.candidate, self.original, 'candidate-hash')

    def test_forced_unresolved_header_address_is_invalid(self):
        self.candidate.invalid_link_fixture = True
        self.candidate.invalid_names = set()
        self.candidate.image_ranges = [(0x501000, 0x530000)]
        field = dict(offset=0, width=4, kind='DIR32', authority='PE_HIGHLOW',
                     candidate_target=0x500000, original_target=0x402000)
        result = compare.resolve_field(field, dict(name='Function', va=0x401000, size=16),
                                       self.original, self.candidate)
        self.assertEqual(result['status'], 'invalid_link_operand')

    def test_complete_fixture_kinds_require_matching_recorded_image_hash(self):
        scratch = Path.home() / '.ai-tmp'
        scratch.mkdir(exist_ok=True)
        image = Image(b'fixture-image')
        with tempfile.TemporaryDirectory(prefix='nemu-manifest-test.', dir=scratch) as directory:
            path = Path(directory) / 'diagnostic.json'
            for kind in ('complete_relocation_diagnostic', 'resolved_relocation_diagnostic'):
                with self.subTest(kind=kind):
                    document = dict(kind=kind, sha256=compare.digest(image.data), command=['link'])
                    path.write_text(json.dumps(document))
                    result = compare.fixture_manifest(image, path)
                    self.assertTrue(result['image_hash_verified'])
                    self.assertFalse(result['forced_unresolved_link'])
                    document['sha256'] = compare.digest(b'a different image')
                    path.write_text(json.dumps(document))
                    with self.assertRaisesRegex(ValueError, 'fixture image hash does not match'):
                        compare.fixture_manifest(image, path)

    def test_unrelated_manifest_top_level_hash_does_not_verify_fixture(self):
        scratch = Path.home() / '.ai-tmp'
        scratch.mkdir(exist_ok=True)
        image = Image(b'fixture-image')
        with tempfile.TemporaryDirectory(prefix='nemu-manifest-test.', dir=scratch) as directory:
            path = Path(directory) / 'diagnostic.json'
            path.write_text(json.dumps(dict(kind='unrelated', sha256=compare.digest(image.data))))
            self.assertFalse(compare.fixture_manifest(image, path)['image_hash_verified'])

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

    def test_metadata_hash_is_bound_to_parsed_bytes(self):
        scratch = Path.home() / '.ai-tmp'
        with tempfile.TemporaryDirectory(prefix='nemu-comparison-test.', dir=scratch) as directory:
            path = Path(directory) / 'metadata.json'
            content = b'{"globals": {"Array": "0x420000"}}'
            path.write_bytes(content)
            hashes = {}
            compare.load_metadata([path], hashes)
            path.write_text('{"globals": {"Other": "0x430000"}}')
            self.assertEqual(hashes[str(path)], compare.digest(content))
            with self.assertRaises(ValueError):
                compare.load_metadata([path], hashes)

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
