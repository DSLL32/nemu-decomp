#!/usr/bin/env python3
"""Compare a Profile rebuild to every byte of the original DLL.

No normalization, relocation masking, or resource exclusions are used.
"""

import argparse
import hashlib
import json
from pathlib import Path

from inspect_delphi_forms import inspect
from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL_SHA256 = 'b082cfd8a8146a5ea967c219e699658d9232347c66f8469a4a741e59becc5e68'


def differences(a, b):
    return sum(x != y for x, y in zip(a, b)) + abs(len(a) - len(b))


def compare(rebuilt):
    original = ROOT / 'master/Plugin/profile.dll'
    target, candidate = PE32(original), PE32(rebuilt)
    original_hash = hashlib.sha256(target.data).hexdigest()
    if original_hash != ORIGINAL_SHA256:
        raise ValueError('Original profile.dll hash changed')
    result = dict(original_sha256=original_hash,
                  rebuilt_sha256=hashlib.sha256(candidate.data).hexdigest(),
                  original_bytes=len(target.data), rebuilt_bytes=len(candidate.data),
                  full_dll_byte_exact=target.data == candidate.data,
                  differing_file_bytes=differences(target.data, candidate.data))
    candidate_sections = {s['name']: s for s in candidate.sections}
    result['sections'] = []
    for section in target.sections:
        other = candidate_sections.get(section['name'])
        a = target.data[section['offset']:section['offset'] + section['raw_size']]
        b = (candidate.data[other['offset']:other['offset'] + other['raw_size']]
             if other else b'')
        result['sections'].append(dict(name=section['name'], original=section,
                                       rebuilt=other, raw_bytes_equal=bool(other and a == b),
                                       differing_bytes=differences(a, b)))
    candidate_resources = {tuple(r['path']): r for r in candidate.resources()}
    result['resources'] = []
    for resource in target.resources():
        other = candidate_resources.get(tuple(resource['path']))
        a = target.data[resource['offset']:resource['offset'] + resource['size']]
        b = (candidate.data[other['offset']:other['offset'] + other['size']]
             if other else b'')
        result['resources'].append(dict(path=resource['path'], original_bytes=len(a),
                                        rebuilt_bytes=len(b), byte_exact=bool(other and a == b),
                                        differing_bytes=differences(a, b)))
    result['exports_exact'] = target.exports() == candidate.exports()
    before, after = inspect(original), inspect(rebuilt)
    result['published_form_metadata_exact'] = before['forms'] == after['forms']
    return result


def summarize(result):
    print(f"Full DLL byte exact: {result['full_dll_byte_exact']}")
    print(f"Differing bytes: {result['differing_file_bytes']}")
    print(f"SHA-256: {result['rebuilt_sha256']}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = compare(args.dll)
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + '\n')
    summarize(result)
    return 0 if result['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
