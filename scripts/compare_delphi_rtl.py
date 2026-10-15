#!/usr/bin/env python3
"""Compare mapped Delphi RTL/VCL code with statically linked target DLLs.

Requires python3-capstone. Supply a reference DLL and its detailed Delphi MAP
file, e.g. the d6u1/probe outputs from check_toolchains.py. This is a baseline
comparison, not an update-version detector. No binaries are executed.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
from capstone.x86 import X86_OP_IMM

from pe_fingerprints import PE32, u16, u32


def identity(path):
    return dict(path=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def code_section(pe):
    return next(s for s in pe.sections if s["name"] == "CODE")


def relocations(pe):
    rva, size = pe.directory(5)
    if not size:
        return set()
    pos = pe.offset(rva)
    end = pos + size
    result = set()
    while pos < end:
        page, length = struct.unpack_from("<II", pe.data, pos)
        if length < 8 or length % 2 or pos + length > end:
            raise ValueError("Malformed relocation block")
        for at in range(pos + 8, pos + length, 2):
            value = u16(pe.data, at)
            if value >> 12 == 3:  # IMAGE_REL_BASED_HIGHLOW
                result.add(page + (value & 4095))
        pos += length
    return result


def pattern(data, mask):
    return re.compile(b"".join(re.escape(bytes([b])) if fixed else b"."
                               for b, fixed in zip(data, mask)), re.S)


def find(pe, signature):
    section = code_section(pe)
    return [m.start() - section["offset"] + section["rva"]
            for m in signature.finditer(pe.data, section["offset"],
                                       section["offset"] + section["virtual_size"])]


def templates(pe, map_path, decoder):
    symbols = {}
    for offset, name in re.findall(r"^ 0001:([0-9A-F]{8})\s+(\S+)\s*$",
                                   map_path.read_text(), re.M):
        symbols.setdefault(int(offset, 16), set()).add(name)
    offsets = sorted(symbols)
    section = code_section(pe)
    reloc = relocations(pe)
    result = []
    for offset, following in zip(offsets, offsets[1:]):
        names = sorted(symbols[offset])
        size = following - offset
        if not ("." in names[0] or names[0].startswith("@")) or not 32 <= size <= 2048:
            continue
        rva = section["rva"] + offset
        va = pe.base + rva
        data = pe.data[pe.offset(rva):pe.offset(rva) + size]
        instructions = list(decoder.disasm(data, va))
        if sum(i.size for i in instructions) != size:
            continue
        mask = bytearray(b"\1" * size)
        for address in reloc:
            if rva <= address <= rva + size - 4:
                mask[address - rva:address - rva + 4] = b"\0" * 4
        for ins in instructions:
            if ((ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP)) and ins.operands
                    and ins.operands[0].type == X86_OP_IMM
                    and not va <= ins.operands[0].imm < va + size):
                # Retain internal branch distances and all ordinary constants.
                if ins.imm_size != 4:
                    break
                at = ins.address - va + ins.imm_offset
                mask[at:at + 4] = b"\0" * 4
        else:
            if sum(mask) < 24:
                continue
            signature = pattern(data, mask)
            if find(pe, signature) != [rva]:
                continue  # Require a unique match in the reference itself.
            result.append(dict(names=names, reference_rva=rva, length=size,
                               fixed_bytes=sum(mask), data=data, mask=mask,
                               signature=signature))
    return result


def lstrcatn_variant(pe, template, decoder):
    """Check a concrete near-match observed in both original Nemu plugins.

    Relax only the register-selection byte of MOV EDI,EAX at offset 0x15.
    All other fixed bytes, including internal branches, must still match.
    This does not assign the variant to an official Delphi update.
    """
    data, mask = template["data"], bytearray(template["mask"])
    if data[0x15:0x17] not in (b"\x89\xc7", b"\x89\xcf"):
        return dict(status="reference has an unsupported instruction variant")
    mask[0x16] = 0
    hits = find(pe, pattern(data, mask))
    result = []
    for rva in hits:
        at = pe.offset(rva)
        candidate = pe.data[at:at + len(data)]
        differences = [dict(offset=i, reference=f"{a:02x}", target=f"{b:02x}")
                       for i, (a, b, fixed) in enumerate(zip(data, candidate,
                                                            template["mask"]))
                       if fixed and a != b]
        ins = next(decoder.disasm(candidate[0x15:0x17], pe.base + rva + 0x15))
        result.append(dict(rva=rva, va=pe.base + rva, file_offset=at,
                           instruction_va=ins.address,
                           instruction=f"{ins.mnemonic} {ins.op_str}",
                           fixed_byte_differences=differences))
    reference_ins = next(decoder.disasm(data[0x15:0x17], 0))
    return dict(reference_instruction=f"{reference_ins.mnemonic} {reference_ins.op_str}",
                compared_bytes=len(data),
                fixed_bytes=template["fixed_bytes"], candidates=result)


def validate_links(ref, target, rows, decoder):
    """Validate masked direct edges when both endpoint blocks were templated.

    This does not resolve untemplated helpers, global data, imports, or indirect
    calls. A small independent check anchors three methods to TControl's VMT.
    """
    by_rva = {r["reference_rva"]: r for r in rows}
    checked = unresolved = 0
    conflicts = []
    for row in rows:
        if len(row["target_rvas"]) != 1:
            continue
        rva, dest = row["reference_rva"], row["target_rvas"][0]
        offset = ref.offset(rva)
        for ins in decoder.disasm(ref.data[offset:offset + row["length"]], ref.base + rva):
            if not ((ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP)) and ins.operands
                    and ins.operands[0].type == X86_OP_IMM):
                continue
            callee = ins.operands[0].imm - ref.base
            if rva <= callee < rva + row["length"]:
                continue
            if callee not in by_rva:
                unresolved += 1
                continue
            pc = dest + ins.address - (ref.base + rva)
            at = target.offset(pc)
            actual = next(decoder.disasm(target.data[at:at + 15], target.base + pc))
            actual_rva = actual.operands[0].imm - target.base
            checked += 1
            if actual_rva not in by_rva[callee]["target_rvas"]:
                conflicts.append(dict(caller=row["names"], callee=by_rva[callee]["names"],
                                      target_rva=actual_rva,
                                      expected_rvas=by_rva[callee]["target_rvas"]))
    anchors = []
    ref_vmt = ref.delphi([])["tcontrol"]["vmt_offset"]
    target_vmt = target.delphi([])["tcontrol"]["vmt_offset"]
    for slot, name in ((-4, "TControl.Destroy"), (104, "TControl.SetParent"),
                       (116, "TControl.WndProc")):
        rva = u32(ref.data, ref_vmt + slot) - ref.base
        row = by_rva[rva]
        if name not in row["names"]:
            raise ValueError(f"Unexpected TControl VMT slot {slot}: {row['names']}")
        target_rva = u32(target.data, target_vmt + slot) - target.base
        anchors.append(dict(name=name, vmt_slot=slot, reference_rva=rva,
                            target_rva=target_rva, length=row["length"],
                            matches=target_rva in row["target_rvas"]))
    return dict(checked_direct_edges=checked, unresolved_direct_edges=unresolved,
                conflicts=conflicts, tcontrol_vmt_anchors=anchors)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("map", type=Path)
    parser.add_argument("targets", nargs="+", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--matches", type=Path, help="Optional full match inventory")
    parser.add_argument("--validate-links", action="store_true",
                        help="Check mapped call/jump endpoints and TControl VMT anchors")
    args = parser.parse_args()
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    ref = PE32(args.reference)
    candidates = templates(ref, args.map, decoder)
    report = dict(
        reference=identity(args.reference), map=identity(args.map),
        method="Map-bounded 32..2048-byte blocks; >=24 fixed bytes; unique in reference. "
               "Mask PE HIGHLOW relocations and external rel32 branch/call operands.",
        limitations=[
            "Matches establish normalized instruction-byte similarity; optional link checks "
            "cover only mapped direct edges and three TControl VMT anchors, not global data "
            "or all callees, so this is not proof of full semantic identity.",
            "Map spans may include alignment or embedded data, not just instructions.",
            "Missing matches may reflect smart linking, code changes, or block boundaries.",
            "Precompiled runtime units identify runtime code, not the exact DCC32 executable.",
            "This single-reference report does not assign an update; compare candidate reports."],
        template_count=len(candidates), targets=[])
    examples = {"@LStrClr", "@LStrAsg", "@LStrCat3", "@LStrCmp",
            "TObject.InitInstance", "TControl.Create", "TControl.Destroy",
                "TControl.ChangeScale", "TReader.ReadPropValue", "TControl.WndProc",
                "@ValInt64", "TThreadLocalCounter2.Open", "TThreadLocalCounter.Open",
                "TMultiReadExclusiveWriteSynchronizer.BeginWrite",
                "TReader.ReadRootComponent", "TApplication.ActivateHint"}
    inventory = []
    lstr = next(t for t in candidates if "@LStrCatN" in t["names"])
    for path in args.targets:
        pe = PE32(path)
        rows = []
        for t in candidates:
            row = {k: t[k] for k in ("names", "reference_rva", "length", "fixed_bytes")}
            row["target_rvas"] = find(pe, t["signature"])
            rows.append(row)
        counts = Counter("unique" if len(r["target_rvas"]) == 1 else
                         "absent" if not r["target_rvas"] else "ambiguous" for r in rows)
        report["targets"].append(dict(
            **identity(path), counts=dict(counts),
            unique_match_bytes=sum(r["length"] for r in rows if len(r["target_rvas"]) == 1),
            examples=[r for r in rows if examples.intersection(r["names"])],
            absent_names=[r["names"] for r in rows if not r["target_rvas"]],
            lstrcatn_variant=lstrcatn_variant(pe, lstr, decoder)))
        if args.validate_links:
            report["targets"][-1]["link_validation"] = validate_links(ref, pe, rows, decoder)
        inventory.append(dict(target=str(path), blocks=rows))
    text = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.write_text(text)
    if args.matches:
        args.matches.write_text(json.dumps(inventory, indent=2) + "\n")
    print(text, end="")


if __name__ == "__main__":
    main()
