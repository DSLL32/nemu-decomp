"""Embed a checked, compact linker-map index as ordinary Win32 RCDATA.

Only the user build uses this. LINK runs twice; code bytes and function RVAs
must agree between passes, so the embedded names cannot silently go stale.
"""
import hashlib
import re
import struct
from pathlib import Path
from pe_fingerprints import PE32
from static_resources import encode, align
from static_fpo import frame_table

RESOURCE_NAME = 'NEMU_CRASH_SYMBOLS'
MAGIC = b'NCRASH01'
HEADER = struct.Struct('<8sIIII16s')
ENTRY = struct.Struct('<III')


def executable_sections(exe):
    pe = PE32(Path(exe))
    table = pe.opt + struct.unpack_from('<H', pe.data, pe.pe + 20)[0]
    sections = []
    for i, section in enumerate(pe.sections):
        flags = struct.unpack_from('<I', pe.data, table + i * 40 + 36)[0]
        if flags & 0x20000000:
            sections.append((section['rva'], section['virtual_size'],
                             pe.data[section['offset']:section['offset'] + section['raw_size']]))
    return pe, sections


def map_symbols(map_path, exe):
    pe, sections = executable_sections(exe)
    result = {}
    # Includes both public and local symbols. Delphi code symbols do not have
    # MSVC's optional 'f' marker; use executable PE section bounds instead.
    pattern = re.compile(r'^\s+[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s+(.*?)\s*$')
    for line in Path(map_path).read_text(encoding='latin1').splitlines():
        match = pattern.match(line)
        if not match:
            continue
        name, address = match[1], int(match[2], 16) - pe.base
        if any(start <= address < start + size for start, size, _ in sections):
            result.setdefault(address, []).append(name)
    return pe, sections, result


def display_name(name):
    component = ''
    match = re.match(r'__static_(\w+?)_(.*)', name)
    if match:
        component, name = match.groups()
        component = {'gfx': 'LemD3D8', 'gs': 'GameShark', 'audio': 'audio', 'kaillera': 'Kaillera'}[component] + '::'
    if name.startswith('?'):
        # Keep class/method names; full type/signature demangling isn't needed
        # for a crash report, and shouldn't introduce a DbgHelp dependency.
        stem = name[1:].split('@@', 1)[0]
        parts = stem.split('@')
        if parts[0].startswith('?'):
            return component + name  # retain unusual operators verbatim
        return component + '::'.join(reversed(parts[1:])) + ('::' if len(parts) > 1 else '') + parts[0]
    if name.startswith('@'):
        stem = name[1:].split('$', 1)[0]
        stem = re.sub(r'(?i)^Static_(debugger|dialogs|input|profile|lemcombinedebugger|shared)_', r'\1.', stem)
        stem = re.sub(r'(?i)^Static_(debugger|dialogs|Input|profile|LemCombineDebugger)@', r'\1.', stem)
        return stem.replace('@', '.')
    return component + re.sub(r'@\d+$', '', name.lstrip('_'))


def table(map_path, exe, build_id):
    _, sections, symbols = map_symbols(map_path, exe)
    entries, strings = [], bytearray(b'\0')
    addresses = sorted(symbols)
    for i, address in enumerate(addresses):
        names = symbols[address]
        names = [n for n in names if not re.search(r'@_\d+$', n)
                 and not n.startswith(('??_C@', '$', '__real@'))]
        if not names:
            continue
        name = min(names, key=lambda n: (n.startswith(('_StaticAPI_', '_StaticTable_', '__imp_')), len(n), n))
        end = next(start + size for start, size, _ in sections if start <= address < start + size)
        if i + 1 < len(addresses):
            end = min(end, addresses[i + 1])
        label = display_name(name).encode('latin1', errors='replace')[:1023]
        entries.append(ENTRY.pack(address, end, len(strings)))
        strings.extend(label + b'\0')
    if not entries:
        raise ValueError('No code symbols found in linker map')
    return HEADER.pack(MAGIC, 1, len(entries), HEADER.size + len(entries) * ENTRY.size,
                       len(strings), build_id.encode('ascii')[:16].ljust(16, b'\0')) + b''.join(entries) + strings


def resource(path, payload, name=RESOURCE_NAME):
    ids = encode(10) + encode(name)
    ids += bytes(align(len(ids)) - len(ids))
    fixed = struct.pack('<IHHII', 0, 0x30, 0, 0, 0)
    data = struct.pack('<IIHHHHIHHII', 0, 32, 0xffff, 0, 0xffff, 0, 0, 0, 0, 0, 0)
    data += struct.pack('<II', len(payload), 8 + len(ids) + 16) + ids + fixed + payload
    data += bytes(align(len(data)) - len(data))
    Path(path).write_bytes(data)


def _checked_link(run_link, map_path, exe, symbol_res, fpo_res, pdb, aggregate_resources, build_id):
    resource(symbol_res, b'')
    resource(fpo_res, b'', 'NEMU_CRASH_FPO')
    aggregate_resources()
    run_link()
    _, before_code, before_names = map_symbols(map_path, exe)
    payload = table(map_path, exe, build_id)
    resource(symbol_res, payload)
    fpo = frame_table(pdb)
    resource(fpo_res, fpo, 'NEMU_CRASH_FPO')
    aggregate_resources()
    run_link()
    _, after_code, after_names = map_symbols(map_path, exe)
    if before_code != after_code or before_names != after_names or fpo != frame_table(pdb):
        Path(exe).unlink(missing_ok=True)
        raise RuntimeError('Symbol embedding changed linked code/RVAs; refusing a stale crash table')
    return dict(build_id=build_id, fpo_records=struct.unpack_from('<I',fpo,8)[0], symbols=HEADER.unpack_from(payload)[2], size=len(payload),
                sha256=hashlib.sha256(payload).hexdigest())


def checked_link(run_link, map_path, exe, symbol_res, fpo_res, pdb, aggregate_resources, build_id):
    try:
        return _checked_link(run_link,map_path,exe,symbol_res,fpo_res,pdb,aggregate_resources,build_id)
    except Exception:
        # A failed second pass must not leave an apparently successful EXE
        # containing missing or stale addresses, including direct script use.
        Path(exe).unlink(missing_ok=True)
        raise
