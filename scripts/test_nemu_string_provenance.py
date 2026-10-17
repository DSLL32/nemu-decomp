#!/usr/bin/env python3
"""Fail-closed complete named-string pointer evidence checks."""

import struct
import unittest

import compare_nemu_functions as compare
from nemu_string_provenance import prove_string_operand, string_record
from test_nemu_float_provenance import Image


def record_name(payload):
    size = len(payload)
    number = str(size - 1) if size <= 10 else ''.join(
        chr(ord('A') + int(char, 16)) for char in format(size, 'x')) + '@'
    # The verifier deliberately leaves the compiler checksum opaque. Fixture
    # names test its length/preview grammar, not a reconstructed hash algorithm.
    preview = ''.join('?$' + chr(65 + (byte >> 4)) + chr(65 + (byte & 15))
                      for byte in payload[:32])
    return '??_C@_0' + number + 'A@' + preview + '@'


class StringProofTests(unittest.TestCase):
    def fixture(self, payload=b'open\0', prefix=b'\x68'):
        self.payload = payload
        self.before_code = prefix + struct.pack('<I', 0x402010) + b'\xc3'
        self.after_code = prefix + struct.pack('<I', 0x502020) + b'\xc3'
        self.before = Image(0x400000, self.before_code, payload, 0x10)
        self.after = Image(0x500000, self.after_code, payload, 0x20)
        self.original = compare.Symbols([])
        self.original.image = self.before
        self.set_rows([dict(name=record_name(payload), va=0x502020,
                            kind='data', address_basis='linked_map')])
        self.field = compare.linked_fields(self.after_code, 0x501000, len(self.after_code),
                                          {0x501000 + len(prefix)}, self.symbols)[0]
        self.field['original_target'] = 0x402010
        return self

    def set_rows(self, rows):
        self.symbols = compare.Symbols(rows)
        self.symbols.image = self.after

    def proof(self):
        return prove_string_operand(self.field, 0x401000, 0x501000, self.original, self.symbols)

    def reject(self, reason):
        result = self.proof()
        self.assertFalse(result['verified'])
        self.assertIn(reason, result['reason'])

    def test_push_register_move_and_stack_move_materializations(self):
        for prefix in [b'\x68', b'\xb8', b'\xc7\x44\x24\x04']:
            with self.subTest(prefix=prefix):
                self.fixture(prefix=prefix)
                proof = self.proof()
                self.assertTrue(proof['verified'], proof)
                self.assertEqual(proof['basis'], 'complete_string_record_pointer')
                self.assertEqual(proof['original_access']['access'], 'record_pointer_materialization')

    def test_short_long_and_embedded_nul_complete_records(self):
        for payload in [b'open\0', bytes(range(65, 110)) + b'\0', b'Filter\0*.bin\0Other\0*.rom\0\0']:
            with self.subTest(payload=payload):
                self.fixture(payload)
                proof = self.proof()
                self.assertTrue(proof['verified'], proof)
                self.assertEqual(proof['size'], len(payload))
                self.assertEqual(proof['original_record']['end'], 0x402010 + len(payload))
                self.assertEqual(proof['encoded_preview_size'], min(32, len(payload)))
                self.assertFalse(proof['name_checksum_verified'])

    def test_actual_short_msvc_preview_and_punctuation_escapes(self):
        self.assertEqual(string_record('??_C@_04PMOCAHAA@open?$AA@')['preview'], b'open\0')
        punctuation = '??_C@_0L@A@?0?1?2?3?4?5?6?7?8?9?$AA@'
        self.assertEqual(string_record(punctuation)['preview'], b',/\\:. \n\t\'-\0')
        self.assertEqual(string_record('??_C@_0M@MOHFBNCB@$$$temp?4tmp?$AA@')['preview'], b'$$$temp.tmp\0')

    def test_comparer_proves_contents_without_adding_masks(self):
        self.fixture()
        function = dict(name='Function', va=0x401000, size=len(self.before_code))
        result = compare.compare_bytes(function, self.after_code, len(self.after_code), 0x501000,
                                       [self.field], self.before, self.original, self.symbols)
        self.assertTrue(result['fully_verified_relocations'])
        self.assertEqual(result['masked_bytes'], 4)
        self.assertEqual(result['inferred_absolute_masks'], 0)
        self.assertEqual(result['relocations'][0]['resolution_bases'], ['complete_string_record_pointer'])

    def test_scalar_loads_have_distinct_bounded_read_scope(self):
        for prefix, width in [(b'\x66\xa1', 2), (b'\xa1', 4)]:
            with self.subTest(prefix=prefix):
                self.fixture(prefix=prefix)
                proof = self.proof()
                self.assertTrue(proof['verified'], proof)
                self.assertEqual(proof['basis'], 'complete_record_scalar_read')
                self.assertEqual(proof['read_width'], width)
                self.assertEqual(proof['read_bytes'], self.payload[:width].hex())
                self.assertEqual(proof['scalar_value_little_endian'], int.from_bytes(self.payload[:width], 'little'))
                self.assertEqual(proof['original_access']['read_end'], 0x402010 + width)
                self.assertEqual(proof['original_access']['access'], 'scalar_memory_read')
                self.assertNotIn('pointer_width', proof['original_access'])
                self.field['address_use'] = 'immediate_address'
                self.reject('pointer materialization')

    def test_scalar_whitelist_rejects_byte_loads_stores_and_read_modify_write(self):
        for prefix in [b'\xa0', b'\xa3', b'\xff\x05', b'\x8c\x1d', b'\x8e\x1d', b'\x0f\xb7\x05']:
            with self.subTest(prefix=prefix):
                self.fixture(prefix=prefix)
                self.reject('16/32-bit MOV register read')

    def test_scalar_sources_must_be_absolute_and_unqualified(self):
        for prefix in [b'\x8b\x80', b'\x64\xa1', b'\x8b\x04\x85']:
            with self.subTest(prefix=prefix):
                self.fixture(prefix=prefix)
                self.reject('absolute displacement')

    def test_scalar_read_must_fit_complete_encoded_record(self):
        self.fixture(payload=b'x\0', prefix=b'\xa1')
        self.reject('extends beyond')

    def test_scalar_equal_read_prefix_does_not_excuse_different_record_tail(self):
        self.fixture(prefix=b'\x66\xa1')
        self.before.data[0x313] = ord('X')  # Read "op" unchanged; full record differs.
        self.reject('bits disagree')

    def test_scalar_record_must_be_complete_even_when_read_itself_fits(self):
        self.fixture(prefix=b'\x66\xa1')
        self.after.sections[1]['virtual_size'] = 0x22
        self.reject('complete literal')

    def test_scalar_duplicate_record_and_addend_remain_unresolved(self):
        self.fixture(prefix=b'\xa1')
        self.after.data[0x351:0x351 + len(self.payload)] = self.payload
        self.reject('not unique')
        self.fixture(prefix=b'\xa1')
        self.field['candidate_target'] += 1
        self.assertIsNone(self.proof())

    def test_scalar_wrong_original_register_and_operand_location_fail_closed(self):
        self.fixture(prefix=b'\x8b\x05')
        self.before.data[0x201] = 0x0d  # ECX instead of EAX.
        self.reject('instructions differ')
        self.fixture(prefix=b'\xa1')
        self.field['offset'] += 1
        self.reject('recorded relocation target')

    def test_scalar_comparer_keeps_proof_separate_without_new_masks(self):
        self.fixture(prefix=b'\xa1')
        function = dict(name='Function', va=0x401000, size=len(self.before_code))
        result = compare.compare_bytes(function, self.after_code, len(self.after_code), 0x501000,
                                       [self.field], self.before, self.original, self.symbols)
        self.assertTrue(result['fully_verified_relocations'])
        self.assertEqual(result['masked_bytes'], 4)
        self.assertEqual(result['relocations'][0]['resolution_bases'], ['complete_record_scalar_read'])
        self.assertIn('string_record_scalar_read_proof', result['relocations'][0])
        self.assertNotIn('string_record_pointer_proof', result['relocations'][0])

    def test_arithmetic_immediate_is_not_pointer_materialization(self):
        self.fixture(prefix=b'\x05')  # ADD EAX, imm32.
        self.reject('pointer materialization')

    def test_wrong_original_operation_or_register_is_rejected(self):
        self.fixture()
        self.before.data[0x200] = 0xb8  # MOV instead of PUSH.
        self.reject('instructions differ')
        self.fixture(prefix=b'\xb8')
        self.before.data[0x200] = 0xb9
        self.reject('instructions differ')

    def test_wrong_displacement_metadata_or_target_is_rejected(self):
        self.fixture()
        self.field['offset'] = 2
        self.reject('recorded relocation target')
        self.fixture()
        self.field['original_target'] += 1
        self.reject('recorded relocation target')

    def test_missing_relocation_authority_is_rejected(self):
        self.fixture()
        self.field['authority'] = 'MAP_address_inference'
        self.reject('PE HIGHLOW')

    def test_unrelated_symbol_and_addend_target_remain_unresolved(self):
        self.fixture()
        self.set_rows([dict(self.symbols.rows[0], name='UnrelatedData')])
        self.assertIsNone(self.proof())
        self.fixture()
        self.field['candidate_target'] += 1
        self.assertIsNone(self.proof())

    def test_alias_wrong_kind_and_repeated_name_locations_are_rejected(self):
        for change, reason in [(dict(name='Alias'), 'aliases'),
                               (dict(va=0x502080), 'more than one linked location')]:
            self.fixture()
            row = self.symbols.rows[0]
            self.set_rows([row, dict(row, **change)])
            self.reject(reason)
        self.fixture()
        self.set_rows([dict(self.symbols.rows[0], kind='function')])
        self.reject('linked MAP data record')

    def test_known_record_overlap_and_named_suffix_are_rejected(self):
        self.fixture()
        row = self.symbols.rows[0]
        self.set_rows([row, dict(row, name='Interior', va=0x502022)])
        self.reject('inside the complete string')
        self.set_rows([row, dict(row, name=record_name(b'prefixopen\0'), va=0x50201a)])
        self.reject('interior suffix')

    def test_encoded_length_and_preview_must_match_complete_record(self):
        self.fixture()
        row = self.symbols.rows[0]
        self.set_rows([dict(row, size=4)])
        self.reject('extent disagrees')
        self.set_rows([dict(row, name=record_name(b'other\0'))])
        self.reject('compiler name preview')
        self.set_rows([dict(row, name=record_name(b'shut\0'))])
        self.reject('compiler name preview')

    def test_malformed_empty_extent_and_wide_records_fail_closed(self):
        for name in ['??_C@_0A@A@@', '??_C@_04A@?$A@', '??_C@_14A@open?$AA@']:
            self.fixture()
            self.set_rows([dict(self.symbols.rows[0], name=name)])
            self.assertFalse(self.proof()['verified'])

    def test_terminal_and_embedded_nul_bytes_are_compared(self):
        self.fixture()
        self.after.data[0x324] = 65
        self.reject('terminal NUL')
        self.fixture(b'one\0two\0')
        self.before.data[0x313] = 65
        self.reject('bits disagree')

    def test_full_contents_beyond_name_preview_are_compared(self):
        self.fixture(b'a' * 40 + b'\0')
        self.before.data[0x310 + 35] = 98
        self.reject('bits disagree')

    def test_duplicate_payloads_and_empty_string_remain_ambiguous(self):
        for image in ['before', 'after']:
            self.fixture()
            getattr(self, image).data[0x351:0x351 + len(self.payload)] = self.payload
            self.reject('not unique')
        self.fixture(b'\0')
        self.reject('not unique')

    def test_storage_must_be_complete_mapped_readonly_and_nonexecutable(self):
        for mode in ['writable', 'executable', 'raw', 'virtual', 'truncated']:
            with self.subTest(mode=mode):
                self.fixture()
                if mode in ('writable', 'executable'):
                    struct.pack_into('<I', self.after.data, 0x1c4,
                                     0xc0000040 if mode == 'writable' else 0x60000040)
                elif mode == 'raw':
                    self.after.sections[1]['raw_size'] = 0x24
                elif mode == 'virtual':
                    self.after.sections[1]['virtual_size'] = 0x24
                else:
                    self.after.data = self.after.data[:0x324]
                self.assertFalse(self.proof()['verified'])

    def test_instruction_body_mismatch_overrides_local_record_proof(self):
        self.fixture()
        function = dict(name='Function', va=0x401000, size=len(self.before_code))
        result = compare.compare_bytes(function, self.after_code[:-1] + b'\x90', len(self.after_code),
                                       0x501000, [self.field], self.before, self.original, self.symbols)
        self.assertFalse(result['relocations'][0]['identity_verified'])
        self.assertEqual(result['relocations'][0]['status'], 'not_compared_instruction_mismatch')


if __name__ == '__main__':
    unittest.main()
