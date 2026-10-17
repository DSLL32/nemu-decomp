#!/usr/bin/env python3
"""Identify Nemu64 library code from complete original COFF function bodies.

Only recorded COFF relocations are masked; local control flow remains exact.
An initial body match does not establish a symbol name when several wrappers
have the same instruction shape. A constraint graph independently checks the
bodies of referenced library functions and PE import identities, and rejects
inconsistent candidates. Data addresses are first observations; complete COFF data sections and their
relocations are checked separately before they receive stronger evidence.

Inputs may be Microsoft .lib archives or ordinary i386 .obj files. Loose objects
share a symbol lookup group. No original executable bytes are emitted as source.
"""

import argparse
from collections import Counter, defaultdict, deque
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile

from inspect_gs_symbols import imported_symbols
from pe_fingerprints import PE32, u32
from verify_gs import COFF, RELOCATION_TYPES

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'master/nemu64.exe'
DEFAULT_LIBRARY = ROOT / 'toolchains/vc7/lib/libc.lib'


def archive_members(path):
    data = path.read_bytes()
    if not data.startswith(b'!<arch>\n'):
        yield path.name, 0, data
        return
    pos, names = 8, b''
    while pos < len(data):
        header = data[pos:pos + 60]
        if len(header) != 60 or header[58:] != b'`\n':
            raise ValueError(f'{path}: invalid archive member at {pos:#x}')
        name, size = header[:16].decode('ascii').strip(), int(header[48:58])
        body = data[pos + 60:pos + 60 + size]
        if name == '//':
            names = body
        elif name not in ('/', ''):
            if name.startswith('/') and name[1:].isdigit():
                start = int(name[1:])
                stops = [end for terminator in (b'\0', b'/\n')
                         if (end := names.find(terminator, start)) >= 0]
                name = names[start:min(stops)].decode('cp1252')
            yield name.rstrip('/'), pos + 60, body
        pos += 60 + size + (size & 1)


def display_path(path):
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def pattern_for(function):
    data = function['data'].rstrip(b'\xcc')
    mask = bytearray(len(data))
    for relocation in function['relocations']:
        width = RELOCATION_TYPES.get(relocation['type'], ('unknown', 0))[1]
        if not width and relocation['type'] != 0:
            return None, None
        offset = relocation['offset']
        if offset + width <= len(mask):
            mask[offset:offset + width] = b'\x01' * width
    pieces = []
    for match in re.finditer(b'\x00+|\x01+', mask):
        start, end = match.span()
        pieces.append(re.escape(data[start:end]) if not mask[start]
                      else b'.{' + str(end - start).encode() + b'}')
    return re.compile(b''.join(pieces), re.S), mask


class Matcher:
    def __init__(self, executable, work):
        self.target = PE32(executable)
        self.work = work
        self.text = next(s for s in self.target.sections if s['name'] == '.text')
        self.text_data = self.target.data[self.text['offset']:
                                          self.text['offset'] + self.text['virtual_size']]
        self.functions, self.members, self.inputs = {}, [], []
        self.objects, self.data_definitions = {}, defaultdict(list)
        self.definitions = defaultdict(list)
        self.external_definitions = defaultdict(list)
        self.patterns, self.body_cache = {}, {}
        self.imports = {row['name']: row['iat_va'] for row in imported_symbols(self.target)}

    def add_input(self, path, comparison=False):
        source = display_path(path)
        group = source if path.suffix.lower() == '.lib' else 'loose_objects'
        summary = dict(path=source, sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                       comparison_only=comparison, members=0, coff_members=0,
                       unsupported_members=Counter(), functions=0, initial_unique_matches=0)
        self.inputs.append(summary)
        for index, (name, offset, data) in enumerate(archive_members(path)):
            summary['members'] += 1
            if data[:2] != b'\x4c\x01':
                summary['unsupported_members'][data[:4].hex()] += 1
                continue
            cache = self.work / f'input-{len(self.inputs)}' / f'{index}.obj'
            cache.parent.mkdir(parents=True, exist_ok=True)
            cache.write_bytes(data)
            obj = COFF(cache)
            comp_id = next((s['value'] for s in obj.symbols if s['name'] == '@comp.id'), None)
            member = dict(library=source, object=name, archive_offset=offset,
                          sha256=hashlib.sha256(data).hexdigest(), comp_id=comp_id,
                          comparison_only=comparison)
            member_index = len(self.members)
            self.members.append(member)
            self.objects[member_index] = obj
            for symbol in obj.symbols:
                if symbol['section'] > 0 and not (
                        obj.sections[symbol['section'] - 1]['characteristics'] & 0x20000020):
                    self.data_definitions[group, symbol['name']].append((member_index, symbol))
            summary['coff_members'] += 1
            for function in obj.functions():
                fid = len(self.functions)
                function = dict(function, member=member_index, group=group,
                                object_cache=str(cache), comparison_only=comparison)
                self.functions[fid] = function
                self.definitions[group, function['name']].append(fid)
                if not comparison:
                    self.external_definitions[function['name']].append(fid)
                pattern, mask = pattern_for(function)
                self.patterns[fid] = (pattern, mask)
                summary['functions'] += 1

    def body_matches(self, fid, va):
        key = fid, va
        if key in self.body_cache:
            return self.body_cache[key]
        pattern, mask = self.patterns[fid]
        good = False
        if pattern and mask:
            try:
                pos = self.target.offset(va - self.target.base)
                good = pattern.fullmatch(self.target.data[pos:pos + len(mask)]) is not None
            except ValueError:
                pass
        self.body_cache[key] = good
        return good

    def graph(self, additional_seeds=()):
        nodes, queue, seeds = {}, deque(), set()
        for summary in self.inputs:
            summary['initial_unique_matches'] = 0

        def add_node(fid, va):
            key = fid, va
            if key not in nodes:
                nodes[key] = dict(requirements=[], failed=[], unresolved=[])
                queue.append(key)
            return key

        for fid, function in self.functions.items():
            pattern, mask = self.patterns[fid]
            if not pattern or mask is None or len(mask) - sum(mask) < 5:
                continue
            locations = []
            for match in pattern.finditer(self.text_data):
                locations.append(self.target.base + self.text['rva'] + match.start())
                if len(locations) > 1:
                    break
            if len(locations) == 1:
                seeds.add(add_node(fid, locations[0]))
                source = self.members[function['member']]['library']
                next(s for s in self.inputs if s['path'] == source)['initial_unique_matches'] += 1
        for fid, va in additional_seeds:
            add_node(fid, va)
        while queue:
            fid, va = key = queue.popleft()
            function, node = self.functions[fid], nodes[key]
            for relocation in function['relocations']:
                offset, kind, symbol = relocation['offset'], relocation['type'], relocation['symbol']
                if kind not in (6, 7, 20) or offset + 4 > len(function['data']):
                    continue
                addend = int.from_bytes(function['data'][offset:offset + 4], 'little')
                observed = u32(self.target.data, self.target.offset(va - self.target.base) + offset)
                if kind == 20:
                    observed = (va + offset + 4 + observed) & 0xffffffff
                elif kind == 7:
                    observed += self.target.base
                observed = (observed - addend) & 0xffffffff
                record = dict(symbol=symbol['name'], observed_va=observed, offset=offset,
                              type=RELOCATION_TYPES[kind][0], source_addend=addend,
                              symbol_section=symbol['section'], storage_class=symbol['storage_class'])
                if (symbol['section'] == function['section'] and
                        function['value'] <= symbol['value'] < function['value'] + function['size']):
                    expected = va + symbol['value'] - function['value']
                    record['basis'] = 'same_function_symbol'
                    (node['requirements'] if observed == expected else node['failed']).append(
                        dict(record, alternatives=None))
                    continue
                imported_name = re.sub(r'@\d+$', '', symbol['name'].removeprefix('__imp_').lstrip('_'))
                if imported_name in self.imports:
                    iat = self.imports[imported_name]
                    good = observed == iat
                    if not symbol['name'].startswith('__imp_'):
                        try:
                            pos = self.target.offset(observed - self.target.base)
                            good = (self.target.data[pos:pos + 2] == b'\xff\x25'
                                    and u32(self.target.data, pos + 2) == iat)
                        except ValueError:
                            good = False
                    record.update(basis='PE_import', expected_iat_va=iat)
                    (node['requirements'] if good else node['failed']).append(dict(record, alternatives=None))
                    continue
                candidates = self.definitions[function['group'], symbol['name']]
                if not candidates and symbol['section'] == 0:
                    candidates = self.external_definitions[symbol['name']]
                if symbol['section'] > 0:
                    candidates = [j for j in candidates
                                  if self.functions[j]['member'] == function['member']]
                if not candidates:
                    node['unresolved'].append(record)
                    continue
                valid = [j for j in candidates if self.body_matches(j, observed)]
                if not valid:
                    node['failed'].append(dict(record, basis='callee_body_mismatch'))
                    continue
                node['requirements'].append(dict(record, basis='independent_callee_body',
                    alternatives=[add_node(j, observed) for j in valid]))
        alive = set(nodes)
        while True:
            remove = {key for key in alive if nodes[key]['failed'] or any(
                r['alternatives'] is not None and not any(k in alive for k in r['alternatives'])
                for r in nodes[key]['requirements'])}
            if not remove:
                break
            alive -= remove
        return nodes, seeds, alive

    def short_function_seeds(self, alive):
        """Resolve small wrappers only in already independently owned members."""
        owners, known = defaultdict(set), defaultdict(set)
        for fid, va in alive:
            function = self.functions[fid]
            if function['comparison_only']:
                continue
            owners[function['name'], va].add(function['member'])
            known[function['group'], function['name']].add(va)
        established = {next(iter(members)) for members in owners.values() if len(members) == 1}
        already = {fid for fid, va in alive}
        result = set()
        for fid, function in self.functions.items():
            if fid in already or function['member'] not in established:
                continue
            pattern, mask = self.patterns[fid]
            if not pattern or not function['relocations'] or len(mask) - sum(mask) >= 8:
                continue
            valid = []
            for match in pattern.finditer(self.text_data):
                va = self.target.base + self.text['rva'] + match.start()
                good = True
                for relocation in function['relocations']:
                    offset, kind, symbol = relocation['offset'], relocation['type'], relocation['symbol']
                    if kind != 20 or not known[function['group'], symbol['name']]:
                        good = False
                        break
                    observed = (va + offset + 4 + u32(self.memory(va + offset, 4), 0)
                                - u32(function['data'], offset)) & 0xffffffff
                    if observed not in known[function['group'], symbol['name']]:
                        good = False
                        break
                if good:
                    valid.append((fid, va))
                    if len(valid) > 1:
                        break
            if len(valid) == 1:
                result.add(valid[0])
        return result

    def memory(self, va, size):
        """Read mapped file bytes, or the PE loader's zero-filled section tail."""
        for section in self.target.sections:
            relative = va - self.target.base - section['rva']
            extent = max(section['virtual_size'], section['raw_size'])
            if 0 <= relative and relative + size <= extent:
                available = max(0, min(size, section['raw_size'] - relative))
                begin = section['offset'] + relative
                return self.target.data[begin:begin + available] + bytes(size - available)
        raise ValueError(f'Unmapped image span {va:#x}+{size:#x}')

    def data_sections(self, nodes, alive):
        """Check whole data sections anchored by independently matched code.

        A zero-filled global is not a unique byte-pattern identification. Its
        name comes from a verified library function's relocation and the COFF
        definition; that provenance is kept separate from independent code maps.
        """
        queued, work, records, function_seeds = set(), deque(), [], set()
        bindings, local_code = defaultdict(set), defaultdict(set)
        member_options = defaultdict(set)
        for fid, va in alive:
            if not self.functions[fid]['comparison_only']:
                member_options[self.functions[fid]['name'], va].add(self.functions[fid]['member'])
        matched_members = {next(iter(options)) for options in member_options.values() if len(options) == 1}
        for fid, va in alive:
            function = self.functions[fid]
            bindings[function['group'], function['name']].add(va)
            obj = self.objects[function['member']]
            for symbol in obj.symbols:
                if (symbol['section'] == function['section'] and
                        function['value'] <= symbol['value'] < function['value'] + function['size']):
                    local_code[function['member'], symbol['name']].add(va + symbol['value'] - function['value'])

        def locate(group, name, owner, va, evidence):
            definitions = self.data_definitions[group, name]
            if owner is not None:
                definitions = [d for d in definitions if d[0] == owner]
            elif any(index in matched_members for index, symbol in definitions):
                # Startup variants reuse several public globals. A verified
                # WinMainCRTStartup contribution selects wincrt0's definitions;
                # equal zero bytes do not establish that every startup was linked.
                definitions = [d for d in definitions if d[0] in matched_members]
            for member_index, symbol in definitions:
                if self.members[member_index]['comparison_only']:
                    continue
                section = self.objects[member_index].sections[symbol['section'] - 1]
                if section['name'].startswith('.debug') or section['name'] == '.drectve':
                    continue
                address = va - symbol['value']
                key = member_index, section['index'], address
                if key not in queued:
                    queued.add(key)
                    work.append((key, group, evidence))

        for fid, va in sorted(alive):
            function = self.functions[fid]
            if function['comparison_only'] or function['member'] not in matched_members:
                continue
            for reference in nodes[fid, va]['unresolved']:
                locate(function['group'], reference['symbol'],
                       function['member'] if reference['symbol_section'] > 0 else None,
                       reference['observed_va'], dict(kind='matched_function_relocation',
                       function=function['name'], function_va=va, offset=reference['offset'],
                       symbol=reference['symbol']))
        while work:
            (member_index, section_index, va), group, evidence = work.popleft()
            obj = self.objects[member_index]
            section = obj.sections[section_index - 1]
            data = section['data']
            alignment = (section['characteristics'] >> 20) & 15
            if alignment and va % (1 << (alignment - 1)):
                continue
            try:
                observed = self.memory(va, len(data))
            except ValueError:
                continue
            mask, supported = bytearray(len(data)), True
            for relocation in section['relocations']:
                kind, offset = relocation['type'], relocation['offset']
                width = RELOCATION_TYPES.get(kind, ('unknown', 0))[1]
                if kind not in (0, 6, 7, 20) or offset + width > len(mask):
                    supported = False
                    break
                mask[offset:offset + width] = b'\x01' * width
            if not supported or any(a != b for i, (a, b) in enumerate(zip(data, observed)) if not mask[i]):
                continue
            relocations, pending, failed, local_seeds = [], [], False, set()
            for relocation in section['relocations']:
                symbol, offset, kind = relocation['symbol'], relocation['offset'], relocation['type']
                if not kind:
                    continue
                addend = u32(data, offset)
                destination = u32(observed, offset)
                if kind == 20:
                    destination = (va + offset + 4 + destination) & 0xffffffff
                elif kind == 7:
                    destination += self.target.base
                destination = (destination - addend) & 0xffffffff
                row = dict(symbol=symbol['name'], va=destination, offset=offset,
                           type=RELOCATION_TYPES[kind][0], source_addend=addend)
                expected = set(bindings[group, symbol['name']])
                expected.update(local_code[member_index, symbol['name']])
                if symbol['section'] == section_index:
                    expected.add(va + symbol['value'])
                if expected:
                    row.update(status='checked', expected_vas=sorted(expected))
                    if destination not in expected:
                        failed = True
                else:
                    if symbol['section'] == 0 and symbol['value']:
                        try:
                            common_valid = self.memory(destination, symbol['value']) == bytes(symbol['value'])
                        except ValueError:
                            common_valid = False
                        if common_valid:
                            row.update(status='checked', basis='COFF_common_definition_and_zero_filled_span',
                                       common_size=symbol['value'])
                            bindings[group, symbol['name']].add(destination)
                            relocations.append(row)
                            continue
                    candidates = self.definitions[group, symbol['name']]
                    if symbol['section'] > 0:
                        candidates = [i for i in candidates if self.functions[i]['member'] == member_index]
                    matches = [(i, destination) for i in candidates if self.body_matches(i, destination)]
                    if candidates and not matches:
                        failed = True
                    if matches:
                        local_seeds.update(matches)
                        row['status'] = 'callee_body_checked_pending_graph_constraints'
                    else:
                        row['status'] = 'data_operand_observation'
                        pending.append((symbol, destination, row))
                relocations.append(row)
            if failed:
                continue
            function_seeds.update(local_seeds)
            symbols = [dict(name=symbol['name'], va=va + symbol['value'],
                            storage_class=symbol['storage_class']) for symbol in obj.symbols
                       if symbol['section'] == section_index and symbol['storage_class'] in (2, 3)
                       and symbol['name'] != section['name']]
            record = dict(**self.members[member_index], member_index=member_index,
                section=section['name'], section_index=section_index, va=va, size=len(data),
                fixed_bytes=len(data) - sum(mask), nonzero_fixed_bytes=sum(
                    bool(value) for i, value in enumerate(data) if not mask[i]),
                zero_filled=not bool(section['offset']),
                comdat=bool(section['characteristics'] & 0x1000),
                address_basis='matched_library_reference_and_complete_data_section',
                anchor=evidence, symbols=symbols, relocations=relocations,
                all_relocations_checked=all(r['status'] == 'checked' for r in relocations))
            records.append(record)
            for symbol in symbols:
                bindings[group, symbol['name']].add(symbol['va'])
            for symbol, destination, row in pending:
                locate(group, symbol['name'], member_index if symbol['section'] > 0 else None,
                       destination, dict(kind='matched_data_section_relocation',
                       object=self.members[member_index]['object'], section=section['name'],
                       section_va=va, offset=row['offset'], symbol=symbol['name']))
        # A later section may resolve an earlier section's data-pointer identity.
        for record in records:
            for relocation in record['relocations']:
                if relocation['status'] != 'data_operand_observation':
                    continue
                # Most inputs are archives with separate lookup groups; resolve
                # only definitions from this record's archive (or loose-object group).
                group = record['library'] if record['library'].lower().endswith('.lib') else 'loose_objects'
                expected = bindings[group, relocation['symbol']]
                if expected and relocation['va'] in expected:
                    relocation.update(status='checked', expected_vas=sorted(expected))
            record['all_relocations_checked'] = all(r['status'] == 'checked' for r in record['relocations'])
        return records, function_seeds

    def report(self, executable):
        additional_seeds, additional_basis = set(), {}
        while True:
            nodes, seeds, alive = self.graph(additional_seeds)
            data_sections, discovered = self.data_sections(nodes, alive)
            for key in discovered:
                additional_basis[key] = 'data_reference_and_independent_callee_body'
            short = self.short_function_seeds(alive)
            for key in short:
                additional_basis[key] = 'unique_complete_body_and_known_callee_in_established_member'
            discovered |= short
            if discovered <= additional_seeds:
                break
            additional_seeds |= discovered
        symbols, observations, rejected = [], defaultdict(list), []
        for fid, va in sorted(alive):
            function = self.functions[fid]
            member = self.members[function['member']]
            if function['comparison_only'] or function['name'].startswith('$$$'):
                continue
            node = nodes[fid, va]
            validation = []
            for requirement in node['requirements']:
                row = {k: v for k, v in requirement.items() if k != 'alternatives'}
                if requirement['alternatives'] is not None:
                    row['callee_candidates'] = [dict(name=self.functions[j]['name'], va=address,
                        object=self.members[self.functions[j]['member']]['object'])
                        for j, address in requirement['alternatives'] if (j, address) in alive]
                validation.append(row)
            symbols.append(dict(name=function['name'], va=va,
                size=len(function['data'].rstrip(b'\xcc')), **member,
                address_basis='independent_library_mapping',
                discovery=('unique_body_scan' if (fid, va) in seeds else
                           additional_basis[fid, va] if (fid, va) in additional_seeds
                           else 'reference_and_independent_callee_body'),
                validated_relocations=validation, unresolved_relocations=node['unresolved']))
            for observed in node['unresolved']:
                # These are observations, never seeds in the code-identity graph.
                scoped = member['object'] if observed['storage_class'] == 3 else None
                observations[observed['symbol'], observed['observed_va'], scoped].append(
                    dict(function=function['name'], function_va=va, offset=observed['offset'],
                         object=member['object']))
        code_aliases = []
        alias_keys = set()
        for fid, va in sorted(alive):
            function = self.functions[fid]
            if function['comparison_only'] or function['name'].startswith('$$$'):
                continue
            for alias in self.objects[function['member']].symbols:
                if (alias['storage_class'] == 2 and alias['type'] != 0x20 and
                        alias['section'] == function['section'] and
                        function['value'] <= alias['value'] < function['value'] + function['size']):
                    address = va + alias['value'] - function['value']
                    if (alias['name'], address) in alias_keys:
                        continue
                    alias_keys.add((alias['name'], address))
                    code_aliases.append(dict(name=alias['name'], va=address,
                        **self.members[function['member']],
                        address_basis='COFF_public_symbol_offset_in_independently_matched_function',
                        containing_function=function['name'], function_va=va,
                        function_offset=alias['value'] - function['value']))
        for fid, va in sorted(seeds - alive):
            function = self.functions[fid]
            rejected.append(dict(name=function['name'], va=va,
                **self.members[function['member']], conflicts=nodes[fid, va]['failed'],
                reason='direct reference mismatch' if nodes[fid, va]['failed'] else 'rejected callee dependency'))
        # Same-named private helpers in startup variants do not establish which
        # archive member was linked. Preserve alternatives instead of counting
        # them as separate contributions.
        grouped = defaultdict(list)
        for symbol in symbols:
            grouped[symbol['name'], symbol['va']].append(symbol)
        consolidated = []
        for candidates in grouped.values():
            entry = dict(candidates[0])
            if len(candidates) > 1:
                entry['member_alternatives'] = [{k: r[k] for k in
                    ('library', 'object', 'archive_offset', 'comp_id')} for r in candidates]
            consolidated.append(entry)
        member_symbols = defaultdict(list)
        for row in consolidated:
            if not row.get('member_alternatives'):
                member_symbols[row['library'], row['object']].append(row['name'])
        contributions = []
        for (library, obj), names in sorted(member_symbols.items()):
            member = next(m for m in self.members if m['library'] == library and m['object'] == obj)
            contributions.append(dict(**member, matched_symbols=names, basis='matched_function_bodies'))
        contributed = {(r['library'], r['object']) for r in contributions}
        for section in data_sections:
            key = section['library'], section['object']
            if section['all_relocations_checked'] and not section['comdat'] and key not in contributed:
                member = self.members[section['member_index']]
                contributions.append(dict(**member, matched_symbols=[s['name'] for s in section['symbols']],
                    basis='matched_function_reference_and_complete_noncomdat_data_section'))
                contributed.add(key)
        for section in data_sections:
            del section['member_index']
        return dict(original=display_path(executable), original_sha256=hashlib.sha256(self.target.data).hexdigest(),
            status='code_and_import_constraints_checked_data_sections_checked_separately', inputs=self.inputs,
            notes=[
                'Complete archive function spans match outside explicit COFF relocation fields; internal branches are exact.',
                'Available referenced functions require independent complete-body matches; PE imports require exact IAT identity.',
                'A callee observation alone never establishes its identity. False same-shaped wrapper candidates are removed to a fixed point.',
                'Data-section checks preserve the code-reference provenance of their addresses; zero-filled data is not identified by a unique byte pattern.',
                'Unresolved operands remain observations; data sections require exact fixed bytes and separately reported relocation checks.',
                'Member alternatives are not counted twice. Contribution counts are a lower bound; data-only and short unmatched members remain.',
                'Comparison libraries are diagnostic variants and do not supply primary labels.',
            ], symbols=sorted(consolidated, key=lambda s: (s['va'], s['name'])),
            contributions=sorted(contributions, key=lambda r: (r['library'], r['object'])),
            data_sections=data_sections, code_aliases=sorted(code_aliases, key=lambda r: (r['va'], r['name'])),
            contribution_comp_ids=dict(Counter(f"{m['comp_id']:08x}" for m in contributions if m['comp_id'] is not None)),
            observed_data_symbols=[dict(name=name, va=va, object=obj,
                address_basis='library_operand_observation_not_independently_verified',
                reference_count=len(refs), references=refs[:4])
                for (name, va, obj), refs in sorted(observations.items(), key=lambda item: (item[0][1], item[0][0]))],
            rejected_candidates=rejected)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inputs', nargs='*', type=Path)
    parser.add_argument('--comparison-library', action='append', type=Path, default=[])
    parser.add_argument('--original', type=Path, default=ORIGINAL)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--work', type=Path)
    args = parser.parse_args()
    scratch = Path.home() / '.ai-tmp'
    scratch.mkdir(exist_ok=True)
    work = (args.work.resolve() if args.work else
            Path(tempfile.mkdtemp(prefix='nemu-libraries.', dir=scratch)))
    if not work.is_relative_to(scratch.resolve()):
        parser.error('--work must be beneath ~/.ai-tmp')
    work.mkdir(parents=True, exist_ok=True)
    matcher = Matcher(args.original.resolve(), work)
    for path in args.inputs or [DEFAULT_LIBRARY]:
        matcher.add_input(path.resolve())
    for path in args.comparison_library:
        matcher.add_input(path.resolve(), comparison=True)
    report = matcher.report(args.original.resolve())
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f"{len(report['symbols'])} function identities; {len(report['contributions'])} supported archive members; "
          f"{len(report['rejected_candidates'])} rejected candidates; report: {args.report}")
