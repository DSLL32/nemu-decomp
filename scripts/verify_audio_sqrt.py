#!/usr/bin/env python3
"""Audit native square-root storage, operand addends and initialized contents.

This checks the inferred private lookup representation without modifying an
image. Equivalent two-array source spelling remains possible.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM
from verify_gs import COFF
from verify_audio_processing_behavior import Fixture

ROOT = Path(__file__).resolve().parents[1]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--objects', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    fixture = Fixture(args.objects)
    path = args.objects / 'AudioProcessing.obj'
    coff = COFF(path)
    symbols = {s['name']: s for s in coff.symbols}
    lookup = symbols['_AudioSqrtMantissa']
    wet, dry = symbols['_AudioEnvelopeWet'], symbols['_AudioEnvelopeDry']
    section = coff.sections[lookup['section'] - 1]
    assert section['name'] == '.bss' and section['size'] == 0x40008
    assert lookup['storage_class'] == 3 and lookup['value'] == 0
    assert wet['section'] == dry['section'] == lookup['section']
    assert (wet['value'], dry['value']) == (0x40000, 0x40004)
    function = next(f for f in coff.functions() if f['name'] == '_InitializeAudioSqrt')
    assert function['size'] == 112
    va = fixture.addresses['_InitializeAudioSqrt']
    offset = fixture.original.offset(va - fixture.original.base)
    target = fixture.original.data[offset:offset + 112]
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    writes = [i for i in decoder.disasm(target, va)
              if i.mnemonic == 'mov' and i.operands[0].type == X86_OP_MEM
              and i.operands[0].mem.disp >= 0x10000000]
    assert len(writes) == len(function['relocations']) == 2
    operands = []
    masked_target, masked_candidate = bytearray(target), bytearray(function['data'])
    for write, relocation in zip(writes, function['relocations']):
        at = relocation['offset']
        assert relocation['type'] == 6
        assert relocation['symbol']['name'] == '_AudioSqrtMantissa'
        addend = struct.unpack_from('<I', function['data'], at)[0]
        address = write.operands[0].mem.disp
        assert address == 0x100ebdf0 + addend
        assert at == write.address - va + write.size - 4
        masked_target[at:at + 4] = bytes(4)
        masked_candidate[at:at + 4] = bytes(4)
        operands.append(dict(instruction_va=hex(write.address), original_address=hex(address),
                             coff_offset=at, addend=addend, value_equal=True))
    assert [r['addend'] for r in operands] == [0x20000, 0]
    assert masked_target == masked_candidate
    original, original_symbols = fixture.machine()
    candidate, candidate_symbols = fixture.machine('AudioProcessing')
    original.mem_write(0x1012bdf0, b'WET!DRY!')
    candidate.mem_write(candidate_symbols['_AudioEnvelopeWet'], b'WET!DRY!')
    fixture.call(original, original_symbols['_InitializeAudioSqrt'], [])
    fixture.call(candidate, candidate_symbols['_InitializeAudioSqrt'], [])
    expected = bytes(original.mem_read(0x100ebdf0, 0x40000))
    actual = bytes(candidate.mem_read(candidate_symbols['_AudioSqrtMantissa'], 0x40000))
    assert expected == actual
    assert bytes(original.mem_read(0x1012bdf0, 8)) == b'WET!DRY!'
    assert bytes(candidate.mem_read(candidate_symbols['_AudioEnvelopeWet'], 8)) == b'WET!DRY!'
    report = dict(status='scoped_native_storage_and_behavior_proof',
                  original_sha256=sha(ROOT / 'master/Plugin/audio.dll'),
                  object_sha256=sha(path), source_sha256=sha(ROOT / 'src/audio/ahle_ds/AudioProcessing.c'),
                  script_sha256=sha(Path(__file__)), private_lookup_bytes=0x40000,
                  native_bss_extent=0x40008, relative_storage=dict(lookup=0, wet=0x40000, dry=0x40004),
                  complete_initializer_bytes=112, complete_body_equal_outside_two_operands=True,
                  operands=operands, initialized_entries=65536, initialized_bytes_equal=True,
                  initialized_bytes_sha256=hashlib.sha256(actual).hexdigest(),
                  adjacent_state_preserved=True,
                  limitations=['Single-table source identity is inferred; two explicitly initialized arrays also reproduce this layout.',
                               'These operand addresses come from native stores, not recovered debug symbols.',
                               'Absolute final linked placement and whole-DLL byte equality remain separate checks.'])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('65536 square-root entries match; 112-byte initializer and 262152-byte BSS extent checked')

if __name__ == '__main__':
    main()
