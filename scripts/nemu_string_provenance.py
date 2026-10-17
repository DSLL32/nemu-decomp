"""Prove complete named narrow-string contents at materialized pointer values.

MSVC ??_C@_0 records encode their complete byte length (including the final
NUL), a checksum, and up to 32 escaped preview bytes. We validate the length
and complete preview; the checksum remains opaque. The independently bounded
candidate record supplies the expected complete contents, including embedded
NULs. Both images must contain those contents uniquely in read-only .rdata.

This proves an address materializes a pointer to equal complete record contents.
It does not establish a subsequent C-string API use, an original source spelling,
or the boundaries of an original allocation. Direct loads, suffix/addend targets,
aliases, and overlapping named records remain unresolved. No masks are created.
"""

import hashlib
import re
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

# Share the exact file/mapped-section bounds and all-byte-offset uniqueness
# checks used by the floating verifier; both modules are hash-pinned in reports.
from nemu_float_provenance import _read, _readonly_records


STRING_NAME = re.compile(r'\?\?_C@_([01])([0-9]|[A-P]{1,8}@)([A-P]{1,8})@([^@]*)@\Z')
ESCAPED_PUNCTUATION = b',/\\:. \n\t\'-'
PROOF_BASIS = 'complete_string_record_pointer'


def _record_size(number):
    return int(number) + 1 if number.isdigit() else int(
        ''.join(format(ord(c) - ord('A'), 'x') for c in number[:-1]), 16)


def string_record(name):
    """Decode a narrow compiler record's full length and bounded name preview."""
    match = STRING_NAME.fullmatch(name)
    if match is None:
        raise ValueError('malformed or unsupported MSVC string record name')
    kind, number, checksum, text = match.groups()
    if kind != '0':
        raise ValueError('wide string records are outside this narrow-record proof')
    size = _record_size(number)
    if not size:
        raise ValueError('compiler string record has zero extent')
    preview, offset = bytearray(), 0
    while offset < len(text):
        char = text[offset]
        offset += 1
        if char != '?':
            if not (char.isascii() and (char.isalnum() or char in '_$')):
                raise ValueError('unsupported unescaped string preview byte')
            preview.append(ord(char))
        elif offset == len(text):
            raise ValueError('truncated string preview escape')
        else:
            char = text[offset]
            offset += 1
            if char in '0123456789':
                preview.append(ESCAPED_PUNCTUATION[int(char)])
            elif char == '$' and re.fullmatch('[A-P]{2}', text[offset:offset + 2]):
                preview.append((ord(text[offset]) - ord('A')) * 16 + ord(text[offset + 1]) - ord('A'))
                offset += 2
            else:
                raise ValueError('unsupported or truncated string preview escape')
    if len(preview) != min(size, 32):
        raise ValueError('name preview does not cover the complete compiler-defined preview extent')
    return dict(size=size, preview=bytes(preview), checksum=checksum)


def _decode_pointer(image, function_va, field, target):
    offset, size = field['instruction_offset'], field['instruction_size']
    if offset < 0 or not 1 <= size <= 15:
        raise ValueError('invalid pointer instruction extent')
    code = _read(image, function_va + offset, size)
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(code, function_va + offset))
    if len(instructions) != 1 or instructions[0].size != size:
        raise ValueError('pointer instruction extent is not one complete instruction')
    instruction = instructions[0]
    operands = instruction.operands
    push = instruction.mnemonic == 'push' and len(operands) == 1
    move = (instruction.mnemonic == 'mov' and len(operands) == 2 and
            operands[0].type in (X86_OP_REG, X86_OP_MEM) and operands[0].size == 4)
    if not (push or move) or operands[-1].type != X86_OP_IMM or operands[-1].size != 4:
        raise ValueError('instruction is not a supported complete pointer materialization')
    if (instruction.imm_size != 4 or offset + instruction.imm_offset != field['offset'] or
            (operands[-1].imm & 0xffffffff) != target):
        raise ValueError('decoded immediate does not identify the recorded relocation target')
    shape = bytearray(code)
    shape[instruction.imm_offset:instruction.imm_offset + 4] = bytes(4)
    return dict(instruction_va=instruction.address, size=size, mnemonic=instruction.mnemonic,
                access='record_pointer_materialization', pointer_width=4), bytes(shape)


def prove_string_operand(field, original_va, candidate_va, original_symbols, candidate_symbols):
    """Return a complete named-string pointer proof/rejection, otherwise None."""
    if candidate_symbols is None:
        return None
    rows = candidate_symbols.addresses.get(field.get('candidate_target'), [])
    names = {row['name'] for row in rows}
    string_names = [name for name in names if name.startswith('??_C@_')]
    if not string_names:
        return None
    try:
        if len(names) != 1 or len(string_names) != 1:
            raise ValueError('string record has unrelated or ambiguous linked aliases')
        name = string_names[0]
        if any(row.get('kind') != 'data' or row.get('address_basis') != 'linked_map' for row in rows):
            raise ValueError('string lacks an exact linked MAP data record')
        named = [row for row in candidate_symbols.rows if row['name'] == name]
        if {row['va'] for row in named} != {field['candidate_target']}:
            raise ValueError('compiler string name has more than one linked location')
        if (field['kind'], field['width'], field['authority']) != ('DIR32', 4, 'PE_HIGHLOW'):
            raise ValueError('string lacks an existing PE HIGHLOW operand')
        if field.get('address_use') != 'immediate_address' or candidate_va is None:
            raise ValueError('direct data access does not establish complete-record pointer materialization')
        record = string_record(name)
        start, size = field['candidate_target'], record['size']
        if any(row.get('size') not in (None, size) for row in named):
            raise ValueError('linked string extent disagrees with the compiler record length')
        for row in candidate_symbols.rows:
            if start < row['va'] < start + size:
                raise ValueError('another linked record starts inside the complete string')
            if row['va'] < start:
                preceding = STRING_NAME.fullmatch(row['name'])
                preceding_size = _record_size(preceding[2]) if preceding else row.get('size', 0)
                if preceding_size and start < row['va'] + preceding_size:
                    raise ValueError('named string is an interior suffix of another complete record')
        original, candidate = original_symbols.image, candidate_symbols.image
        before, before_shape = _decode_pointer(original, original_va, field, field['original_target'])
        after, after_shape = _decode_pointer(candidate, candidate_va, field, start)
        if before_shape != after_shape:
            raise ValueError('original and rebuilt pointer instructions differ outside the address field')
        payload = _read(candidate, start, size)
        if payload[-1:] != b'\0':
            raise ValueError('complete compiler string record lacks its terminal NUL')
        if payload[:len(record['preview'])] != record['preview']:
            raise ValueError('candidate string contents disagree with the compiler name preview')
        candidate_record = _readonly_records(candidate, start, payload)
        original_record = _readonly_records(original, field['original_target'], payload)
        return dict(verified=True, basis=PROOF_BASIS, symbol=name, size=size,
                    content_sha256=hashlib.sha256(payload).hexdigest(),
                    encoded_preview_size=len(record['preview']), name_checksum_verified=False,
                    original_access=before, candidate_access=after,
                    original_record=original_record, candidate_record=candidate_record,
                    identity_scope='Pointer to equal complete named-record contents, including terminal and embedded NULs; no consuming API, original source spelling, or original allocation boundary is asserted.',
                    uniqueness_scope='Every byte offset in file-backed .rdata, including unaligned incidental contents.')
    except (AttributeError, KeyError, ValueError, struct.error, OverflowError) as error:
        return dict(verified=False, reason=str(error))
