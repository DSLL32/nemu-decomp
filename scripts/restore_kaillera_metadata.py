#!/usr/bin/env python3
"""Restore archival linker metadata only after the entire UPX image matches.

The native linked DLL is packed and unpacked with the pinned historical tool.
A fixed canonical hash then checks all nonmetadata bytes, including every
instruction and relocation. The untouched native DLL is retained separately.
No code, program data, import, resource, or relocation entry is restored.
"""

import hashlib
import json
from pathlib import Path
import shutil
import struct

from pe_fingerprints import PE32, rol32, u16, u32

ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / 'analysis/kaillera-build-metadata.json'
PACKER = ROOT / 'toolchains/upx120/upx.exe'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def winpath(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')


def layout(path, spec, roundtrip):
    pe = PE32(path)
    raw = pe.data
    if len(raw) != spec['original_size']:
        raise ValueError('Unexpected DLL size; refusing metadata restoration')
    count, optional = u16(raw, pe.pe + 6), u16(raw, pe.pe + 20)
    header_end = pe.opt + optional + count * 40
    zero_end = spec['header_zero_end']
    expected_headers = spec['roundtrip_header_size'] if roundtrip else spec['native_header_size']
    expected_iat = spec['roundtrip_iat_directory'] if roundtrip else spec['native_iat_directory']
    if (count != 5 or optional != 224 or not 128 <= pe.pe <= 0x128
            or header_end > zero_end or u32(raw, pe.opt + 60) != expected_headers
            or list(pe.directory(12)) != expected_iat
            or any(s['offset'] < zero_end for s in pe.sections)):
        raise ValueError('Unexpected PE layout; refusing metadata restoration')
    reloc = next((i for i, s in enumerate(pe.sections) if s['name'] == '.reloc'), None)
    if reloc is None:
        raise ValueError('Native relocation section is missing')
    section = pe.sections[reloc]
    directory_rva, directory_size = pe.directory(5)
    if (directory_rva != section['rva'] or directory_size != spec['used_relocation_size']
            or not directory_size <= section['virtual_size'] <= section['raw_size']
            or not directory_size <= spec['reloc_virtual_size'] <= section['raw_size']):
        raise ValueError('Unexpected active relocation bounds')
    unused = raw[section['offset'] + directory_size:section['offset'] + section['raw_size']]
    if any(unused):
        raise ValueError('Unused relocation allocation is not zero-filled')
    export_time = pe.offset(pe.directory(0)[0]) + 4
    return pe, header_end, 24 + optional + reloc * 40 + 8, export_time


def normalized(path, spec):
    pe, header_end, reloc_size_at, export_time = layout(path, spec, True)
    header = bytearray(pe.data[pe.pe:header_end])
    header[8:12] = bytes(4)
    header[reloc_size_at:reloc_size_at + 4] = bytes(4)
    result = bytearray(pe.data)
    result[128:spec['header_zero_end']] = bytes(spec['header_zero_end'] - 128)
    start = spec['pe_offset']
    result[start:start + len(header)] = header
    struct.pack_into('<I', result, 60, start)
    result[export_time:export_time + 4] = bytes(4)
    return bytes(result)


def restore(path, work, command):
    """Use the build driver's logged, timeout-checked command runner."""
    spec = json.loads(SPEC.read_text())
    if digest(PACKER.read_bytes()) != spec['packer_sha256']:
        raise ValueError('Pinned UPX tool changed')
    path, work = Path(path), Path(work)
    parent = (Path.home() / '.ai-tmp').resolve()
    if not work.resolve().is_relative_to(parent) or work.resolve() == parent:
        raise ValueError('Metadata scratch must be under ~/.ai-tmp')
    work.mkdir(parents=True, exist_ok=True)
    pe, header_end, reloc_size_at, export_time = layout(path, spec, False)
    raw_hash = digest(pe.data)
    packed, unpacked = work / 'guard.packed.dll', work / 'guard.unpacked.dll'
    native_copy = work / 'kailleraclient.raw-linked.dll'
    if path.resolve() in {output.resolve() for output in (packed, unpacked, native_copy)}:
        raise ValueError('Metadata scratch output would overwrite the native input')
    for output in (packed, unpacked):
        if output.exists():
            output.unlink()
    command('metadata-guard-pack', ['wine', str(PACKER), '-9',
            '-o' + winpath(packed), winpath(path)])
    command('metadata-guard-unpack', ['wine', str(PACKER), '-d',
            '-o' + winpath(unpacked), winpath(packed)])
    if digest(path.read_bytes()) != raw_hash:
        raise ValueError('Native linker input changed during metadata verification')
    canonical = digest(normalized(unpacked, spec))
    if canonical != spec['canonical_sha256']:
        raise ValueError('Linked code, data, resources, imports, exports, relocations or layout '
                         'differ; refusing metadata restoration (canonical SHA-256 ' + canonical + ')')

    # Retain the actual native LINK output before restoring archival metadata.
    shutil.copyfile(path, native_copy)
    header = bytearray(pe.data[pe.pe:header_end])
    struct.pack_into('<I', header, 8, spec['timestamp'])
    struct.pack_into('<I', header, reloc_size_at, spec['reloc_virtual_size'])
    result = bytearray(pe.data)
    result[128:spec['header_zero_end']] = bytes(spec['header_zero_end'] - 128)
    start = spec['pe_offset']
    result[start:start + len(header)] = header
    struct.pack_into('<I', result, 60, start)
    struct.pack_into('<I', result, export_time, spec['export_timestamp'])
    rich_start = 128
    key = rich_start + sum(rol32(result[i], i) for i in range(rich_start)
                           if not 0x3c <= i < 0x40)
    key += sum(rol32(int(row['comp_id'], 16), row['count']) for row in spec['rich_records'])
    key &= 0xffffffff
    values = [0x536e6144, 0, 0, 0]
    for row in spec['rich_records']:
        values.extend((int(row['comp_id'], 16), row['count']))
    rich = b''.join(struct.pack('<I', value ^ key) for value in values)
    rich += b'Rich' + struct.pack('<I', key)
    if rich_start + len(rich) > start:
        raise ValueError('Archival Rich provenance does not fit before the PE header')
    result[rich_start:rich_start + len(rich)] = rich
    allowed = set(range(128, spec['header_zero_end'])) | set(range(60, 64))
    allowed.update(range(export_time, export_time + 4))
    if any(a != b and i not in allowed for i, (a, b) in enumerate(zip(pe.data, result))):
        raise ValueError('Restoration changed a nonmetadata byte')
    path.write_bytes(result)
    return dict(applied=True, canonical_sha256=canonical,
                all_nonmetadata_bytes_exact=True, raw_linked_sha256=raw_hash,
                restored_native_sha256=digest(result), raw_linked_copy=str(native_copy),
                guard_roundtrip=str(unpacked), original_pe_offset=pe.pe,
                restored_pe_offset=start, specification=str(SPEC),
                executable_or_initialized_payload_bytes_restored=False)
