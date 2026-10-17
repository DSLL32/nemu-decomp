#!/usr/bin/env python3
"""Check complete Memory function spans and independently identify address operands.

A relocated fixture must include nemu64.exe, nemu64.map and its Memory.cpp
source snapshot. This is a function comparison, not whole-image byte equality.
Unidentified operands or unequal function extents stop verification; known
mismatches are reported and fail unless their function is explicitly allowed.
"""
import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from pe_fingerprints import PE32
from compare_nemu_functions import read_map, aliases, base_relocations, image_imports
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
from capstone.x86 import X86_OP_IMM
root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('fixture', type=Path)
parser.add_argument('--source-dir', type=Path)
parser.add_argument('--output', type=Path)
parser.add_argument('--allow-unmatched', action='append', default=[], metavar='FUNCTION')
args = parser.parse_args()
fixture = args.fixture.resolve()
source_dir = args.source_dir
if source_dir is None:
    source_dir = next((d for d in (fixture, fixture / 'src/nemu64', fixture / 'src', fixture / 'project') if (d / 'Memory.cpp').is_file()), None)
if source_dir is None:
    parser.error('Fixture has no Memory.cpp snapshot; use --source-dir.')
source = (source_dir / 'Memory.cpp').read_text()
original = PE32(root / 'master/nemu64.exe')
rebuilt = PE32(fixture / 'nemu64.exe')
metadata = json.loads((root / 'analysis/nemu-memory.json').read_text())
maps = read_map(fixture / 'nemu64.map')

def va(name):
    values = {r['va'] for r in maps if name in aliases(r['name'])}
    assert len(values) == 1, (name, values)
    return values.pop()

def u32(image, address):
    return struct.unpack_from('<I', image.data, image.offset(address - image.base))[0]

def blob(image, address, size):
    return image.data[image.offset(address - image.base):image.offset(address - image.base) + size]

def sha(data):
    return hashlib.sha256(data).hexdigest()
spans = {f['name']: f for f in metadata['functions']}
lengths = {
    'CopyByteSwapped': 327,
    'TLBRead': 81,
    'TLBWrite': 725,
    'TLBProbe': 87,
    'CopyAndSwapWords': 36,
    'InvalidateRecompiledRange': 94,
    'ReadPhysicalByte': 199,
    'ReadPhysicalHalf': 273,
    'ReadPhysicalWord': 1640,
    'WritePhysicalByte': 286,
    'WritePhysicalHalf': 312,
    'WritePhysicalWord': 5856,
    'WritePhysicalDoubleWord': 34,
    'TranslateTLB': 287,
}
# Global identities come from independent hardware/source inventories, never
# from pairing the two operands under comparison.
data = [dict(g) for g in metadata['globals']]
data += [
    dict(name='CPU_COP0', va=0x406f048, size=128),
    dict(name='CPU_ActionFlags', va=0x3e69368, size=4),
    dict(name='CPU_Use32BitMemoryAccess', va=0x97ae64, size=4),
    dict(name='Dynarec_Use32BitMemoryAccess', va=0x3343d88, size=4),
    dict(name='SPMemory', va=0x406b048, size=16384),
    dict(name='Dynarec_Mode', va=0x1240e58, size=4),
    dict(name='Dynarec_BlockCode', va=0x1240ef0, size=0x800000),
]
global_inventory = json.loads((root / 'analysis/nemu-globals.json').read_text())
for symbol in ('CPU_AudioEnabled', 'CPU_AudioCyclesRemaining', 'VideoNTSC', 'AIGetReg', 'SRAMData'):
    row = next((g for g in global_inventory['globals'] if g['symbol'] == symbol))
    data.append(dict(name=symbol, va=int(row['va'], 16), size=row['extent_bytes']))
for row in global_inventory['globals']:
    if not row.get('va') or not row.get('extent_bytes') or any((x['name'] == row['symbol'] for x in data)):
        continue
    candidates = {r['va'] for r in maps if row['symbol'] in aliases(r['name'])}
    if len(candidates) == 1:
        data.append(dict(name=row['symbol'], va=int(row['va'], 16), size=row['extent_bytes']))
for row in data:
    row['rebuilt_va'] = va(row['name'])
functions = {f['name']: f['va'] for f in metadata['functions']}
functions.update(
    UpdateAudioTiming=0x4157d0,
    NotifyTools=0x408330,
    SetStatusText=0x41a6c0,
    WriteSRAM=0x4211b0,
    ReadSRAM=0x4211e0,
    SaveFunctionSignatures=0x41ffa0,
    ReportMemoryError=0x414e30,
    ReportCPUError=0x414eb0,
    ProcessPIF=0x420680,
)
library = json.loads((root / 'analysis/nemu-library-addresses.json').read_text())
assert library['original_sha256'] == sha(original.data)
except_rows = [r for r in library['symbols'] if r['name'] == '__except_handler3' and (not r.get('comparison_only'))]
assert except_rows and len({r['va'] for r in except_rows}) == 1
functions['__except_handler3'] = except_rows[0]['va']
for name in ('__allmul', '__alldiv', '_sprintf'):
    rows = [r for r in library['symbols'] if r['name'] == name and (not r.get('comparison_only'))]
    assert len({r['va'] for r in rows}) == 1
    functions[name] = rows[0]['va']
functions = {va(n): (n, a) for n, a in functions.items()}
iat_orig = {r['name']: r['va'] for r in image_imports(original)}
iats = {r['va']: (r['name'], iat_orig[r['name']]) for r in image_imports(rebuilt) if r['name'] in iat_orig}
affine = []
for name, guest in [('SPMemory', 0x4000000), ('PIFRam', 0x1fc007c0), ('PIFRom', 0x1fc00000)]:
    row = next((g for g in data if g['name'] == name))
    affine.append((row['rebuilt_va'] - guest & 0xffffffff, row['va'] - guest & 0xffffffff, name, -guest))
strings = {'CopyByteSwapped': b'Error 0 in ua_memcpy\x00', 'ReadPhysicalWord': b'Trying to Read32 from unhandled or invalid memory\x00', 'ReadPhysicalHalf': b'Trying to Read16 from unhandled or invalid memory\x00', 'ReadPhysicalByte': b'Trying to Read8 from unhandled or invalid memory\x00', 'WritePhysicalByte': b'Trying to Write8 to unhandled or invalid memory\x00', 'WritePhysicalHalf': b'Trying to Write16 to unhandled or invalid memory\x00'}
newfix = base_relocations(rebuilt)
assert newfix, 'Verification requires a genuine relocation-bearing fixture.'
decoder = Cs(CS_ARCH_X86, CS_MODE_32)
decoder.detail = True
reports = []
# Compare full spans, including alignment, embedded jump tables and SEH fragments.
for name, length in lengths.items():
    f = spans[name]
    oa = f['va']
    ca = va(name)
    size = f['size']
    before = blob(original, oa, size)
    after = blob(rebuilt, ca, size)
    next_va = min((r['va'] for r in maps if r['kind'] == 'function' and r['va'] > ca))
    assert next_va - ca == size, (name, size, next_va - ca)
    fields = {off - ca: ('absolute', u32(rebuilt, off), u32(original, oa + off - ca)) for off in newfix if ca <= off < ca + size}
    for ins in decoder.disasm(after[:length], ca):
        if (ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP)) and ins.operands and (ins.operands[0].type == X86_OP_IMM):
            dest = ins.operands[0].imm & 0xffffffff
            if not ca <= dest < ca + size:
                assert ins.imm_size == 4
                off = ins.address - ca + ins.imm_offset
                old = u32(original, oa + off) + oa + off + 4 & 0xffffffff
                assert off not in fields
                fields[off] = ('relative', dest, old)
    checks = []
    occupied = set()
    for off, (kind, actual, observed) in sorted(fields.items()):
        candidates = []
        if ca <= actual < ca + size:
            candidates.append((oa + actual - ca, 'same_function_label_and_table_entry', name, actual - ca))
        if actual in functions:
            label, expected = functions[actual]
            candidates.append((expected, 'independent_function_identity', label, 0))
        if actual in iats:
            label, expected = iats[actual]
            candidates.append((expected, 'import_table_identity', label, 0))
        for d in data:
            if d['rebuilt_va'] <= actual < d['rebuilt_va'] + d['size']:
                addend = actual - d['rebuilt_va']
                candidates.append((d['va'] + addend, 'declared_object_and_field_offset', d['name'], addend))
        for value, expected, label, addend in affine:
            if value == actual:
                candidates.append((expected, 'source_guest_base_subtraction', label, addend))
        if name in strings:
            lit = strings[name]
            try:
                if blob(rebuilt, actual, len(lit)) == lit and blob(original, observed, len(lit)) == lit:
                    candidates.append((observed, 'exact_source_literal', lit[:-1].decode(), 0))
            except (ValueError, KeyError):
                pass
        if name == 'TLBWrite':
            for lit in [b'TLB Error 0 in TLBWI\x00', b'Missing feature: ASID used by ROM\x00']:
                try:
                    if blob(rebuilt, actual, len(lit)) == lit and blob(original, observed, len(lit)) == lit:
                        candidates.append((observed, 'exact_source_literal', lit[:-1].decode(), 0))
                except (ValueError, KeyError):
                    pass
        if name in ('WritePhysicalByte', 'WritePhysicalHalf') and off == 6:
            old = struct.unpack('<III', blob(original, observed, 12))
            new = struct.unpack('<III', blob(rebuilt, actual, 12))
            assert old[0] == new[0] == 0xffffffff
            assert old[1] - oa == new[1] - ca == 58 and old[2] - oa == new[2] - ca == 64
            candidates.append((observed, 'validated_SEH_filter_and_handler_offsets', 'single RAM-store exception scope', 0))
        if name == 'WritePhysicalWord':
            start = source.index('void __stdcall WritePhysicalWord(')
            end = source.index('void __stdcall WritePhysicalDoubleWord(', start)
            literals = [(x.encode() + b'\x00', x) for x in re.findall('"([^"\\n]*)"', source[start:end])]
            literals += [(struct.pack('<f', value), 'source float ' + str(value)) for value in (12.0, 0.001)]
            literals += [(struct.pack('<f', 4294967296.0), 'compiler unsigned-to-float conversion correction')]
            for lit, label in literals:
                try:
                    if blob(rebuilt, actual, len(lit)) == lit and blob(original, observed, len(lit)) == lit:
                        candidates.append((observed, 'exact_source_literal', label, 0))
                except (ValueError, KeyError):
                    pass
            if off == 6:
                old = struct.unpack('<6I', blob(original, observed, 24))
                new = struct.unpack('<6I', blob(rebuilt, actual, 24))
                assert old[0] == new[0] == old[3] == new[3] == 0xffffffff
                assert [old[k] - oa for k in (1, 2, 4, 5)] == [new[k] - ca for k in (1, 2, 4, 5)] == [77, 83, 1882, 1888]
                candidates.append((observed, 'validated_SEH_filter_and_handler_offsets', 'RAM-store and graphics-plugin exception scopes', 0))
        assert candidates, (name, hex(off), hex(actual), hex(observed))
        assert len({x[0] for x in candidates}) == 1, ('Ambiguous independent identity', name, hex(off), candidates)
        chosen = candidates[0]
        checks.append(dict(offset=off, kind=kind, candidate_target=actual, original_target=observed, independent_expected_target=chosen[0], basis=chosen[1], symbol_or_literal=chosen[2], addend=chosen[3], equal=chosen[0] == observed))
        occupied.update(range(off, off + 4))
    differences = [i for i, (a, b) in enumerate(zip(before, after)) if a != b and i not in occupied]
    targets_equal = all((check['equal'] for check in checks))
    reports.append(dict(name=name, original_va=oa, rebuilt_va=ca, instruction_body_bytes=length, complete_link_span_bytes=size, all_nonrelocation_bytes_equal=not differences, non_address_differences=differences, all_address_fields_independently_identified=True, all_address_targets_equal=targets_equal, function_match=not differences and targets_equal, raw_bytes_equal=before == after, validated_address_fields=checks))
matched = [report for report in reports if report['function_match']]
unmatched = [report['name'] for report in reports if not report['function_match']]
report = dict(scope='Complete linked function spans; individually identified relocated operands. This does not certify whole-image equality.', original_sha256=sha(original.data), rebuilt_sha256=sha(rebuilt.data), map_sha256=sha((fixture / 'nemu64.map').read_bytes()), fixture=str(fixture), source_sha256={name: sha((source_dir / name).read_bytes()) for name in ('Memory.cpp', 'Memory.h', 'ByteSwap.c') if (source_dir / name).is_file()}, functions=reports, verified_function_count=len(matched), verified_link_span_bytes=sum((row['complete_link_span_bytes'] for row in matched)), verified_address_fields=sum((len(row['validated_address_fields']) for row in matched)), unmatched_functions=unmatched, unexpected_unmatched_functions=sorted(set(unmatched) - set(args.allow_unmatched)))
if args.output:
    args.output.write_text(json.dumps(report, indent=2) + '\n')
print('Matched', len(matched), 'of', len(reports), 'functions;', report['verified_link_span_bytes'], 'complete-span bytes;', report['verified_address_fields'], 'address fields.')
for row in reports:
    if not row['function_match']:
        wrong = [check for check in row['validated_address_fields'] if not check['equal']]
        print(row['name'] + ':', len(row['non_address_differences']), 'non-address bytes,', len(wrong), 'address identity mismatches.')
        for check in wrong:
            print(' ', hex(check['offset']), check['symbol_or_literal'], 'expected original target', hex(check['original_target']))
sys.exit(bool(report['unexpected_unmatched_functions']))
