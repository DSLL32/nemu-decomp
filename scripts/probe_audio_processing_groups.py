#!/usr/bin/env python3
"""Compile scratch candidates for the inferred eleventh/twelfth audio C units.

This is an emission/source-visibility experiment, not a canonical build or a
byte-exact success test. No artificial references or patched code are used.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_gs import COFF, compare_object
from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/audio/ahle_ds'
HELPER = 'DecodeAudioADPCMBlock'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def windows(path):
    return 'Z:' + str(path.resolve()).replace('/', '\\')


def between(text, first, following):
    assert text.count(first) == text.count(following) == 1
    return text[text.index(first):text.index(following)]


def groups():
    processing = (SOURCE / 'AudioProcessing.c').read_text()
    adpcm = (SOURCE / 'AudioADPCM.c').read_text()
    early = between(processing, '/* Audio sample', 'void ScaleAudioBuffer(')
    for name in ('AudioSSE.c', 'AudioEnvelope.c'):
        early += (SOURCE / name).read_text() + '\n'
    early += between(processing, 'void ScaleAudioBuffer(', 'void LoadAudioHistory(')
    early += between(processing, 'void FilterAudioBuffer(', 'void DumpAudioCommand(')
    early += (SOURCE / 'AudioProfile.c').read_text() + '\n'
    early += processing[processing.index('void DumpAudioCommand('):]
    early += adpcm[:adpcm.index('void DecodeAudioADPCM(')]
    early += (SOURCE / 'AudioPoleFilter.c').read_text()
    late = '#include "AudioProcessing.h"\n#include "AudioResampleTables.h"\n#include <string.h>\n'
    for name in ('AudioResample.c', 'AudioADPCMWide.c', 'AudioADPCMLegacy.c'):
        late += (SOURCE / name).read_text() + '\n'
    late += between(processing, 'void LoadAudioHistory(', 'void FilterAudioBuffer(')
    late += adpcm[adpcm.index('__forceinline void DecodeAudioADPCMBlock('):]
    return early, late


def emission(obj):
    coff = COFF(obj)
    helpers = [f for f in coff.functions() if f['name'] == '_' + HELPER]
    if not helpers:
        return dict(emitted=False)
    function, = helpers
    section = coff.sections[function['section'] - 1]
    symbol, = [s for s in coff.symbols if s['name'] == '.text' and
               s['section'] == section['index'] and s['auxiliary']]
    return dict(emitted=True, bytes=len(function['data']),
                raw_sha256=hashlib.sha256(function['data']).hexdigest(),
                storage_class=function['storage_class'],
                comdat_selection=symbol['auxiliary'][0][14],
                section=section['index'], characteristics=hex(section['characteristics']))


def coefficient_data(obj):
    coff = COFF(obj)
    original = PE32(ROOT / 'master/Plugin/audio.dll')
    result = []
    for index, name in enumerate(('AudioResampleKernel', 'AudioResampleTable',
                                  'AudioResampleShift1', 'AudioResampleShift2',
                                  'AudioResampleShift3')):
        symbols = [s for s in coff.symbols if s['name'] == '_' + name and s['section'] > 0]
        if not symbols:
            continue
        symbol, = symbols
        section = coff.sections[symbol['section'] - 1]
        candidate = section['data'][symbol['value']:symbol['value'] + 512]
        va = 0x1003c8d0 + index * 512
        offset = original.offset(va - original.base)
        target = original.data[offset:offset + 512]
        result.append(dict(name=name, original_va=hex(va), bytes=512,
                           section=section['name'], offset=symbol['value'],
                           writable=bool(section['characteristics'] & 0x80000000),
                           exact_bytes=candidate == target,
                           original_sha256=hashlib.sha256(target).hexdigest(),
                           candidate_sha256=hashlib.sha256(candidate).hexdigest()))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scratch', required=True, type=Path)
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    scratch, prefix = args.scratch.resolve(), args.prefix.resolve()
    allowed = Path.home() / '.ai-tmp'
    assert scratch.is_relative_to(allowed) and prefix.is_relative_to(allowed)
    scratch.mkdir(parents=True, exist_ok=True)
    rows = {}
    maps = ['audio-addresses.json', 'audio-envelope-addresses.json',
            'audio-adpcm-wide-addresses.json', 'audio-adpcm-legacy-addresses.json',
            'audio-processing-addresses.json']
    for name in maps:
        for row in json.loads((ROOT / 'analysis' / name).read_text())['symbols']:
            rows[row['name']] = row
    symbols = scratch / 'groups-addresses.json'
    symbols.write_text(json.dumps(dict(symbols=list(rows.values())), indent=2) + '\n')
    early, late = groups()
    variants = [('Group11Force', early, '__forceinline '),
                ('Group11Inline', early, '__inline '),
                ('Group11External', early, ''),
                ('Group12Force', late, '__forceinline '),
                ('Group12Inline', late, '__inline '),
                ('Group12Static', late, 'static __forceinline ')]
    generated = []
    for name, source, declaration in variants:
        path = scratch / (name + '.c')
        marker = '__forceinline void ' + HELPER + '('
        assert source.count(marker) == 1
        path.write_text(source.replace(marker, declaration + 'void ' + HELPER + '('))
        generated.append(path)
    command = [sys.executable, str(ROOT / 'scripts/run_toolchain.py'), 'vc7', 'cl',
               '/nologo', '/O2', '/Ob1', '/MT', '/c', '/I' + windows(SOURCE),
               *map(windows, generated), '/Fo' + windows(scratch) + '\\']
    environment = dict(os.environ, WINEPREFIX=str(prefix))
    process = subprocess.run(command, cwd=ROOT, env=environment, capture_output=True)
    log = scratch / 'groups-compile.log'
    log.write_bytes(process.stdout + process.stderr)
    if process.returncode:
        raise RuntimeError(f'Compiler failed; see {log}')
    results = []
    for source in generated:
        obj = source.with_suffix('.obj')
        comparison = compare_object(obj, ROOT / 'master/Plugin/audio.dll', symbols)
        comparison['generated_source_sha256'] = digest(source)
        comparison['helper_emission'] = emission(obj)
        comparison['coefficient_data'] = coefficient_data(obj)
        results.append(comparison)
        print(source.stem, comparison['relocation_masked_matches'], '/',
              len(comparison['functions']), 'masked matches;',
              comparison['helper_emission'])
    paths = list(SOURCE.glob('*.h')) + [SOURCE / n for n in (
        'AudioProcessing.c', 'AudioSSE.c', 'AudioEnvelope.c', 'AudioProfile.c',
        'AudioADPCM.c', 'AudioPoleFilter.c', 'AudioResample.c',
        'AudioADPCMWide.c', 'AudioADPCMLegacy.c')]
    paths += [ROOT / 'analysis' / n for n in maps]
    paths += [Path(__file__).resolve(), ROOT / 'scripts/verify_gs.py']
    paths += [ROOT / 'toolchains/vc7/bin' / n for n in ('cl.exe', 'c1.dll', 'c2.dll')]
    report = dict(status='incomplete_translation_unit_and_visibility_probe',
                  original_sha256=digest(ROOT / 'master/Plugin/audio.dll'),
                  hashes={str(p.relative_to(ROOT)): digest(p) for p in paths},
                  command=command, compile_log=str(log), compile_log_sha256=digest(log),
                  objects=results,
                  limitations=[
                      'Live source files are unchanged. These are scratch hypotheses for inferred original units 11 and 12.',
                      'No candidate was linked; duplicate public COMDAT selection and final ownership are not proven.',
                      'Private compiler-generated constant/scope names change in amalgamation; unresolved relocation identities remain explicit.',
                      'Matched opcode extents and emitted helper bodies do not resolve the remaining processing/resampling/ADPCM mismatches.',
                      'The ordinary external helper in unit11 emits its exact body; unused inline/forceinline definitions disappear.',
                      'Unit12 ordinary inline leaves calls and a 224-byte dispatcher; forced inline retains the incomplete 864-byte dispatcher.',
                      'Static forceinline still emits an unused private helper with different calling-convention code, so that control is not an ownership solution.'
                  ])
    args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
