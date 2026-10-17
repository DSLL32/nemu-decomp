#!/usr/bin/env python3
"""Compare typed retained data and symbolic callback references after linking."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]
OBJECTS = [
    ('LegacyNibblePermutation', 0x440B00, 16, 'unsigned char[16]',
     'Nibble permutation (5*i+11)&15; original use unknown.'),
    ('LegacyHighNibbleBitLength', 0x477754, 16, 'unsigned char[16]',
     'High-nibble bit length, with zero mapped to zero; original use unknown.'),
    ('LegacyLowNibbleBitLength', 0x477764, 16, 'unsigned char[16]',
     'Low-nibble bit length; original use unknown.'),
    ('LegacyEmptyCallbacks', 0x477774, 36, 'void (__cdecl *[9])()',
     'Nine references to a shared empty function; signatures and array versus separate declarations are inferred.'),
    ('LegacyNtscClockRate', 0x477798, 4, 'unsigned int',
     '48681812 equals the NTSC rate used by CPU control and memory code; no reference to this object survives.'),
    ('LegacyIdentityMatrix', 0x4777A0, 64, 'float[4][4]',
     'Single-precision identity matrix; original owner and use unknown.'),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('map_file', type=Path)
    parser.add_argument('--report', type=Path, default=ROOT/'analysis/nemu-legacy-data.json')
    args = parser.parse_args()
    original = PE32(ROOT/'master/nemu64.exe')
    candidate = PE32(args.candidate)
    symbols = {m[1]: int(m[2],16) for m in re.finditer(
        r'^\s+[0-9a-fA-F]+:[0-9a-fA-F]+\s+\?([^@]+)@@\S+\s+([0-9a-fA-F]+)',
        args.map_file.read_text(), re.M)}
    rows = []
    for name, va, size, typ, purpose in OBJECTS:
        cv = symbols[name]
        a = original.data[original.offset(va-original.base):original.offset(va-original.base)+size]
        b = candidate.data[candidate.offset(cv-candidate.base):candidate.offset(cv-candidate.base)+size]
        row = dict(name=name, va=hex(va), size=size, type=typ, source='src/nemu64/LegacyData.cpp',
                   spelling='inferred', original_file_and_grouping='unknown; provisional LegacyData.cpp',
                   purpose=purpose, compiled_va=hex(cv), original_sha256=hashlib.sha256(a).hexdigest())
        if name == 'LegacyEmptyCallbacks':
            assert struct.unpack('<9I', a) == (0x41FEF0,)*9
            assert struct.unpack('<9I', b) == (symbols['DynarecNoOperation'],)*9
            row['validation'] = dict(kind='symbolic_function_pointer_identity', count=9,
                original_target='0x41fef0', reconstructed_symbol='DynarecNoOperation', equal=True,
                note='Pointer values are compared through independently known function identity, not masked or patched.')
        else:
            assert a == b, name
            row['validation'] = dict(kind='complete_compiled_bytes', equal=True,
                                     sha256=hashlib.sha256(b).hexdigest())
        rows.append(row)
    report = dict(original='master/nemu64.exe', original_sha256=hashlib.sha256(original.data).hexdigest(),
        source='src/nemu64/LegacyData.cpp', source_sha256=hashlib.sha256((ROOT/'src/nemu64/LegacyData.cpp').read_bytes()).hexdigest(),
        status='116 literal data bytes exact; nine callback pointer identities exact; final section layout not matched',
        compiler='Microsoft Visual C++ 7.0 /O2 /Ob1 /ML /GL; /LTCG /DLL /NOENTRY /NODEFAULTLIB',
        candidate_sha256=hashlib.sha256(candidate.data).hexdigest(), globals=rows,
        notes=['No original native machine-code bytes are emitted as data.',
               'No storage definition is introduced for the four zero bytes at 0x47779C.',
               'No direct native code or initialized-data references to these retained objects were found.',
               'The 16-byte permutation is adjacent to PIF tables; this does not prove its original source filename.'])
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('Six typed objects verified: 116 literal bytes and nine symbolic callback pointers.')


if __name__ == '__main__':
    main()
