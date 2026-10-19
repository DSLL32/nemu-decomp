"""Read x86 FPO records and FrameData recipes from MSVC 2002 linker PDBs at build time.

PDBs remain build artifacts. The executable receives compact records in its
own resource, and never needs a PDB reader or debugging DLL at runtime.
"""
import struct
from pathlib import Path


def streams(path):
    data = Path(path).read_bytes()
    if not data.startswith(b'Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0'):
        raise ValueError('Expected MSF 7 PDB')
    page, _, count, directory_size, _, root = struct.unpack_from('<IIIIII', data, 32)
    if page < 512 or page & (page-1) or count > len(data)//page or directory_size > len(data):
        raise ValueError('Invalid PDB superblock')
    def pages(numbers, size):
        if any(n >= count for n in numbers):
            raise ValueError('Invalid PDB page')
        return b''.join(data[n*page:(n+1)*page] for n in numbers)[:size]
    directory_pages = (directory_size+page-1)//page
    if directory_pages*4 > page:
        raise ValueError('Unsupported oversized PDB directory page map')
    numbers = struct.unpack_from('<'+'I'*directory_pages, data, root*page)
    directory = pages(numbers, directory_size)
    total = struct.unpack_from('<I', directory)[0]
    if total > (len(directory)-4)//4:
        raise ValueError('Invalid PDB stream directory')
    sizes = struct.unpack_from('<'+'I'*total, directory, 4)
    offset, result = 4+4*total, []
    for size in sizes:
        if size == 0xffffffff:
            result.append(None)
            continue
        blocks = (size+page-1)//page
        numbers = struct.unpack_from('<'+'I'*blocks, directory, offset)
        offset += blocks*4
        result.append(pages(numbers, size))
    return result


def fpo_records(path):
    records = streams(path)
    dbi = records[3]
    if not dbi or len(dbi) < 64 or struct.unpack_from('<i', dbi)[0] != -1:
        raise ValueError('Missing PDB DBI header')
    mod, contrib, mapping, files, types = struct.unpack_from('<IIIII', dbi, 24)
    debug_size, ec = struct.unpack_from('<II', dbi, 48)
    offset = 64+mod+contrib+mapping+files+types+ec
    if offset+debug_size > len(dbi) or debug_size < 2:
        raise ValueError('Missing PDB debug stream index')
    indices = struct.unpack_from('<'+'H'*(debug_size//2), dbi, offset)
    fpo = b'' if indices[0] == 0xffff else records[indices[0]]
    if fpo is None or len(fpo)%16:
        raise ValueError('Malformed classic FPO records')
    result = list(struct.iter_unpack('<IIIHH', fpo))
    result.sort()
    return result, dict(classic=len(result), new_fpo_stream=indices[9] if len(indices)>9 else 0xffff)


def frame_records(path):
    """Return extended FrameData records and their compiler unwind programs."""
    data = streams(path)
    dbi = data[3]
    sizes = struct.unpack_from('<IIIII', dbi, 24)
    debug_size, ec = struct.unpack_from('<II', dbi, 48)
    offset = 64+sum(sizes)+ec
    indices = struct.unpack_from('<'+'H'*(debug_size//2), dbi, offset)
    if len(indices) <= 9 or indices[9] == 0xffff:
        raise ValueError('The linker PDB has no extended x86 FPO recipes')
    frames = data[indices[9]]
    if frames is None or len(frames)%32:
        raise ValueError('Malformed x86 FrameData stream')
    # The PDB /names stream has its own unambiguous signature and version.
    tables = [s for s in data if s and len(s)>=12 and s[:8] == struct.pack('<II',0xeffeeffe,1)]
    if len(tables) != 1:
        raise ValueError('Missing/ambiguous PDB string table')
    strings = tables[0]
    length = struct.unpack_from('<I',strings,8)[0]
    if length > len(strings)-12:
        raise ValueError('Truncated PDB string table')
    strings = strings[12:12+length]
    result = []
    for record in struct.iter_unpack('<IIIIIIHHI',frames):
        begin,size,locals_,params,maximum,program,prolog,saved,flags = record
        if not size or program >= len(strings):
            raise ValueError('Invalid x86 FrameData record')
        end = strings.find(b'\0',program)
        if end < 0 or end-program > 2048:
            raise ValueError('Invalid x86 unwind recipe')
        result.append((begin,size,locals_,params,maximum,strings[program:end],prolog,saved,flags))
    return sorted(result)


def frame_table(path):
    """A runtime-friendly index; recipes are shared and independently bounded."""
    programs = bytearray(b'\0'); offsets = {}; output = []
    for begin,size,locals_,params,maximum,program,prolog,saved,flags in frame_records(path):
        if program not in offsets:
            offsets[program] = len(programs);programs.extend(program+b'\0')
        output.append(struct.pack('<IIIIIIHHI',begin,size,locals_,params,maximum,offsets[program],prolog,saved,flags))
    header = struct.pack('<8sIII',b'NFPO0001',len(output),20+len(output)*32,len(programs))
    return header+b''.join(output)+programs
