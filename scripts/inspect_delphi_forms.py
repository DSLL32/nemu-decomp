#!/usr/bin/env python3
"""Recover Delphi form resources and published field/method metadata from PE32.

The JSON records names present in the binary, without inferring method bodies.
--forms-dir writes the original TPF0 resource streams for delphi_dfm.dpr.
"""

import argparse
import hashlib
import json
from pathlib import Path

from pe_fingerprints import PE32, shortstring, u16, u32


def classes(pe):
    b = pe.data
    code = next(s for s in pe.sections if s['name'] == 'CODE')

    def off(va):
        return pe.offset(va - pe.base)

    def class_name(vmt):
        return shortstring(b, off(u32(b, off(vmt) - 44)))[0]

    result = []
    for rva in range(code['rva'], code['rva'] + code['virtual_size'] - 76, 4):
        at = pe.offset(rva)
        vmt = pe.base + rva + 76
        if u32(b, at) != vmt:
            continue
        at += 76
        parent = u32(b, at - 36)
        # A self-referencing DWORD can also occur in ordinary machine code.
        name_va = u32(b, at - 44)
        if not pe.base + code['rva'] <= name_va < pe.base + code['rva'] + code['virtual_size']:
            continue
        try:
            name = class_name(vmt)
        except UnicodeDecodeError:
            continue
        if not name.isidentifier():
            continue
        entry = dict(name=name, vmt_va=vmt,
                     instance_size=u32(b, at - 40),
                     parent=class_name(u32(b, off(parent))) if parent else None,
                     fields=[], methods=[])
        info = u32(b, at - 60)
        if info:
            pos = off(info)
            if b[pos] == 7:  # tkClass
                _, pos = shortstring(b, pos + 1)
                entry['unit'] = shortstring(b, pos + 10)[0]
        table = u32(b, at - 56)
        if table:
            pos = off(table)
            count, types = u16(b, pos), off(u32(b, pos + 2))
            pos += 6
            for _ in range(count):
                field_offset, type_index = u32(b, pos), u16(b, pos + 4)
                name, pos = shortstring(b, pos + 6)
                field_vmt = u32(b, off(u32(b, types + 2 + 4 * type_index)))
                entry['fields'].append(dict(name=name, offset=field_offset,
                                            type=class_name(field_vmt)))
        table = u32(b, at - 52)
        if table:
            pos = off(table)
            count = u16(b, pos)
            pos += 2
            for _ in range(count):
                length, va = u16(b, pos), u32(b, pos + 2)
                name, end = shortstring(b, pos + 6)
                if length != end - pos:
                    raise ValueError('Unexpected published method entry layout')
                entry['methods'].append(dict(name=name, va=va))
                pos += length
        result.append(entry)
    return result


def inspect(path, forms_dir=None):
    pe = PE32(path)
    resources = pe.resources()
    metadata = pe.delphi(resources)
    by_name = {c['name']: c for c in classes(pe)}
    forms = []
    for form in metadata.get('forms', []):
        data = pe.data[form['offset']:form['offset'] + form['size']]
        entry = dict(form, sha256=hashlib.sha256(data).hexdigest(),
                     **by_name[form['class_name']])
        forms.append(entry)
        if forms_dir:
            forms_dir.mkdir(parents=True, exist_ok=True)
            (forms_dir / (form['resource'] + '.bin')).write_bytes(data)
    return dict(path=str(path), sha256=hashlib.sha256(pe.data).hexdigest(),
                units=metadata['package']['units'], exports=pe.exports(), forms=forms)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--forms-dir', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = json.dumps(inspect(args.dll, args.forms_dir), indent=2) + '\n'
    if args.output:
        args.output.write_text(result)
    else:
        print(result, end='')


if __name__ == '__main__':
    main()
