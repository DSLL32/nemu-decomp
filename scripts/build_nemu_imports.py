#!/usr/bin/env python3
"""Rebuild Nemu64's two external import libraries from named declarations.

The resulting archives contain import records only. This does not build the
emulator or implement either imported DLL. A new ~/.ai-tmp directory and Wine
prefix are allocated unless --build-dir / WINEPREFIX are supplied.
"""

import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def winpath(path):
    return 'Z:' + str(path.resolve()).replace('/', '\\')


def inspect_library(path, expected_dll, expected_names, compiler_id):
    data = path.read_bytes()
    if data[:8] != b'!<arch>\n':
        raise ValueError(f'{path}: invalid archive')
    imports = []
    descriptors = 0
    offset = 8
    long_names = b''
    while offset < len(data):
        header = data[offset:offset + 60]
        size = int(header[48:58])
        name = header[:16].rstrip(b' ').decode('ascii')
        member = data[offset + 60:offset + 60 + size]
        offset += 60 + size + (size & 1)
        if name == '//':
            long_names = member
            continue
        if name == '/':
            continue  # Archive symbol indexes.
        if name.startswith('/') and name[1:].isdigit():
            pos = int(name[1:])
            name = long_names[pos:long_names.index(0, pos)].decode('ascii')
        else:
            name = name.rstrip('/')
        if name.lower() != expected_dll.lower():
            raise ValueError(f'{path}: unexpected archive member {name}')
        if member[:4] == b'\0\0\xff\xff':
            fields = struct.unpack_from('<HHHHIIHH', member)
            symbol, dll = member[20:].split(b'\0')[:2]
            if fields[7] != 4 or dll.decode() != expected_dll:
                raise ValueError(f'{path}: expected exact named import')
            imports.append(dict(name=symbol.decode(), hint=fields[6]))
        else:
            # Only the three import-directory objects may accompany imports.
            needle = b'@comp.id' + struct.pack('<I', compiler_id)
            if member[:2] != b'L\x01' or needle not in member:
                raise ValueError(f'{path}: unexpected descriptor provenance')
            sections = struct.unpack_from('<H', member, 2)[0]
            optional_size = struct.unpack_from('<H', member, 16)[0]
            for i in range(sections):
                pos = 20 + optional_size + i * 40
                section_name = member[pos:pos + 8].rstrip(b'\0')
                if not (section_name.startswith(b'.idata$') or section_name == b'.debug$S'):
                    raise ValueError(f'{path}: unexpected section {section_name!r}')
            descriptors += 1
    expected = [dict(name=name, hint=i) for i, name in enumerate(expected_names)]
    if imports != expected or descriptors != 3:
        raise ValueError(f'{path}: unexpected import inventory')
    return dict(path=str(path), dll=expected_dll, imports=imports,
                descriptor_compiler_id=f'{compiler_id:08x}',
                descriptor_objects=descriptors, contains_implementation=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--source-dir', type=Path, default=ROOT / 'src/nemu64/imports')
    args = parser.parse_args()
    if args.build_dir is None:
        parent = Path.home() / '.ai-tmp'
        parent.mkdir(exist_ok=True)
        output = Path(tempfile.mkdtemp(prefix='nemu-imports.', dir=parent))
    else:
        output = args.build_dir.resolve()
        output.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.setdefault('WINEPREFIX', str(output / 'wine'))
    source = args.source_dir.resolve()

    def run(toolchain, tool, *arguments):
        subprocess.run([sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                        toolchain, tool, *arguments], cwd=output, env=env, check=True)

    run('vc7', 'ml', '/nologo', '/c', '/coff', '/Fokailleraclient_symbols.obj',
        winpath(source / 'kailleraclient_symbols.asm'))
    run('vc6rtm', 'lib', '/nologo', '/machine:ix86',
        '/def:' + winpath(source / 'kailleraclient.def'),
        'kailleraclient_symbols.obj', '/out:kailleraclient.lib')
    run('vc6', 'lib', '/nologo', '/machine:ix86',
        '/def:' + winpath(source / 'gs_dll.def'), '/out:gs_dll.lib')
    libraries = [
        inspect_library(output / 'kailleraclient.lib', 'kailleraclient.dll', [
            '_kailleraChatSend@4', '_kailleraEndGame@0', '_kailleraGetVersion@4',
            '_kailleraInit@0', '_kailleraModifyPlayValues@8',
            '_kailleraSelectServerDialog@4', '_kailleraSetInfos@4',
            '_kailleraShutdown@0'], 0x00041fe8),
        inspect_library(output / 'gs_dll.lib', 'gs_dll.dll', [
            '?GSConfigure@@YAXXZ', '?GSInitialize@@YAXPAD00@Z', '?GSUpdate@@YAXXZ'],
            0x000420ff),
    ]
    report = dict(build_dir=str(output), wine_prefix=env['WINEPREFIX'],
                  libraries=libraries)
    (output / 'import-libraries.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
