#!/usr/bin/env python3
"""Replay the observed VC7 import sort using its tracked CRT implementation.

This read-only diagnostic compiles qsort.c for the host inside ~/.ai-tmp.
It compares the observed linker permutation, then tests selection positions
for the already selected but ultimately discarded IsBadWritePtr import.
It never changes application objects, linker inputs, or executable bytes.
"""

import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from inspect_gs_symbols import imported_symbols
from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]


def analyze(evidence):
    source = ROOT / evidence['qsort_model']['source']
    raw = source.read_bytes()
    if hashlib.sha256(raw).hexdigest() != evidence['qsort_model']['source_sha256']:
        raise ValueError('Tracked qsort source changed')
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-import-sort.', dir=parent))
    # The algorithm itself is unchanged. Replace private CRT includes and
    # calling-convention spelling so the standalone diagnostic builds on Linux.
    text = re.sub(r'^#include.*$', '', raw.decode('cp1252'), flags=re.M)
    text = '#include <stddef.h>\n#define __cdecl\n#define qsort msvc_qsort\n' + text
    (scratch / 'qsort.c').write_text(text)
    subprocess.run(['cc', '-shared', '-fPIC', '-O2', str(scratch / 'qsort.c'),
                    '-o', str(scratch / 'qsort.so')], check=True)
    rows = evidence['observed_pre_sort']
    runtime = ctypes.CDLL(str(scratch / 'qsort.so'))
    callback = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(ctypes.c_uint32),
                               ctypes.POINTER(ctypes.c_uint32))

    def compare(a, b):
        left, right = rows[a[0]]['module_name'], rows[b[0]]['module_name']
        return (left > right) - (left < right)

    comparator = callback(compare)
    runtime.msvc_qsort.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t, callback]

    def sort(order):
        array = (ctypes.c_uint32 * len(order))(*order)
        runtime.msvc_qsort(array, len(array), 4, comparator)
        return list(array)

    baseline = sort(list(range(len(rows))))
    if baseline != evidence['observed_sort_permutation']:
        raise ValueError('Model differs from observed linker qsort result')
    unused = [i for i, row in enumerate(rows) if row.get('discarded')]
    if len(unused) != 1:
        raise ValueError('Expected exactly one observed discarded import')
    target = [(row['dll'].lower(), row['name']) for row in sorted(
        imported_symbols(PE32(ROOT / evidence['original'])), key=lambda row: row['iat_va'])]

    def identities(order):
        # LINK moves null terminators to each DLL's end after sorting. They do
        # not affect the relative identities of the surviving import entries.
        return [(rows[i]['module_name'].lower(), rows[i]['import_name']) for i in sort(order)
                if rows[i]['import_name'] is not None and not rows[i].get('discarded')]

    results = []
    for position in range(len(rows)):
        order = [i for i in range(len(rows)) if i != unused[0]]
        order.insert(position, unused[0])
        actual = identities(order)
        results.append(dict(position=position,
                            matching_slots=sum(a == b for a, b in zip(actual, target)),
                            complete_order_equal=actual == target))
    return dict(scratch=str(scratch),
                observed_sort_permutation_reproduced=True,
                baseline_exact_iat_slots=sum(a == b for a, b in zip(
                    identities(list(range(len(rows)))), target)),
                original_import_count=len(target),
                exact_selection_positions=[row['position'] for row in results
                                           if row['complete_order_equal']],
                selection_position_results=results)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence', type=Path,
                        default=ROOT / 'analysis/nemu-import-sort.json')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    report = json.dumps(analyze(json.loads(args.evidence.read_text())), indent=2) + '\n'
    if args.output:
        args.output.write_text(report)
    else:
        print(report, end='')


if __name__ == '__main__':
    main()
