#!/usr/bin/env python3
"""Diagnose reconstructed Nemu64 functions; never certify the complete EXE.

Input metadata supplies names, original VAs and full sizes; incomplete rows
remain visible in the coverage matrix. COFF objects supply exact candidate
spans and relocation records. Linked /GL probes require a MAP; specify measured
rebuilt_size and rebuilt_image_sha256 per function to certify its extent,
otherwise only the original-sized candidate prefix is compared. Alternatively,
--also-compare-map-spans checks independent MAP blocks, including alignment.
For linked absolute addresses,
build with /FIXED:NO: only real PE HIGHLOW fixups are normalized by default.
Internal branch bytes remain exact. Independent CRT/IAT identities take
precedence over application observations, so masking cannot hide wrong callees.

Metadata example:
{"functions": [{"name": "Function", "va": "0x401000", "size": 30,
                "symbol": "?Function@@YAXXZ", "rebuilt_size": 30}],
 "globals": [{"name": "Memory", "va": "0x440000", "size": 16,
              "address_basis": "application_observation"}]}

Function/global dictionaries keyed by name and length instead of size are also
accepted. Repeated --metadata files add known application address constraints.
Unique, conflict-free objects in the hash-pinned map_nemu_globals inventory
constrain linked data addresses. A decoded memory access to the wrong mapped
object is a mismatch. Containing-range conflicts and pointer boundary aliases
remain unresolved; a MAP does not recover the source symbol and signed addend.
Reviewed aliases can use per-function linked_operand_provenance entries with
offset, symbol, addend, candidate_image_sha256, source_file, source_sha256, and
evidence. They apply only to that image and an existing PE HIGHLOW operand;
the actual linked operand must equal the unique linked symbol plus addend.
Adjacent diagnostic.json manifests flag forced unresolved links automatically;
invalid header/out-of-image operands cannot support matching coverage.
Linked floating literals additionally require matching decoded scalar reads,
complete compiler-declared width/bits, read-only storage, and unique contents;
duplicate byte patterns remain unresolved even at unaligned incidental sites.
Named narrow string-record pointers require independently decoded PUSH/MOV
immediates and unique complete record contents. Absolute 16/32-bit MOV register
reads have a separate bounded scalar-read proof; neither proves C-string API use.
Suffixes remain unresolved without independent bounds.
No files are modified except explicitly requested JSON reports.
"""

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP, CS_GRP_RET
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

from pe_fingerprints import PE32
from verify_gs import COFF, differences
from nemu_float_provenance import prove_float_operand
from nemu_string_provenance import prove_string_operand

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'master/nemu64.exe'
LIBRARIES = ROOT / 'analysis/nemu-library-addresses.json'
VERIFIED_BASES = {'independent_library_mapping', 'import_table',
                  'independently_validated_application', 'absolute_coff_symbol',
                  'same_function_label', 'authoritative_application_storage'}


def integer(value):
    return int(value, 0) if isinstance(value, str) else value


def digest(data):
    return hashlib.sha256(data).hexdigest()


def image_bytes(image, va, size):
    """Read one complete file-backed extent, rejecting section/file overruns."""
    if size <= 0:
        raise ValueError(f'{image.path}: invalid extent size {size}')
    rva = va - image.base
    section = next((s for s in image.sections if s['rva'] <= rva < s['rva'] + s['raw_size']), None)
    if section is None or rva + size > section['rva'] + section['raw_size']:
        raise ValueError(f'{image.path}: extent {va:#x}+{size:#x} is not inside one file-backed section')
    offset = image.offset(rva)
    result = image.data[offset:offset + size]
    if len(result) != size:
        raise ValueError(f'{image.path}: extent exceeds file length')
    return result


def aliases(name):
    if not name:
        return set()
    names = {name}
    if name.startswith('?') and '@@' in name:
        names.add(name[1:name.index('@@')])
    if name.startswith('_') and not name.startswith('__'):
        names.add(re.sub(r'@\d+$', '', name[1:]))
    if name.startswith('__imp_'):
        names.add(re.sub(r'@\d+$', '', name))
    return names


def load_metadata(paths, input_hashes=None):
    functions, symbols = [], []
    for path in paths:
        content = Path(path).read_bytes()
        if input_hashes is not None:
            key, value = str(path), digest(content)
            if key in input_hashes and input_hashes[key] != value:
                raise ValueError(f'{path}: metadata changed between reads in one comparison')
            input_hashes[key] = value
        document = json.loads(content)
        storage_inventory = (isinstance(document, dict) and
                             document.get('generator') == 'scripts/map_nemu_globals.py' and
                             bool(document.get('original_sha256')))
        conflicting_storage = set()
        if storage_inventory:
            for conflict in document.get('conflicts', []):
                if conflict.get('kind') == 'missing_nonzero_initializer':
                    continue
                conflicting_storage.update(conflict.get('symbols', []))
                if conflict.get('symbol'):
                    conflicting_storage.add(conflict['symbol'])
        for key in ('functions', 'globals', 'symbols'):
            entries = document.get(key, []) if isinstance(document, dict) else document
            if isinstance(entries, dict):
                entries = [dict(value, name=name) if isinstance(value, dict)
                           else dict(name=name, va=value) for name, value in entries.items()]
            for index, value in enumerate(entries):
                row = dict(value)
                row['name'] = row.get('symbol', row.get('name'))
                row['va'] = integer(row.get('original_va', row.get('va', row.get('address'))))
                row['metadata_file'] = str(path)
                row['metadata_index'] = index
                row['kind'] = 'function' if key == 'functions' else 'data'
                row['address_basis'] = row.get('address_basis', 'application_function_metadata'
                                               if key == 'functions' else 'application_observation')
                row['size'] = integer(row.get('original_size', row.get('size', row.get('length', row.get('extent', row.get('extent_bytes'))))))
                if storage_inventory and key == 'globals':
                    addresses = {integer(o['va']) for o in row.get('observations', []) if o.get('va') is not None}
                    if (row['name'] not in conflicting_storage and row['va'] is not None and
                            addresses == {row['va']} and row['size'] and
                            row.get('original_storage', {}).get('section') and
                            row.get('storage_kind') != 'compile_time_guest_address_constant'):
                        row.update(address_basis='authoritative_application_storage',
                                   original_image_sha256=document['original_sha256'],
                                   identity_scope='Mapped concrete storage and offset; inferred source spelling is not an original symbol claim.')
                if row['size'] is None and key == 'functions' and row.get('link_span') is not None:
                    row['size'] = integer(row['link_span'])
                    row['extent_basis'] = 'explicit_link_span_including_alignment'
                    if (row.get('end_va_exclusive') is not None and row['va'] is not None and
                            integer(row['end_va_exclusive']) - row['va'] != row['size']):
                        raise ValueError(f'{path}: inconsistent link span for {row["name"]}')
                # Ghidra reachable-byte counts and reference_size are not
                # necessarily contiguous extents. Preserve them as evidence,
                # never silently promote them to a complete function size.
                if row.get('rebuilt_size') is not None:
                    row['rebuilt_size'] = integer(row['rebuilt_size'])
                    if isinstance(document, dict):
                        row.setdefault('rebuilt_image_sha256', document.get('rebuilt_image_sha256',
                                       document.get('candidate_image_sha256')))
                    if row.get('measurement'):
                        row.setdefault('rebuilt_extent_basis', row['measurement'])
                if row['name'] and row['va'] is not None:
                    symbols.append(row)
                    for alias in row.get('source_aliases', []):
                        alias_va = integer(alias.get('original_va', row['va']))
                        if alias_va != row['va']:
                            raise ValueError(f'{path}: source alias has a different physical address: {alias["name"]}')
                        symbols.append(dict(row, name=alias['name'], source_alias_of=row['name'],
                                            source_alias_evidence=alias))
                if key == 'functions':
                    if row['size'] is not None and row['size'] <= 0:
                        raise ValueError(f'{path}: invalid function size for {row["name"]}')
                    row['metadata_problems'] = [label for label, present in (
                        ('missing_source_symbol', bool(row['name'])),
                        ('missing_original_address', row['va'] is not None),
                        ('missing_original_extent', row['size'] is not None)) if not present]
                    functions.append(row)
            if isinstance(document, list):
                break
    return functions, symbols


def image_imports(image):
    rows = []
    rva, _ = image.directory(1)
    if not rva:
        return rows
    pos = image.offset(rva)
    while struct.unpack_from('<I', image.data, pos + 12)[0]:
        lookup, _, _, dll_rva, iat = struct.unpack_from('<IIIII', image.data, pos)
        off = image.offset(dll_rva)
        dll = image.data[off:image.data.index(0, off)].decode('ascii')
        off = image.offset(lookup or iat)
        index = 0
        while True:
            value = struct.unpack_from('<I', image.data, off + index * 4)[0]
            if not value:
                break
            if value & 0x80000000:
                name = f'{dll}!ordinal:{value & 0xffff}'
            else:
                n = image.offset(value) + 2
                name = image.data[n:image.data.index(0, n)].decode('ascii')
            for symbol in ('__imp_' + name, '__imp__' + name):
                rows.append(dict(name=symbol, va=image.base + iat + index * 4,
                                 kind='import', address_basis='import_table', dll=dll, import_name=name))
            index += 1
        pos += 20
    return rows


class Symbols:
    def __init__(self, rows):
        self.rows = rows
        self.names = defaultdict(list)
        self.addresses = defaultdict(list)
        for row in rows:
            for name in aliases(row['name']):
                self.names[name].append(row)
            self.addresses[row['va']].append(row)

    def named(self, name):
        rows = self.names.get(name, [])
        if not rows:
            rows = [r for alias in aliases(name) for r in self.names.get(alias, [])]
        # An exact independent library symbol defeats an inferred wrong alias.
        ranks = {'independent_library_mapping': 4, 'import_table': 4,
                 'independently_validated_application': 3,
                 'authoritative_application_storage': 3,
                 'application_function_metadata': 2}
        if rows:
            rank = max(ranks.get(r.get('address_basis'), 0) for r in rows)
            rows = [r for r in rows if ranks.get(r.get('address_basis'), 0) == rank]
        unique = {}
        for row in rows:
            unique.setdefault(row['va'], row)
        return list(unique.values())

    def at(self, va):
        exact = self.addresses.get(va, [])
        if exact:
            return [(r, 0) for r in exact]
        return [(r, va - r['va']) for r in self.rows
                if r.get('size') and r['va'] < va < r['va'] + r['size']]


def target_symbols(image, metadata, library_path):
    image_hash = digest(image.data)
    for row in metadata:
        if row.get('original_image_sha256') not in (None, image_hash):
            raise ValueError('Authoritative storage inventory does not describe the original image')
    rows = list(metadata) + image_imports(image)
    library_hash = None
    if library_path and Path(library_path).exists():
        content = Path(library_path).read_bytes()
        document = json.loads(content)
        library_hash = digest(content)
        if document['original_sha256'] != digest(image.data):
            raise ValueError('Independent library map does not describe the original image')
        rows += [dict(r, kind='function') for r in document['symbols']
                 if not r.get('comparison_only', False)]
        # Deliberately do not load observed_data_symbols as established identities.
    result = Symbols(rows)
    result.image = image
    result.library_map_sha256 = library_hash
    return result


def read_map(path):
    rows = []
    for line in Path(path).read_text(errors='replace').splitlines():
        pieces = line.split()
        if len(pieces) < 3 or not re.fullmatch(r'[0-9a-fA-F]{4}:[0-9a-fA-F]{8}', pieces[0]):
            continue
        if not re.fullmatch(r'[0-9a-fA-F]{8}', pieces[2]):
            continue
        rows.append(dict(name=pieces[1], va=int(pieces[2], 16), object=pieces[-1],
                         kind='function' if 'f' in pieces[3:-1] else 'data',
                         address_basis='linked_map'))
    return rows


def map_function_spans(image, rows):
    """Independent MAP public blocks, including tables and alignment padding."""
    spans = {}
    for section in image.sections:
        start = image.base + section['rva']
        end = start + section['raw_size']
        entries = sorted({r['va'] for r in rows if r['kind'] == 'function' and start <= r['va'] < end})
        for left, right in zip(entries, entries[1:]):
            spans[left] = right - left
    return spans


def base_relocations(image):
    rva, size = image.directory(5)
    if not rva:
        return set()
    pos, end = image.offset(rva), image.offset(rva) + size
    fields = set()
    while pos < end:
        page, length = struct.unpack_from('<II', image.data, pos)
        if length < 8 or pos + length > end:
            raise ValueError('Malformed PE base relocation block')
        for at in range(pos + 8, pos + length, 2):
            value = struct.unpack_from('<H', image.data, at)[0]
            if value >> 12 == 3:
                fields.add(image.base + page + (value & 0xfff))
            elif value >> 12:
                raise ValueError(f'Unsupported PE relocation type {value >> 12}')
        pos += length
    return fields


def instructions(data, va):
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    return list(decoder.disasm(data, va))


def seh_scope_entries(image, va, size, fixups, symbols):
    """Validate VC7 __except_handler3 prolog and relocated scope-table rows."""
    handler_addresses = {row['va'] for row in symbols.named('__except_handler3')}
    if not handler_addresses:
        return []
    prolog = instructions(image_bytes(image, va, min(size, 128)), va)
    scopes = []
    for first, table_push, handler_push in zip(prolog, prolog[1:], prolog[2:]):
        triple = (first, table_push, handler_push)
        if not all(ins.mnemonic == 'push' and ins.operands and ins.operands[0].type == X86_OP_IMM
                   for ins in triple):
            continue
        if first.operands[0].imm & 0xffffffff != 0xffffffff:
            continue
        if handler_push.operands[0].imm & 0xffffffff not in handler_addresses:
            continue
        if any(ins.address + ins.imm_offset not in fixups for ins in (table_push, handler_push)):
            continue
        table = table_push.operands[0].imm & 0xffffffff
        if va <= table < va + size:
            continue
        for index in range(64):
            row_va = table + index * 12
            try:
                level, filter_va, handler_va = struct.unpack('<iII', image_bytes(image, row_va, 12))
            except ValueError:
                break
            if not -1 <= level < index or not va <= handler_va < va + size:
                break
            if filter_va and not va <= filter_va < va + size:
                break
            if row_va + 8 not in fixups or (filter_va and row_va + 4 not in fixups):
                break
            scopes.append(dict(table_va=table, entry_index=index, enclosing_level=level,
                               filter_va=filter_va, handler_va=handler_va,
                               evidence='relocated_scope_table_referenced_by_validated___except_handler3_prolog'))
    return scopes


def reachable_instructions(data, va, fixups, extra_entries=(), table_data=None, table_reads=None):
    """Decode control flow; never manufacture branch masks from table bytes.

    Direct branches/calls within the declared block are followed. Indexed
    indirect jumps can add switch successors only through actual contiguous
    PE HIGHLOW table entries addressed by that jump instruction. table_data
    may include the rest of the independently bounded MAP block when a measured
    code body excludes its following tables. Successors remain inside data.
    """
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    end = va + len(data)
    table_data = data if table_data is None else table_data
    if not table_data.startswith(data):
        raise ValueError('Switch-table view must contain the unchanged candidate body prefix')
    table_end = va + len(table_data)
    pending, decoded = [va, *extra_entries], {}
    while pending:
        address = pending.pop()
        while va <= address < end and address not in decoded:
            ins = next(decoder.disasm(data[address - va:address - va + 15], address, count=1), None)
            if ins is None:
                break
            decoded[address] = ins
            if ins.group(CS_GRP_RET):
                break
            if (ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP)) and ins.operands:
                operand = ins.operands[0]
                if operand.type == X86_OP_IMM:
                    target = operand.imm & 0xffffffff
                    if va <= target < end:
                        pending.append(target)
                elif (ins.group(CS_GRP_JUMP) and operand.type == X86_OP_MEM and
                      not operand.mem.base and operand.mem.scale == 4):
                    table = operand.mem.disp & 0xffffffff
                    first, targets = table, []
                    while table in fixups and va <= table and table + 4 <= table_end:
                        target = int.from_bytes(table_data[table - va:table - va + 4], 'little')
                        if not va <= target < end:
                            break
                        pending.append(target)
                        targets.append(target)
                        table += 4
                    if targets and table_reads is not None:
                        table_reads.append(dict(instruction_va=ins.address, table_va=first,
                            entry_count=len(targets), end_va=table,
                            outside_compared_body=first < va or table > end,
                            evidence='indexed_indirect_jump_and_contiguous_PE_HIGHLOW_entries',
                            code_successor_bound=[va, end], table_read_bound=[va, table_end]))
                if ins.mnemonic in ('jmp', 'ljmp'):
                    break
            address += ins.size
    return [decoded[address] for address in sorted(decoded)]


def linked_fields(data, va, size, fixups, candidate_symbols, infer_map_addresses=False, extra_entries=(),
                  table_data=None, table_reads=None):
    fields = {}
    local_fixups = sorted(address for address in fixups if va <= address < va + len(data))
    fixup_bytes = {byte for address in local_fixups for byte in range(address, address + 4)}
    for address in local_fixups:
        if va <= address:
            off = address - va
            fields[off] = dict(offset=off, width=4, kind='DIR32', authority='PE_HIGHLOW',
                               candidate_target=int.from_bytes(data[off:off + 4], 'little'))
    for ins in reachable_instructions(data, va, fixups, extra_entries, table_data, table_reads):
        off = ins.address - va
        # PE fixups certify the field; decoding only describes its use. A
        # concrete memory access differs from a pointer value that may denote
        # one-past an array, or a signed addend to a neighboring symbol.
        for operand in ins.operands:
            if operand.type == X86_OP_MEM:
                field = fields.get(off + ins.disp_offset)
                if field is not None and ins.disp_size == 4:
                    field.update(address_use='address_calculation' if ins.mnemonic == 'lea' else 'memory_access',
                                 instruction_offset=off, instruction_size=ins.size,
                                 instruction=f'{ins.mnemonic} {ins.op_str}')
            elif operand.type == X86_OP_IMM:
                field = fields.get(off + ins.imm_offset)
                if field is not None and ins.imm_size == 4:
                    field.update(address_use='immediate_address', instruction_offset=off,
                                 instruction_size=ins.size, instruction=f'{ins.mnemonic} {ins.op_str}')
        # An instruction overlapping a recorded
        # relocation field cannot provide additional branch-mask authority.
        overlaps_fixup = any(byte in fixup_bytes for byte in range(ins.address, ins.address + ins.size))
        if (ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP)) and ins.operands and ins.operands[0].type == X86_OP_IMM:
            if overlaps_fixup:
                continue
            dest = ins.operands[0].imm & 0xffffffff
            if not va <= dest < va + size:
                field = off + ins.imm_offset
                fields[field] = dict(offset=field, width=ins.imm_size, kind='REL',
                                     authority='decoded_external_branch', candidate_target=dest,
                                     instruction_offset=off, instruction_size=ins.size)
            continue
        if not infer_map_addresses:
            continue
        for offset, width in ((ins.imm_offset, ins.imm_size), (ins.disp_offset, ins.disp_size)):
            field = off + offset
            if width != 4 or field in fields:
                continue
            value = int.from_bytes(data[field:field + width], 'little')
            if candidate_symbols.at(value):
                fields[field] = dict(offset=field, width=4, kind='DIR32',
                    authority='MAP_address_inference', candidate_target=value,
                    note='May coincide with an integer or emulated guest address; never verified normalization.')
    return list(fields.values())


def read_original_target(data, field, va):
    off, width = field['offset'], field['width']
    value = int.from_bytes(data[off:off + width], 'little')
    if field['kind'] == 'REL':
        signed = int.from_bytes(data[off:off + width], 'little', signed=True)
        value = (va + off + width + signed) & 0xffffffff
    return value


def import_thunk_identity(symbols, address):
    """Identify the concrete FF25 [IAT] thunk at a direct branch destination."""
    image = getattr(symbols, 'image', None)
    if image is None:
        return None
    try:
        code = image_bytes(image, address, 6)
    except ValueError:
        return None
    if code[:2] != b'\xff\x25':
        return None
    slot = int.from_bytes(code[2:], 'little')
    identities = {(row['dll'].lower(), row['import_name'])
                  for row in symbols.addresses.get(slot, [])
                  if row.get('address_basis') == 'import_table' and row.get('import_name')}
    if len(identities) != 1:
        return None
    dll, name = next(iter(identities))
    return dict(dll=dll, name=name, iat_va=slot, thunk_va=address)


def apply_operand_provenance(function, fields, candidates, targets, image_hash):
    """Apply reviewed source expressions only to their exact recorded operands.

    This supports proven array-end/signed-addend aliases that a MAP alone
    cannot disambiguate. The source and image are hash-pinned, and both linked
    and original addresses remain constrained by independently mapped symbols.
    No new normalization field is created by this evidence.
    """
    applied = []
    by_offset = {field['offset']: field for field in fields}
    for proof in function.get('linked_operand_provenance', []):
        if proof.get('candidate_image_sha256') != image_hash:
            continue
        source = Path(proof['source_file'])
        if digest(source.read_bytes()) != proof['source_sha256'] or not proof.get('evidence'):
            raise ValueError(f'{function["name"]}: source operand provenance is not hash-pinned/reviewed')
        offset, addend = integer(proof['offset']), integer(proof.get('addend', 0))
        field = by_offset.get(offset)
        if field is None or (field['kind'], field['width'], field['authority']) != ('DIR32', 4, 'PE_HIGHLOW'):
            raise ValueError(f'{function["name"]}+{offset:#x}: provenance lacks an actual PE HIGHLOW field')
        linked = {row['va'] for row in candidates.named(proof['symbol']) if row.get('kind') == 'data'}
        known = [row for row in targets.named(proof['symbol'])
                 if row.get('address_basis') == 'authoritative_application_storage']
        if len(linked) != 1 or len({row['va'] for row in known}) != 1:
            raise ValueError(f'{function["name"]}+{offset:#x}: provenance symbol lacks a unique linked/original storage mapping')
        if (next(iter(linked)) + addend) & 0xffffffff != field['candidate_target']:
            raise ValueError(f'{function["name"]}+{offset:#x}: recorded operand does not equal the claimed linked symbol+addend')
        field.update(symbol=proof['symbol'], addend=addend, source_operand_provenance=proof)
        applied.append(dict(offset=offset, symbol=proof['symbol'], addend=addend,
                            source_file=str(source), source_sha256=proof['source_sha256'],
                            evidence=proof['evidence']))
    return applied


def resolve_field(field, function, target, candidate_symbols=None):
    """Resolve a recorded field without learning identities from its target."""
    observed = field['original_target']
    result = dict(field, original_target_names=sorted(set(
        r['name'] for r in target.addresses.get(observed, []))))
    if candidate_symbols is not None and getattr(candidate_symbols, 'invalid_link_fixture', False):
        address = field.get('candidate_target')
        references = candidate_symbols.at(address) if address is not None else []
        invalid_names = getattr(candidate_symbols, 'invalid_names', set())
        names = {name for row, _ in references for name in aliases(row['name'])}
        outside_image = (address is not None and not any(start <= address < end
                         for start, end in candidate_symbols.image_ranges))
        if outside_image or names.intersection(invalid_names):
            result.update(status='invalid_link_operand', identity_verified=False,
                          invalid_link_operand=True,
                          note='Forced unresolved linker fixture: this target is not a valid linked identity.')
            return result
    if candidate_symbols is not None and field['kind'] == 'REL':
        before = import_thunk_identity(target, observed)
        after = import_thunk_identity(candidate_symbols, field['candidate_target'])
        if before is not None and after is not None:
            match = (before['dll'], before['name']) == (after['dll'], after['name'])
            result.update(status='matching' if match else 'mismatch', identity_verified=match,
                          resolution_bases=['decoded_import_thunk_and_import_tables'],
                          original_import_thunk=before, candidate_import_thunk=after)
            return result
    if field.get('literal_data'):
        literal = bytes.fromhex(field['literal_data'])
        try:
            off = target.image.offset(observed - target.image.base)
            actual = target.image.data[off:off + len(literal)]
        except ValueError:
            actual = b''
        equal = actual == literal
        result.update(status='matching' if equal else 'mismatch', identity_verified=equal,
                      resolution_bases=['compiled_literal_content'], expected_target=None,
                      expected_content_sha256=digest(literal), original_content_sha256=digest(actual))
        return result
    candidates = []
    if field.get('absolute_coff_value') is not None:
        candidates = [dict(name=field.get('symbol'), va=field['absolute_coff_value'],
                           address_basis='absolute_coff_symbol')]
    elif field.get('local_offset') is not None:
        candidates = [dict(name='<same function>', va=function['va'] + field['local_offset'],
                           address_basis='same_function_label')]
    elif field.get('symbol'):
        candidates = [dict(r, va=(r['va'] + field.get('addend', 0)) & 0xffffffff)
                      for r in target.named(field['symbol'])]
    elif candidate_symbols is not None:
        references = candidate_symbols.at(field['candidate_target'])
        unmapped = []
        range_inferences = []
        for symbol, addend in references:
            known_rows = target.named(symbol['name'])
            if not known_rows:
                unmapped.append(symbol['name'])
            for known in known_rows:
                row = dict(known, va=(known['va'] + addend) & 0xffffffff)
                if addend:
                    range_inferences.append(dict(name=symbol['name'], addend=addend,
                                                 expected_target=row['va'],
                                                 address_basis=known.get('address_basis')))
                else:
                    candidates.append(row)
        result['candidate_target_names'] = sorted(set(r['name'] for r, _ in references))
        if range_inferences:
            # A MAP has symbol starts, not operand symbol/addend provenance.
            # ApplicationDirectory[-1], for example, can land inside the
            # preceding ToolbarWindow variable in one link layout. A guessed
            # containing range must never establish a wrong-source-target claim.
            authoritative = [row for row in range_inferences
                             if row.get('address_basis') == 'authoritative_application_storage']
            expected = {row['expected_target'] for row in authoritative}
            # A matching, independently mapped object+offset establishes the
            # concrete storage location, without claiming its source expression.
            match = bool(authoritative) and expected == {observed}
            result.update(status='matching' if match else 'unresolved',
                          identity_verified=match and field['authority'] != 'MAP_address_inference',
                          resolution_bases=['authoritative_application_storage_offset'] if match else ['containing_symbol_range_inference'],
                          candidate_range_observations=range_inferences,
                          potential_data_target_mismatch=bool(authoritative) and not match,
                          note='Concrete mapped storage offset agrees; source expression is not inferred.' if match else
                               'Containing MAP ranges do not identify the source symbol or signed addend.')
            return result
        if len(result['candidate_target_names']) > 1 and unmapped:
            result.update(status='ambiguous', identity_verified=False,
                          note='Linked aliases share an address and one or more identities are unmapped; ICF may have folded probe stubs.',
                          unmapped_candidate_aliases=sorted(set(unmapped)))
            return result
    unique = {}
    for row in candidates:
        key = (row['va'], row.get('address_basis'))
        unique.setdefault(key, row)
    candidates = list(unique.values())
    if not candidates:
        result.update(status='unresolved', identity_verified=False)
    else:
        independent = [r for r in candidates if r.get('address_basis') in VERIFIED_BASES]
        if independent:
            candidates = independent
        addresses = {r['va'] for r in candidates}
        if len(addresses) != 1:
            result.update(status='ambiguous', identity_verified=False,
                          expected_targets=sorted(addresses))
        else:
            expected = next(iter(addresses))
            bases = sorted(set(r.get('address_basis', 'unknown') for r in candidates))
            match = observed == expected
            result.update(status='matching' if match else 'mismatch', expected_target=expected,
                          resolution_bases=bases, identity_verified=match and bool(independent))
            if (candidate_symbols is not None and not field.get('symbol') and
                    field.get('local_offset') is None and candidates and
                    all(r.get('kind') == 'data' for r in candidates)):
                # Even an exact data-symbol address can instead denote the
                # end of a preceding array. PE fixups prove address operands;
                # a MAP alone does not recover their source symbol/addend.
                authoritative = all(r.get('address_basis') == 'authoritative_application_storage'
                                    for r in candidates)
                if authoritative and (match or field.get('address_use') == 'memory_access'):
                    result.update(identity_verified=match,
                                  note='Unique authoritative inventory mapping constrains the concrete storage accessed; source spelling is inferred.')
                else:
                    result.update(status='matching' if match else 'unresolved', identity_verified=False,
                                  potential_data_target_mismatch=not match,
                                  note='Exact MAP data name lacks source-operand provenance; an array-end or signed-addend alias can have this address.')
    if field['authority'] == 'MAP_address_inference':
        result['identity_verified'] = False
    return result


def compare_bytes(function, candidate_data, candidate_size, candidate_va, fields, original,
                  targets, candidate_symbols=None, extent_verified=True):
    before = image_bytes(original, function['va'], function['size'])
    after = candidate_data
    a, b = bytearray(before), bytearray(after)
    relocations, invalid = [], []
    occupied = set()
    for value in fields:
        field = dict(value)
        off, width = field['offset'], field['width']
        if (width not in (1, 2, 4) or off < 0 or off + width > min(len(a), len(b)) or
                occupied.intersection(range(off, off + width))):
            invalid.append(dict(field, error='field outside body, unsupported width, or overlap'))
            continue
        occupied.update(range(off, off + width))
        field['original_target'] = read_original_target(before, field, function['va'])
        if field['kind'] == 'DIR32NB':
            field['original_target'] += original.base
        # A relative branch that enters/leaves a body in just one image is a
        # control-flow mismatch, not a relocatable address difference.
        if (field['kind'] == 'REL' and field.get('local_offset') is None and
                function['va'] <= field['original_target'] < function['va'] + function['size']):
            invalid.append(dict(field, error='candidate external branch is original internal branch'))
            continue
        if (candidate_va is not None and field.get('candidate_target') is not None and
                candidate_va <= field['candidate_target'] < candidate_va + candidate_size):
            field['local_offset'] = field['candidate_target'] - candidate_va
        resolved = resolve_field(field, function, targets, candidate_symbols)
        if resolved['status'] == 'unresolved':
            # These establish concrete complete contents, not original source
            # names. They cannot create a normalization field or replace
            # conflicting independent symbol evidence.
            for key, verifier in (('floating_literal_proof', prove_float_operand),
                                  ('string_record_scalar_read_proof' if field.get('address_use') == 'memory_access'
                                   else 'string_record_pointer_proof', prove_string_operand)):
                proof = verifier(field, function['va'], candidate_va, targets, candidate_symbols)
                if proof is not None:
                    resolved[key] = proof
                    if proof['verified']:
                        resolved.update(status='matching', identity_verified=True,
                                        resolution_bases=[proof['basis']])
                    break
        relocations.append(resolved)
        if field['kind'] == 'REL' and field.get('local_offset') is not None:
            # COFF can record a self-call/local-label relocation. Materialize
            # its exact invariant displacement instead of masking a local
            # branch distance.
            relative = field['local_offset'] - off - width
            b[off:off + width] = (relative & ((1 << (width * 8)) - 1)).to_bytes(width, 'little')
            occupied.difference_update(range(off, off + width))
            resolved['normalization'] = 'materialized_internal_relative_displacement'
        else:
            a[off:off + width] = b[off:off + width] = bytes(width)
    size_equal = extent_verified and function['size'] == candidate_size
    inferred_masks = sum(r['authority'] == 'MAP_address_inference' for r in relocations)
    shape = a == b and not invalid
    if not shape:
        # Operand offsets have no shared meaning after the instruction streams
        # diverge. Do not turn arbitrary original bytes into alleged callees.
        for relocation in relocations:
            relocation.update(status='not_compared_instruction_mismatch', identity_verified=False)
            relocation.pop('expected_target', None)
            relocation.pop('expected_targets', None)
    mismatches = sum(r['status'] == 'mismatch' for r in relocations)
    invalid_link_operands = sum(r.get('invalid_link_operand', False) for r in relocations)
    possible_data_mismatches = sum(r.get('potential_data_target_mismatch', False) for r in relocations) if shape else 0
    unresolved = sum(r['status'] in ('unresolved', 'ambiguous') for r in relocations)
    unverified = sum(not r['identity_verified'] for r in relocations) if shape else 0
    return dict(name=function['name'], original_va=function['va'], original_size=function['size'],
                rebuilt_va=candidate_va, rebuilt_size=candidate_size,
                rebuilt_extent_verified=extent_verified, size_equal=size_equal,
                raw_bytes_equal=before == after, raw_differing_bytes=differences(before, after),
                normalized_prefix_equal=shape, full_function_shape_match=shape and size_equal and not inferred_masks,
                prefix_instruction_and_known_target_match=shape and not mismatches and not possible_data_mismatches and not inferred_masks and not invalid_link_operands,
                instruction_and_known_target_match=shape and size_equal and not mismatches and not possible_data_mismatches and not inferred_masks and not invalid_link_operands,
                fully_verified_relocations=shape and size_equal and not unverified and not inferred_masks,
                normalized_differing_bytes=differences(a, b), masked_bytes=len(occupied),
                inferred_absolute_masks=inferred_masks, target_mismatches=mismatches,
                invalid_link_operands=invalid_link_operands,
                possible_data_target_mismatches=possible_data_mismatches,
                target_comparison_performed=shape,
                unresolved_targets=unresolved, unverified_targets=unverified,
                not_compared_target_fields=len(relocations) if not shape else 0,
                invalid_normalizations=invalid, relocations=relocations,
                first_unmasked_differences=[i for i, (x, y) in enumerate(zip(a, b)) if x != y][:32])


def find_named(rows, name):
    exact = [r for r in rows if r['name'] == name]
    if exact:
        return exact
    return [r for r in rows if aliases(r['name']).intersection(aliases(name))]


def compare_object(path, functions, original, targets):
    obj = COFF(path)
    available = list(obj.functions())
    results = []
    for function in functions:
        if function.get('metadata_problems'):
            continue
        names = find_named(available, function.get('rebuilt_symbol', function['name']))
        if not names:
            continue
        if len(names) != 1:
            results.append(dict(name=function['name'], original_va=function['va'],
                                original_size=function['size'], status='ambiguous_candidate_symbol'))
            continue
        candidate = names[0]
        size = function.get('rebuilt_size', candidate['size'])
        if size > candidate['size'] or size <= 0:
            raise ValueError(f'{path}: rebuilt_size exceeds COFF span for {function["name"]}')
        fields, unsupported = [], []
        for rel in candidate['relocations']:
            if rel['offset'] >= size:
                continue
            symbol = rel['symbol']
            if rel['type'] == 0:
                continue
            if rel['type'] not in (6, 7, 20):
                unsupported.append(rel['type'])
                continue
            off = rel['offset']
            addend = int.from_bytes(candidate['data'][off:off + 4], 'little')
            field = dict(offset=off, width=4, kind={6: 'DIR32', 7: 'DIR32NB', 20: 'REL'}[rel['type']],
                         symbol=symbol['name'], addend=addend, authority='COFF_relocation')
            if symbol['section'] == -1:
                field['absolute_coff_value'] = (symbol['value'] + addend) & 0xffffffff
            elif (symbol['section'] == candidate['section'] and
                  candidate['value'] <= symbol['value'] + addend < candidate['value'] + candidate['size']):
                field['local_offset'] = symbol['value'] + addend - candidate['value']
            elif (symbol['section'] > 0 and symbol['name'].startswith('??_C@') and
                  rel['type'] == 6):
                section = obj.sections[symbol['section'] - 1]
                # A compiler-generated string COMDAT owns its complete byte
                # content, so identity can be checked independently of names
                # inferred from original operands.
                if section['characteristics'] & 0x1000:
                    literal = section['data'][symbol['value'] + addend:]
                    if 0 < len(literal) <= 65536:
                        field['literal_data'] = literal.hex()
            fields.append(field)
        result = compare_bytes(function, candidate['data'][:size], size, None, fields, original, targets)
        result.update(status='compared', candidate_symbol=candidate['name'], object=str(path),
                      coff_span_size=candidate['size'],
                      candidate_extent_basis=('explicit_metadata' if 'rebuilt_size' in function else 'COFF_function_span_including_alignment'),
                      unsupported_relocation_types=unsupported)
        if unsupported:
            result.update(full_function_shape_match=False, instruction_and_known_target_match=False,
                          fully_verified_relocations=False)
        results.append(result)
    return dict(kind='COFF', path=str(path), sha256=digest(obj.data), functions=results)


def fixture_manifest(image, explicit_path=None):
    path = explicit_path or image.path.with_name('diagnostic.json')
    if not path.exists():
        return None
    document = json.loads(path.read_text())
    row = document.get(image.path.name, {})
    recorded_hash = row.get('sha256') if isinstance(row, dict) else None
    if document.get('kind') in ('complete_relocation_diagnostic', 'resolved_relocation_diagnostic'):
        recorded_hash = document.get('sha256')
    # An unrelated diagnostic.json must not silently authorize fixture claims.
    if recorded_hash and recorded_hash != digest(image.data):
        raise ValueError(f'{path}: fixture image hash does not match {image.path}')
    forced = (document.get('status') == 'unresolved_diagnostic_only' or
              any(str(arg).upper().startswith('/FORCE') for arg in document.get('command', [])))
    unresolved = document.get('unresolved_symbols', [])
    unresolved += re.findall(r'\(([^()\n]+)\)\s*$', document.get('unresolved_symbols_log', ''), re.M)
    return dict(path=str(path), sha256=digest(path.read_bytes()), status=document.get('status'),
                forced_unresolved_link=forced, image_hash_verified=bool(recorded_hash),
                invalid_symbols=sorted(set(unresolved)), parent_artifact=document.get('parent_artifact'),
                existing_sections=document.get('existing_sections'),
                provenance_chain=document.get('provenance_chain'))


def compare_linked(path, map_path, functions, original, targets, infer_map_addresses=False,
                   manifest_path=None, use_map_spans=False):
    image = PE32(path)
    rows = read_map(map_path)
    spans = map_function_spans(image, rows)
    image_hash = digest(image.data)
    starts = sorted(set(r['va'] for r in rows))
    for symbol in rows:
        known = targets.named(symbol['name'])
        sizes = {r['size'] for r in known if r.get('size') and r.get('kind') != 'function'}
        if symbol['kind'] != 'function' and len(sizes) == 1:
            following = next((va for va in starts if va > symbol['va']), None)
            size = next(iter(sizes))
            symbol['size'] = min(size, following - symbol['va']) if following else size
    candidates = Symbols(rows + image_imports(image))
    candidates.image = image
    manifest = fixture_manifest(image, manifest_path)
    candidates.invalid_link_fixture = bool(manifest and manifest['forced_unresolved_link'])
    candidates.invalid_names = {a for name in (manifest or {}).get('invalid_symbols', []) for a in aliases(name)}
    candidates.image_base = image.base
    candidates.image_end = image.base + max(s['rva'] + max(s['raw_size'], s['virtual_size']) for s in image.sections)
    candidates.image_ranges = [(image.base + s['rva'], image.base + s['rva'] +
                                max(s['raw_size'], s['virtual_size'])) for s in image.sections]
    fixups = base_relocations(image)
    results = []
    for function in functions:
        if function.get('metadata_problems'):
            continue
        found = find_named(rows, function.get('rebuilt_symbol', function['name']))
        unique = {r['va']: r for r in found}
        if not unique:
            continue
        if len(unique) != 1:
            results.append(dict(name=function['name'], original_va=function['va'],
                                original_size=function['size'], status='ambiguous_candidate_symbol'))
            continue
        symbol = next(iter(unique.values()))
        size = function.get('rebuilt_size', function['size'])
        extent_hash = function.get('rebuilt_image_sha256')
        extent_verified = 'rebuilt_size' in function and (extent_hash is None or extent_hash == image_hash)
        basis = function.get('rebuilt_extent_basis', 'explicit_metadata')
        if 'only reached instruction bytes' in basis.lower():
            extent_verified = False
        if not extent_verified:
            size = function['size']
            basis = 'original_sized_candidate_prefix_only'
        if use_map_spans and symbol['va'] in spans:
            size = spans[symbol['va']]
            extent_verified = True
            basis = 'linked_MAP_span_to_next_function_symbol_including_alignment'
        data = image_bytes(image, symbol['va'], size)
        map_span = spans.get(symbol['va'], size)
        table_data = image_bytes(image, symbol['va'], max(size, map_span))
        scopes = seh_scope_entries(image, symbol['va'], size, fixups, candidates)
        entries = {s[k] for s in scopes for k in ('filter_va', 'handler_va') if s[k]}
        table_reads = []
        fields = linked_fields(data, symbol['va'], size, fixups, candidates, infer_map_addresses, entries,
                               table_data, table_reads)
        source_operands = apply_operand_provenance(function, fields, candidates, targets, image_hash)
        result = compare_bytes(function, data, size, symbol['va'], fields, original, targets,
                               candidates, extent_verified=extent_verified)
        result.update(status='compared', candidate_symbol=symbol['name'],
                      candidate_extent_basis=basis, candidate_map_span=spans.get(symbol['va']),
                      candidate_extent_metadata_file=function.get('rebuilt_extent_metadata_file'),
                      seh_scope_entries=scopes,
                      applied_source_operand_provenance=source_operands,
                      switch_table_reads=table_reads,
                      candidate_extent_hash_matches=extent_hash == image_hash if extent_hash else None)
        results.append(result)
    return dict(kind='linked_PE', path=str(path), sha256=digest(image.data), map=str(map_path),
                map_sha256=digest(Path(map_path).read_bytes()), pe_highlow_relocations=len(fixups),
                fixture_manifest=manifest,
                extent_mode='MAP_blocks_including_alignment' if use_map_spans else 'explicit_extent_or_original_sized_prefix',
                map_inferred_absolute_enabled=infer_map_addresses, functions=results)


def coverage(reports, functions):
    all_rows = [row for report in reports for row in report['functions']]
    compared = [r for r in all_rows if r['status'] == 'compared']
    # Prefer an independently extent-checked match to a mismatching partial
    # object probe of the same function, but retain every result above.
    passing = {(r['original_va'], r['original_size']): r for r in compared
               if r['instruction_and_known_target_match']}
    intervals = sorted((va, va + size) for va, size in passing)
    merged = []
    for start, end in intervals:
        if merged and start <= merged[-1][1]:
            merged[-1][1] = max(end, merged[-1][1])
        else:
            merged.append([start, end])
    located = {(r['original_va'], r['original_size']) for r in compared}
    requested = {(f['va'], f['size']) for f in functions if not f.get('metadata_problems')}
    inventory = []
    for function in functions:
        matches = [r for r in all_rows if r.get('original_va') == function['va'] and
                   r.get('original_size') == function['size'] and
                   aliases(r['name']).intersection(aliases(function['name']))]
        reasons = list(function.get('metadata_problems', []))
        if reasons:
            status = 'incomplete_metadata'
        elif not matches:
            status = 'missing_candidate_symbol'
        elif any(r.get('instruction_and_known_target_match') for r in matches):
            status = 'instruction_and_known_target_match'
        else:
            compared_matches = [r for r in matches if r['status'] == 'compared']
            if not compared_matches:
                status = 'ambiguous_candidate_symbol'
            else:
                for label, test in (
                    ('missing_candidate_extent', lambda r: not r['rebuilt_extent_verified']),
                    ('different_function_size', lambda r: r['rebuilt_extent_verified'] and not r['size_equal']),
                    ('instruction_mismatch', lambda r: not r['normalized_prefix_equal']),
                    ('target_mismatch', lambda r: r['target_mismatches'] > 0),
                    ('unresolved_data_target_conflict', lambda r: r.get('possible_data_target_mismatches', 0) > 0),
                    ('invalid_link_operand', lambda r: r.get('invalid_link_operands', 0) > 0),
                    ('inferred_operand_normalization', lambda r: r['inferred_absolute_masks'] > 0)):
                    if any(test(r) for r in compared_matches):
                        reasons.append(label)
                if any(r.get('target_mismatches', 0) for r in compared_matches):
                    status = 'target_mismatch'
                elif any(r.get('possible_data_target_mismatches', 0) for r in compared_matches):
                    status = 'unresolved_data_target_conflict'
                elif any(r.get('invalid_link_operands', 0) for r in compared_matches):
                    status = 'invalid_link_operand'
                elif any(r.get('prefix_instruction_and_known_target_match') and not r['rebuilt_extent_verified']
                         for r in compared_matches):
                    status = 'matching_prefix_unverified_extent'
                elif any(not r['normalized_prefix_equal'] and
                         (not r['rebuilt_extent_verified'] or r['size_equal']) for r in compared_matches):
                    status = 'instruction_mismatch'
                else:
                    status = reasons[0] if reasons else 'not_matching'
        inventory.append(dict(name=function['name'], original_va=function['va'],
            original_size=function['size'], metadata_file=function['metadata_file'],
            metadata_index=function.get('metadata_index'), source=function.get('source'),
            original_extent_basis=function.get('extent_basis', function.get('boundary_evidence',
                                    'explicit_metadata_size_no_separate_boundary_evidence')),
            status=status, blockers=reasons,
            comparisons=[{key: r[key] for key in (
                'candidate_symbol', 'rebuilt_va', 'rebuilt_size', 'candidate_extent_basis',
                'candidate_map_span', 'candidate_extent_hash_matches',
                'candidate_extent_metadata_file',
                'rebuilt_extent_verified', 'raw_bytes_equal', 'full_function_shape_match',
                'normalized_prefix_equal', 'prefix_instruction_and_known_target_match',
                'instruction_and_known_target_match', 'normalized_differing_bytes',
                'first_unmasked_differences', 'target_mismatches', 'unresolved_targets',
                'unverified_targets', 'invalid_link_operands', 'possible_data_target_mismatches') if key in r} for r in matches],
            target_findings=[{key: operand[key] for key in (
                'offset', 'kind', 'status', 'original_target', 'original_target_names', 'expected_target',
                'candidate_target', 'candidate_target_names', 'symbol', 'potential_data_target_mismatch',
                'address_use', 'instruction', 'resolution_bases', 'note') if key in operand}
                for row in matches for operand in row.get('relocations', [])
                if operand['status'] == 'mismatch' or
                (row.get('target_comparison_performed') and operand.get('potential_data_target_mismatch'))]))
    modules = []
    for path in dict.fromkeys(f['metadata_file'] for f in functions):
        selected = [r for r in inventory if r['metadata_file'] == path]
        modules.append(dict(metadata_file=path, inventory_entries=len(selected),
                            statuses=dict(Counter(r['status'] for r in selected)),
                            matching_unmeasured_prefixes=sum(any(c.get('prefix_instruction_and_known_target_match') and
                                not c.get('rebuilt_extent_verified') for c in r['comparisons']) for r in selected),
                            blockers=dict(Counter(x for r in selected for x in r['blockers']))))
    return dict(requested_functions=len(requested), located_functions=len(located),
                inventory_entries=len(functions),
                unique_original_entry_addresses=len({f['va'] for f in functions if f['va'] is not None}),
                matched_original_entry_addresses=len({va for va, _ in passing}),
                incomplete_metadata_entries=sum(bool(f.get('metadata_problems')) for f in functions),
                missing_functions=sorted({f['name'] for f in functions if not f.get('metadata_problems') and
                                          (f['va'], f['size']) not in located}),
                compared_function_instances=len(compared),
                full_shape_matches=sum(r['full_function_shape_match'] for r in compared),
                matching_unmeasured_prefixes=sum(r['prefix_instruction_and_known_target_match'] and
                                                 not r['rebuilt_extent_verified'] for r in compared),
                matching_prefixes_without_full_match=sum(r['status'] != 'instruction_and_known_target_match' and
                    any(c.get('prefix_instruction_and_known_target_match') and not c.get('rebuilt_extent_verified')
                        for c in r['comparisons']) for r in inventory),
                unique_instruction_and_known_target_matches=len(passing),
                covered_original_bytes=sum(end - start for start, end in merged),
                covered_original_ranges=[dict(start=s, end=e) for s, e in merged],
                target_mismatches=sum(r['target_mismatches'] for r in compared),
                invalid_link_operands=sum(r.get('invalid_link_operands', 0) for r in compared),
                possible_data_target_mismatches=sum(r.get('possible_data_target_mismatches', 0) for r in compared),
                unresolved_targets=sum(r['unresolved_targets'] for r in compared),
                unverified_targets=sum(r['unverified_targets'] for r in compared),
                not_compared_target_fields=sum(r['not_compared_target_fields'] for r in compared),
                modules=modules, functions=inventory,
                note='Coverage includes exact instructions/extents and satisfied known target constraints; unverified data/callee identities remain listed. It is not executable equality.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--metadata', action='append', type=Path, required=True)
    parser.add_argument('--symbols', action='append', type=Path, default=[],
                        help='additional metadata used only as address constraints, not comparison requests')
    parser.add_argument('--extents', action='append', type=Path, default=[],
                        help='hash-pinned candidate extent sidecar; overlays existing requests without adding functions')
    parser.add_argument('--object', action='append', type=Path, default=[])
    parser.add_argument('--image', type=Path)
    parser.add_argument('--map', type=Path)
    parser.add_argument('--fixture-manifest', type=Path,
                        help='link diagnostic manifest (default: adjacent diagnostic.json if present)')
    parser.add_argument('--candidate-map-spans', action='store_true',
                        help='compare complete candidate MAP blocks, including padding, instead of measured body extents')
    parser.add_argument('--also-compare-map-spans', action='store_true',
                        help='retain prefix diagnostics and separately compare complete candidate MAP blocks')
    parser.add_argument('--original', type=Path, default=ORIGINAL)
    parser.add_argument('--libraries', type=Path, default=LIBRARIES)
    parser.add_argument('--function', help='select function names containing this string')
    parser.add_argument('--infer-map-addresses', action='store_true',
                        help='diagnose MAP-only absolute operands; disables verified normalization for affected functions')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--coverage-report', type=Path,
                        help='write compact per-function/module matrix without all operand observations')
    parser.add_argument('--summary-only', action='store_true', help='omit per-function console details')
    args = parser.parse_args()
    if not args.object and not args.image:
        parser.error('provide --object and/or --image')
    if args.image and not args.map:
        parser.error('--image requires --map')
    original = PE32(args.original)
    input_hashes = {}
    functions, known = load_metadata(args.metadata, input_hashes)
    if args.extents:
        image_hash = digest(args.image.read_bytes()) if args.image else None
        extents, _ = load_metadata(args.extents, input_hashes)
        for function in functions:
            matching = [row for row in extents if row.get('rebuilt_size') is not None and
                        row.get('extent_verified', True) and row.get('rebuilt_image_sha256') == image_hash and
                        row['va'] == function['va'] and aliases(row['name']).intersection(aliases(function['name']))]
            sizes = {row['rebuilt_size'] for row in matching}
            if len(sizes) > 1:
                raise ValueError(f'Conflicting candidate extents for {function["name"]}')
            if matching:
                row = matching[0]
                function.update({k: v for k, v in row.items() if k.startswith('rebuilt_')})
                function['rebuilt_extent_metadata_file'] = row['metadata_file']
    for path in args.symbols:
        _, extra = load_metadata([path], input_hashes)
        known.extend(extra)
    if args.function:
        functions = [f for f in functions if args.function in (f['name'] or '')]
    if not functions:
        parser.error('metadata contains no selected function rows')
    target = target_symbols(original, known, args.libraries)
    reports = [compare_object(p, functions, original, target) for p in args.object]
    if args.image:
        reports.append(compare_linked(args.image, args.map, functions, original, target,
                                      args.infer_map_addresses, args.fixture_manifest, args.candidate_map_spans))
        if args.also_compare_map_spans and not args.candidate_map_spans:
            reports.append(compare_linked(args.image, args.map, functions, original, target,
                                          args.infer_map_addresses, args.fixture_manifest, True))
    result = dict(diagnostic_only=True, original=str(args.original), original_sha256=digest(original.data),
                  tool_sha256=digest(Path(__file__).read_bytes()), command_line=sys.argv,
                  floating_literal_verifier=dict(path='scripts/nemu_float_provenance.py',
                      sha256=digest(Path(__file__).with_name('nemu_float_provenance.py').read_bytes())),
                  string_literal_verifier=dict(path='scripts/nemu_string_provenance.py',
                      sha256=digest(Path(__file__).with_name('nemu_string_provenance.py').read_bytes())),
                  metadata=[dict(path=str(p), sha256=input_hashes[str(p)]) for p in args.metadata],
                  symbol_metadata=[dict(path=str(p), sha256=input_hashes[str(p)]) for p in args.symbols],
                  extent_metadata=[dict(path=str(p), sha256=input_hashes[str(p)]) for p in args.extents],
                  library_map=str(args.libraries), library_map_sha256=target.library_map_sha256,
                  comparisons=reports, coverage=coverage(reports, functions))
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + '\n')
    if args.coverage_report:
        compact = dict(result)
        compact['comparisons'] = [{k: v for k, v in report.items() if k != 'functions'} for report in reports]
        compact['full_report_path'] = str(args.report) if args.report else None
        args.coverage_report.write_text(json.dumps(compact, indent=2) + '\n')
    summary = result['coverage']
    print(f"Functions located: {summary['located_functions']}/{summary['requested_functions']}; "
          f"full instruction/known-target matches: {summary['unique_instruction_and_known_target_matches']}; "
          f"covered bytes: {summary['covered_original_bytes']}")
    print(f"Targets: {summary['target_mismatches']} mismatches, {summary['unresolved_targets']} unresolved, "
          f"{summary['unverified_targets']} not independently verified, {summary['invalid_link_operands']} invalid linked operands")
    print(f"Metadata: {summary['inventory_entries']} rows, {summary['incomplete_metadata_entries']} incomplete")
    for report in reports:
        if args.summary_only:
            break
        for row in report['functions']:
            if row.get('instruction_and_known_target_match'):
                continue
            print(row['name'], row['status'], 'normalized differences:', row.get('normalized_differing_bytes'),
                  'extent verified:', row.get('rebuilt_extent_verified'))
            for rel in row.get('relocations', []):
                if rel['status'] == 'mismatch':
                    expected = (f"{rel['expected_target']:#x}" if rel.get('expected_target') is not None
                                else 'matching literal content')
                    print(f"  +{rel['offset']:#x}: original {rel['original_target']:#x}, "
                          f"expected {expected}, {rel.get('symbol', rel.get('candidate_target_names'))}")
    # Zero means the selected diagnostic comparisons satisfy known constraints;
    # only verify_nemu.py can certify every byte of the complete executable.
    return 0 if (not summary['missing_functions'] and not summary['incomplete_metadata_entries'] and
                 summary['unique_instruction_and_known_target_matches'] == summary['requested_functions']) else 1


if __name__ == '__main__':
    raise SystemExit(main())
