#!/usr/bin/env python3
"""Stage typed Nemu application storage in original address order.

Only a new scratch copy is changed. Initializers remain ordinary typed C++;
unknown zero ranges retain their explicit inference labels from the layout
report. The output is input to build_nemu.py's normal VC7 /GL compilation,
not a binary patch or a claim that these inferred files are original.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

from nemu_storage_source import decls, extent

ROOT = Path(__file__).resolve().parents[1]
HEADERS = ['CPU', 'CpuControl', 'Memory', 'Dynarec', 'DynarecFPU',
           'DynarecOpcodes', 'Plugins', 'PIF', 'RSP', 'FrontendIni', 'Rom',
           'RomBrowser', 'WinMain', 'State']


def declaration(name, typ):
    if '(' in typ:
        return re.sub(r'\*\s*\)', '*' + name + ')', typ, count=1)
    at = typ.find('[')
    return typ + ' ' + name if at < 0 else typ[:at].strip() + ' ' + name + typ[at:]


def mask_comments(text):
    return re.sub(r'//[^\n]*|/\*.*?\*/',
                  lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]),
                  text, flags=re.S)


def stage(source, output, inventory, layout):
    source = source.resolve()
    output = output.resolve()
    scratch = (Path.home() / '.ai-tmp').resolve()
    if not output.is_relative_to(scratch) or output == scratch:
        raise ValueError('output must be a new directory under ~/.ai-tmp')
    if output.exists():
        raise ValueError('output already exists; refusing to transform a second time')
    if not source.is_dir():
        raise ValueError('source directory does not exist')
    data = json.loads(inventory.read_text())
    if data['conflicts'] or data['unresolved_declarations']:
        raise ValueError('inventory contains unresolved storage or conflicting declarations')
    entries = {row['symbol']: row for row in data['globals']}
    retained = json.loads(layout.read_text())['retained_ranges']
    shutil.copytree(source, output)
    evidence = output / 'storage-inputs'
    evidence.mkdir(exist_ok=True)
    shutil.copyfile(inventory, evidence / 'nemu-globals.json')
    shutil.copyfile(layout, evidence / 'nemu-storage-layout.json')
    if not (output / 'PifTables.inc').exists():
        subprocess.run([sys.executable, str(output / 'res/generate_pif_tables.py'),
                        str(output / 'PifTables.inc')], check=True)

    definitions = {}
    groups = {}
    inputs = {}
    for path in sorted(output.iterdir()):
        if path.suffix in ('.cpp', '.c', '.h', '.inc'):
            inputs[path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
        if path.suffix not in ('.cpp', '.inc'):
            continue
        for name, typ, initializer, external, static, line, raw in decls(path):
            entry = entries.get(name)
            if external or not entry or entry.get('original_storage', {}).get('section') != '.data':
                continue
            if name in definitions:
                raise ValueError('multiple definitions: ' + name)
            if extent(typ) not in (None, entry['extent_bytes']):
                raise ValueError('source extent differs from inventory: ' + name)
            zero = entry['original_storage']['all_zero']
            if not zero and initializer is None:
                raise ValueError('missing nonzero initializer: ' + name)
            definitions[name] = dict(type=typ, initializer=initializer, zero=zero,
                                     file=path.name, raw=raw, was_static=static)
            groups.setdefault((path.name, raw), []).append(name)
    expected = {name for name, row in entries.items()
                if row.get('original_storage', {}).get('section') == '.data'
                and row['definition_status'] == 'defined'}
    if expected != set(definitions):
        raise ValueError('source/inventory definitions differ: ' + str(sorted(expected ^ set(definitions))))

    for (filename, raw), names in groups.items():
        # The normal build regenerates this include. Preserve its original
        # contents and remove its now-redundant inclusion below; the same typed
        # initializers have moved to NemuInitializedStorage.cpp.
        if filename == 'PifTables.inc':
            continue
        path = output / filename
        text = path.read_text()
        pattern = r'\s*'.join(re.escape(token) for token in re.findall(r'\w+|[^\w\s]', raw)) + r'\s*;'
        matches = list(re.finditer(pattern, mask_comments(text)))
        if len(matches) != 1:
            raise ValueError(f'cannot identify unique declaration in {filename}: {names}')
        match = matches[0]
        replacement = '\n'.join('extern ' + declaration(name, definitions[name]['type']) + ';'
                                for name in names)
        path.write_text(text[:match.start()] + replacement + text[match.end():])

    for path in output.glob('*.cpp'):
        text = path.read_text()
        text = re.sub(r'^\s*#include\s+"PifTables\.inc"[^\n]*',
                      '// CIC data definitions are in NemuInitializedStorage.cpp.',
                      text, flags=re.M)
        path.write_text(text)

    includes = ''.join('#include "' + name + '.h"\n' for name in HEADERS)
    zero_rows = []
    initial_rows = []
    pif_names = [name for name, item in definitions.items() if item['file'] == 'PifTables.inc']
    initial_names = sorted((name for name, item in definitions.items() if not item['zero']),
                           key=lambda name: int(entries[name]['va'], 16))
    if pif_names:
        start = initial_names.index(pif_names[0])
        if initial_names[start:start + len(pif_names)] != pif_names:
            raise ValueError('generated CIC declaration order differs from original storage order')
    for name, item in definitions.items():
        address = int(entries[name]['va'], 16)
        comment = '// Original ' + hex(address) + '; inferred source spelling.\n'
        if item['zero']:
            zero_rows.append((address, comment + declaration(name, item['type']) + ' = {0};\n'))
        elif item['file'] == 'PifTables.inc':
            if name == pif_names[0]:
                initial_rows.append((address, '// Generated typed CIC tables, in original order.\n#include "PifTables.inc"\n'))
        else:
            initial_rows.append((address, comment + declaration(name, item['type']) + ' = ' + item['initializer'] + ';\n'))
    for row in retained:
        comment = ('// Observed zero interval; extent is inferred from neighboring live objects.\n'
                   '// Original type, object count, purpose, and source file are unknown.\n')
        if row.get('alternative_explanation'):
            comment += '// ' + row['alternative_explanation'] + '\n'
        zero_rows.append((int(row['start_va'], 16), comment + row['declaration'] + '\n'))
    preface = ('// Generated source storage staging; original file grouping is unknown.\n'
               '// No application header alignment or compiler options are changed.\n')
    (output / 'NemuZeroStorage.cpp').write_text(preface + includes + '\n' +
                                               ''.join(text for _, text in sorted(zero_rows)))
    functions = set()
    for name, item in definitions.items():
        if 'OpcodeHandler' in item['type'] or name == 'LegacyEmptyCallbacks':
            functions.update(value.strip() for value in item['initializer'].strip('{} ').split(',')
                             if value.strip())
    forward = 'typedef void (__cdecl *LegacyCallback)();\n'
    forward += ''.join('extern void ' + name + '();\n' for name in sorted(functions))
    (output / 'NemuInitializedStorage.cpp').write_text(preface + includes + forward + '\n' +
                                                     ''.join(text for _, text in sorted(initial_rows)))
    report = dict(source=str(source), output=str(output), inventory=str(inventory), layout=str(layout),
                  inventory_sha256=hashlib.sha256(inventory.read_bytes()).hexdigest(),
                  layout_sha256=hashlib.sha256(layout.read_bytes()).hexdigest(), input_hashes=inputs,
                  modified_cpp=sorted({filename for filename, _ in groups if filename.endswith('.cpp')}),
                  generated_cpp=['NemuZeroStorage.cpp', 'NemuInitializedStorage.cpp'],
                  known_zero_objects=sum(item['zero'] for item in definitions.values()),
                  known_initialized_objects=sum(not item['zero'] for item in definitions.values()),
                  retained_ranges=len(retained), retained_bytes=sum(row['size'] for row in retained),
                  compiler_requirement='Compile both generated units with the ordinary application VC7 /GL flags.',
                  link_requirement='Append NemuZeroStorage.obj then NemuInitializedStorage.obj to application objects, before vendor objects.',
                  limitation='Original source grouping and retained storage identities are inferred; initialized data still require CRT contribution ordering.')
    (output / 'storage-stage.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'src/nemu64')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--inventory', type=Path, default=ROOT / 'analysis/nemu-globals.json')
    parser.add_argument('--layout', type=Path, default=ROOT / 'analysis/nemu-storage-layout.json')
    args = parser.parse_args()
    try:
        report = stage(args.source, args.output, args.inventory, args.layout)
    except ValueError as error:
        parser.error(str(error))
    print(json.dumps({key: value for key, value in report.items() if key != 'input_hashes'}, indent=2))


if __name__ == '__main__':
    main()
