#!/usr/bin/env python3
"""Build and measure the native JPEG/MP3 reconstruction without modifying a PE.

This is a component diagnostic. A matching object contribution does not establish
whole-DLL identity; unmatched Mp3DCT and unresolved storage identities remain explicit.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from pe_fingerprints import PE32
from verify_gs import COFF, compare_object

ROOT = Path(__file__).resolve().parents[1]
UNITS = ('Jpeg', 'Mp3')
SOURCES = [ROOT / 'src/audio/ahle_ds' / (name + '.c') for name in UNITS]
DATA = (
    ('Jpeg', '_JpegZigZag', 0x1003ad48, 256),
    ('Jpeg', '_JpegCosine', 0x1003ae48, 256),
    ('Jpeg', '__real@4069000000000000', 0x10031110, 8),
    ('Jpeg', '__real@4000000000000000', 0x10031118, 8),
    ('Jpeg', '__real@3fc921fb54442d18', 0x10031120, 8),
    ('Jpeg', '_t_dewindow', 0x1003a4c8, 2176),
    ('Jpeg', '_b1', 0x10031090, 124),
    ('Jpeg', '_nch', 0x1003af48, 4),
    ('Mp3', '_t_dewindow', 0x1003af50, 2176),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def winpath(path):
    return 'Z:' + str(path.resolve()).replace('/', '\\')


def run(output):
    env = os.environ.copy()
    env.update(WINEPREFIX=str(output / 'wine'), WINEDEBUG='-all')
    flags = ['/nologo', '/c', '/O2', '/Ob1', '/MT']
    for source in SOURCES:
        subprocess.run([sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
                        'vc7', 'cl', *flags,
                        '/Fo' + winpath(output / (source.stem + '.obj')),
                        winpath(source)], cwd=ROOT, env=env, check=True)
    original = ROOT / 'master/Plugin/audio.dll'
    image = PE32(original)
    address_map = ROOT / 'analysis/audio-mp3-addresses.json'
    reports = [compare_object(output / (unit + '.obj'), original, address_map)
               for unit in UNITS]
    objects = {unit: COFF(output / (unit + '.obj')) for unit in UNITS}
    data = []
    for unit, name, address, size in DATA:
        obj = objects[unit]
        symbol = next(s for s in obj.symbols if s['name'] == name and s['section'] > 0)
        section = obj.sections[symbol['section'] - 1]
        rebuilt = section['data'][symbol['value']:symbol['value'] + size]
        offset = image.offset(address - image.base)
        target = image.data[offset:offset + size]
        data.append(dict(unit=unit, symbol=name, va=hex(address), size=size,
                         equal=rebuilt == target,
                         rebuilt_sha256=hashlib.sha256(rebuilt).hexdigest(),
                         original_sha256=hashlib.sha256(target).hexdigest(),
                         unique_raw_occurrence=image.data.count(target) == 1,
                         relocation_fields=sum(symbol['value'] <= r['offset'] <
                                               symbol['value'] + size
                                               for r in section['relocations'])))
    # This single initialized contribution establishes shared translation-unit
    # ownership independently of any whole-DLL linker placement.
    jpeg = objects['Jpeg']
    layout = [('_t_dewindow', 0), ('_JpegZigZag', 0x880),
              ('_JpegCosine', 0x980), ('_nch', 0xa80)]
    symbols = [next(s for s in jpeg.symbols if s['name'] == name and s['section'] > 0)
               for name, _ in layout]
    section = jpeg.sections[symbols[0]['section'] - 1]
    expected_offset = image.offset(0x3a4c8)
    contiguous = dict(
        unit='Jpeg', original_va='0x1003a4c8', size=0xa84,
        symbol_offsets={symbol['name']: symbol['value'] for symbol in symbols},
        symbol_offsets_match=all(symbol['section'] == symbols[0]['section'] and
                                 symbol['value'] == offset
                                 for symbol, (_, offset) in zip(symbols, layout)),
        entire_section_equal=section['data'] == image.data[expected_offset:expected_offset + 0xa84],
        section_sha256=hashlib.sha256(section['data']).hexdigest())
    # Original operands independently identify these offsets. Private C storage
    # explains the shared contribution; inferred names still affect its order.
    bss_layout = [('_JpegBlock', 0), ('_JpegOutput', 0x600),
                  ('_JpegQuantizationCb', 0xa00), ('_JpegPixels', 0xb00),
                  ('_JpegQuantizationCr', 0x1b00), ('?u@?1??poly@@9@9', 0x1c00),
                  ('_JpegQuantizationY', 0x2d00),
                  ('?initialized@?1??JpegInverseDCT@@9@9', 0x2e00),
                  ('?initialized@?1??JpegInverseDCTAlternate@@9@9', 0x2e04),
                  ('?u_start@?1??poly@@9@9', 0x2e08),
                  ('?u_div@?1??poly@@9@9', 0x2e10)]
    bss_symbols = [next(s for s in jpeg.symbols if s['name'] == name and s['section'] > 0)
                   for name, _ in bss_layout]
    bss = jpeg.sections[bss_symbols[0]['section'] - 1]
    storage = dict(
        unit='Jpeg', original_va='0x100e8fd8', original_size=0x2e18,
        rebuilt_size=bss['size'], size_equal=bss['size'] == 0x2e18,
        private_symbols=[dict(symbol=s['name'], rebuilt_offset=s['value'],
                              original_offset=offset,
                              equal=s['section'] == bss['index'] and s['value'] == offset)
                         for s, (_, offset) in zip(bss_symbols, bss_layout)],
        symbol_offsets_match=all(s['section'] == bss['index'] and s['value'] == offset
                                 for s, (_, offset) in zip(bss_symbols, bss_layout)),
        notes='Private ownership and extent are supported; original private names and array order remain unresolved.')
    functions = [f for r in reports for f in r['functions']]
    result = dict(
        status='partial_native_source_reconstruction',
        original=str(original.relative_to(ROOT)), original_sha256=digest(original),
        compiler_flags=flags,
        compiler={str(p.relative_to(ROOT)): digest(p)
                  for p in [ROOT / 'toolchains/vc7/bin' / n
                            for n in ('cl.exe', 'c1.dll', 'c2.dll')]},
        source_sha256={str(p.relative_to(ROOT)): digest(p)
                       for p in [*SOURCES, ROOT / 'src/audio/ahle_ds/transform.h']},
        address_map_sha256=digest(address_map), objects=reports, initialized_data=data,
        contiguous_initialized_layout=contiguous,
        private_uninitialized_layout=storage,
        summary=dict(functions=len(functions),
                     original_function_bytes=sum(f.get('original_bytes', 0) for f in functions),
                     matching_complete_extents=sum(f.get('relocation_masked_equal', False)
                                                   for f in functions),
                     matching_complete_extent_bytes=sum(f.get('original_bytes', 0)
                                                         for f in functions
                                                         if f.get('relocation_masked_equal')),
                     matching_extents_with_consistent_symbolic_relocations=sum(
                         f.get('inferred_relocated_bytes_equal', False) for f in functions),
                     initialized_data_bytes=sum(r['size'] for r in data),
                     initialized_data_equal=all(r['equal'] for r in data)),
        missing=[],
        unmatched=[f['name'] for f in functions if not f.get('relocation_masked_equal')],
        whole_dll_byte_exact=False,
        notes=[
            'Code comparisons cover complete function extents, including stock compiler alignment.',
            'Only recorded COFF relocation fields are masked; local branches and SIB bytes are compared.',
            'Reconstruction names and BSS addresses inferred from operands remain explicitly labelled.',
            'Independent initialized-data comparisons do not identify zero-filled storage.',
            'No target bytes are injected into compiler output or emitted as executable data.',
        ])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path)
    parser.add_argument('--report', type=Path, default=ROOT / 'analysis/audio-mp3-comparison.json')
    args = parser.parse_args()
    if args.output_dir is None:
        parent = Path.home() / '.ai-tmp'
        parent.mkdir(exist_ok=True)
        output = Path(tempfile.mkdtemp(prefix='audio-mp3.', dir=parent))
    else:
        output = args.output_dir.resolve()
        output.mkdir(parents=True, exist_ok=True)
    if not output.is_relative_to((Path.home() / '.ai-tmp').resolve()) or output == (Path.home() / '.ai-tmp').resolve():
        parser.error('--output-dir must be a folder below ~/.ai-tmp')
    result = run(output)
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result['summary'], indent=2))
    print('Whole DLL byte exact: false; component reconstruction remains incomplete.')


if __name__ == '__main__':
    main()
