#!/usr/bin/env python3
"""Compile and audit the inferred eleventh/twelfth audio C translation units.

The earlier six-way inline ownership experiment is frozen at commit 2027a7c
and in analysis/audio-processing-groups.json. This checks the coherent source
layout: one ordinary signed decoder and two genuine macro expansions.
Remaining mismatched kernels keep this an incomplete reconstruction diagnostic.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from verify_audio import compare_objects, load_symbols
from verify_gs import COFF
from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/audio/ahle_ds'
UNITS = ('AudioProcessing', 'AudioResample')

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def windows(path):
    return 'Z:' + str(path.resolve()).replace('/', '\\')

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
    inputs = [SOURCE / (name + '.c') for name in UNITS] + sorted(SOURCE.glob('*.h'))
    for source in inputs:
        shutil.copyfile(source, scratch / source.name)
    command = [sys.executable, str(ROOT / 'scripts/run_toolchain.py'), 'vc7', 'cl',
               '/nologo', '/O2', '/Ob1', '/MT', '/c',
               *[windows(scratch / (n + '.c')) for n in UNITS],
               '/Fo' + windows(scratch) + '\\']
    environment = dict(os.environ, WINEPREFIX=str(prefix))
    process = subprocess.run(command, cwd=ROOT, env=environment, capture_output=True)
    log = scratch / 'groups-compile.log'
    log.write_bytes(process.stdout + process.stderr)
    if process.returncode:
        raise RuntimeError(f'Compiler failed; see {log}')
    objects = [scratch / (name + '.obj') for name in UNITS]
    results = compare_objects(objects)
    original = PE32(ROOT / 'master/Plugin/audio.dll')
    data, alignment, owners = [], [], {}
    records = {r['name']: r for r in load_symbols() if r.get('alignment_span')}
    for obj, va in zip(objects, (0x1003b7d0, 0x1003c050)):
        coff = COFF(obj)
        sections = [s for s in coff.sections if s['name'] == '.data']
        section, = sections
        target_offset = original.offset(va - original.base)
        target = original.data[target_offset:target_offset + section['size']]
        data.append(dict(object=obj.name, original_va=hex(va), bytes=section['size'],
                         writable=bool(section['characteristics'] & 0x80000000),
                         exact_bytes=section['data'] == target,
                         original_sha256=hashlib.sha256(target).hexdigest(),
                         candidate_sha256=hashlib.sha256(section['data']).hexdigest()))
        for function in coff.functions():
            owners.setdefault(function['name'], []).append(obj.name)
            if function['name'] not in records:
                continue
            record = records[function['name']]
            start = function['value'] + function['size']
            size = record['alignment_span'] - function['size']
            if size <= 0:
                continue
            raw = coff.sections[function['section'] - 1]['data'][start:start + size]
            original_start = original.offset(record['va'] - original.base) + function['size']
            expected = original.data[original_start:original_start + size]
            alignment.append(dict(name=function['name'], bytes=size,
                                  status='coff_bytes' if len(raw) == size else 'requires_final_link_alignment',
                                  complete_alignment_equal=raw == expected if len(raw) == size else None,
                                  original_hex=expected.hex(), candidate_hex=raw.hex()))
    duplicates = {name: units for name, units in owners.items() if len(units) > 1}
    assert not duplicates
    assert owners['_DecodeAudioADPCMBlock'] == ['AudioProcessing.obj']
    early = COFF(objects[0])
    definitions = {s['name']: s for s in early.symbols}
    lookup = definitions['_AudioSqrtMantissa']
    uninitialized = dict(object=objects[0].name, section='.bss',
                         bytes=early.sections[lookup['section'] - 1]['size'],
                         lookup_offset=lookup['value'],
                         wet_offset=definitions['_AudioEnvelopeWet']['value'],
                         dry_offset=definitions['_AudioEnvelopeDry']['value'],
                         lookup_private=lookup['storage_class'] == 3,
                         proof='analysis/audio-sqrt-storage.json')
    assert uninitialized['bytes'] == 0x40008
    assert (uninitialized['lookup_offset'], uninitialized['wet_offset'],
            uninitialized['dry_offset']) == (0, 0x40000, 0x40004)
    paths = inputs + [Path(__file__).resolve(), ROOT / 'scripts/verify_gs.py',
                     ROOT / 'scripts/verify_audio.py']
    paths += [ROOT / 'toolchains/vc7/bin' / name for name in ('cl.exe', 'c1.dll', 'c2.dll')]
    report = dict(status='incomplete_native_source_groups',
                  original_sha256=digest(ROOT / 'master/Plugin/audio.dll'),
                  hashes={str(p.relative_to(ROOT)): digest(p) for p in paths},
                  command=command, compile_log=str(log), compile_log_sha256=digest(log),
                  objects=results, initialized_data=data, alignment=alignment,
                  uninitialized_data=uninitialized,
                  coefficient_data=coefficient_data(objects[1]),
                  function_owners=owners, duplicate_function_definitions=duplicates,
                  limitations=[
                      'Complete DLL byte equality remains unproven.',
                      'Four incomplete kernels retain their actual differing bytes and operand failures.',
                      'Macro ownership is inferred from repeated algorithm code and coherent ordinary linkage; original macro names are unknown.',
                      'The preceding external-plus-inline ownership probe failed LNK2005; no duplicate fallback or forced linker option is used here.'
                  ])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    for obj in results:
        print(Path(obj['object']).name, obj['relocation_masked_matches'], '/',
              len(obj['functions']), 'complete bodies match outside relocations')
    print('Initialized .data:', sum(r['bytes'] for r in data), 'bytes;',
          'exact:', all(r['exact_bytes'] for r in data))
    return 0 if all(f.get('relocated_bytes_equal', False)
                    for obj in results for f in obj['functions']) else 1

if __name__ == '__main__':
    raise SystemExit(main())
