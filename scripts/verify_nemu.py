#!/usr/bin/env python3
"""Compare every byte of a rebuilt Nemu64 executable against the original.

No instructions, addresses, metadata, resources, or padding are masked. Section
and resource comparisons diagnose differences; only complete file identity is
success. This script does not modify either input.
"""

import argparse
import hashlib
import json
from pathlib import Path

from pe_fingerprints import PE32
from verify_gs import differences


ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'master/nemu64.exe'


def compare(path, original=ORIGINAL):
    target, rebuilt = PE32(original), PE32(path)
    result = dict(original=str(original), rebuilt=str(path),
                  original_sha256=hashlib.sha256(target.data).hexdigest(),
                  rebuilt_sha256=hashlib.sha256(rebuilt.data).hexdigest(),
                  original_bytes=len(target.data), rebuilt_bytes=len(rebuilt.data),
                  full_exe_byte_exact=target.data == rebuilt.data,
                  differing_file_bytes=differences(target.data, rebuilt.data))
    sections = {section['name']: section for section in rebuilt.sections}
    result['sections'] = []
    for section in target.sections:
        other = sections.get(section['name'])
        a = target.data[section['offset']:section['offset'] + section['raw_size']]
        b = (rebuilt.data[other['offset']:other['offset'] + other['raw_size']]
             if other else b'')
        result['sections'].append(dict(name=section['name'], original=section,
                                       rebuilt=other, layout_equal=section == other,
                                       raw_bytes_equal=other is not None and a == b,
                                       differing_bytes=differences(a, b)))
    resources = {tuple(row['path']): row for row in rebuilt.resources()}
    result['resources'] = []
    for row in target.resources():
        other = resources.get(tuple(row['path']))
        a = target.data[row['offset']:row['offset'] + row['size']]
        b = (rebuilt.data[other['offset']:other['offset'] + other['size']]
             if other else b'')
        result['resources'].append(dict(path=row['path'], original_bytes=len(a),
                                        rebuilt_bytes=len(b),
                                        payload_equal=other is not None and a == b,
                                        differing_bytes=differences(a, b)))
    result['resource_count_equal'] = len(target.resources()) == len(resources)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--original', type=Path, default=ORIGINAL)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = compare(args.exe, args.original)
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(f'Complete executable byte exact: {result["full_exe_byte_exact"]}')
    print(f'Differing file bytes: {result["differing_file_bytes"]}')
    return 0 if result['full_exe_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
