#!/usr/bin/env python3
"""Fail-closed tests for complete, typed floating-literal identities."""

from pathlib import Path
import struct
import unittest

import compare_nemu_functions as compare
from nemu_float_provenance import prove_float_operand


class Image:
    """In-memory PE section/header fixture with real section characteristics."""
    path = Path('<floating-literal-test>')
    pe, opt = 0x80, 0x98

    def __init__(self, base, code, literal, literal_offset):
        self.base = base
        self.sections = [dict(name='.text', rva=0x1000, virtual_size=0x100,
                              raw_size=0x100, offset=0x200),
                         dict(name='.rdata', rva=0x2000, virtual_size=0x100,
                              raw_size=0x100, offset=0x300)]
        self.data = bytearray(0x400)
        struct.pack_into('<H', self.data, self.pe + 20, 0xe0)
        table = self.opt + 0xe0
        struct.pack_into('<I', self.data, table + 36, 0x60000020)
        struct.pack_into('<I', self.data, table + 40 + 36, 0x40000040)
        self.data[0x200:0x200 + len(code)] = code
        self.data[0x300 + literal_offset:0x300 + literal_offset + len(literal)] = literal

    def offset(self, rva):
        for section in self.sections:
            if section['rva'] <= rva < section['rva'] + section['raw_size']:
                return section['offset'] + rva - section['rva']
        raise ValueError('unmapped fixture address')


class FloatProofTests(unittest.TestCase):
    def fixture(self, bits='3f7ae148', opcode=b'\xd9\x05'):
        self.payload = int(bits, 16).to_bytes(len(bits) // 2, 'little')
        self.before_code = opcode + struct.pack('<I', 0x402010) + b'\xc3'
        self.after_code = opcode + struct.pack('<I', 0x502020) + b'\xc3'
        self.before = Image(0x400000, self.before_code, self.payload, 0x10)
        self.after = Image(0x500000, self.after_code, self.payload, 0x20)
        self.original = compare.Symbols([])
        self.original.image = self.before
        self.symbols = compare.Symbols([dict(name='__real@' + bits, va=0x502020,
                                            kind='data', address_basis='linked_map')])
        self.symbols.image = self.after
        self.field = compare.linked_fields(self.after_code, 0x501000, len(self.after_code),
                                          {0x501002}, self.symbols)[0]
        self.field['original_target'] = 0x402010
        return self

    def proof(self):
        return prove_float_operand(self.field, 0x401000, 0x501000, self.original, self.symbols)

    def reject(self, reason):
        result = self.proof()
        self.assertFalse(result['verified'])
        self.assertIn(reason, result['reason'])

    def set_rows(self, rows):
        self.symbols = compare.Symbols(rows)
        self.symbols.image = self.after

    def test_complete_single_and_double_records(self):
        for bits, opcode in [('3f7ae148', b'\xd9\x05'), ('3f847ae147ae147b', b'\xdd\x05')]:
            with self.subTest(bits=bits):
                self.fixture(bits, opcode)
                proof = self.proof()
                self.assertTrue(proof['verified'], proof)
                self.assertEqual(proof['width'], len(self.payload))
                self.assertEqual(proof['original_record']['end'], 0x402010 + len(self.payload))

    def test_comparer_changes_identity_without_creating_masks(self):
        self.fixture()
        function = dict(name='Function', va=0x401000, size=len(self.before_code))
        result = compare.compare_bytes(function, self.after_code, len(self.after_code), 0x501000,
                                       [self.field], self.before, self.original, self.symbols)
        self.assertTrue(result['fully_verified_relocations'])
        self.assertEqual(result['masked_bytes'], 4)
        self.assertEqual(result['inferred_absolute_masks'], 0)
        result = compare.compare_bytes(function, self.after_code, len(self.after_code), 0x501000,
                                       [], self.before, self.original, self.symbols)
        self.assertFalse(result['full_function_shape_match'])
        self.assertEqual(result['masked_bytes'], 0)

    def test_unrelated_symbol_does_not_gain_float_identity(self):
        self.fixture()
        row = dict(self.symbols.rows[0], name='UnrelatedInteger')
        self.set_rows([row])
        self.assertIsNone(self.proof())

    def test_interior_pointer_is_not_a_complete_literal_record(self):
        self.fixture()
        self.field['candidate_target'] += 1
        self.assertIsNone(self.proof())

    def test_wrong_symbol_kind_and_basis(self):
        for change in [dict(kind='function'), dict(address_basis='application_observation')]:
            with self.subTest(change=change):
                self.fixture()
                self.set_rows([dict(self.symbols.rows[0], **change)])
                self.reject('exact linked MAP data record')

    def test_alias_and_duplicate_name_locations(self):
        self.fixture()
        row = self.symbols.rows[0]
        self.set_rows([row, dict(row, name='UnrelatedAlias')])
        self.reject('aliases')
        self.set_rows([row, dict(row, va=0x502080)])
        self.reject('more than one linked location')

    def test_internal_or_mis_sized_record(self):
        self.fixture()
        row = self.symbols.rows[0]
        self.set_rows([row, dict(row, name='Interior', va=0x502022)])
        self.reject('inside the complete literal')
        self.set_rows([dict(row, size=8)])
        self.reject('extent disagrees')

    def test_missing_relocation_or_memory_context(self):
        for change, reason in [(dict(authority='MAP_address_inference'), 'PE HIGHLOW'),
                               (dict(width=2), 'PE HIGHLOW'),
                               (dict(address_use='immediate_address'), 'memory access')]:
            with self.subTest(change=change):
                self.fixture()
                self.field.update(change)
                self.reject(reason)

    def test_integer_read_and_float_write_are_rejected(self):
        # Capstone reports read access for FSTP in some releases: semantic
        # instruction checks must reject stores even if that flag says read.
        for opcode in [b'\xdb\x05', b'\xd9\x1d']:
            for original_only in [False, True]:
                with self.subTest(opcode=opcode, original_only=original_only):
                    self.fixture()
                    self.before.data[0x200:0x202] = opcode
                    if not original_only:
                        self.after.data[0x200:0x202] = opcode
                    self.reject('scalar floating read')

    def test_record_width_must_match_both_decoded_accesses(self):
        for original_only in [False, True]:
            self.fixture()
            self.before.data[0x200] = 0xdd  # FLD qword, symbol still dword.
            if not original_only:
                self.after.data[0x200] = 0xdd
            self.reject('decoded data width')

    def test_same_width_different_float_operation_rejected(self):
        self.fixture()
        self.before.data[0x200] = 0xd8  # FADD rather than FLD.
        self.reject('access instructions differ')

    def test_indexed_address_rejected(self):
        self.fixture()
        # FADD dword [EAX+disp32], same width/disp offset as an absolute access.
        self.before.data[0x200:0x202] = b'\xd8\x80'
        self.after.data[0x200:0x202] = b'\xd8\x80'
        self.reject('absolute displacement')

    def test_wrong_displacement_offset_and_target_rejected(self):
        self.fixture()
        self.field['offset'] = 3
        self.reject('recorded relocation target')
        self.fixture()
        self.field['original_target'] += 1
        self.reject('recorded relocation target')

    def test_symbol_bits_must_equal_each_complete_record(self):
        for image in ['before', 'after']:
            with self.subTest(image=image):
                self.fixture()
                getattr(self, image).data[0x310 if image == 'before' else 0x320] ^= 1
                self.reject('bits disagree')

    def test_unaligned_duplicate_in_either_image_is_ambiguous(self):
        for image in ['before', 'after']:
            with self.subTest(image=image):
                self.fixture()
                getattr(self, image).data[0x351:0x351 + len(self.payload)] = self.payload
                self.reject('not unique')

    def test_writable_executable_and_uninitialized_storage_rejected(self):
        for flags in [0xc0000040, 0x60000040, 0x40000000]:
            for image in ['before', 'after']:
                with self.subTest(flags=flags, image=image):
                    self.fixture()
                    struct.pack_into('<I', getattr(self, image).data, 0x1c4, flags)
                    self.reject('nonwritable, nonexecutable')

    def test_truncation_raw_and_virtual_bounds_fail_closed(self):
        for extent in ['file', 'raw', 'virtual']:
            with self.subTest(extent=extent):
                self.fixture()
                if extent == 'file':
                    self.after.data = self.after.data[:0x322]
                elif extent == 'raw':
                    self.after.sections[1]['raw_size'] = 0x22
                else:
                    self.after.sections[1]['virtual_size'] = 0x22
                self.assertFalse(self.proof()['verified'])

    def test_absent_pe_characteristics_fail_closed(self):
        self.fixture()
        self.symbols.image = type('MissingHeaders', (), dict(data=self.after.data,
            base=self.after.base, sections=self.after.sections))()
        self.assertFalse(self.proof()['verified'])

    def test_body_mismatch_cannot_retain_verified_operand(self):
        self.fixture()
        function = dict(name='Function', va=0x401000, size=len(self.before_code))
        result = compare.compare_bytes(function, self.after_code[:-1] + b'\x90', len(self.after_code),
                                       0x501000, [self.field], self.before, self.original, self.symbols)
        self.assertEqual(result['relocations'][0]['status'], 'not_compared_instruction_mismatch')
        self.assertFalse(result['relocations'][0]['identity_verified'])


if __name__ == '__main__':
    unittest.main()
