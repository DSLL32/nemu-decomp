#!/usr/bin/env python3
"""Extract the exact 46-file Profile toolchain subset from original installers.

The configuration uses RTM RTL/VCL and the General Update 1 Personal compiler.
It does not claim to identify the executable used for the original compilation.
"""

import argparse
import json
from pathlib import Path
import shutil
import tempfile

from unpack_toolchains import digest, extract_d6_updates, install_d6_base


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--staging', type=Path, default=repo / 'toolchains-staging')
    parser.add_argument('--dest', type=Path, default=repo / 'toolchains/d6rtm')
    args = parser.parse_args()
    if args.dest.exists():
        parser.error(f'Refusing to overwrite {args.dest}')
    metadata = json.loads((repo / 'toolchains/profile-toolchain.json').read_text())
    for source in metadata['sources']:
        path = args.staging / Path(source['path']).relative_to('toolchains-staging')
        if digest(path) != source['sha256']:
            raise ValueError(f'Installer hash mismatch: {path}')
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix='nemu-profile-toolchain.', dir=parent))
    print(f'Scratch: {scratch}', flush=True)
    base = scratch / 'base'
    install_d6_base(args.staging, scratch / 'media', base)
    personal = extract_d6_updates(args.staging, scratch / 'updates', ('u1p',))['u1p']
    # Validate all inputs before creating the destination.
    sources = []
    for item in metadata['files']:
        root = base if item['origin'] == 'Delphi 6 RTM Enterprise media' else personal
        source = root / item['source_path']
        if digest(source) != item['sha256']:
            raise ValueError(f'Extracted file hash mismatch: {source}')
        sources.append((source, args.dest / item['path']))
    for source, destination in sources:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    print(f'Verified and installed {len(sources)} unmodified files into {args.dest}')


if __name__ == '__main__':
    main()
