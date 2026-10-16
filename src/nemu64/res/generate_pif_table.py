#!/usr/bin/env python3
"""Generate Nemu64 resource 187 from the CIC-NUS-6105 challenge algorithm.

The resource contains the eight response nibbles for each five-nibble challenge
prefix in ascending order, followed by three zero challenge nibbles. This is a
build-time reconstruction of the data, not a reconstruction of Nemu64's original
table-generation source. The modern algorithm postdates Nemu64.

Algorithm adapted from X-Scale's 2011 implementation; see CIC6105-LICENSE.txt and
https://github.com/mupen64plus/mupen64plus-core/blob/master/src/device/pif/n64_cic_nus_6105.c
"""

import argparse
import hashlib
from pathlib import Path


EXPECTED_SHA256 = 'f7c883d9bd2ef4910665dd6deccae79e0d2f33b385795eca5eaa8e423720bc0c'
KEY_TABLES = (
    (4, 7, 10, 7, 14, 5, 14, 1, 12, 15, 8, 15, 6, 3, 6, 9),
    (4, 1, 10, 7, 14, 5, 14, 1, 12, 9, 8, 5, 6, 3, 12, 9),
)


def next_state(table, response):
    """Return the next key and table after one response nibble."""
    sign = response >> 3
    magnitude = (~response if sign else response) & 7
    next_table = sign if magnitude % 3 == 1 else 1 - sign
    if table == 1:
        if response in (1, 9):
            next_table = 1
        elif response in (11, 14):
            next_table = 0
    return KEY_TABLES[table][response], next_table


def generate():
    transitions = tuple(tuple(next_state(table, response)
                              for response in range(16))
                        for table in range(2))
    # Three zero challenge nibbles have only 32 possible starting states.
    suffixes = []
    for table in range(2):
        row = []
        for key in range(16):
            state, value, suffix = table, key, 0
            for _ in range(3):
                suffix = (suffix << 4) | value
                value, state = transitions[state][value]
            row.append(suffix)
        suffixes.append(tuple(row))

    result = bytearray()

    def append_prefix(depth, key, table, response):
        if depth == 5:
            word = (response << 12) | suffixes[table][key]
            result.extend(word.to_bytes(4, 'big'))
            return
        for challenge in range(16):
            nibble = (key + 5 * challenge) & 15
            new_key, new_table = transitions[table][nibble]
            append_prefix(depth + 1, new_key, new_table,
                          (response << 4) | nibble)

    append_prefix(0, 11, 0, 0)
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path,
                        help='destination res/pif-table.bin in the build directory')
    args = parser.parse_args()
    data = generate()
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != 4194304 or digest != EXPECTED_SHA256:
        raise RuntimeError('CIC table generation does not match the original resource')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f'{args.output}: {len(data)} bytes, SHA-256 {digest}')


if __name__ == '__main__':
    main()
