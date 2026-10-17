#!/usr/bin/env python3
"""Compare original and rebuilt ProcessPIF behavior in controlled packet cases.

Both complete PE images execute in Unicorn. Application code is unchanged;
only the input-plugin callbacks and MessageBoxA are modeled at their external
boundaries. The original DisableSaveWrites flag prevents filesystem writes.
This is behavioral evidence, not evidence of executable byte equality.
Movie files and the encrypted CIC challenge path are outside this test set.
Malformed packets are labeled separately; executions stopped by the write
guard are observations of invalid-input behavior, not successful comparisons.
"""
import argparse
from collections import Counter
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct

from compare_nemu_functions import aliases, image_imports, read_map
from pe_fingerprints import PE32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

ROOT = Path(__file__).resolve().parents[1]
STACK, RETURN = 0x10000000, 0x20000000
JOY, RUMBLE, MESSAGE = RETURN + 0x100, RETURN + 0x200, RETURN + 0x300
METADATA = ('nemu-pif', 'nemu-plugins', 'nemu-memory', 'nemu-cpu')
ARRAY_SIZES = dict(PIFRam=64, PIFResponse=64, PIFCommand=64, PackBlock=32,
    PIFCommandLengths=2048, Controllers=32, EEPROMData=2048, MemPackData=32768)
SCALARS = ('CPU_PC', 'NetPlayActive', 'NetPlayPlayerCount', 'EEPROMEnabled',
    'EEPROMSize', 'CICState', 'MovieRecord', 'MoviePlay', 'MovieFile',
    'MovieNewRun', 'MovieLastButtons', 'ForcePIFSuccess', 'MemPackUsed',
    'EEPROMUsed', 'SRAMUsed', 'CIC6105Table', 'DisableSaveWrites')
REGISTERS = (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
             UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)
SEED = 0x4e454d55
LENGTHS = {0: (1, 3), 1: (1, 4), 2: (3, 33), 3: (35, 1),
           4: (2, 8), 5: (10, 1), 255: (1, 3)}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def original_symbols():
    result = {}
    for name in METADATA:
        metadata = json.loads((ROOT / 'analysis' / (name + '.json')).read_text())
        for kind in ('globals', 'functions'):
            rows = metadata.get(kind, [])
            if isinstance(rows, dict):
                rows = [dict(value, name=key) for key, value in rows.items()]
            for row in rows:
                address = row.get('va', row.get('address'))
                if address is not None:
                    result[row['name']] = (int(address, 0)
                                          if isinstance(address, str) else address)
    return result


def rebuilt_symbols(path):
    result = {}
    for row in read_map(path):
        for name in aliases(row['name']):
            result[name] = row['va']
    return result


class Image:
    def __init__(self, path, symbols):
        self.pe, self.symbols = PE32(path), symbols
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        end = max(s['rva'] + max(s['raw_size'], s['virtual_size'])
                  for s in self.pe.sections)
        self.uc.mem_map(self.pe.base, (end + 0xfff) & ~0xfff)
        for section in self.pe.sections:
            offset = section['offset']
            self.uc.mem_write(self.pe.base + section['rva'],
                self.pe.data[offset:offset + section['raw_size']])
        self.uc.mem_map(STACK, 0x10000)
        self.uc.mem_map(RETURN, 0x1000)
        self.uc.hook_add(UC_HOOK_CODE, self.callback, begin=JOY, end=MESSAGE)
        self.write_ranges = [(symbols[name], symbols[name] + size, name)
                             for name, size in ARRAY_SIZES.items()]
        self.write_ranges.extend((symbols[name], symbols[name] + 4, name)
            for name in ('CICState', 'MemPackUsed', 'EEPROMUsed', 'SRAMUsed'))
        self.write_ranges.append((STACK, STACK + 0x10000, 'stack'))
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self.check_write)
        message_imports = {row['va'] for row in image_imports(self.pe)
                           if row['import_name'] == 'MessageBoxA'}
        if len(message_imports) != 1:
            raise RuntimeError('expected exactly one MessageBoxA import')
        self.uc.mem_write(message_imports.pop(), struct.pack('<I', MESSAGE))

    def check_write(self, uc, access, address, size, value, data):
        if any(start <= address and address + size <= end
               for start, end, name in self.write_ranges):
            return
        nearest = min(self.write_ranges,
                      key=lambda row: max(row[0] - address, address - row[1], 0))
        self.unexpected_write = dict(address=f'{address:#x}', size=size,
            nearest_state=nearest[2], state_offset=address - nearest[0],
            value=f'{value & ((1 << (size * 8)) - 1):#x}',
            instruction=f'{uc.reg_read(UC_X86_REG_EIP):#x}')
        self.restore_write = (address, bytes(uc.mem_read(address, size)))
        raise RuntimeError('write outside permitted application state')

    def put(self, name, value):
        self.uc.mem_write(self.symbols[name], struct.pack('<I', value & 0xffffffff))

    def read(self, name, size=4):
        return bytes(self.uc.mem_read(self.symbols[name], size))

    def string(self, address):
        result = bytearray()
        for offset in range(1000):
            value = bytes(self.uc.mem_read(address + offset, 1))
            if value == b'\0':
                return result.decode('latin1')
            result.extend(value)
        raise RuntimeError('unterminated MessageBoxA text')

    def callback(self, uc, pc, size, data):
        if pc not in (JOY, RUMBLE, MESSAGE):
            raise RuntimeError(f'unexpected callback address {pc:#x}')
        stack = uc.reg_read(UC_X86_REG_ESP)
        return_address = struct.unpack('<I', uc.mem_read(stack, 4))[0]
        if pc == JOY:
            self.calls.append(['joystick'])
            for index in range(4):
                uc.mem_write(self.symbols['Controllers'] + index * 8 + 4,
                             struct.pack('<I', self.joy_buttons[index]))
        elif pc == MESSAGE:
            args = struct.unpack('<4I', uc.mem_read(stack + 4, 16))
            self.calls.append(['MessageBoxA', args[0], self.string(args[1]),
                               self.string(args[2]), args[3]])
        else:
            channel = struct.unpack('<I', uc.mem_read(stack + 4, 4))[0]
            active = uc.mem_read(self.symbols['Controllers'] + channel * 8 + 3, 1)
            self.calls.append(['rumble', channel, active[0]])
        # Callers must tolerate the documented volatile-register clobbers.
        for register in (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX):
            uc.reg_write(register, 0x76543210)
        if pc == MESSAGE:
            uc.reg_write(UC_X86_REG_EAX, 1)  # IDOK
        # The plugin callbacks use cdecl; MessageBoxA uses stdcall.
        uc.reg_write(UC_X86_REG_ESP, stack + (20 if pc == MESSAGE else 4))
        uc.reg_write(UC_X86_REG_EIP, return_address)

    def run(self, case):
        self.unexpected_write = None
        self.restore_write = None
        for name, size in ARRAY_SIZES.items():
            self.uc.mem_write(self.symbols[name], bytes(size))
        for name in SCALARS:
            self.put(name, 0)
        self.uc.mem_write(self.symbols['MovieRepeatCount'], b'\0\0')
        for name, value in dict(CPU_PC=case.get('pc', 0x80001000),
                NetPlayActive=case.get('net', 0),
                NetPlayPlayerCount=case.get('players', 4),
                EEPROMEnabled=case.get('ee', 1),
                EEPROMSize=case.get('eesize', 2048), DisableSaveWrites=1,
                ForcePIFSuccess=case.get('force', 0), CICState=case.get('cic', 0),
                CIGetJoyData=JOY, CIRumble=RUMBLE).items():
            self.put(name, value)
        self.uc.mem_write(self.symbols['PIFRam'], bytes(case['ram']))
        flags = case.get('flags', (1, 1, 0, 0))
        controller_flags = case.get('controller_flags', [flags] * 4)
        buttons = case.get('buttons', [0x87654321 + i * 0x11111111 for i in range(4)])
        self.joy_buttons = case.get('joy_buttons',
                                   [0x12345678 + i * 0x11111111 for i in range(4)])
        controllers = b''.join(bytes(controller_flags[index]) +
            struct.pack('<I', buttons[index]) for index in range(4))
        self.uc.mem_write(self.symbols['Controllers'], controllers)
        for name, size in (('MemPackData', 32768), ('EEPROMData', 2048)):
            self.uc.mem_write(self.symbols[name],
                              bytes((index * 13 + 29) & 255 for index in range(size)))
        if 'initial_seed' in case:
            rng = random.Random(case['initial_seed'])
            for name in ('PIFResponse', 'PIFCommand', 'PackBlock',
                         'EEPROMData', 'MemPackData'):
                self.uc.mem_write(self.symbols[name],
                    bytes(rng.getrandbits(8) for _ in range(ARRAY_SIZES[name])))
        self.calls = []
        self.uc.mem_write(STACK, bytes(0x10000))
        for register in REGISTERS:
            self.uc.reg_write(register, 0x5a5a5a5a)
        self.uc.reg_write(UC_X86_REG_ESP, STACK + 0xfff0)
        self.uc.mem_write(STACK + 0xfff0, struct.pack('<I', RETURN))
        self.uc.reg_write(UC_X86_REG_EFLAGS, 2)
        try:
            self.uc.emu_start(self.symbols['ProcessPIF'], RETURN, count=100000)
        finally:
            # Unicorn may already have committed the faulting store. Keep
            # subsequent probes independent even after a malformed packet.
            if self.restore_write:
                self.uc.mem_write(*self.restore_write)
        if self.uc.reg_read(UC_X86_REG_EIP) != RETURN:
            raise RuntimeError('instruction limit reached at ' +
                               hex(self.uc.reg_read(UC_X86_REG_EIP)))
        return dict(calls=self.calls,
            arrays={name: digest(self.read(name, size))
                    for name, size in ARRAY_SIZES.items()},
            ram=self.read('PIFRam', 64).hex(),
            controllers=self.read('Controllers', 32).hex(),
            scalars={name: self.read(name).hex()
                     for name in ('CICState', 'MemPackUsed', 'EEPROMUsed', 'SRAMUsed')})


def frame(command, channel=0, rx=None, tx=None, address=0, block=0, rumble=False):
    payload = [command]
    if command in (2, 3):
        word = address * 32 + 7
        payload += [word >> 8, word & 255]
        if command == 3:
            payload += [(index * 7 + 3) & 255 for index in range(32)]
            if rumble:
                payload[5] = payload[6] = 1
    if command in (4, 5):
        payload += [block]
        if command == 5:
            payload += [(index * 9 + 5) & 255 for index in range(8)]
    expected = {0: 3, 1: 4, 2: 33, 3: 1, 4: 8, 5: 1, 255: 3}.get(command, 0)
    if tx is not None:
        payload = (payload + [0x44] * 64)[:tx]
    stream = ([0] * channel + [len(payload), expected if rx is None else rx] +
              payload + [0xee] * expected + [0xfe])
    assert len(stream) < 64, (command, channel, len(stream))
    return stream + [0] * (64 - len(stream))


def cases():
    result = []
    for command in (0, 1, 2, 3, 4, 5, 255, 6, 254):
        channels = range(4, 6) if command in (4, 5) else range(6)
        flags = ((0, 0, 0, 0), (1, 0, 0, 0), (1, 1, 0, 0), (1, 0, 1, 0))
        for channel, state, net in itertools.product(channels, flags, (0, 1)):
            result.append(dict(kind='packet', cmd=command, ch=channel,
                flags=state, net=net, ram=frame(command, channel)))
    for command, address, channel in itertools.product(
            (2, 3), (0, 1, 0x3ff, 0x400, 0x600), (0, 3)):
        for state in ((1, 1, 0, 0), (1, 0, 1, 0)):
            result.append(dict(kind='pak_address', cmd=command, addr=address,
                ch=channel, flags=state,
                ram=frame(command, channel, address=address, rumble=True)))
    for command, channel, block in itertools.product((4, 5), (4, 5), (0, 1, 255)):
        result.append(dict(kind='eeprom_block', cmd=command, ch=channel,
                           block=block, ram=frame(command, channel, block=block)))
    for enabled, size, channel in itertools.product((0, 1), (512, 2048), (4, 5)):
        result.append(dict(kind='eeprom_status', ee=enabled, eesize=size,
                           ch=channel, ram=frame(0, channel)))
    for command in (0, 1, 2, 3, 4, 5, 255):
        channel = 4 if command in (4, 5) else 0
        result.append(dict(kind='rx_clamp', cmd=command,
                           ram=frame(command, channel, rx=63)))
        length = {0: 2, 1: 2, 2: 4, 3: 36, 4: 3, 5: 11, 255: 2}[command]
        result.append(dict(kind='tx_long', cmd=command,
                           ram=frame(command, channel, tx=length)))
    for pc in (0xa4001000, 0xa4001ffe, 0xa4001fff, 0x1fc0056c):
        result.append(dict(kind='boot_pc', pc=pc, ram=frame(1)))
    for cic, force in itertools.product((0, 1), (0, 1)):
        result.append(dict(kind='cic_force', cic=cic, force=force, ram=frame(1)))
    result.append(dict(kind='empty', ram=[0xfe] + [0] * 63))
    # Four controller reads must use one input-plugin update for the full block.
    stream = [1, 4, 1, 0xee, 0xee, 0xee, 0xee] * 4
    result.append(dict(kind='four_reads',
                       ram=stream + [0xfe] + [0] * (63 - len(stream))))
    return result


def packet(command, rng, *, rx=None, tx=None, block=None, address=None):
    """Encode a command; callers decide where its response area ends."""
    transmit, receive = LENGTHS[command]
    payload = [command]
    if command in (2, 3):
        if address is None:
            address = rng.choice((0, 1, 0x3fe, 0x3ff, 0x400, 0x600, 0x7ff))
        word = address * 32 + rng.randrange(32)
        payload += [word >> 8, word & 255]
    elif command in (4, 5):
        payload += [rng.randrange(256) if block is None else block]
    payload += [rng.getrandbits(8) for _ in range(transmit - len(payload))]
    if tx is not None:
        payload = (payload + [0x44] * 64)[:tx]
    receive = receive if rx is None else rx
    return [len(payload), receive] + payload + [0xfe] * receive


def single_packet(command, rng, *, offset=0, channel=None, **fields):
    if channel is None:
        channel = 4 if command in (4, 5) else 0
    stream = [0] * channel + [0xff] * offset + packet(command, rng, **fields)
    assert len(stream) <= 62, (command, len(stream))
    return stream + [0xfe] + [0] * (63 - len(stream))


def edge_cases():
    rng, result = random.Random(SEED), []
    # Noncanonical int values are intentional: these are state inputs, not bool
    # storage representations. Every complete packet remains within PIF RAM.
    for enabled, size, channel, rx in itertools.product(
            (0, 1, 2, -1), (0, 1, 511, 512, 513, 2047, 2048, 2049, -1),
            (4, 5), (0, 1, 2, 3)):
        result.append(dict(kind='edge_eeprom_status', domain='bounded_edge',
            ee=enabled, eesize=size, ch=channel, initial_seed=rng.getrandbits(32),
            ram=single_packet(0, rng, channel=channel, rx=rx)))
    # Start/end offsets, masks in the RX header, and exact command lengths.
    for command in LENGTHS:
        tx, rx = LENGTHS[command]
        channel = 4 if command in (4, 5) else 0
        last_offset = 62 - channel - 2 - tx - rx
        for offset in sorted({0, 1, last_offset // 2, last_offset}):
            for high_rx in (0, 0x40, 0x80, 0xc0):
                ram = single_packet(command, rng, offset=offset)
                ram[channel + offset + 1] |= high_rx
                result.append(dict(kind='edge_packet_offset', domain='valid',
                    cmd=command, offset=offset, high_rx=high_rx,
                    initial_seed=rng.getrandbits(32), ram=ram))
    # Short status/button/EEPROM responses exercise each memcpy tail length.
    # Pak reads always write 32 data bytes regardless of RX; they are treated
    # separately as malformed below when RX differs from the protocol length.
    for command in (0, 1, 4, 5, 255):
        for rx in range(LENGTHS[command][1] + 1):
            for net, players in itertools.product((0, 1, 2), (0, 1, 3, 4)):
                result.append(dict(kind='edge_rx_length', domain='bounded_edge',
                    cmd=command, rx=rx, net=net, players=players,
                    initial_seed=rng.getrandbits(32),
                    ram=single_packet(command, rng, rx=rx)))
    # Partial pak/EEPROM writes have nonnegative memcpy lengths and remain in
    # their arrays. Marked as bounded edges, not canonical protocol packets.
    for command, lengths in ((3, range(3, 36)), (5, range(2, 11))):
        for tx, block in itertools.product(lengths, (0, 255)):
            for flags in ((1, 1, 0, 0), (1, 0, 1, 0)):
                result.append(dict(kind='edge_tx_length', domain='bounded_edge',
                    cmd=command, tx=tx, block=block, flags=flags,
                    initial_seed=rng.getrandbits(32),
                    ram=single_packet(command, rng, tx=tx, block=block)))
    return result


def fuzz_cases(count):
    rng, result = random.Random(SEED ^ 0x504946), []
    for index in range(count):
        stream, commands = [], []
        for channel in range(6):
            if len(stream) >= 62:
                break
            if rng.randrange(5) == 0:
                stream.append(0)
                continue
            command = rng.choice((0, 1, 2, 3, 255) if channel < 4 else (0, 4, 5, 255))
            encoded = packet(command, rng)
            padding = [0xff] * rng.randrange(4)
            if len(stream) + len(padding) + len(encoded) > 62:
                # A later channel may have a smaller packet that still fits.
                if len(stream) < 62:
                    stream.append(0)
                continue
            commands.append(dict(channel=channel, command=command,
                                 offset=len(stream) + len(padding)))
            stream += padding + encoded
        result.append(dict(kind='seeded_valid_mix', domain='valid', ordinal=index,
            commands=commands, initial_seed=rng.getrandbits(32),
            controller_flags=[[rng.randrange(2) for _ in range(4)] for _ in range(4)],
            buttons=[rng.getrandbits(32) for _ in range(4)],
            joy_buttons=[rng.getrandbits(32) for _ in range(4)],
            net=rng.choice((0, 1, 2)), players=rng.randrange(5),
            ee=rng.choice((0, 1, 2, -1)), eesize=rng.choice((0, 1, 512, 2048, -1)),
            force=rng.randrange(2),
            ram=stream + [0xfe] + [0] * (63 - len(stream))))
    return result


def malformed_cases():
    """Keep finite invalid-input observations separate from valid comparisons.

    Some original paths perform negative-length copies or exceed PIF RAM.
    Write-range checks stop those paths; a shared rejection is not a match.
    """
    rng, result = random.Random(SEED ^ 0x424144), []
    for command in LENGTHS:
        maximum, receive = LENGTHS[command]
        channel = 4 if command in (4, 5) else 0
        for tx, rx, offset in itertools.product(
                sorted({0, 1, 2, maximum + 1, 63, 64}),
                sorted({0, receive, min(receive + 1, 63), 63}), (0, 50)):
            encoded = packet(command, rng, tx=tx, rx=rx)
            stream = [0] * channel + [0xff] * offset + encoded + [0xfe]
            ram = (stream + [0] * 64)[:64]
            # Do not accidentally invoke the separately excluded CIC path.
            if ram[46:48] == [15, 15]:
                ram[46] = 0
            result.append(dict(kind='malformed_length', domain='malformed',
                cmd=command, tx=tx, rx=rx, offset=offset,
                initial_seed=rng.getrandbits(32), ram=ram))
    return result


def execute(image, case):
    try:
        return dict(result=image.run(case))
    except Exception as error:
        return dict(error=str(error), unexpected_write=image.unexpected_write)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifact', type=Path, required=True,
                        help='directory containing nemu64.exe and nemu64.map')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--suite', choices=('all', 'controlled', 'extended'),
                        default='all', help='extended omits the original 452 cases')
    parser.add_argument('--fuzz-cases', type=int, default=1024)
    args = parser.parse_args()
    map_path = args.artifact / 'nemu64.map'
    original = Image(ROOT / 'master/nemu64.exe', original_symbols())
    rebuilt = Image(args.artifact / 'nemu64.exe', rebuilt_symbols(map_path))
    pif_metadata = json.loads((ROOT / 'analysis/nemu-pif.json').read_text())
    if digest(original.pe.data) != pif_metadata['original_sha256']:
        raise RuntimeError('reference image differs from the recorded target hash')
    if not 0 <= args.fuzz_cases <= 10000:
        parser.error('--fuzz-cases must be between 0 and 10000')
    tests = cases() if args.suite in ('all', 'controlled') else []
    if args.suite in ('all', 'extended'):
        tests += edge_cases() + fuzz_cases(args.fuzz_cases) + malformed_cases()
    failures, malformed_observations = [], []
    results = [hashlib.sha256(), hashlib.sha256()]
    compared, matching, controlled_matching, malformed_compared = 0, 0, 0, 0
    for index, case in enumerate(tests):
        one, two = execute(original, case), execute(rebuilt, case)
        if 'result' in one and 'result' in two:
            compared += 1
            malformed_compared += case.get('domain') == 'malformed'
            for value, aggregate in zip((one, two), results):
                aggregate.update(json.dumps(value, sort_keys=True).encode())
            if one != two:
                failures.append(dict(index=index, case=case, original=one, rebuilt=two))
            else:
                matching += 1
                controlled_matching += case.get('domain') != 'malformed'
        elif case.get('domain') == 'malformed':
            malformed_observations.append(dict(index=index, case=case,
                                               original=one, rebuilt=two))
        else:
            failures.append(dict(index=index, case=case, original=one, rebuilt=two))
    report = dict(status='passed' if not failures else 'different',
        evidence_type='behavioral_differential_only_not_byte_equality',
        original_sha256=digest(original.pe.data), rebuilt_sha256=digest(rebuilt.pe.data),
        rebuilt_map_sha256=digest(map_path.read_bytes()),
        script_sha256=digest(Path(__file__).read_bytes()),
        metadata_sha256={name: digest((ROOT / 'analysis' / (name + '.json')).read_bytes())
                         for name in METADATA},
        suite=args.suite, deterministic_seed=SEED, fuzz_cases=args.fuzz_cases,
        tests=len(tests), completed_comparisons=compared,
        controlled_cases=sum(c.get('domain') != 'malformed' for c in tests),
        controlled_matches=controlled_matching,
        malformed_completed_comparisons=malformed_compared,
        malformed_rejection_count=len(malformed_observations),
        test_cases_sha256=digest(json.dumps(tests, sort_keys=True).encode()),
        categories=dict(sorted(Counter(c['kind'] for c in tests).items())),
        original_results_sha256=results[0].hexdigest(),
        rebuilt_results_sha256=results[1].hexdigest(), failures=failures,
        malformed_rejections=malformed_observations,
        malformed_rejections_are_not_matching_evidence=True,
        write_range_checks='all emulated writes must stay within stack or named PIF state',
        write_range_limitations='checks object ranges, not per-instruction intent; '
            'a malformed write into another permitted object is not rejected by range alone',
        external_boundary_hooks=['CIGetJoyData', 'CIRumble', 'MessageBoxA'],
        file_writes_disabled_by_original_DisableSaveWrites_flag=True,
        exclusions=['movie files', 'encrypted CIC challenge path'])
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{matching}/{compared} completed PIF comparisons passed; '
          f'{len(malformed_observations)} malformed cases rejected; '
          f'{len(failures)} differences/errors')
    for failure in failures[:3]:
        print(json.dumps(failure, indent=2))
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
