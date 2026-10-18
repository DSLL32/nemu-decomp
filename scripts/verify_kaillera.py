#!/usr/bin/env python3
"""Verify all bytes of the packed Kaillera client, without exclusions."""

import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'master/kailleraclient.dll'
ORIGINAL_SHA256 = '410503fdfd1e931adf6024df7b8d88c725260b568688c5a5a1ab9897c83ff72d'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def compare(path):
    reference = ORIGINAL.read_bytes()
    if digest(reference) != ORIGINAL_SHA256:
        raise ValueError('Original kailleraclient.dll hash changed')
    candidate = Path(path).read_bytes()
    differences = [i for i, (a, b) in enumerate(zip(reference, candidate)) if a != b]
    return dict(original_sha256=ORIGINAL_SHA256, rebuilt_sha256=digest(candidate),
                original_bytes=len(reference), rebuilt_bytes=len(candidate),
                full_dll_byte_exact=reference == candidate,
                differing_file_bytes=len(differences) + abs(len(reference) - len(candidate)),
                first_differing_offsets=differences[:32])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = compare(args.dll)
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(f'Full DLL byte exact: {result["full_dll_byte_exact"]}\n'
          f'Differing bytes: {result["differing_file_bytes"]}\n'
          f'SHA-256: {result["rebuilt_sha256"]}')
    return 0 if result['full_dll_byte_exact'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
