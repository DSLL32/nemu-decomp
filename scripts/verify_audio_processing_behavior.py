#!/usr/bin/env python3
"""Compare audio processing COFF candidates with the original x86 routines.

This is a behavioral diagnostic, not a byte-exact DLL certification. Each
side runs in a fresh Unicorn instance. Ordinary COFF relocations are resolved
in memory for the test; this script does not generate or patch a DLL.
Unchanged external callees execute their original, separately mapped bodies.
"""

import argparse
import hashlib
import json
from pathlib import Path
import random
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP

from pe_fingerprints import PE32
from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]
RDRAM = 0x30000000
CONTEXT = 0x31000000
COMMAND = 0x31001000
BUFFER = 0x31010000
BOOK = 0x31020000
OUTPUT = 0x31030000
HISTORY = 0x31040000
STACK = 0x40080000
STOP = 0x60000000


def number(value):
    return int(value, 0) if isinstance(value, str) else value


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class Fixture:
    def __init__(self, object_directory):
        self.objects = Path(object_directory)
        self.original = PE32(ROOT / 'master/Plugin/audio.dll')
        document = json.loads((ROOT / 'analysis/audio-processing-addresses.json').read_text())
        self.addresses = {r['name']: number(r['va']) for r in document['symbols']}
        library = json.loads((ROOT / 'analysis/audio-libraries.json').read_text())
        if library['original_sha256'] != sha256(self.original.path):
            raise ValueError('Library mappings do not describe the current original DLL')
        for row in library['symbols']:
            self.addresses.setdefault(row['name'], number(row['va']))

    def machine(self, object_name=None):
        machine = Uc(UC_ARCH_X86, UC_MODE_32)
        machine.mem_map(self.original.base, 0x200000)
        for section in self.original.sections:
            machine.mem_write(self.original.base + section['rva'],
                              self.original.data[section['offset']:
                                                 section['offset'] + section['raw_size']])
        machine.mem_map(RDRAM, 0x800000)
        machine.mem_map(CONTEXT, 0x100000)
        machine.mem_map(0x40000000, 0x100000)
        machine.mem_map(STOP, 4096)
        symbols = dict(self.addresses)
        if object_name:
            coff = COFF(self.objects / (object_name + '.obj'))
            machine.mem_map(0x20000000, (len(coff.sections) + 1) * 0x100000)
            for index, section in enumerate(coff.sections):
                machine.mem_write(0x20000000 + index * 0x100000, section['data'])
            for symbol in coff.symbols:
                if symbol['section'] > 0:
                    symbols[symbol['name']] = (0x20000000 +
                        (symbol['section'] - 1) * 0x100000 + symbol['value'])
            for index, section in enumerate(coff.sections):
                for relocation in section['relocations']:
                    address = 0x20000000 + index * 0x100000 + relocation['offset']
                    addend = struct.unpack('<I', machine.mem_read(address, 4))[0]
                    value = symbols[relocation['symbol']['name']]
                    if relocation['type'] == 20:       # IMAGE_REL_I386_REL32
                        value -= address + 4
                    elif relocation['type'] == 7:      # IMAGE_REL_I386_DIR32NB
                        value -= 0x20000000
                    elif relocation['type'] != 6:      # IMAGE_REL_I386_DIR32
                        raise ValueError('Unsupported COFF relocation: ' + str(relocation))
                    machine.mem_write(address, struct.pack('<I', (value + addend) & 0xffffffff))
        return machine, symbols

    def prepare(self, machine, symbols, seed):
        rng = random.Random(seed)
        for address, size in [(BUFFER, 0x4000), (BOOK, 0x400), (OUTPUT, 0x1000),
                              (HISTORY, 0x100), (RDRAM, 0x2000)]:
            machine.mem_write(address, bytes(rng.randrange(256) for _ in range(size)))
        context = bytearray(0x70)
        struct.pack_into('<II', context, 0, BUFFER, BOOK)
        struct.pack_into('<iii', context, 12, 0x800, 0x200, 64)
        struct.pack_into('<hh', context, 0x24, 0x200, 0x800)
        struct.pack_into('<I', context, 0x30, 0x1200)
        for offset in range(0x34, 0x70, 4):
            struct.pack_into('<I', context, offset, rng.randrange(1 << 32))
        machine.mem_write(CONTEXT, bytes(context))
        machine.mem_write(self.addresses['_RDRAM'], struct.pack('<I', RDRAM))
        for name in ('_AudioEnvelopeWet', '_AudioEnvelopeDry'):
            machine.mem_write(symbols[name], struct.pack('<I', rng.randrange(1 << 32)))

    @staticmethod
    def call(machine, address, arguments):
        machine.mem_write(STACK, struct.pack('<' + 'I' * (len(arguments) + 1),
                                            STOP, *[v & 0xffffffff for v in arguments]))
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.emu_start(address, STOP, count=2000000)
        if machine.reg_read(UC_X86_REG_EIP) != STOP:
            raise ValueError('Instruction limit reached before the function returned')
        if machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise ValueError('Function did not preserve the expected cdecl stack')

    def compare(self, object_name, name, arguments, seed, label, writes=()):
        original, original_symbols = self.machine()
        candidate, candidate_symbols = self.machine(object_name)
        self.prepare(original, original_symbols, seed)
        self.prepare(candidate, candidate_symbols, seed)
        for address, data in writes:
            original.mem_write(address, data)
            candidate.mem_write(address, data)
        result = dict(function=name, case=label, seed=seed)
        try:
            self.call(original, original_symbols['_' + name], arguments)
            self.call(candidate, candidate_symbols['_' + name], arguments)
        except Exception as error:
            return dict(result, status='emulation_error', error=str(error))
        differences = []
        regions = [('buffer', BUFFER, 0x4000), ('rdram', RDRAM, 0x2000),
                   ('context', CONTEXT, 0x70), ('output', OUTPUT, 0x1000),
                   ('history', HISTORY, 0x100)]
        for region, address, size in regions:
            left, right = original.mem_read(address, size), candidate.mem_read(address, size)
            changed = [index for index in range(size) if left[index] != right[index]]
            if changed:
                differences.append(dict(region=region, count=len(changed),
                    first_offset=changed[0], original=left[changed[0]], candidate=right[changed[0]]))
        for name in ('_AudioEnvelopeWet', '_AudioEnvelopeDry'):
            if original.mem_read(original_symbols[name], 4) != candidate.mem_read(candidate_symbols[name], 4):
                differences.append(dict(region=name, count=4))
        return dict(result, status='different' if differences else 'matching_outputs',
                    differences=differences)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--objects', required=True, type=Path,
                        help='Directory with VC7 /O2 /Ob1 /MT candidate COFF files')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    fixture = Fixture(args.objects)
    results = []
    objects = ('AudioProcessing', 'AudioADPCM', 'AudioPoleFilter', 'AudioResample')
    for seed in (1, 7, 0x1234):
        for name, arguments in [
            ('ReadAudioSamples', [OUTPUT, 0x1000, 64]),
            ('LoadAudioEnvelope', [0x1000, CONTEXT]),
            ('SaveAudioEnvelope', [0x1000, CONTEXT]),
            ('ResetAudioEnvelope', [CONTEXT]),
            ('ScaleAudioBuffer', [0x200, 64, 0x180, CONTEXT]),
            ('FilterAudioBlock', [0x100, BOOK, CONTEXT]),
        ]:
            results.append(fixture.compare('AudioProcessing', name, arguments, seed, 'random samples'))
        for bits in (2, 4):
            for offset in (0x200, 0x201):
                results.append(fixture.compare('AudioADPCM', 'DecodeAudioADPCMBlock',
                    [OUTPUT, HISTORY, offset, BOOK, bits, CONTEXT], seed,
                    f'{bits}-bit block, offset {offset:#x}'))
        results.append(fixture.compare('AudioADPCM', 'DecodeAudioADPCMUnsigned',
            [OUTPUT, HISTORY, 0x200, BOOK, 4, CONTEXT], seed, 'unsigned predictor'))
        for flags in range(8):
            results.append(fixture.compare('AudioADPCM', 'DecodeAudioADPCM',
                [0x1000, 0x200, 0x800, 64, flags, CONTEXT], seed, f'flags {flags}'))
        for flags in range(4):
            for count in (0, 2, 32, 96):
                writes = [(COMMAND, struct.pack('<II', (flags << 16) | 0x8000, 0x1000)),
                          (RDRAM + 0x1008, struct.pack('<hh', 0x1234, seed & 7)),
                          (CONTEXT + 0x14, struct.pack('<i', count))]
                results.append(fixture.compare('AudioResample', 'ResampleAudioScalar',
                    [COMMAND, CONTEXT], seed, f'flags {flags}, bytes {count}', writes))
        for flags in (0, 1):
            for gain in (0, 0x1800, 0x8000, 0xffff):
                writes = [(COMMAND, struct.pack('<II', (flags << 16) | gain, 0x1000))]
                results.append(fixture.compare('AudioPoleFilter', 'PoleFilterAudioCommand',
                    [COMMAND, CONTEXT], seed, f'flags {flags}, gain {gain:#x}', writes))
    sources = [ROOT / 'src/audio/ahle_ds' / (name + '.c') for name in objects]
    sources += [ROOT / 'src/audio/ahle_ds/AudioProcessing.h', ROOT / 'src/audio/ahle_ds/AudioHLE.h']
    report = dict(status='behavioral_diagnostic_only',
        original='master/Plugin/audio.dll', original_sha256=sha256(fixture.original.path),
        objects=[dict(name=name, sha256=sha256(args.objects / (name + '.obj'))) for name in objects],
        sources=[dict(path=str(path.relative_to(ROOT)), sha256=sha256(path)) for path in sources],
        cases=len(results), matching=sum(r['status'] == 'matching_outputs' for r in results),
        limitations=['Finite synthetic vectors do not prove equivalence for every input.',
                     'External callees execute the original mapped bodies.',
                     'This does not establish byte equality or a complete DLL rebuild.'],
        results=results)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f"{report['matching']}/{report['cases']} audio processing behavior probes matched")
    for row in results:
        if row['status'] != 'matching_outputs':
            print(row)
    return 0 if report['matching'] == report['cases'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
