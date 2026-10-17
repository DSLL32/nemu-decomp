#!/usr/bin/env python3
"""Rebuild and compare audio.dll's recovered DirectSound/configuration/log units.

This is a contribution diagnostic, never a whole-DLL success check. Complete
aligned routine extents are compared, with COFF relocations reported separately.
The two readable resource definitions are checked against complete original
resource payloads. All generated artifacts and Wine state stay under ~/.ai-tmp.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

from verify_gs import COFF, TargetSymbols, compare_object, summarize_objects
from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/audio/ahle_ds'
ORIGINAL = ROOT / 'master/Plugin/audio.dll'
ORIGINAL_SHA256 = 'f05b3f9e239041758a4aca09a891e2ebebe4136a3b326f8bbc5c160d76c73808'
UNITS = ('DirectSound', 'AudioLog')
FLAGS = ['/nologo', '/O2', '/Ob1', '/MT', '/c']


def winpath(path):
    return "Z:" + str(path.resolve()).replace("/", "\\")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def original_bytes(pe, va, size):
    for section in pe.sections:
        begin = pe.base + section['rva']
        extent = max(section['virtual_size'], section['raw_size'])
        if begin <= va and va + size <= begin + extent:
            position = va - begin
            count = max(0, min(size, section['raw_size'] - position))
            start = section['offset'] + position
            return pe.data[start:start + count] + bytes(size - count)
    return None


def compare_data(path, pe, symbols):
    """Retain explicit attribution limits for common/BSS symbols and literals."""
    obj = COFF(path)
    rows = []
    for symbol in obj.symbols:
        if symbol['type'] == 0x20 or symbol['storage_class'] not in (2, 3):
            continue
        targets = symbols.candidates(symbol['name'], path.name)
        if len(targets) != 1:
            continue
        target = targets[0]
        if symbol['section'] == 0 and symbol['value']:
            size = symbol['value']
            data = bytes(size)
            kind = 'common_zero_initialized'
        elif symbol['section'] > 0:
            section = obj.sections[symbol['section'] - 1]
            following = [s['value'] for s in obj.symbols if s['section'] == symbol['section']
                         and s['storage_class'] in (2, 3) and s['value'] > symbol['value']]
            end = min(following + [section['size']])
            size = end - symbol['value']
            if target.get('size'):
                size = target['size']
                end = symbol['value'] + size
            data = section['data'][symbol['value']:end]
            if any(symbol['value'] <= r['offset'] < end for r in section['relocations']):
                rows.append(dict(name=symbol['name'], va=target['va'], size=size,
                                 kind='initialized_relocated_data', bytes_equal=None,
                                 note='Data pointer relocation requires integrated linker layout.'))
                continue
            kind = 'initialized_data_or_literal'
        else:
            continue
        expected = original_bytes(pe, target['va'], size)
        rows.append(dict(name=symbol['name'], va=target['va'], size=size, kind=kind,
                         bytes_equal=expected == data,
                         address_basis=target['address_basis'],
                         original_extent_independently_verified=bool(target.get('size')),
                         note=('Zero bytes validate initialization, not a unique symbol identity or original extent.'
                               if kind == 'common_zero_initialized' else None)))
    return rows


def compare_initialized_contribution(path, pe, symbols):
    """Check the complete source data contribution at its independently audited base.

    This resolves the two text pointers for comparison only. It neither rewrites
    an object nor asserts that the integrated linker selected these addresses.
    """
    bases = {'DirectSound.obj': 0x10038b18, 'AudioLog.obj': 0x100393c8}
    obj = COFF(path)
    section, = [s for s in obj.sections if s['name'] == '.data']
    base = bases[path.name]
    data = bytearray(section['data'])
    relocations = []
    for relocation in section['relocations']:
        target, = symbols.candidates(relocation['symbol']['name'], path.name)
        if relocation['type'] != 6 or target['address_basis'] != 'identified_symbol':
            raise ValueError('Initialized-data pointer lacks independent target attribution')
        offset = relocation['offset']
        addend = struct.unpack_from('<I', data, offset)[0]
        value = target['va'] + addend
        original_value = struct.unpack('<I', original_bytes(pe, base + offset, 4))[0]
        struct.pack_into('<I', data, offset, value)
        relocations.append(dict(offset=offset, name=relocation['symbol']['name'],
                                target_va=value, original_target_va=original_value,
                                target_equal=value == original_value,
                                address_basis=target['address_basis']))
    layout = []
    for symbol in obj.symbols:
        if symbol['section'] != section['index'] or symbol['name'] == '.data':
            continue
        target, = symbols.candidates(symbol['name'], path.name)
        layout.append(dict(name=symbol['name'], offset=symbol['value'],
                           va=target['va'], layout_equal=base + symbol['value'] == target['va']))
    expected = original_bytes(pe, base, len(data))
    return dict(original_va=base, size=len(data),
                original_sha256=hashlib.sha256(expected).hexdigest(),
                source_data_sha256=hashlib.sha256(section['data']).hexdigest(),
                resolved_data_sha256=hashlib.sha256(data).hexdigest(),
                resolved_bytes_equal=data == expected, relocations=relocations,
                symbol_layout=layout,
                note='Complete native .data contribution; linker alignment and final linked addresses remain integration checks.')


def resources(path, original):
    data = path.read_bytes()
    pos, result = 0, []
    original_spans = {(5, 101): (266400, 2386), (16, 1): (268792, 760)}
    while pos < len(data):
        size, header_size = struct.unpack_from('<II', data, pos)
        if size:
            type_marker, resource_type, name_marker, resource_id = struct.unpack_from('<4H', data, pos + 8)
            if type_marker != 0xffff or name_marker != 0xffff:
                raise ValueError('Unexpected named resource in reconstructed .res')
            language = struct.unpack_from('<H', data, pos + 22)[0]
            offset, original_size = original_spans[(resource_type, resource_id)]
            rebuilt = data[pos + header_size:pos + header_size + size]
            expected = original[offset:offset + original_size]
            result.append(dict(type=resource_type, id=resource_id, language=language,
                               original_file_offset=offset, original_size=original_size,
                               rebuilt_size=size, payload_byte_exact=expected == rebuilt,
                               sha256=hashlib.sha256(rebuilt).hexdigest()))
        pos = (pos + header_size + size + 3) & ~3
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = args.work.resolve() if args.work else Path(tempfile.mkdtemp(prefix='audio-dsound-check.', dir=parent))
    if not scratch.is_relative_to(parent.resolve()):
        parser.error('--work must be under ~/.ai-tmp')
    scratch.mkdir(parents=True, exist_ok=True)
    if sha(ORIGINAL) != ORIGINAL_SHA256:
        raise ValueError('Original audio.dll SHA-256 does not match the audited input')
    artifact = Path(tempfile.mkdtemp(prefix='run.', dir=scratch))
    snapshot = artifact / 'source'
    snapshot.mkdir()
    source_files = [SOURCE / (unit + '.c') for unit in UNITS] + [SOURCE / 'DirectSound.h', SOURCE / 'transform.h', SOURCE / 'Audio.rc']
    for source in source_files:
        shutil.copyfile(source, snapshot / source.name)
    env = dict(os.environ, WINEPREFIX=str(scratch / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    command = ['python3', str(ROOT / 'scripts/run_toolchain.py'), 'vc7']
    commands = []
    for unit in UNITS:
        commands.append([*command, 'cl', *FLAGS, '/Fo' + winpath(artifact / (unit + '.obj')),
                         winpath(snapshot / (unit + '.c'))])
        subprocess.run(commands[-1], cwd=ROOT, env=env, check=True)
    commands.append([*command, 'rc', '/l', '0x409', '/fo' + winpath(artifact / 'Audio.res'),
                     winpath(snapshot / 'Audio.rc')])
    subprocess.run(commands[-1], cwd=ROOT, env=env, check=True)
    application = json.loads((ROOT / 'analysis/audio-dsound-addresses.json').read_text())
    library = json.loads((ROOT / 'analysis/audio-libraries.json').read_text())
    if library['original_sha256'] != ORIGINAL_SHA256:
        raise ValueError('Library attribution report belongs to another input')
    combined = artifact / 'symbols.json'
    combined.write_text(json.dumps(dict(symbols=application['symbols'] + library['symbols'])))
    objects = [compare_object(artifact / (unit + '.obj'), ORIGINAL, combined) for unit in UNITS]
    pe = PE32(ORIGINAL)
    symbols = TargetSymbols(pe, combined)
    report = dict(original=str(ORIGINAL.relative_to(ROOT)), original_sha256=ORIGINAL_SHA256,
                  status='partial_source_reconstruction_not_whole_dll_exact',
                  compiler_flags=FLAGS, work=str(scratch), artifact=str(artifact), commands=commands,
                  source_sha256={str(p.relative_to(ROOT)): sha(snapshot / p.name) for p in source_files},
                  tool_sha256={str(p.relative_to(ROOT)): sha(p) for p in [ROOT / 'toolchains/vc7/bin/cl.exe',
                                ROOT / 'toolchains/vc7/bin/c1.dll', ROOT / 'toolchains/vc7/bin/c2.dll',
                                ROOT / 'toolchains/vc7/bin/rc.exe']},
                  objects=objects, object_summary=summarize_objects(objects),
                  data={unit: compare_data(artifact / (unit + '.obj'), pe, symbols) for unit in UNITS},
                  initialized_contributions={unit: compare_initialized_contribution(
                      artifact / (unit + '.obj'), pe, symbols) for unit in UNITS},
                  resources=resources(artifact / 'Audio.res', pe.data),
                  notes=['Private filenames remain inferred; DirectSound/config grouping and the separate log unit are constrained by original initialized-data contributions and native compiler counts.',
                         'Function extents include original switch tables and natural compiler alignment.',
                         'Source uses original explicit sample-byte-swapping assembly and a compiler byte-swap intrinsic; no opcode blobs or instruction patching.',
                         'No whole-DLL build or byte equality is asserted by these contribution diagnostics.',
                         'Common/BSS dimensions without independent extent evidence remain inferred.'])
    output = args.report or scratch / 'comparison.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    (artifact / 'comparison.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Report: {output}')
    print(f"Complete aligned function-body matches: {sum(o['relocation_masked_matches'] for o in objects)}/{sum(len(o['functions']) for o in objects)}")
    print(f"Resource payloads exact: {all(r['payload_byte_exact'] for r in report['resources'])}")
    functions_equal = all(f.get('relocation_masked_equal') and not f.get('relocation_target_mismatch_count')
                          for o in objects for f in o['functions'])
    data_equal = all(c['resolved_bytes_equal'] and all(s['layout_equal'] for s in c['symbol_layout'])
                     for c in report['initialized_contributions'].values())
    return 0 if functions_equal and data_equal and all(r['payload_byte_exact'] for r in report['resources']) else 1


if __name__ == '__main__':
    raise SystemExit(main())
