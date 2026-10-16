#!/usr/bin/env python3
"""Inspect surviving GameShark linker names without inventing symbol addresses.

The original DLL has an NB10 reference to an external PDB, not embedded CodeView
procedure records. Its incremental-link padding also retains a partial linker
heap and a truncated external-name pool. Names in that pool are useful evidence,
but their file positions are not function addresses. Only PE imports and exports
receive addresses here. This script never changes the original image.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess

from pe_fingerprints import PE32, cstring, u32

ROOT = Path(__file__).resolve().parents[1]


def imported_symbols(pe):
    rva, _ = pe.directory(1)
    pos = pe.offset(rva)
    result = []
    while u32(pe.data, pos + 12):
        dll = cstring(pe.data, pe.offset(u32(pe.data, pos + 12)))
        names = pe.offset(u32(pe.data, pos) or u32(pe.data, pos + 16))
        iat = u32(pe.data, pos + 16)
        index = 0
        while (value := u32(pe.data, names + 4 * index)):
            name = ('ordinal:' + str(value & 0xffff) if value & 0x80000000
                    else cstring(pe.data, pe.offset(value) + 2))
            result.append(dict(name=name, dll=dll, iat_rva=iat + 4 * index,
                               iat_va=pe.base + iat + 4 * index))
            index += 1
        pos += 20
    return result


def demangle(names):
    program = shutil.which('llvm-undname')
    if not program:
        return {}
    selected = {name for name in names if name.startswith('?')}
    proc = subprocess.run([program], input='\n'.join(sorted(selected)) + '\n',
                          text=True, capture_output=True, check=False)
    lines = proc.stdout.splitlines()
    result = {}
    for index, line in enumerate(lines[:-1]):
        if line in selected and lines[index + 1]:
            result[line] = lines[index + 1]
    return result


def inspect(path, with_demangling=True):
    pe = PE32(path)
    b = pe.data
    text = next(section for section in pe.sections if section['name'] == '.text')
    start = b.find(b'__afxForceEXCLUDE\0__afxForceUSRDLL\0', text['offset'])
    if start < 0:
        raise ValueError('The expected retained external-name pool was not found')
    end = text['offset'] + text['virtual_size']
    symbols = []
    pos = start
    while pos < end:
        stop = b.find(b'\0', pos, end)
        if stop < 0:
            break
        name = b[pos:stop]
        if not name or any(byte < 32 or byte > 126 for byte in name):
            break
        symbols.append(dict(name=name.decode('ascii'), name_file_offset=pos))
        pos = stop + 1
    decoded = demangle([symbol['name'] for symbol in symbols]) if with_demangling else {}
    exports = pe.exports()
    for row in exports:
        offset = pe.offset(row['rva'])
        if b[offset] == 0xe9:
            displacement = struct.unpack_from('<i', b, offset + 1)[0]
            row['implementation_rva'] = row['rva'] + 5 + displacement
            row['implementation_va'] = pe.base + row['implementation_rva']
    by_export = {row['name']: row for row in exports}
    imports = imported_symbols(pe)
    by_import = {row['name']: row for row in imports}
    for symbol in symbols:
        name = symbol['name']
        if name in decoded:
            symbol['demangled'] = decoded[name]
        export = by_export.get(name)
        if not export and name.startswith('_'):
            export = by_export.get(re.sub(r'@\d+$', '', name[1:]))
        if export:
            symbol.update(rva=export['rva'],
                          va=pe.base + export['rva'],
                          address_evidence='PE export directory')
            if 'implementation_rva' in export:
                symbol.update(implementation_rva=export['implementation_rva'],
                              implementation_va=export['implementation_va'])
        if name.startswith('__imp__'):
            imported_name = re.sub(r'@\d+$', '', name[len('__imp__'):])
            if imported_name in by_import:
                item = by_import[imported_name]
                symbol.update(rva=item['iat_rva'], va=item['iat_va'],
                              address_evidence='PE import address table',
                              import_dll=item['dll'])
    command = b.find(b' "/nologo" "/subsystem:windows" "/dll"')
    command_text = cstring(b, command) if command >= 0 else None
    directives = []
    if command >= 0:
        directive_start = command + len(command_text) + 1
        for match in re.finditer(rb'(?:DEFAULTLIB|EXPORT|OUT|include):[^\x00]+\x00|EDITANDCONTINUE\x00',
                                 b[directive_start:start]):
            directives.append(dict(file_offset=directive_start + match.start(),
                                   text=match.group()[:-1].decode('ascii')))
    source_paths = []
    library_paths = []
    for match in re.finditer(rb'[A-Za-z]:\\[\x20-\x7e]+?\x00', b):
        value = match.group()[:-1].decode('ascii')
        row = dict(file_offset=match.start(), path=value)
        if value.lower().endswith(('.cpp', '.c', '.h')):
            source_paths.append(row)
        if value.lower().endswith('.lib'):
            library_paths.append(row)
    pdb = []
    debug_rva, debug_size = pe.directory(6)
    if debug_rva:
        debug_pos = pe.offset(debug_rva)
        for offset in range(debug_pos, debug_pos + debug_size, 28):
            kind, size, _, raw = struct.unpack_from('<IIII', b, offset + 12)
            if kind == 2 and b[raw:raw + 4] == b'NB10':
                _, signature, age = struct.unpack_from('<III', b, raw + 4)
                pdb.append(dict(format='NB10', file_offset=raw, size=size,
                                signature=signature, age=age,
                                path=cstring(b, raw + 16)))
    thunks = []
    thunk_pos = text['offset']
    while b[thunk_pos] == 0xcc:
        thunk_pos += 1
    while b[thunk_pos] == 0xe9:
        rva = text['rva'] + thunk_pos - text['offset']
        destination = rva + 5 + struct.unpack_from('<i', b, thunk_pos + 1)[0]
        thunks.append(dict(rva=rva, va=pe.base + rva,
                           implementation_rva=destination,
                           implementation_va=pe.base + destination))
        thunk_pos += 5
    return dict(
        original=str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),
        sha256=hashlib.sha256(b).hexdigest(), bytes=len(b), image_base=pe.base,
        evidence_notes=[
            'name_file_offset identifies the retained name text, not executable code.',
            'The external-name pool is incomplete: the .text virtual extent truncates its final name.',
            'NB10 references an external PDB; no embedded CodeView procedure/type records are exposed here.',
            'Undecoration describes the decorated symbol, and does not establish exact source declarations.',
            'Only PE imports and exports receive addresses; no addresses are inferred from stale linker heap records.',
        ],
        linker_command=dict(file_offset=command, text=command_text),
        linker_objects=re.findall(r'"([^"\n]+\.obj)"', command_text or '', re.I),
        linker_directives=directives,
        pdb_references=pdb,
        source_paths=source_paths,
        library_paths=library_paths,
        name_pool=dict(file_offset=start, end_file_offset=end,
                       complete_names=len(symbols), trailing_fragment=b[pos:end].decode('ascii', errors='replace')),
        exports=exports, imports=imports, incremental_thunks=thunks, symbols=symbols)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', nargs='?', type=Path, default=ROOT / 'master/gs_dll.dll')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--no-demangle', action='store_true')
    args = parser.parse_args()
    result = inspect(args.dll.resolve(), not args.no_demangle)
    encoded = json.dumps(result, indent=2) + '\n'
    if args.report:
        args.report.write_text(encoded)
        print(f"{len(result['symbols'])} retained names; {len(result['exports'])} exports; "
              f"{len(result['imports'])} imports; report: {args.report}")
    else:
        print(encoded, end='')
