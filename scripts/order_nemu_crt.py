#!/usr/bin/env python3
"""Extract unchanged VC7 CRT objects in their observed contribution order.

Independent original function and non-COMDAT data-section addresses constrain
the member order. A topological sort must satisfy all those constraints. This
is ordinary linker input reconstruction: no member bytes or PE bytes are edited.
The emitted response file is for an isolated link experiment; it is not proof
that the original project explicitly listed these runtime objects.
"""

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import tempfile

from map_nemu_libraries import archive_members
from verify_gs import COFF

ROOT = Path(__file__).resolve().parents[1]


def extract_order(inventory, output):
    members, objects = {}, {}
    library = ROOT / 'toolchains/vc7/lib/libc.lib'
    selected = {row['archive_offset']: row for row in inventory['contributions']
                if row['library'] == 'toolchains/vc7/lib/libc.lib'
                and not row.get('comparison_only')}
    output.mkdir(parents=True, exist_ok=True)
    member_dir = output / 'members'
    member_dir.mkdir(exist_ok=True)
    for name, offset, data in archive_members(library):
        if offset not in selected:
            continue
        original = selected[offset]
        digest = hashlib.sha256(data).hexdigest()
        if name != original['object'] or digest != original['sha256']:
            raise ValueError(f'Changed runtime archive member: {name}')
        destination = member_dir / (str(offset) + '-' + name.rsplit('\\', 1)[-1])
        destination.write_bytes(data)
        objects[name] = COFF(destination)
        members[name] = dict(original, extracted_path=str(destination), size=len(data))
    if len(members) != len(selected):
        raise ValueError('The tracked archive does not contain every mapped member')

    sequences = defaultdict(set)
    code_va, data_va = {}, {}
    for function in inventory['symbols']:
        member = function['object']
        if member not in objects:
            continue
        obj = objects[member]
        definitions = [symbol for symbol in obj.symbols
                       if symbol['name'] == function['name'] and symbol['section'] > 0]
        if len(definitions) != 1:
            raise ValueError('Ambiguous library function: ' + function['name'])
        symbol = definitions[0]
        section = obj.sections[symbol['section'] - 1]
        section_va = function['va'] - symbol['value']
        sequences[section['name']].add((section_va, member, section['index']))
        code_va[member] = min(code_va.get(member, function['va']), function['va'])
    for section in inventory['data_sections']:
        member = section['object']
        if member not in objects:
            continue
        data_va[member] = min(data_va.get(member, section['va']), section['va'])
        # Pooling/selection of COMDAT constants does not impose object order.
        if not section['comdat']:
            sequences[section['section']].add((section['va'], member, section['section_index']))

    constraints, edges = [], defaultdict(set)
    for section, sequence in sorted(sequences.items()):
        previous = None
        for va, member, index in sorted(sequence):
            if previous and previous[1] != member and previous[0] != va:
                edges[previous[1]].add(member)
                constraints.append(dict(before=previous[1], after=member, section=section,
                                        before_va=previous[0], after_va=va))
            previous = va, member
    degrees = dict.fromkeys(members, 0)
    for destinations in edges.values():
        for destination in destinations:
            degrees[destination] += 1
    order = []
    while len(order) < len(members):
        ready = [member for member in members if degrees[member] == 0 and member not in order]
        if not ready:
            remaining = [member for member in members if member not in order]
            raise ValueError('Conflicting original contribution constraints: ' + ', '.join(remaining))
        member = min(ready, key=lambda name: (code_va.get(name, 0xffffffff),
                                            data_va.get(name, 0xffffffff), name))
        order.append(member)
        for destination in edges[member]:
            degrees[destination] -= 1
    response = ['/NODEFAULTLIB:libc']
    for member in order:
        path = members[member]['extracted_path']
        if any(character in path for character in '\r\n"'):
            raise ValueError('Unsupported character in response-file path')
        response.append('"Z:' + path.replace('/', '\\') + '"')
    (output / 'crt.rsp').write_text('\n'.join(response) + '\n')
    return dict(kind='observed_crt_contribution_order',
                library=str(library.relative_to(ROOT)),
                library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),
                original_sha256=inventory['original_sha256'],
                member_count=len(order), members=[members[name] for name in order],
                constraints=constraints, response_file=str(output / 'crt.rsp'),
                member_bytes_unchanged=True,
                scope='Member order reconstruction only; no complete-image equality claim.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', type=Path,
                        default=ROOT / 'analysis/nemu-library-addresses.json')
    parser.add_argument('--output-dir', type=Path)
    args = parser.parse_args()
    scratch = Path.home() / '.ai-tmp'
    scratch.mkdir(exist_ok=True)
    output = (args.output_dir.resolve() if args.output_dir else
              Path(tempfile.mkdtemp(prefix='nemu-crt-order.', dir=scratch)))
    if not output.is_relative_to(scratch.resolve()):
        parser.error('--output-dir must be under ~/.ai-tmp')
    report = extract_order(json.loads(args.inventory.read_text()), output)
    report.update(inventory=str(args.inventory),
                  inventory_sha256=hashlib.sha256(args.inventory.read_bytes()).hexdigest())
    (output / 'order.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{report["member_count"]} unchanged runtime members: {output / "crt.rsp"}')


if __name__ == '__main__':
    main()
