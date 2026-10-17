#!/usr/bin/env python3
"""Reconcile Nemu64 entry evidence, Ghidra exports and reconstruction ownership.

The coverage here is inspection coverage, not reconstructed-source or matching
coverage. Ownership bands assign work; they do not recover original object or
function extents. Ghidra's body size is retained as reported and never converted
to a contiguous function span.

Usage:
  python3 scripts/map_nemu_coverage.py --ghidra-functions PATH/functions.tsv
"""

import argparse
from collections import Counter
import csv
import hashlib
import io
import json
from pathlib import Path
import re

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM, X86_OP_REG, X86_OP_IMM
from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]

# Explicit assignments confirmed by the module owners. These are work bands,
# not claims about source file order. Individual frontend assignments override
# the broad ranges because frontend contributions interleave in the executable.
BANDS = [
    (0x401000, 0x406A60, 'rsp', 'RSP.cpp'),
    (0x406A60, 0x409E00, 'dynarec', 'Dynarec.cpp'),
    (0x409E00, 0x40D590, 'cpu', 'CPU.cpp / CpuControl.cpp'),
    (0x40D590, 0x414420, 'dynarec', 'Dynarec.cpp'),
    (0x414420, 0x416A00, 'cpu', 'CpuControl.cpp'),
    (0x416A00, 0x4196A0, 'memory', 'Memory.cpp'),
    (0x4196A0, 0x419BD0, 'plugins', 'Plugins.cpp'),
    (0x41E4C0, 0x41FEF0, 'frontend', 'FrontendIni.cpp / Rom.cpp / RomBrowser.cpp'),
    (0x41FF00, 0x420340, 'dynarec', 'RuntimeSignatures.cpp'),
    (0x420340, 0x421210, 'pif_movie', 'PIF.cpp / Movie.cpp'),
    (0x421210, 0x427BF0, 'zlib_minizip', 'vendor/zlib-1.1.3'),
    (0x427BF0, 0x42F80D, 'crt', 'toolchains/vc7/lib/libc.lib and import libraries'),
]

EXACT_OWNERS = {
    'frontend': [0x419BD0, 0x419CF0, 0x41A030, 0x41A0E0, 0x41A620,
                 0x41A6A0, 0x41A6C0, 0x41A6F0, 0x41A810, 0x41AA20,
                 0x41AC50, 0x41E090, 0x41E380, 0x41E420],
    'plugins': [0x41A6E0, 0x41A710, 0x41ACF0, 0x41B100, 0x41B2C0,
                0x41B300, 0x41B5A0, 0x41B6C0, 0x41B780],
    'main_window': [0x41B130, 0x41B340, 0x41B930, 0x41B9B0, 0x41BA40,
                    0x41BFC0, 0x41C130, 0x41D0B0, 0x41D110],
    'shared_empty_function': [0x41FEF0],
}

OWNERS = {
    'rsp': dict(agent='/root/gs_interpreter', source='src/nemu64/RSP.cpp'),
    'dynarec': dict(agent='/root/memory', source='src/nemu64/Dynarec.cpp'),
    'cpu': dict(agent='/root/commands', source='src/nemu64/CPU.cpp / CpuControl.cpp'),
    'memory': dict(agent='/root/components', source='src/nemu64/Memory.cpp'),
    'plugins': dict(agent='/root/host', source='src/nemu64/Plugins.cpp'),
    'frontend': dict(agent='/root/gs_small_dialogs', source='src/nemu64 frontend modules'),
    'main_window': dict(agent='/root/gs_main_dialog', source='src/nemu64/WinMain.cpp'),
    'pif_movie': dict(agent='/root', source='src/nemu64 PIF/movie modules'),
    'zlib_minizip': dict(agent='/root/gs_storage', source='src/nemu64/vendor/zlib-1.1.3'),
    'crt': dict(agent='/root/gs_symbols', source='toolchains/vc7/lib'),
    'shared_empty_function': dict(agent='multiple', source='Identical COMDAT folding'),
}

# Disassembly gaps that are reachable through compiler data rather than direct
# control flow. These are fragments of existing source functions, not proposed
# extra function boundaries. The evidence pointers are validated below.
REVIEWED_FRAGMENTS = [
    dict(start=0x414688, end_exclusive=0x41474E, parent_entry=0x414490,
         kind='host_exception_switch_cases', pointers={0x414A8C: 0x414688, 0x414A90: 0x4146B4}),
    dict(start=0x415A16, end_exclusive=0x415A2D, parent_entry=0x415840,
         kind='seh_filter_and_handler_fragment', pointers={0x43E094: 0x415A16, 0x43E098: 0x415A23}),
    dict(start=0x41695A, end_exclusive=0x416974, parent_entry=0x415840,
         kind='seh_filter_and_handler_fragment', pointers={0x43E0A0: 0x41695A, 0x43E0A4: 0x416960}),
    dict(start=0x417A4A, end_exclusive=0x417A62, parent_entry=0x417A10,
         kind='seh_filter_and_handler_fragment', pointers={0x43E07C: 0x417A4A, 0x43E080: 0x417A50}),
    dict(start=0x417B6A, end_exclusive=0x417B82, parent_entry=0x417B30,
         kind='seh_filter_and_handler_fragment', pointers={0x43E06C: 0x417B6A, 0x43E070: 0x417B70}),
    dict(start=0x417CBD, end_exclusive=0x417CE0, parent_entry=0x417C70,
         kind='seh_filter_and_handler_fragment', pointers={0x43E054: 0x417CBD, 0x43E058: 0x417CC3}),
    dict(start=0x4183CA, end_exclusive=0x4183E0, parent_entry=0x417C70,
         kind='seh_filter_and_handler_fragment', pointers={0x43E060: 0x4183CA, 0x43E064: 0x4183D0}),
]


def owner(va):
    for key, values in EXACT_OWNERS.items():
        if va in values:
            return key, 'explicit_function_assignment'
    for start, end, key, _ in BANDS:
        if start <= va < end:
            return key, 'confirmed_work_band'
    return None, None


def read_json(path):
    return json.loads(path.read_text())


def runs(values, predicate):
    start = None
    for i, value in enumerate(values):
        if predicate(value):
            start = i if start is None else start
        elif start is not None:
            yield start, i
            start = None
    if start is not None:
        yield start, len(values)


def is_alignment(data, address, cs):
    """Recognize compiler alignment instructions, including LEA self,NOP.

    This deliberately does not call zero-filled gaps alignment. Bytes such as
    zero are also common switch-index data in this executable.
    """
    end = address + len(data)
    decoded = list(cs.disasm(data, address))
    if sum(i.size for i in decoded) != len(data):
        return False
    for ins in decoded:
        if ins.mnemonic in ('nop', 'int3'):
            continue
        op = ins.operands
        if (ins.mnemonic == 'mov' and len(op) == 2 and
                op[0].type == op[1].type == X86_OP_REG and op[0].reg == op[1].reg):
            continue
        if (ins.mnemonic == 'lea' and len(op) == 2 and op[0].type == X86_OP_REG
                and op[1].type == X86_OP_MEM and op[0].reg == op[1].mem.base
                and op[1].mem.disp == 0 and not op[1].mem.index):
            continue
        if (ins.mnemonic == 'jmp' and op[0].type == X86_OP_IMM
                and ins.address < op[0].imm <= end):
            continue
        return False
    return True


def build_report(args):
    inventory = read_json(args.inventory)
    crt = read_json(ROOT / 'analysis/nemu-library-addresses.json')
    vendor = read_json(ROOT / 'analysis/nemu-zlib-addresses.json')
    pe = PE32(ROOT / inventory['input'])
    lo, hi = inventory['text_start'], inventory['text_end']
    text = pe.data[pe.offset(lo - pe.base):pe.offset(lo - pe.base) + hi - lo]
    sha = hashlib.sha256(pe.data).hexdigest()
    assert sha == inventory['sha256'] == crt['original_sha256'] == vendor['original_sha256']
    ghidra = {}
    export_bytes = args.ghidra_functions.read_bytes()
    with io.StringIO(export_bytes.decode()) as stream:
        for row in csv.DictReader(stream, delimiter='\t'):
            va = int(row['address'], 16)
            ghidra[va] = dict(name=row['name'], body_address_count=int(row['size']),
                              prototype=row['prototype'], decompiled=row['decompiled'] == 'true')
    entries = {r['va']: dict(va=r['va'], inventory=r) for r in inventory['entry_candidates']}
    for va, row in ghidra.items():
        entries.setdefault(va, dict(va=va))['ghidra'] = row
    for row in crt['symbols']:
        entries.setdefault(row['va'], dict(va=row['va'])).setdefault('library_symbols', []).append(
            dict(name=row['name'], size=row['size'], source=row['object'], kind='crt_archive_body'))
    for row in vendor['symbols'] + vendor['folded_helpers']:
        va = int(row['va'], 0)
        entries.setdefault(va, dict(va=va)).setdefault('library_symbols', []).append(
            dict(name=row['name'], size=row['compared_bytes'], source=row['source'],
                 kind='upstream_vendor_function_block'))
    rows = []
    for va, entry in sorted(entries.items()):
        key, basis = owner(va)
        entry.update(owner=key, ownership_basis=basis,
                     application_entry=va < 0x421210)
        rows.append(entry)

    # A bitset for independently identifiable byte categories. A Ghidra body is
    # read instruction-by-instruction; its reported size does not imply that
    # its address range is contiguous. File names absent from functions.tsv
    # are ignored, avoiding stale exports from earlier analysis passes.
    coverage = bytearray(hi - lo)
    categories = {'ghidra_instruction': 1, 'inventory_direct_cfg_instruction': 2,
                  'embedded_switch_pointer_data': 4, 'matched_vendor_function_block': 8,
                  'matched_crt_function_body': 16, 'bounded_switch_byte_index_data': 32,
                  'reviewed_internal_handler_instructions': 64, 'crt_runtime_metadata': 128}

    def mark(start, end, flag):
        for i in range(max(lo, start) - lo, min(hi, end) - lo):
            coverage[i] |= flag

    cs = Cs(CS_ARCH_X86, CS_MODE_32)
    cs.detail = True
    asm_issues = []
    for va in ghidra:
        path = args.ghidra_functions.parent / f'{va:08x}.asm'
        if not path.exists():
            asm_issues.append(dict(va=va, issue='missing_export_file'))
            continue
        for line in path.read_text().splitlines():
            match = re.match(r'^([0-9a-fA-F]{8})\s', line)
            if not match:
                continue
            address = int(match.group(1), 16)
            if not lo <= address < hi:
                continue
            ins = next(cs.disasm(text[address - lo:address - lo + 15], address, count=1), None)
            if ins is None:
                asm_issues.append(dict(va=va, at=address, issue='cannot_decode_instruction'))
            else:
                mark(address, address + ins.size, 1)
    for row in inventory['entry_candidates']:
        for span in row['reached_instruction_ranges']:
            mark(span['start'], span['end_exclusive'], 2)
    for span in inventory['decoded_switch_data_ranges']:
        mark(span['start'], span['end'], 4)
    for span in inventory.get('decoded_switch_index_data_ranges', []):
        mark(span['start'], span['end'], 32)
    for row in vendor['symbols'] + vendor['folded_helpers']:
        start = int(row['va'], 0)
        mark(start, start + row['compared_bytes'], 8)
    for row in crt['symbols']:
        mark(row['va'], row['va'] + row['size'], 16)
    for fragment in REVIEWED_FRAGMENTS:
        a, b = fragment['start'], fragment['end_exclusive']
        for pointer, target in fragment['pointers'].items():
            offset = pe.offset(pointer - pe.base)
            assert int.from_bytes(pe.data[offset:offset+4], 'little') == target
        assert sum(i.size for i in cs.disasm(text[a-lo:b-lo], a)) == b-a
        mark(a, b, 64)
    marker = 0x42D06C
    assert text[marker-lo:marker-lo+8] == b'VC20XC00'
    mark(marker, marker+8, 128)

    gaps = []
    for a, b in runs(coverage, lambda v: v == 0):
        data = text[a:b]
        padding = is_alignment(data, lo+a, cs)
        # Contiguous code evidence may end before padding then local byte-index
        # switch data. Mixed gaps are kept intact for review instead of guessing
        # a code/data boundary from values alone.
        gaps.append(dict(start=lo+a, end_exclusive=lo+b, size=b-a,
                         classification='alignment_padding_candidate' if padding else 'unclassified',
                         contains_nonpadding_bytes=not padding))
    app = [r for r in rows if r['application_entry']]
    report = dict(
        input=inventory['input'], sha256=sha,
        evidence_inputs=dict(inventory=str(args.inventory.relative_to(ROOT)),
                             ghidra_export_sha256=hashlib.sha256(export_bytes).hexdigest(),
                             ghidra_function_count=len(ghidra),
                             library_map='analysis/nemu-library-addresses.json',
                             vendor_map='analysis/nemu-zlib-addresses.json'),
        limitations=[
            'Inspection coverage is not reconstructed-source coverage or a claim of byte-exact rebuild.',
            'Work ownership bands and nearest entry addresses do not establish original source-file or function extents.',
            'Ghidra body_address_count is a possibly noncontiguous analysis body, not an original function length.',
            'Inventory entry candidates use direct calls and known dispatch tables; Ghidra adds callbacks and other analysis-derived entries.',
            'Matched library bodies retain the relocation and data-operand limitations described by their independent maps.',
            'Unclassified byte ranges may include switch index data, unreachable code or missed functions; recognized NOP/INT3/self-LEA gaps are padding candidates.',
        ],
        owners=OWNERS,
        work_bands=[dict(start=a, end_exclusive=b, owner=o, inferred_source=s)
                    for a,b,o,s in BANDS],
        summary=dict(text_bytes=hi-lo, reconciled_entries=len(rows),
                     application_entries=len(app),
                     application_entries_by_owner=dict(sorted(Counter(r['owner'] or 'unassigned' for r in app).items())),
                     unassigned_application_entries=[r['va'] for r in app if r['owner'] is None],
                     inventory_entries_without_ghidra=[r['va'] for r in rows if 'inventory' in r and 'ghidra' not in r],
                     ghidra_entries_without_inventory=[r['va'] for r in rows if 'ghidra' in r and 'inventory' not in r],
                     inspected_bytes=sum(bool(v) for v in coverage),
                     bytes_by_overlapping_evidence={k:sum(bool(v & flag) for v in coverage) for k,flag in categories.items()},
                     padding_candidate_bytes=sum(r['size'] for r in gaps if not r['contains_nonpadding_bytes']),
                     unclassified_bytes=sum(r['size'] for r in gaps if r['contains_nonpadding_bytes']),
                     unclassified_application_bytes=sum(r['size'] for r in gaps if r['contains_nonpadding_bytes'] and r['start'] < 0x421210)),
        unclassified_and_padding_ranges=gaps, ghidra_assembly_issues=asm_issues,
        reviewed_internal_fragments=[dict(r, owner=owner(r['parent_entry'])[0]) for r in REVIEWED_FRAGMENTS],
        library_code_aliases=crt.get('code_aliases', []),
        runtime_metadata=[dict(start=marker, end_exclusive=marker+8, text='VC20XC00',
                               basis='Microsoft exception runtime marker confirmed against CRT COFF member.')],
        entries=rows)
    if args.ghidra_functions.read_bytes() != export_bytes:
        raise RuntimeError('Ghidra export changed during inspection; rerun after export finishes.')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', type=Path, default=ROOT / 'analysis/nemu-code-map.json')
    parser.add_argument('--ghidra-functions', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/nemu-coverage.json')
    args = parser.parse_args()
    args.inventory = args.inventory.resolve()
    report = build_report(args)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report['summary'], indent=2))


if __name__ == '__main__':
    main()
