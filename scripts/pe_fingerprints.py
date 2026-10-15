#!/usr/bin/env python3
"""Read compiler evidence from the project's PE32 files using only the stdlib.

Usage: python3 scripts/pe_fingerprints.py master/nemu64.exe ... > report.json
Does not execute binaries, fetch signatures, or modify inputs. Delphi version
inference follows Detect It Easy's TControl rule; see toolchains/README.md.
This is a focused inspection helper, not a general compiler detector.
"""

import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import struct


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def cstring(data, offset):
    return data[offset:data.index(0, offset)].decode("ascii", errors="replace")


def shortstring(data, offset):
    end = offset + 1 + data[offset]
    return data[offset + 1:end].decode("cp1252"), end


def rol32(value, count):
    count %= 32
    return ((value << count) | (value >> ((32 - count) % 32))) & 0xffffffff


class PE32:
    def __init__(self, path):
        self.path = path
        self.data = b = path.read_bytes()
        if b[:2] != b"MZ":
            raise ValueError(f"{path}: not an MZ image")
        self.pe = pe = u32(b, 0x3c)
        self.opt = opt = pe + 24
        if b[pe:pe + 4] != b"PE\0\0" or u16(b, opt) != 0x10b:
            raise ValueError(f"{path}: expected PE32")
        self.base = u32(b, opt + 28)
        self.sections = []
        table = opt + u16(b, pe + 20)
        for i in range(u16(b, pe + 6)):
            name, size, rva, raw_size, offset = struct.unpack_from(
                "<8sIIII", b, table + 40 * i)
            self.sections.append(dict(
                name=name.rstrip(b"\0").decode("ascii"), virtual_size=size,
                rva=rva, raw_size=raw_size, offset=offset))

    def offset(self, rva):
        if rva < u32(self.data, self.opt + 60):
            return rva
        for s in self.sections:
            if s["rva"] <= rva < s["rva"] + s["raw_size"]:
                return s["offset"] + rva - s["rva"]
        raise ValueError(f"{self.path}: RVA {rva:#x} has no file data")

    def directory(self, index):
        if index >= u32(self.data, self.opt + 92):
            return 0, 0
        return struct.unpack_from("<II", self.data, self.opt + 96 + 8 * index)

    def rich(self):
        b = self.data
        end = b.find(b"Rich", 0x40, self.pe)
        if end < 0:
            return None
        key = u32(b, end + 4)
        start = b.rfind(struct.pack("<I", 0x536e6144 ^ key), 0x40, end)
        if start < 0 or (end - start) % 8:
            raise ValueError("Malformed Rich header")
        records = []
        for pos in range(start + 16, end, 8):
            compid, count = u32(b, pos) ^ key, u32(b, pos + 4) ^ key
            records.append(dict(comp_id=f"{compid:08x}",
                                product_id=compid >> 16,
                                build=compid & 0xffff, count=count))
        checksum = start
        for i in range(start):
            if not 0x3c <= i < 0x40:  # e_lfanew changes after the checksum.
                checksum += rol32(b[i], i)
        for record in records:
            checksum += rol32(int(record["comp_id"], 16), record["count"])
        return dict(offset=start, xor_key=f"{key:08x}",
                    checksum_valid=(checksum & 0xffffffff) == key,
                    records=records)

    def imports(self):
        rva, _ = self.directory(1)
        if not rva:
            return []
        b = self.data
        pos = self.offset(rva)
        result = []
        while u32(b, pos + 12):
            dll = cstring(b, self.offset(u32(b, pos + 12)))
            thunk = self.offset(u32(b, pos) or u32(b, pos + 16))
            symbols = []
            while u32(b, thunk):
                val = u32(b, thunk)
                symbols.append(f"ordinal:{val & 0xffff}" if val & 0x80000000
                               else cstring(b, self.offset(val) + 2))
                thunk += 4
            entry = dict(dll=dll, symbol_count=len(symbols))
            if dll.lower() in ("gs_dll.dll", "kailleraclient.dll"):
                entry["symbols"] = symbols
            result.append(entry)
            pos += 20
        return result

    def exports(self):
        rva, _ = self.directory(0)
        if not rva:
            return []
        b = self.data
        pos = self.offset(rva)
        base, count, names_count, funcs, names, ords = struct.unpack_from(
            "<IIIIII", b, pos + 16)
        funcs, names, ords = map(self.offset, (funcs, names, ords))
        result = []
        for i in range(names_count):
            ordinal = u16(b, ords + 2 * i)
            if ordinal >= count:
                raise ValueError("Invalid export ordinal")
            result.append(dict(name=cstring(b, self.offset(u32(b, names + 4 * i))),
                               ordinal=base + ordinal,
                               rva=u32(b, funcs + 4 * ordinal)))
        return result

    def resources(self):
        rva, _ = self.directory(2)
        if not rva:
            return []
        b = self.data
        root = self.offset(rva)
        result = []

        def walk(relative, path):
            if len(path) > 3:
                raise ValueError("Unexpected resource directory depth")
            pos = root + relative
            count = u16(b, pos + 12) + u16(b, pos + 14)
            for i in range(count):
                name, target = struct.unpack_from("<II", b, pos + 16 + 8 * i)
                if name & 0x80000000:
                    off = root + (name & 0x7fffffff)
                    name = b[off + 2:off + 2 + 2 * u16(b, off)].decode("utf-16le")
                subpath = path + [name]
                if target & 0x80000000:
                    walk(target & 0x7fffffff, subpath)
                else:
                    data_rva, size = struct.unpack_from("<II", b, root + target)
                    result.append(dict(path=subpath, offset=self.offset(data_rva),
                                       size=size))

        walk(0, [])
        return result

    def debug(self):
        rva, size = self.directory(6)
        if not rva:
            return []
        b = self.data
        result = []
        for pos in range(self.offset(rva), self.offset(rva) + size, 28):
            kind, length, _, offset = struct.unpack_from("<IIII", b, pos + 12)
            item = dict(type=kind, size=length, offset=offset)
            if kind == 2 and b[offset:offset + 4] == b"NB10":
                item.update(format="NB10", signature=f"{u32(b, offset + 8):08x}",
                            age=u32(b, offset + 12), pdb=cstring(b, offset + 16))
            result.append(item)
        return result

    def delphi(self, resources):
        b = self.data
        result = {}
        for item in resources:
            data = b[item["offset"]:item["offset"] + item["size"]]
            name = item["path"][1]
            if name == "PACKAGEINFO":
                flags, requires_count = struct.unpack_from("<II", data)
                pos = 8
                requires = []
                for _ in range(requires_count):
                    requires.append(cstring(data, pos))
                    pos = data.index(0, pos) + 1
                unit_count = u32(data, pos)
                pos += 4
                units = []
                for _ in range(unit_count):
                    unit = dict(flags=data[pos], hash=data[pos + 1],
                                name=cstring(data, pos + 2))
                    units.append(unit)
                    pos = data.index(0, pos + 2) + 1
                result["package"] = dict(offset=item["offset"], flags=flags,
                                         requires=requires, units=units)
            elif name == "DVCLAL":
                result["dvclal"] = dict(offset=item["offset"], hex=data.hex())
            if data.startswith(b"TPF0"):
                cls, pos = shortstring(data, 4)
                obj, _ = shortstring(data, pos)
                form = dict(resource=name, offset=item["offset"],
                            size=item["size"], class_name=cls, object_name=obj)
                caption = data.find(b"\x07Caption\x06")
                if caption >= 0:
                    form["first_caption"] = shortstring(data, caption + 9)[0]
                result.setdefault("forms", []).append(form)
        code = self.sections[0]
        start, end = code["offset"], code["offset"] + code["raw_size"]
        pos = b.find(b"\x07\x08TControl", start, end)
        if pos >= 0:
            vmt_va = u32(b, pos + 10)
            vmt = self.offset(vmt_va - self.base)
            size, discriminator = u32(b, vmt - 40), u32(b, vmt - 84)
            evidence = dict(rtti_offset=pos, vmt_va=vmt_va, vmt_offset=vmt,
                            instance_size=size, preceding_dword=discriminator)
            if size in (0x15c, 0x160):
                evidence["die_rule_version"] = (
                    "7" if discriminator in (0x40100000, 0x100000) else "6")
            result["tcontrol"] = evidence
        return result or None

    def report(self):
        b, opt = self.data, self.opt
        resources = self.resources()
        selected_strings = []
        pattern = re.compile(
            rb"Borland\\Delphi\\RTL|\.pdb|/incremental:|DEFAULTLIB:|EDITANDCONTINUE|"
            rb"[A-Z]:\\.*\.cpp$|Microsoft Visual C\+\+ (?:Debug|Runtime) Library|"
            rb"^(?:ToolInit|ToolFree|ToolFunc|ToolGetMenuText|NemuSettings)$")
        for match in re.finditer(rb"[ -~]{4,}", b):
            if pattern.search(match.group()):
                selected_strings.append(dict(offset=match.start(),
                                             text=match.group().decode("ascii")))
        versions = []
        for item in resources:
            if item["path"][0] != 16:
                continue
            data = b[item["offset"]:item["offset"] + item["size"]]
            pos = data.find(bytes.fromhex("bd04effe"))
            if pos >= 0:
                parts = struct.unpack_from("<IIII", data, pos + 8)
                versions.append(dict(
                    file_version=f"{parts[0] >> 16}.{parts[0] & 65535}."
                                 f"{parts[1] >> 16}.{parts[1] & 65535}",
                    product_version=f"{parts[2] >> 16}.{parts[2] & 65535}."
                                    f"{parts[3] >> 16}.{parts[3] & 65535}"))
        return dict(
            path=str(self.path), size=len(b), sha256=hashlib.sha256(b).hexdigest(),
            machine=u16(b, self.pe + 4), image_base=self.base,
            entry_rva=u32(b, opt + 16), linker_version=f"{b[opt+2]}.{b[opt+3]}",
            timestamp_raw=u32(b, self.pe + 8),
            timestamp_utc=datetime.datetime.fromtimestamp(
                u32(b, self.pe + 8), datetime.timezone.utc).isoformat(),
            sections=self.sections, rich=self.rich(), imports=self.imports(),
            exports=self.exports(), debug=self.debug(), versions=versions,
            delphi=self.delphi(resources), evidence_strings=selected_strings)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args()
    print(json.dumps([PE32(path).report() for path in args.files], indent=2))


if __name__ == "__main__":
    main()
