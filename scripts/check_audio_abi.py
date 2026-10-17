#!/usr/bin/env python3
"""Check the five reconstructed audio ABI units, without claiming DLL equality."""
import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from pe_fingerprints import PE32
from verify_audio import load_symbols
from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]
UNITS = [('ABI1', 0x10035040, 2176), ('ABI2', 0x100358C0, 2176),
         ('AudioHLE', 0x10036140, 2252), ('ABI3', 0x10036A10, 6276),
         ('AudioLegacy', 0x10038298, 2176)]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def literal_address(pe, value):
    offsets = []
    start = 0
    while True:
        start = pe.data.find(value, start)
        if start < 0:
            break
        offsets.append(start)
        start += 1
    if len(offsets) != 1:
        raise ValueError('Data pointer literal does not have a unique original identity')
    offset = offsets[0]
    for section in pe.sections:
        if section['offset'] <= offset < section['offset'] + section['raw_size']:
            return pe.base + section['rva'] + offset - section['offset']
    raise ValueError('Literal lies outside the original sections')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    work = args.work.resolve() if args.work else Path(tempfile.mkdtemp(prefix='audio-abi-check.', dir=parent))
    if not work.is_relative_to(parent.resolve()) or work == parent.resolve():
        parser.error('--work must be below ~/.ai-tmp')
    work.mkdir(parents=True, exist_ok=True)
    build_report = work / 'build-comparison.json'
    command = [sys.executable, str(ROOT / 'scripts/build_audio.py'), '--work', str(work),
               '--objects-only', '--report', str(build_report)]
    for unit, _, _ in UNITS:
        command.extend(['--unit', unit])
    result = subprocess.run(command)
    build = json.loads(build_report.read_text())
    artifact = Path(build['artifact'])
    pe = PE32(ROOT / 'master/Plugin/audio.dll')
    rows = load_symbols(artifact / 'symbols')
    addresses = {row['name']: int(row['va'], 0) if isinstance(row['va'], str) else row['va']
                 for row in rows}
    data_checks = []
    for unit, address, size in UNITS:
        obj = COFF(Path(build['work']) / (unit + '.obj'))
        section = next(s for s in obj.sections if s['name'] == '.data')
        data = bytearray(section['data'])
        bindings = []
        for relocation in section['relocations']:
            if relocation['type'] != 6:
                raise ValueError('Unexpected initialized-data relocation')
            symbol = relocation['symbol']
            target = addresses.get(symbol['name'])
            basis = 'independent symbol identity'
            if target is None and symbol['section'] > 0 and symbol['name'].startswith('??_C@'):
                value = obj.sections[symbol['section'] - 1]['data'][symbol['value']:]
                value = value[:value.index(0) + 1]
                target = literal_address(pe, value)
                basis = 'unique complete source literal'
            if target is None:
                raise ValueError('Unresolved initialized-data symbol: ' + symbol['name'])
            offset = relocation['offset']
            target += struct.unpack_from('<I', data, offset)[0]
            struct.pack_into('<I', data, offset, target)
            bindings.append(dict(offset=offset, symbol=symbol['name'], target=target, basis=basis))
        expected = pe.data[pe.offset(address - pe.base):][:size]
        data_checks.append(dict(unit=unit, original_va=address, expected_bytes=size,
                                rebuilt_bytes=len(data), byte_exact=bytes(data) == expected,
                                sha256=hashlib.sha256(data).hexdigest(), relocations=bindings))
    functions = [f for obj in build['objects'] for f in obj['functions']]
    counts = collections.Counter(f['name'] for f in functions)
    unique = {f['name']: f for f in functions}
    duplicates = {name: count for name, count in counts.items() if count > 1}
    success = (result.returncode == 0 and len(unique) == 100 and
               duplicates == {'_ClearAudioBuffer': 2} and
               all(d['byte_exact'] for d in data_checks) and
               sum(f['original_bytes'] for f in unique.values()) == 0x2C40)
    report = dict(scope='Complete native contributions and initialized data of five inferred C units; not DLL acceptance.',
                  full_dll_byte_exact=False, diagnostic_passed=success, artifact=str(artifact),
                  build_report_sha256=digest(artifact / 'comparison.json'),
                  checker_sha256=digest(Path(__file__)), command=command,
                  unique_function_count=len(unique), native_bytes=0x2C40,
                  duplicate_inline_definitions=duplicates,
                  instruction_matches=sum(f.get('relocation_masked_equal', False) for f in unique.values()),
                  independently_verified_matches=sum(f.get('relocated_bytes_equal', False) for f in unique.values()),
                  data=data_checks,
                  limitations=['Private filenames, names and shared inline header placement are inferred.',
                               'ClearAudioBuffer emits identical ANY COMDAT definitions in ABI1 and Legacy; the complete DLL link must select ABI1.',
                               'Twelve function contributions retain explicitly inferred global identities in the scoped symbol map.'])
    destination = artifact / 'abi-comparison.json'
    destination.write_text(json.dumps(report, indent=2) + '\n')
    if args.report:
        args.report.write_text(destination.read_text())
    print('Unique complete native spans:', report['instruction_matches'], '/', len(unique))
    print('Initialized data:', sum(d['rebuilt_bytes'] for d in data_checks), 'bytes; exact:', all(d['byte_exact'] for d in data_checks))
    print('Frozen report:', destination)
    return 0 if success else 1


if __name__ == '__main__':
    raise SystemExit(main())
