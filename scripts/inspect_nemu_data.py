#!/usr/bin/env python3
"""Account for initialized Nemu64 data without synthesizing missing objects.

Content matching is recorded separately from compiler-verified object matching.
Uncovered zero bytes remain unclassified; they are not silently called padding.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import struct

from pe_fingerprints import PE32, u32

ROOT = Path(__file__).resolve().parents[1]


def number(value):
    return int(value, 0) if isinstance(value, str) else value


def intervals(flags):
    start = None
    for index, flag in enumerate(flags):
        if not flag:
            start = index if start is None else start
        elif start is not None:
            yield start, index
            start = None
    if start is not None:
        yield start, len(flags)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library-map', type=Path,
                        default=ROOT / 'analysis/nemu-library-addresses.json')
    parser.add_argument('--output', type=Path,
                        default=ROOT / 'analysis/nemu-data-coverage.json')
    args = parser.parse_args()
    pe = PE32(ROOT / 'master/nemu64.exe')
    globals_map = json.loads((ROOT / 'analysis/nemu-globals.json').read_text())
    library = json.loads(args.library_map.read_text())
    vendor = json.loads((ROOT / 'analysis/nemu-vendor-data.json').read_text())
    legacy = json.loads((ROOT / 'analysis/nemu-legacy-data.json').read_text())
    code = json.loads((ROOT / 'analysis/nemu-code-map.json').read_text())
    assert legacy['original_sha256'] == hashlib.sha256(pe.data).hexdigest()
    assert legacy['source_sha256'] == hashlib.sha256(
        (ROOT / legacy['source']).read_bytes()).hexdigest()
    sections = {}
    for sec in pe.sections:
        if sec['name'] in ('.data', '.rdata'):
            size = min(sec['raw_size'], sec['virtual_size'])
            sections[sec['name']] = dict(start=pe.base+sec['rva'], size=size,
                data=pe.data[sec['offset']:sec['offset']+size], flags=bytearray(size))
    spans = []

    def mark(start, size, kind, label, evidence=None):
        for name, sec in sections.items():
            a, b = max(start, sec['start']), min(start+size, sec['start']+sec['size'])
            if a < b:
                sec['flags'][a-sec['start']:b-sec['start']] = b'\1'*(b-a)
                spans.append(dict(start=a, end_exclusive=b, kind=kind, label=label,
                                  section=name, evidence=evidence))

    for obj in globals_map['globals']:
        if obj.get('va') and obj.get('extent_bytes'):
            mark(number(obj['va']), obj['extent_bytes'], 'declared_application_object',
                 obj['symbol'], 'analysis/nemu-globals.json; extent may be inferred there')
    for obj in legacy['globals']:
        assert obj['validation']['equal']
        mark(number(obj['va']), obj['size'], 'compiled_retained_application_data',
             obj['name'], dict(manifest='analysis/nemu-legacy-data.json',
                               validation=obj['validation'],
                               original_file_and_grouping=obj['original_file_and_grouping']))
    data_sections = {}
    for obj in library['data_sections'] + vendor['data_sections']:
        data_sections[obj['sha256'], obj['section_index'], obj['va']] = obj
    for obj in data_sections.values():
        if obj['all_relocations_checked']:
            mark(obj['va'], obj['size'], 'matched_library_data_section',
                 obj['object'] + ':' + obj['section'],
                 dict(object_sha256=obj['sha256'], address_basis=obj['address_basis']))
    for table in code['opcode_dispatch_tables']:
        mark(table['va'], table['count']*4, 'audited_opcode_dispatch_table',
             table['family'] + ':' + table['kind'], 'Known dispatch operands and index masks')

    # Import directory structures and the exact reachable name/thunk records.
    rva, size = pe.directory(1)
    mark(pe.base+rva, size, 'pe_import_metadata', 'import_descriptors')
    pos = pe.offset(rva)
    while u32(pe.data, pos+12):
        name_rva = u32(pe.data, pos+12)
        off = pe.offset(name_rva)
        end = pe.data.index(0, off)+1
        name = pe.data[off:end-1].decode('ascii')
        mark(pe.base+name_rva, end-off, 'pe_import_metadata', name)
        int_rva = u32(pe.data, pos) or u32(pe.data, pos+16)
        thunk = pe.offset(int_rva)
        count = 0
        while u32(pe.data, thunk+4*count):
            value = u32(pe.data, thunk+4*count)
            if not value & 0x80000000:
                off = pe.offset(value)
                end = pe.data.index(0, off+2)+1
                mark(pe.base+value, end-off, 'pe_import_metadata', 'hint_and_name')
            count += 1
        mark(pe.base+int_rva, (count+1)*4, 'pe_import_metadata', name+':INT')
        mark(pe.base+u32(pe.data, pos+16), (count+1)*4, 'pe_import_metadata', name+':IAT')
        pos += 20

    # Literal content can be present in reconstructed source before layout has
    # matched. This accounts for content only, not original declaration identity.
    literals = {}
    sources = list((ROOT / 'src/nemu64').rglob('*'))
    # These SDK names are used through macros in the frontend source.
    sources.append(ROOT / 'toolchains/vc7/platformsdk/include/commctrl.h')
    for source in sources:
        if source.suffix not in ('.cpp', '.c', '.h', '.inc'):
            continue
        source_text = source.read_text(errors='replace')
        matches = list(re.finditer(r'"(?:\\.|[^"\\])*"', source_text))
        groups = []
        for match in matches:
            if groups and re.fullmatch(r'[\s\\]*', source_text[groups[-1][-1].end():match.start()]):
                groups[-1].append(match)
            else:
                groups.append([match])
        for group in groups:
            try:
                value = ''.join(ast.literal_eval(match[0]) for match in group).encode('latin1') + b'\0'
            except (ValueError, SyntaxError, UnicodeError):
                continue
            if len(value) >= 2:
                literals.setdefault(value, set()).add(str(source.relative_to(ROOT)))
    for literal, sources in literals.items():
        for sec in sections.values():
            cursor = 0
            while True:
                offset = sec['data'].find(literal, cursor)
                if offset < 0:
                    break
                mark(sec['start']+offset, len(literal), 'source_literal_content_match',
                     literal[:-1].decode('latin1'), sorted(sources))
                cursor = offset+1

    # Compiler SEH scopes are established by the registration prologues and the
    # table's filter/landing-pad pointers, not ordinary source global objects.
    for start, count, function in [(0x43E050, 2, 0x417C70), (0x43E068, 1, 0x417B30),
                                    (0x43E078, 1, 0x417A10), (0x43E090, 2, 0x415840)]:
        for i in range(count):
            row = struct.unpack_from('<III', pe.data, pe.offset(start-pe.base)+12*i)
            assert row[0] == 0xffffffff or row[0] < i
            assert 0x401000 <= row[1] < 0x42f80d and 0x401000 <= row[2] < 0x42f80d
        mark(start, 12*count, 'compiler_exception_scope_table', f'{function:08X}')

    # These floating constants have their width and interpretation fixed by
    # x87 operands. They are compiler literal-pool entries, not missing arrays.
    for row in code['global_memory_references']:
        if not (0x43E01C <= row['va'] < 0x43E04C or 0x43E084 <= row['va'] < 0x43E08C):
            continue
        refs = row['references']
        assert all(r['instruction'] in ('fadd', 'fmul', 'fdiv', 'fdivr', 'fld') for r in refs)
        widths = {r['access_size'] for r in refs}
        assert len(widths) == 1
        size = widths.pop()
        value = struct.unpack_from('<f' if size == 4 else '<d', pe.data, pe.offset(row['va']-pe.base))[0]
        mark(row['va'], size, 'observed_native_floating_constant', str(value), refs)

    names = {r['va']: r['name'] for r in library['symbols']}
    for address, expected in [(0x43F004, '___security_init_cookie'), (0x43F010, '___initstdio'),
                              (0x43F014, '___onexitinit'), (0x43F018, '___initmbctable'),
                              (0x43F024, '___endstdio')]:
        target = u32(pe.data, pe.offset(address-pe.base))
        assert names[target] == expected
        mark(address, 4, 'crt_initializer_or_terminator', expected,
             dict(target=target, basis='Pointer between CRT marker symbols targets independently matched complete CRT body.'))

    gaps = []
    summary = {}
    for name, sec in sections.items():
        for a,b in intervals(sec['flags']):
            data = sec['data'][a:b]
            lo, hi = sec['start']+a, sec['start']+b
            refs = [row for row in code['global_memory_references'] if lo <= row['va'] < hi]
            gaps.append(dict(section=name, start=lo, end_exclusive=hi, size=b-a,
                             nonzero_bytes=sum(bool(x) for x in data), memory_references=refs))
        summary[name] = dict(file_backed_virtual_bytes=sec['size'],
            accounted_bytes=sum(sec['flags']), unaccounted_bytes=sec['flags'].count(0),
            unaccounted_nonzero_bytes=sum(bool(value) for flag, value in
                                         zip(sec['flags'], sec['data']) if not flag))
    report = dict(original='master/nemu64.exe', original_sha256=hashlib.sha256(pe.data).hexdigest(),
        scope='Initialized .data/.rdata content evidence, not linked-source equality or original object boundaries',
        inputs=dict(globals='analysis/nemu-globals.json', code='analysis/nemu-code-map.json',
                    vendor='analysis/nemu-vendor-data.json',
                    legacy='analysis/nemu-legacy-data.json',
                    library_map_sha256=hashlib.sha256(args.library_map.read_bytes()).hexdigest()),
        input_sha256={name: hashlib.sha256((ROOT / 'analysis' / name).read_bytes()).hexdigest()
                      for name in ('nemu-globals.json', 'nemu-code-map.json',
                                   'nemu-vendor-data.json', 'nemu-legacy-data.json')},
        limitations=[
            'Application object extents retain inference and validation status from the global inventory.',
            'Matching literal contents do not establish declaration identity, source order or original translation units.',
            'Gaps containing zero bytes are not assumed to be padding; memory reference lists exclude address-only immediate operands.',
            'Known opcode tables and SEH records are content evidence; exact compiler-generated section layout remains separate.',
        ], summary=summary, accounted_spans=spans, unaccounted_ranges=gaps)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
