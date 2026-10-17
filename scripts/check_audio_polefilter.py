#!/usr/bin/env python3
"""Audit the seven remaining pole-filter instruction-encoding differences.

This is deliberately a failed byte-exact comparison with an instruction-level
explanation. It is not normalized-match certification and writes no code image.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM, X86_OP_REG
from pe_fingerprints import PE32
from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    original = PE32(ROOT / 'master/Plugin/audio.dll')
    assert sha256(original.data) == 'f05b3f9e239041758a4aca09a891e2ebebe4136a3b326f8bbc5c160d76c73808'
    comparison = json.loads((ROOT / 'analysis/audio-processing-comparison.json').read_text())
    object_row = next(o for o in comparison['objects'] if Path(o['object']).name == args.object.name)
    coff = COFF(args.object)
    assert sha256(coff.data) == object_row['sha256']
    row = next(f for f in object_row['functions'] if f['name'] == '_PoleFilterAudioCommand')
    function = next(f for f in coff.functions() if f['name'] == row['name'])
    assert row['size_equal'] and row['original_size_verified']
    assert row['unresolved_target_count'] == row['relocation_target_mismatch_count'] == 0
    va = row['original_va']
    start = original.offset(va - original.base)
    target = original.data[start:start + row['original_bytes']]
    candidate = bytearray(function['data'])
    operands = []
    for relocation in row['relocations']:
        assert relocation['target_equal']
        value = relocation['expected_target_va']
        offset = relocation['offset']
        if relocation['type'] == 'REL32':
            value -= va + offset + 4
        else:
            assert relocation['type'] == 'DIR32'
        struct.pack_into('<I', candidate, offset, value & 0xffffffff)
        operands.append(dict(offset=offset, symbol=relocation['symbol'],
                             address_basis=relocation['resolution_basis'],
                             target=hex(relocation['expected_target_va'])))
    different_bytes = [i for i, (a, b) in enumerate(zip(target, candidate)) if a != b]
    assert len(target) == len(candidate) == 2320
    assert len(different_bytes) == 7
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    left = list(decoder.disasm(target, va))
    right = list(decoder.disasm(bytes(candidate), va))
    assert len(left) == len(right)
    assert sum(i.size for i in left) == len(target)
    differences = []
    for a, b in zip(left, right):
        assert a.address == b.address and a.size == b.size
        if a.bytes == b.bytes:
            continue
        assert a.id == b.id and a.mnemonic in ('lea', 'movsx')
        assert len(a.operands) == len(b.operands) == 2
        assert a.operands[0].type == b.operands[0].type == X86_OP_REG
        assert a.operands[0].reg == b.operands[0].reg
        x, y = a.operands[1], b.operands[1]
        assert x.type == y.type == X86_OP_MEM and x.size == y.size
        assert x.size == (4 if a.mnemonic == 'lea' else 2)
        assert x.mem.scale == y.mem.scale == 1
        assert x.mem.base == y.mem.index and x.mem.index == y.mem.base
        assert x.mem.segment == y.mem.segment and x.mem.disp == y.mem.disp
        # Swapping a stack-base register could change the implicit segment.
        assert not {'esp', 'ebp'} & {a.reg_name(x.mem.base), a.reg_name(x.mem.index)}
        assert sum(u != v for u, v in zip(a.bytes, b.bytes)) == 1
        differences.append(dict(offset=hex(a.address - va), original_bytes=a.bytes.hex(),
                                candidate_bytes=b.bytes.hex(),
                                original=f'{a.mnemonic} {a.op_str}',
                                candidate=f'{b.mnemonic} {b.op_str}'))
    assert len(differences) == 7
    report = dict(status='byte_exact_failure_with_seven_equivalent_address_encodings',
                  byte_exact=False, differing_bytes=len(different_bytes),
                  function=row['name'], original_va=hex(va), complete_bytes=len(target),
                  original_complete_sha256=sha256(target), object_sha256=sha256(coff.data),
                  candidate_after_standard_relocations_sha256=sha256(candidate),
                  relocations=operands, differences=differences,
                  limitation='All seven differences exchange base and index at scale one, '
                  'with the same non-stack default segment. These remain seven real byte '
                  'differences and do not satisfy the reconstruction criterion. RDRAM '
                  'identity remains an observed operand inference.')
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('Pole filter: 7 genuine byte differences, all equivalent address encodings')


if __name__ == '__main__':
    main()
