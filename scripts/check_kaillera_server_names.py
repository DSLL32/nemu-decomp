#!/usr/bin/env python3
"""Record surviving application RTTI names from the publisher's server archives.

This reads reference binaries only. It neither executes them nor supplies any
bytes to the client reconstruction build.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import tarfile


REFERENCES = {
    "linux": {
        "archive_sha256": "6cf5b7cb5797df67b70c1abda988828cc7b3d5bb3d78ea846c243c9a0707eb88",
        "binary_sha256": "a600a98a2cb3443c5e6d71704a16bd966fe9be1f13209f39aba1d7c72c615c95",
        "name_offsets": {"UDPCom": 0xF014, "KLRCom": 0xF074},
    },
    "freebsd": {
        "archive_sha256": "568e97d11daf37970d9e231bf424ed9411544bbc6deec8feebf8a4b98cc7811f",
        "binary_sha256": "22c708b36ac1d841dba8e5a2209dc8cc827a8b23110c31a5b4e7eea73a8d78c2",
        "name_offsets": {"UDPCom": 0x10F4D, "KLRCom": 0x10F64},
    },
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def inspect_archive(path, platform):
    expected = REFERENCES[platform]
    archive = Path(path).read_bytes()
    if digest(archive) != expected["archive_sha256"]:
        raise ValueError(f"Unexpected {platform} archive SHA-256")
    member = "kaillerasrv-0.86/kaillerasrv"
    with tarfile.open(path) as source:
        data = source.extractfile(member).read()
    if digest(data) != expected["binary_sha256"]:
        raise ValueError(f"Unexpected {platform} binary SHA-256")
    names = []
    for name, offset in expected["name_offsets"].items():
        encoded = f"{len(name)}{name}".encode() + b"\0"
        if data[offset:offset + len(encoded)] != encoded:
            raise ValueError(f"Missing {name} GCC RTTI name")
        names.append({"name": name, "gcc_encoding": encoded[:-1].decode(),
                      "file_offset": offset})
    result = {
        "url": f"http://www.kaillera.com/files/kaillerasrv-0.86-{platform}.tgz",
        "archive_size": len(archive), "archive_sha256": digest(archive),
        "binary_member": member, "binary_size": len(data),
        "binary_sha256": digest(data), "application_rtti_names": names,
    }
    if platform == "linux":
        # GCC 2.91's vtable prefix contains a zero word and an RTTI factory.
        # These are reference metadata pointers, not reconstructed instructions.
        tables = []
        for name, offset in [("UDPCom", 0xEFE0), ("KLRCom", 0xF040)]:
            words = struct.unpack_from("<13I", data, offset)
            tables.append({"class": name, "file_offset": offset,
                           "zero_prefix": words[0], "rtti_factory_va": words[1],
                           "virtual_method_vas": list(words[2:])})
        result["vtables"] = tables
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--linux-archive", required=True, type=Path)
    parser.add_argument("--freebsd-archive", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    report = {
        "purpose": "Historical class-name evidence; not client byte-equality certification",
        "server_version": "0.86",
        "references": [inspect_archive(args.linux_archive, "linux"),
                       inspect_archive(args.freebsd_archive, "freebsd")],
    }
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print("Verified UDPCom and KLRCom RTTI names in both publisher archives")


if __name__ == "__main__":
    main()
