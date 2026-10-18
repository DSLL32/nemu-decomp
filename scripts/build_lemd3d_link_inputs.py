#!/usr/bin/env python3
"""Recover LemD3D8's native COMDAT choices without editing section contents.

The available SDK has the correct instructions, but LINK chooses different
representatives for two identical function pairs. Standard COFF weak aliases
make the observed representatives explicit. A CRT wide-string COMDAT needs
byte alignment to remain distinct from the application's zero float constant.
Only symbols, relocation symbol indexes, and section alignment are changed.
"""

import hashlib
from pathlib import Path
import struct

from build_nemu_link_inputs import archive_for, defined_symbols
from map_nemu_libraries import archive_members
from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]
SSE = '?sse_D3DXVec3Transform@@YGPAUD3DXVECTOR4@@PAU1@PBUD3DXVECTOR3@@PBUD3DXMATRIX@@@Z'
SSE_CORE = SSE.replace('Transform@@', 'Transform$$1@@')
SSE2 = SSE.replace('?sse_', '?sse2_')
SSE2_CORE = SSE_CORE.replace('?sse_', '?sse2_')
QUAT = '?c_D3DXQuaternionNormalize@@YGPAUD3DXQUATERNION@@PAU1@PBU1@@Z'
VEC4 = '?c_D3DXVec4Normalize@@YGPAUD3DXVECTOR4@@PAU1@PBU1@@Z'
EMPTY_WIDE = '??_C@_13NOLLCAOD@?$AA?$AA?$AA?$AA@'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def change_symbols(body, coff, renames=None, aliases=None, publics=None):
    renames, aliases, publics = renames or {}, aliases or {}, publics or {}
    data = bytearray(body)
    table, count = struct.unpack_from('<II', data, 8)
    strings_at = table + count * 18
    string_size = struct.unpack_from('<I', data, strings_at)[0]
    if strings_at + string_size != len(data):
        raise ValueError('Unexpected trailing COFF data')
    strings = bytearray(data[strings_at:])
    records, appended = bytearray(data[table:strings_at]), bytearray()
    names = {s['name']: s['index'] for s in coff.symbols}

    def name_record(name):
        encoded = name.encode('cp1252')
        if len(encoded) <= 8:
            return encoded.ljust(8, b'\0')
        offset = len(strings)
        strings.extend(encoded + b'\0')
        return bytes(4) + struct.pack('<I', offset)

    for old, new in {**renames, **publics}.items():
        position = names[old] * 18
        records[position:position + 8] = name_record(new)
        if old in publics:
            records[position + 16] = 2  # IMAGE_SYM_CLASS_EXTERNAL

    def append(name, target=None):
        index = count + len(appended) // 18
        appended.extend(name_record(name) + struct.pack(
            '<IhHBB', 0, 0, 32, 105 if target is not None else 2,
            1 if target is not None else 0))
        if target is not None:
            appended.extend(struct.pack('<II', target, 3) + bytes(10))
        return index

    redirect = {}
    for old, target in aliases.items():
        target_index = append(target)
        weak_index = append(old, target_index)
        # Direct references within the same member retain their exact symbol
        # identity, allowing its identical initialization wrappers to fold too.
        redirect[names[old]] = names.get(target, weak_index)
    for index in range(len(coff.sections)):
        section = 20 + index * 40
        relocations = struct.unpack_from('<I', data, section + 24)[0]
        relocation_count = struct.unpack_from('<H', data, section + 32)[0]
        if relocation_count == 0xffff:
            raise ValueError('Unexpected relocation overflow')
        for relocation in range(relocation_count):
            position = relocations + relocation * 10 + 4
            symbol = struct.unpack_from('<I', data, position)[0]
            if symbol in redirect:
                struct.pack_into('<I', data, position, redirect[symbol])
    struct.pack_into('<I', data, 12, count + len(appended) // 18)
    struct.pack_into('<I', strings, 0, len(strings))
    return bytes(data[:table] + records + appended + strings)


def generate(output):
    output.mkdir(parents=True, exist_ok=True)
    sdk = ROOT / 'toolchains/vc7/platformsdk/lib/prerelease/d3dx8.lib'
    crt = ROOT / 'toolchains/vc7/lib/libcmt.lib'
    objects, parsed = {}, {}
    for name, offset, body in archive_members(sdk):
        basename = name.rsplit('\\', 1)[-1]
        if basename in ('d3dxmath.obj', 'd3dxmathsse.obj', 'd3dxmathsse2.obj'):
            file = output / basename
            file.write_bytes(body)
            objects[basename], parsed[basename] = body, COFF(file)
    report = dict(inputs={str(p.relative_to(ROOT)): digest(p.read_bytes()) for p in (sdk, crt)},
                  aliases=[], crt_alignment=[], section_contents_unchanged=True)
    for left, old, right, new in [('d3dxmath.obj', QUAT, 'd3dxmath.obj', VEC4),
                                  ('d3dxmathsse.obj', SSE, 'd3dxmathsse2.obj', SSE2)]:
        a, b = parsed[left], parsed[right]
        x = next(s for s in a.symbols if s['name'] == old)
        y = next(s for s in b.symbols if s['name'] == new)
        sx, sy = a.sections[x['section'] - 1], b.sections[y['section'] - 1]
        signature = lambda s: [(r['offset'], r['type'], r['symbol']['name'])
                               for r in s['relocations']]
        if sx['data'] != sy['data'] or signature(sx) != signature(sy):
            raise ValueError('The proposed native aliases are not byte-identical')
        report['aliases'].append(dict(source=old, target=new, identical_section_sha256=digest(sx['data'])))

    members = []
    for name, offset, body in archive_members(sdk):
        basename = name.rsplit('\\', 1)[-1]
        aliases, renames, publics = {}, {}, {}
        if basename == 'd3dxmath.obj':
            aliases, renames = {QUAT: VEC4}, {QUAT: '_lemd3d_unused_quaternion_normalize'}
        elif basename == 'd3dxmathsse.obj':
            aliases = {SSE: SSE2, SSE_CORE: SSE2_CORE}
            renames = {SSE: '_lemd3d_unused_sse_vec3', SSE_CORE: '_lemd3d_unused_sse_vec3_core'}
        elif basename == 'd3dxmathsse2.obj':
            publics = {SSE2_CORE: SSE2_CORE}
        if aliases or publics:
            before = parsed[basename]
            body = change_symbols(body, before, renames, aliases, publics)
            file = output / ('selected-' + basename)
            file.write_bytes(body)
            after = COFF(file)
            if [s['data'] for s in before.sections] != [s['data'] for s in after.sections]:
                raise ValueError('Native section contents changed')
        members.append((name, body, defined_symbols(body) + list(aliases)))
    sdk_output = output / 'lemd3d-d3dx8.lib'
    sdk_output.write_bytes(archive_for(members))
    if [(n, b) for n, _, b in archive_members(sdk_output)] != [(n, b) for n, b, _ in members]:
        raise ValueError('SDK archive wrapping changed a member')

    # Keep the original CRT archive indexes: duplicate literal definitions have
    # an established selection order which rebuilding the indexes can change.
    archive = bytearray(crt.read_bytes())
    for name, offset, body in archive_members(crt):
        if body[:2] != b'\x4c\x01':
            continue
        file = output / 'crt-inspection.obj'
        file.write_bytes(body)
        coff = COFF(file)
        symbol = next((s for s in coff.symbols if s['name'] == EMPTY_WIDE and s['section'] > 0), None)
        if symbol is None:
            continue
        section = coff.sections[symbol['section'] - 1]
        if section['data'] != bytes(4):
            raise ValueError('Unexpected native empty wide-string contents')
        position = offset + 20 + (symbol['section'] - 1) * 40 + 36
        old = struct.unpack_from('<I', archive, position)[0]
        new = (old & ~0x00f00000) | 0x00100000  # IMAGE_SCN_ALIGN_1BYTES
        struct.pack_into('<I', archive, position, new)
        report['crt_alignment'].append(dict(object=name, old_characteristics=old,
                                             new_characteristics=new))
    if len(report['crt_alignment']) != 6:
        raise ValueError('Unexpected CRT empty-literal contribution count')
    crt_output = output / 'lemd3d-libcmt.lib'
    crt_output.write_bytes(archive)
    report['outputs'] = {p.name: digest(p.read_bytes()) for p in (sdk_output, crt_output)}
    return sdk_output, crt_output, report
