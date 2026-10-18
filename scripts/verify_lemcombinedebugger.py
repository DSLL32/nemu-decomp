#!/usr/bin/env python3
"""Compare a LemCombineDebugger rebuild with master, without masking or modifying bytes."""

import argparse
import hashlib
import json
from pathlib import Path

from pe_fingerprints import PE32


ROOT = Path(__file__).resolve().parents[1]
MASTER = ROOT / 'master/Plugin/LemCombineDebugger.dll'
MASTER_SHA256 = '0d1105d3638f7c3d0203de44a948003e7c77a67f4185b13e9458be220d9a8fbd'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def compare(path):
    original, rebuilt = PE32(MASTER), PE32(path)
    a, b = original.data, rebuilt.data
    if digest(a) != MASTER_SHA256:
        raise ValueError('Unexpected master/Plugin/LemCombineDebugger.dll SHA-256')
    differences = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
    first = differences[0] if differences else (min(len(a), len(b)) if a != b else None)
    sections = []
    for old in original.sections:
        new = next((s for s in rebuilt.sections if s['name'] == old['name']), None)
        old_data = a[old['offset']:old['offset'] + old['raw_size']]
        new_data = b[new['offset']:new['offset'] + new['raw_size']] if new else b''
        sections.append(dict(name=old['name'], original=old, rebuilt=new,
                             byte_exact=old_data == new_data,
                             original_sha256=digest(old_data), rebuilt_sha256=digest(new_data)))
    expected_exports, actual_exports = original.exports(), rebuilt.exports()
    expected_abi = [(s['name'], s['ordinal']) for s in expected_exports]
    actual_abi = [(s['name'], s['ordinal']) for s in actual_exports]
    old_resources = {tuple(r['path']): a[r['offset']:r['offset'] + r['size']]
                     for r in original.resources()}
    new_resources = {tuple(r['path']): b[r['offset']:r['offset'] + r['size']]
                     for r in rebuilt.resources()}
    resources = [dict(path=list(key), original_size=len(old_resources.get(key, b'')),
                      rebuilt_size=len(new_resources.get(key, b'')),
                      byte_exact=key in old_resources and key in new_resources and
                      old_resources[key] == new_resources[key])
                 for key in sorted(old_resources.keys() | new_resources.keys(), key=str)]
    return dict(original=str(MASTER), rebuilt=str(path.resolve()),
                original_size=len(a), rebuilt_size=len(b),
                original_sha256=digest(a), rebuilt_sha256=digest(b),
                full_dll_byte_exact=a == b,
                differing_file_bytes=len(differences) + abs(len(a) - len(b)),
                first_difference_offset=first, sections=sections,
                export_abi_exact=expected_abi == actual_abi,
                resource_payloads_exact=old_resources == new_resources, resources=resources,
                original_exports=expected_exports, rebuilt_exports=actual_exports,
                original_imports=original.imports(), rebuilt_imports=rebuilt.imports())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    report = compare(args.dll)
    data = json.dumps(report, indent=2) + '\n'
    if args.report:
        args.report.write_text(data)
    print(data, end='')
    return 0 if report['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
