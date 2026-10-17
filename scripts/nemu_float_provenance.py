"""Conservative identity proof for linked MSVC scalar floating constants.

A MAP spelling or equal address/bytes is insufficient. This verifier combines
the compiler's complete __real@ bit/width record with independently decoded
floating reads in both images, complete read-only storage extents, and unique
full-record contents. It establishes the concrete floating value read, not an
original source identifier. Incidental duplicate bytes (even unaligned ones)
remain ambiguous; this intentionally does not infer a typed record directory.
The caller supplies an existing relocation field; this module creates no masks.
"""

import hashlib
import re
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM


FLOAT_NAME = re.compile(r'__real@([0-9a-fA-F]{8}|[0-9a-fA-F]{16})\Z')
# Explicit semantic whitelist: some Capstone versions mark FSTP memory operands
# as reads, so operand.access alone cannot distinguish stores or integer loads.
FLOAT_READS = {'fld', 'fadd', 'fmul', 'fdiv', 'fdivr'}
PROOF_BASIS = 'decoded_unique_readonly_floating_literal'


def _read(image, va, size):
    if size <= 0:
        raise ValueError('invalid extent')
    for section in image.sections:
        start = image.base + section['rva']
        if start <= va and va + size <= start + section['raw_size']:
            offset = section['offset'] + va - start
            data = image.data[offset:offset + size]
            if len(data) == size:
                return data
    raise ValueError('extent is not wholly file-backed')


def _readonly_records(image, address, payload):
    """Bound the complete record and search every byte of every .rdata section."""
    table = image.opt + struct.unpack_from('<H', image.data, image.pe + 20)[0]
    bounds, matches, record = [], [], None
    for index, section in enumerate(image.sections):
        if section.get('name') != '.rdata':
            continue
        flags = struct.unpack_from('<I', image.data, table + index * 40 + 36)[0]
        if (flags & 0xc0000040) != 0x40000040 or flags & 0x20000000:
            raise ValueError('.rdata is not initialized, readable, nonwritable, nonexecutable data')
        start = image.base + section['rva']
        size = section['raw_size']
        mapped_size = min(size, section['virtual_size'])
        data = _read(image, start, size)
        bounds.append(dict(start=start, end=start + size, mapped_end=start + mapped_size))
        offset = data.find(payload)
        while offset >= 0:
            matches.append(start + offset)
            offset = data.find(payload, offset + 1)
        if start <= address and address + len(payload) <= start + mapped_size:
            if record is not None:
                raise ValueError('overlapping record storage')
            record = _read(image, address, len(payload))
    if not bounds or record is None:
        raise ValueError('complete literal is not within mapped file-backed .rdata')
    if record != payload:
        raise ValueError('complete literal bits disagree with the compiler record')
    if matches != [address]:
        raise ValueError('complete payload is not unique across all .rdata byte offsets')
    return dict(start=address, end=address + len(payload), section='.rdata',
                unique_occurrence=address, searched_file_backed_sections=bounds)


def _decode_read(image, function_va, field, width, target):
    offset, size = field['instruction_offset'], field['instruction_size']
    if offset < 0 or not 1 <= size <= 15:
        raise ValueError('invalid instruction extent')
    code = _read(image, function_va + offset, size)
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(code, function_va + offset))
    if len(instructions) != 1 or instructions[0].size != size:
        raise ValueError('instruction extent is not one complete instruction')
    instruction = instructions[0]
    if instruction.mnemonic not in FLOAT_READS or len(instruction.operands) != 1:
        raise ValueError('instruction is not a supported scalar floating read')
    operand = instruction.operands[0]
    if operand.type != X86_OP_MEM or operand.size != width:
        raise ValueError('decoded data width differs from the complete compiler record')
    if operand.mem.base or operand.mem.index or operand.mem.segment:
        raise ValueError('floating address is not an unqualified absolute displacement')
    if (instruction.disp_size != 4 or offset + instruction.disp_offset != field['offset'] or
            (operand.mem.disp & 0xffffffff) != target):
        raise ValueError('decoded displacement does not identify the recorded relocation target')
    shape = bytearray(code)
    shape[instruction.disp_offset:instruction.disp_offset + 4] = bytes(4)
    return dict(instruction_va=instruction.address, size=size, mnemonic=instruction.mnemonic,
                access='floating_read', width=width), bytes(shape)


def prove_float_operand(field, original_va, candidate_va, original_symbols, candidate_symbols):
    """Return a proof/rejection for exact linked __real@ targets, otherwise None.

    Rejections never assert a wrong target: absent identity proof stays unresolved.
    Actual instruction mismatch is handled independently by the enclosing comparer.
    """
    if candidate_symbols is None:
        return None
    rows = candidate_symbols.addresses.get(field.get('candidate_target'), [])
    names = {row['name'] for row in rows}
    float_names = [name for name in names if FLOAT_NAME.fullmatch(name)]
    if not float_names:
        return None
    try:
        if len(names) != 1 or len(float_names) != 1:
            raise ValueError('literal has unrelated or ambiguous linked aliases')
        name = float_names[0]
        if any(row.get('kind') != 'data' or row.get('address_basis') != 'linked_map' for row in rows):
            raise ValueError('literal lacks an exact linked MAP data record')
        named = [row for row in candidate_symbols.rows if row['name'] == name]
        if {row['va'] for row in named} != {field['candidate_target']}:
            raise ValueError('compiler literal name has more than one linked location')
        if (field['kind'], field['width'], field['authority']) != ('DIR32', 4, 'PE_HIGHLOW'):
            raise ValueError('literal lacks an existing PE HIGHLOW operand')
        if field.get('address_use') != 'memory_access' or candidate_va is None:
            raise ValueError('literal lacks a decoded memory access context')
        digits = FLOAT_NAME.fullmatch(name).group(1)
        width = len(digits) // 2
        payload = int(digits, 16).to_bytes(width, 'little')
        start = field['candidate_target']
        if any(row.get('size') not in (None, width) for row in named):
            raise ValueError('linked record extent disagrees with the compiler literal width')
        if any(start < row['va'] < start + width for row in candidate_symbols.rows):
            raise ValueError('another linked record starts inside the complete literal')
        original = original_symbols.image
        candidate = candidate_symbols.image
        before, before_shape = _decode_read(original, original_va, field, width, field['original_target'])
        after, after_shape = _decode_read(candidate, candidate_va, field, width, start)
        if before_shape != after_shape or before['access'] != after['access']:
            raise ValueError('original and rebuilt floating access instructions differ')
        original_record = _readonly_records(original, field['original_target'], payload)
        candidate_record = _readonly_records(candidate, start, payload)
        return dict(verified=True, basis=PROOF_BASIS, symbol=name, width=width,
                    bits=digits.lower(), content_sha256=hashlib.sha256(payload).hexdigest(),
                    original_access=before, candidate_access=after,
                    original_record=original_record, candidate_record=candidate_record,
                    identity_scope='Complete scalar floating value read; no original source spelling is asserted.',
                    uniqueness_scope='Every byte offset in file-backed .rdata, including unaligned incidental contents.')
    except (AttributeError, KeyError, ValueError, struct.error, OverflowError) as error:
        return dict(verified=False, reason=str(error))
