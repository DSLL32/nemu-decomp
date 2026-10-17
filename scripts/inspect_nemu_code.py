#!/usr/bin/env python3
"""Inspect the fixed-address Nemu64 executable without inventing symbols.

The JSON distinguishes decoded call targets, pointer tables and string
references. A call target is an entry candidate, not proof of the following
source function's length. Linear decoding can cross embedded tables; the report
retains that limitation instead of silently treating each decoded byte as code.

Examples:
  python3 scripts/inspect_nemu_code.py --report analysis/nemu-code-map.json
  python3 scripts/inspect_nemu_code.py --disassemble 0x420420 0x420600
  python3 scripts/inspect_nemu_code.py --string 'DRCPUM_DYNREC'
"""

import argparse
from bisect import bisect_right
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP, CS_GRP_RET
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

from inspect_gs_symbols import imported_symbols
from pe_fingerprints import PE32, u32

ROOT = Path(__file__).resolve().parents[1]

# The table divisions follow independently decoded index masks and dispatch
# operands. All three primary tables are called as procedures. Subdispatchers
# tail-jump to entries with their own returns/tail calls, including dynrec. An
# automatic disassembler can misleadingly name these independent handlers as
# switch case labels; that name alone is not evidence of one enclosing function.
DISPATCH_TABLES = [
    ('dynrec', 0x477820, 64, 'primary', 'OpcodeNames', [0x4088B7, 0x40D6A2]),
    ('dynrec', 0x477920, 64, 'special', 'SpecialNames', [0x40D6E6]),
    ('dynrec', 0x477A20, 32, 'regimm', 'RegImmNames', [0x40D726]),
    ('dynrec', 0x477AA0, 32, 'cop0', 'Cop0Names', [0x40DC38]),
    ('dynrec', 0x477B20, 64, 'tlb', 'TLBNames', [0x410FB7]),
    ('dynrec', 0x477C20, 32, 'cop1', 'Cop1Names', [0x40DC78]),
    ('dynrec', 0x477CA0, 64, 'cop1_function', 'Cop1FunctionNames', [0x41177B]),
    ('cpu_interpreter', 0x477DA0, 64, 'primary', 'OpcodeNames', [0x409ECC]),
    ('cpu_interpreter', 0x477EA0, 64, 'special', 'SpecialNames', [0x40A01A]),
    ('cpu_interpreter', 0x477FA0, 32, 'regimm', 'RegImmNames', [0x40A03A]),
    ('cpu_interpreter', 0x478020, 32, 'cop0', 'Cop0Names', [0x40A3DB]),
    ('cpu_interpreter', 0x4780A0, 64, 'tlb', 'TLBNames', [0x40BEFA]),
    ('cpu_interpreter', 0x4781A0, 32, 'cop1', 'Cop1Names', [0x40A3FB]),
    ('cpu_interpreter', 0x478220, 64, 'cop1_function', 'Cop1FunctionNames', [0x40C18A]),
    ('rsp', 0x479368, 64, 'primary', 'OpcodeNames', [0x40D2A0, 0x416369]),
    ('rsp', 0x479468, 64, 'special', 'SpecialNames', [0x4026EA]),
    ('rsp', 0x479568, 32, 'regimm', 'RegImmNames', [0x40270A]),
    ('rsp', 0x4795E8, 32, 'cop0', 'Cop0Names', [0x402A4B]),
    ('rsp', 0x479668, 32, 'cop2', None, [0x402A6B]),
    ('rsp', 0x4796E8, 32, 'vector_load', None, [0x402E6C]),
    ('rsp', 0x479768, 32, 'vector_store', None, [0x402EAC]),
    ('rsp', 0x4797E8, 64, 'vector_function', None, [0x403ACA]),
]

# These addresses appear as immediates passed to the generated-call emitter,
# not as native CALL operands. Independent prologues/returns were inspected;
# preserving the audited list avoids promoting arbitrary local-label constants
# (for example CRT continuation 42DB48) to function entries.
GENERATED_CALL_HELPERS = {
    0x40D550, 0x40D5D0, 0x40D5E0, 0x40EAE0, 0x40EC10, 0x40EEF0,
    0x40EF30, 0x4106A0, 0x410B50, 0x410BC0, 0x410BE0, 0x410FD0,
    0x414BA0, 0x414CF0, 0x414D50, 0x414FA0,
}


def dispatch_tables(pe):
    source = (ROOT / 'src/debugger/UnitOpcode.pas').read_text()
    decoder = (ROOT / 'src/debugger/UnitDisassembly.pas').read_text()
    vector_names = {int(index): name for index, name in re.findall(
        r"^\s+(\d+): Result := '(V[A-Z]+)\s", decoder, re.M)}
    names = {name: re.findall(r"'([^']*)'", body) for name, body in re.findall(
        r'(\w+): array\[[^]]+\] of string = \((.*?)\);', source, re.S)}
    result = []
    for family, va, count, kind, names_key, refs in DISPATCH_TABLES:
        mnemonics = list(names.get(names_key, []))
        mnemonics += ['reserved'] * (count - len(mnemonics))
        if family == 'rsp' and kind == 'primary':
            mnemonics[18] = 'COP2'
            mnemonics[50] = 'LWC2_VECTOR'
            mnemonics[58] = 'SWC2_VECTOR'
        if family == 'rsp' and kind == 'cop2':
            mnemonics = ['reserved'] * 32
            for index, name in {0: 'MFC2', 2: 'CFC2', 4: 'MTC2', 6: 'CTC2'}.items():
                mnemonics[index] = name
            mnemonics[16:] = ['VECTOR'] * 16
        if family == 'rsp' and kind in ('vector_load', 'vector_store'):
            stem = 'L' if kind == 'vector_load' else 'S'
            mnemonics = [stem + name + 'V' for name in
                         ['B', 'S', 'L', 'D', 'Q', 'R', 'P', 'U', 'H', 'F', 'W', 'T']]
            mnemonics += ['reserved'] * 20
        if family == 'rsp' and kind == 'vector_function':
            mnemonics = [vector_names.get(i, 'reserved') for i in range(64)]
        values = struct.unpack_from('<' + 'I' * count, pe.data, pe.offset(va - pe.base))
        result.append(dict(family=family, kind=kind, va=va, count=count,
                           dispatch_instruction_addresses=refs,
                           target_kind='handler_entry_candidate',
                           entries=[dict(index=i, mnemonic=mnemonics[i], target=value)
                                    for i, value in enumerate(values)]))
    return result


def semantic_regions():
    """Inspection bands, deliberately distinct from original object extents."""
    rows = [
        (0x401000, 0x406A60, 'RSP memory checks, scalar/vector interpreter and reciprocal ROM',
         [0x479368, 0x4797E8, 0x43E00C, 0x43DE14],
         'Dispatch tables and handler error strings. 406900 initializes the reciprocal lookup table; dynrec helpers start at406A60.'),
        (0x406A60, 0x409E00, 'Dynarec tracking, intermediate representation and x86 emission helpers',
         [0x43DDD0, 0x43DD54, 0x4088B7],
         'AddJumpTarget/CheckInlineFunction diagnostics and writes of x86 instruction bytes to generated-code buffers.'),
        (0x409E00, 0x40C9A0, 'CPU interpreter, including COP0/COP1',
         [0x477DA0, 0x478220, 0x43DD28],
         'Primary opcode table called at409ECC; subsidiary tables tail-dispatch to independent handlers.'),
        (0x40C9A0, 0x40D590, 'Execution loop and exception/event scheduling',
         [0x40D2A0, 0x40D4A8, 0x43DB28, 0x43DC64],
         'Calls CPU/RSP dispatch tables and references interrupt/exception diagnostics.'),
        (0x40D590, 0x4133B0, 'CPU dynarec opcode handlers',
         [0x477820, 0x477CA0, 0x43DAA0],
         'Primary table is called at4088B7 and4165B4. Targets have independent epilogues/tail calls; caseD names from automatic analysis are misleading.'),
        (0x4133B0, 0x414420, 'Dynarec register management and block compilation helpers',
         [0x43D980, 0x413F19],
         'GetCOP1Reg diagnostic and indirect dynarec handler invocation. Band boundary remains provisional.'),
        (0x414420, 0x416A00, 'Host exception recovery, diagnostics and emulation control',
         [0x43D8E0, 0x43D948, 0x434854, 0x434800],
         'Host access-violation recovery and emulation-status/exception strings.'),
        (0x416A00, 0x4196A0, 'Memory access, MMIO, DMA and TLB translation',
         [0x434764, 0x43474C, 0x4346F4, 0x4344E4],
         'Width-specific read/write error strings and SP/PI DMA paths.'),
        (0x4196A0, 0x419BD0, 'Plugin loading and binding',
         [0x4342F0, 0x4344D8, 0x434240],
         'LoadLibrary/GetProcAddress binding of ToolInit, GetDllInfo, AI/CI/VI exports.'),
        (0x419BD0, 0x41B2C0, 'Savestates, favorite folders and frontend helpers',
         [0x434210, 0x4341D8, 0x4341B0],
         'Save/load-state diagnostics and fav.dat handling.'),
        (0x41B2C0, 0x41FF00, 'Windows frontend, settings and ROM loading',
         [0x43412C, 0x434098, 0x433120, 0x432C18],
         'Windows controls/settings and ROM loading/byteswapping diagnostics.'),
        (0x41FF00, 0x420340, 'Guest OS symbol/signature search',
         [0x431F80, 0x431FBC, 0x432AC4],
         'Search status and embedded libultra function-name data. This is evidence of guest symbol recognition, not native implementations of those guest functions.'),
        (0x420340, 0x420DF0, 'PIF/CIC challenge and controller/accessory commands',
         [0x431F5C, 0x431EA4, 0x431F04],
         'CIC dump path, resource187 lookup consumer420420 and PIF command errors.'),
        (0x420DF0, 0x421210, 'Compressed-file wrappers and movie file handling',
         [0x434204, 0x420F10, 0x421000],
         'Application wrappers end before independently matched zlib/minizip begins at421210.'),
        (0x421210, 0x427BF0, 'zlib 1.1.3 and minizip 0.15',
         [0x421210, 0x430288],
         '72 complete upstream function blocks cover this band without gaps; see nemu-zlib-addresses.json.'),
        (0x427BF0, 0x42F80D, 'Microsoft C runtime and import thunks',
         [0x427C02, 0x428932],
         'Archive body and callee matching identifies CRT functions; see nemu-library-addresses.json. The band label alone does not establish every byte.'),
    ]
    return [dict(start=a, end_exclusive=b, description=label, evidence_addresses=evidence,
                 confidence='semantic_band_not_original_object_or_function_extent', notes=notes)
            for a, b, label, evidence, notes in rows]


def inspect(path):
    pe = PE32(path)
    section = next(s for s in pe.sections if s['name'] == '.text')
    lo = pe.base + section['rva']
    hi = lo + section['virtual_size']
    cs = Cs(CS_ARCH_X86, CS_MODE_32)
    cs.detail = True
    cs.skipdata = True
    instructions = list(cs.disasm(
        pe.data[section['offset']:section['offset'] + section['virtual_size']], lo))
    decoded = {i.address: i for i in instructions if i.id}
    # Recover compiler switch data first, then restart decoding after it. This
    # avoids spurious instructions and lost alignment inside DWORD jump tables.
    raw_tables = []
    start, values = None, []
    text_data = pe.data[section['offset']:section['offset'] + section['virtual_size']]
    for off in range(0, len(text_data) - 3, 4):
        value = u32(text_data, off)
        if lo <= value < hi:
            start = off if start is None else start
            values.append(value)
        else:
            if len(values) >= 3:
                raw_tables.append((lo + start, lo + start + len(values) * 4))
            start, values = None, []
    excluded = []
    for start, end in raw_tables:
        refs = [i.address for i in decoded.values() if i.group(CS_GRP_JUMP)
                and i.operands and i.operands[0].type == X86_OP_MEM
                and start <= i.operands[0].mem.disp < end]
        if refs:
            excluded.append(dict(start=start, end=end, dispatch_addresses=refs))
    # This compiler-generated five-entry switch table is not DWORD-aligned.
    # Its bound is CMP [EBP-1A4],4 at41608C, followed by JA/default and the
    # indexed jump at41609F. The aligned pointer-run scan cannot find it.
    excluded.append(dict(start=0x4169C3, end=0x4169D7,
                         dispatch_addresses=[0x41609F], bound_instruction=0x41608C))
    excluded.sort(key=lambda r: r['start'])
    decoded = {}
    cursor = lo
    for table in excluded + [dict(start=hi, end=hi)]:
        offset = section['offset'] + cursor - lo
        for ins in cs.disasm(pe.data[offset:offset + table['start'] - cursor], cursor):
            if ins.id:
                decoded[ins.address] = ins
        cursor = table['end']
    # MSVC sometimes compresses switch entries through a byte-index lookup.
    # Recover only the bounded form whose register comparison, unsigned guard
    # and following pointer-table jump are all decoded independently.
    ordered = sorted(decoded.values(), key=lambda i: i.address)
    byte_tables = []
    for index, ins in enumerate(ordered):
        if ins.mnemonic != 'movzx' or len(ins.operands) != 2:
            continue
        dest, source = ins.operands
        if (dest.type != X86_OP_REG or source.type != X86_OP_MEM or source.size != 1
                or not lo <= source.mem.disp < hi or source.mem.scale != 1):
            continue
        reg = source.mem.base or source.mem.index
        before = [i for i in ordered[max(0, index-8):index] if i.address >= ins.address-32]
        guard = next((i for i in reversed(before) if i.mnemonic == 'cmp'
                      and len(i.operands) == 2 and i.operands[0].type == X86_OP_REG
                      and i.operands[0].reg == reg and i.operands[1].type == X86_OP_IMM
                      and 0 <= i.operands[1].imm < 256), None)
        unsigned_check = guard and any(i.mnemonic == 'ja' and i.address > guard.address for i in before)
        following = ordered[index+1:index+4]
        jump = next((i for i in following if i.mnemonic == 'jmp' and i.operands[0].type == X86_OP_MEM
                     and i.operands[0].mem.index == dest.reg and i.operands[0].mem.scale == 4
                     and any(t['start'] <= i.operands[0].mem.disp < t['end'] for t in excluded)), None)
        if unsigned_check and jump:
            byte_tables.append(dict(start=source.mem.disp,
                end=source.mem.disp + guard.operands[1].imm + 1,
                bound_instruction=guard.address, lookup_instruction=ins.address,
                dispatch_instruction=jump.address))
    imports = imported_symbols(pe)
    iat = {r['iat_va']: r for r in imports}
    strings = []
    for sec in pe.sections:
        if sec['name'] not in ('.rdata', '.data'):
            continue
        data = pe.data[sec['offset']:sec['offset'] + sec['raw_size']]
        for match in re.finditer(rb'[\x20-\x7e]{4,}\x00', data):
            strings.append(dict(va=pe.base + sec['rva'] + match.start(),
                                text=match.group()[:-1].decode('ascii'),
                                section=sec['name'], references=[]))
    strings.sort(key=lambda r: r['va'])
    string_starts = [r['va'] for r in strings]
    calls, branches, indirect_branches, code_addresses = [], [], [], []
    global_references = defaultdict(list)
    entry = pe.base + u32(pe.data, pe.opt + 16)
    candidates = defaultdict(list)
    candidates[entry].append(dict(kind='PE_entry_point'))
    dispatch = dispatch_tables(pe)
    for table in dispatch:
        for row in table['entries']:
            candidates[row['target']].append(dict(kind='indirect_dispatch_table',
                table=table['va'], index=row['index'], family=table['family'],
                opcode_group=table['kind'], mnemonic=row['mnemonic']))
    import_references = defaultdict(list)
    for ins in decoded.values():
        if ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP):
            op = ins.operands[0] if ins.operands else None
            if op and op.type == X86_OP_IMM:
                row = dict(at=ins.address, target=op.imm, instruction=ins.mnemonic)
                if ins.group(CS_GRP_CALL):
                    calls.append(row)
                    if lo <= op.imm < hi:
                        candidates[op.imm].append(dict(kind='decoded_direct_call', at=ins.address))
                else:
                    branches.append(row)
            elif op:
                row = dict(at=ins.address, instruction=ins.mnemonic, operand=ins.op_str)
                if op.type == X86_OP_MEM:
                    row.update(displacement=op.mem.disp, scale=op.mem.scale,
                               base=ins.reg_name(op.mem.base), index=ins.reg_name(op.mem.index))
                    if not op.mem.base and not op.mem.index and op.mem.disp in iat:
                        row['import'] = iat[op.mem.disp]['name']
                indirect_branches.append(row)
        for op in ins.operands:
            value = None
            if op.type == X86_OP_IMM:
                value = op.imm & 0xffffffff
            elif op.type == X86_OP_MEM and not op.mem.base:
                value = op.mem.disp & 0xffffffff
            if value is None:
                continue
            if (op.type == X86_OP_IMM and lo <= value < hi
                    and not ins.group(CS_GRP_CALL) and not ins.group(CS_GRP_JUMP)):
                code_addresses.append(dict(at=ins.address, target=value,
                    instruction=ins.mnemonic, is_decoded_instruction_start=value in decoded))
                if value in GENERATED_CALL_HELPERS:
                    candidates[value].append(dict(kind='audited_generated_call_helper_address',
                        at=ins.address, basis='Code address passed to dynarec call emitter; independent return inspected.'))
            if op.type == X86_OP_MEM:
                for sec in pe.sections:
                    start = pe.base + sec['rva']
                    if sec['name'] in ('.data', '.rdata') and start <= value < start + sec['virtual_size']:
                        global_references[value].append(dict(at=ins.address, instruction=ins.mnemonic,
                            access_size=op.size, index=ins.reg_name(op.mem.index), scale=op.mem.scale))
                        break
            if value in iat:
                import_references[value].append(ins.address)
            index = bisect_right(string_starts, value) - 1
            if index >= 0:
                row = strings[index]
                if value < row['va'] + len(row['text']):
                    row['references'].append(dict(at=ins.address, offset=value - row['va'],
                                                  instruction=ins.mnemonic))
    # Contiguous code-pointer arrays are table candidates. They can be switch
    # labels, callbacks or dispatch entries; they are not all function pointers.
    tables = []
    for sec in pe.sections:
        if sec['name'] not in ('.text', '.rdata', '.data'):
            continue
        data = pe.data[sec['offset']:sec['offset'] + sec['raw_size']]
        start = None
        values = []
        for off in range(0, len(data) - 3, 4):
            value = u32(data, off)
            if lo <= value < hi:
                if start is None:
                    start = off
                values.append(value)
            else:
                if len(values) >= 3:
                    tables.append(dict(va=pe.base + sec['rva'] + start,
                                       section=sec['name'], count=len(values),
                                       targets=values, references=[]))
                start, values = None, []
        if len(values) >= 3:
            tables.append(dict(va=pe.base + sec['rva'] + start, section=sec['name'],
                               count=len(values), targets=values, references=[]))
    for ins in decoded.values():
        for op in ins.operands:
            value = op.imm if op.type == X86_OP_IMM else (
                op.mem.disp if op.type == X86_OP_MEM and not op.mem.base else None)
            if value is None:
                continue
            for table in tables:
                if table['va'] <= value < table['va'] + table['count'] * 4:
                    table['references'].append(dict(at=ins.address, offset=value - table['va'],
                                                    instruction=ins.mnemonic))
    entries = sorted(candidates)
    for string in strings:
        for ref in string['references']:
            index = bisect_right(entries, ref['at']) - 1
            if index >= 0:
                ref['preceding_entry_candidate'] = entries[index]
    # Follow direct control flow from each call target. Indirect jumps terminate
    # this conservative graph. A following candidate ends the traversal: no
    # source function size is inferred from adjacency alone.
    functions = []
    for va in entries:
        todo, seen, exits, targets, unresolved = [va], set(), [], [], []
        while todo:
            addr = todo.pop()
            while addr not in seen:
                if addr != va and addr in candidates:
                    exits.append(dict(at=addr, kind='other_entry_candidate'))
                    break
                ins = decoded.get(addr)
                if ins is None:
                    unresolved.append(addr)
                    break
                seen.add(addr)
                next_addr = addr + ins.size
                if ins.group(CS_GRP_RET):
                    exits.append(dict(at=addr, kind='return'))
                    break
                if ins.group(CS_GRP_CALL) and ins.operands[0].type == X86_OP_IMM:
                    targets.append(ins.operands[0].imm)
                if ins.group(CS_GRP_JUMP):
                    op = ins.operands[0]
                    if op.type != X86_OP_IMM:
                        exits.append(dict(at=addr, kind='indirect_jump'))
                        break
                    if lo <= op.imm < hi:
                        todo.append(op.imm)
                    else:
                        exits.append(dict(at=addr, kind='jump_outside_text', target=op.imm))
                    if ins.mnemonic == 'jmp':
                        break
                if ins.mnemonic in ('int3', 'ud2', 'hlt'):
                    exits.append(dict(at=addr, kind='trap_or_padding'))
                    break
                addr = next_addr
        ranges = []
        for address in sorted(seen):
            end = address + decoded[address].size
            if ranges and ranges[-1]['end_exclusive'] == address:
                ranges[-1]['end_exclusive'] = end
            else:
                ranges.append(dict(start=address, end_exclusive=end))
        functions.append(dict(va=va, evidence=candidates[va],
                              decoded_instruction_count=len(seen),
                              reached_byte_count=sum(decoded[a].size for a in seen),
                              reached_instruction_ranges=ranges,
                              reached_last_byte=max((a + decoded[a].size for a in seen), default=va),
                              direct_call_targets=sorted(set(targets)), exits=exits,
                              unresolved_instruction_addresses=sorted(set(unresolved))))
    report = dict(
        input=str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),
        sha256=hashlib.sha256(pe.data).hexdigest(), image_base=pe.base,
        entry_point=entry, text_start=lo, text_end=hi, sections=pe.sections,
        limitations=[
            'Names and source function lengths are not recovered from debug symbols; this image has no debug directory.',
            'Entry candidates are decoded direct-call destinations, known opcode dispatch targets, audited generated-call helpers and the PE entry. Linear decoding can cross embedded data.',
            'Reached bytes describe a conservative direct-control-flow walk, not a complete function extent. Indirect jumps stop it.',
            'preceding_entry_candidate is an address-location hint, not a claim that the string reference belongs to that function.',
            'Code-pointer arrays may contain switch labels, callbacks or dispatch handlers. Their length is the contiguous aligned run, not a recovered C array bound.',
            'Recovered semantic labels below are evidence-based hypotheses until compiler-verified source establishes identity.',
        ],
        rich_header=pe.rich(),
        source_path_evidence=dict(
            original_source_paths_recovered=False,
            source_like_strings=[r for r in strings if re.search(r'\.(?:cpp|c|h|asm|pdb)$', r['text'])],
            absolute_data_file_paths=[r for r in strings if re.match(r'^[a-zA-Z]:\\', r['text'])],
            note='d:\\n64\\lastin.cic is a runtime data/dump path, not an original C/C++ source filename.'),
        semantic_regions=semantic_regions(),
        imports=[dict(r, references=import_references[r['iat_va']]) for r in imports],
        entry_candidates=functions, direct_calls=calls, direct_branches=branches,
        indirect_control_transfers=indirect_branches, code_pointer_tables=tables,
        decoded_switch_data_ranges=excluded, decoded_switch_index_data_ranges=byte_tables,
        opcode_dispatch_tables=dispatch,
        code_address_immediates=code_addresses,
        global_memory_references=[dict(va=va, references=refs) for va, refs in sorted(global_references.items())],
        strings=strings,
    )
    return pe, decoded, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image', type=Path, default=ROOT / 'master/nemu64.exe')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--disassemble', nargs=2, type=lambda v: int(v, 0), metavar=('START', 'END'))
    parser.add_argument('--string', help='case-insensitive substring')
    args = parser.parse_args()
    pe, instructions, report = inspect(args.image.resolve())
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print(f"{len(report['entry_candidates'])} entry candidates; "
              f"{len(report['code_pointer_tables'])} pointer-table candidates; {args.report}")
    if args.string:
        print(json.dumps([s for s in report['strings'] if args.string.lower() in s['text'].lower()], indent=2))
    if args.disassemble:
        start, end = args.disassemble
        cs = Cs(CS_ARCH_X86, CS_MODE_32)
        cs.detail = True
        cs.skipdata = True
        imports = {r['iat_va']: r['name'] for r in report['imports']}
        strings = {r['va']: r['text'] for r in report['strings']}
        data = pe.data[pe.offset(start - pe.base):pe.offset(start - pe.base) + end - start]
        for ins in cs.disasm(data, start):
            notes = []
            for op in ins.operands if ins.id else []:
                value = op.imm if op.type == X86_OP_IMM else (
                    op.mem.disp if op.type == X86_OP_MEM and not op.mem.base else None)
                if value in imports:
                    notes.append(imports[value])
                if value in strings:
                    notes.append(repr(strings[value]))
            print(f'{ins.address:08X} {ins.bytes.hex():<22} {ins.mnemonic:7} {ins.op_str}'
                  + (' ; ' + ', '.join(notes) if notes else ''))


if __name__ == '__main__':
    main()
