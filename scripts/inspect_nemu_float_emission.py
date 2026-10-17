#!/usr/bin/env python3
"""Replay an immutable VC7 build under Wine/GDB and observe C2 float emission.

This diagnostic reads compiler/linker state; it does not change registers,
memory, inputs, or the build recipe. It checks all output program sections
against the supplied build. The entrypoints require the pinned tool binaries.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

from compare_nemu_functions import read_map
from pe_fingerprints import PE32

ROOT = Path(__file__).resolve().parents[1]
TOOL_HASHES = {
    'link.exe': '1560d85164dfb763dd13f90f33ff9c6f1cd80f2afada03eed8d0a314593581f1',
    'c2.dll': '3755d22dd896ca0045b22b0478d6ad56185d0c98845e2aa2e1d2300460289ad0',
}

# LINK is loaded at its preferred base. C2 addresses are rebased from the
# LoadLibrary return value, so its current function/TU pointers are not guessed.
GDB_SCRIPT = r'''
set pagination off
set confirm off
target remote TARGET
break *0x474cf9
continue
python
import gdb, json, struct
inferior = gdb.selected_inferior()
def u32(address):
    return int.from_bytes(bytes(inferior.read_memory(address, 4)), 'little')
def words(address, count):
    return list(struct.unpack('<' + 'I' * count, bytes(inferior.read_memory(address, count * 4))))
def string(address):
    return inferior.read_memory(address, 256).tobytes().split(b'\0', 1)[0].decode('cp1252')
base = int(gdb.parse_and_eval('$eax'))
delta = base - 0x10700000
functions, reals, sections = [], [], []
def context():
    function = u32(0x1086f5bc + delta)
    module = u32(0x10870cfc + delta)
    return dict(function=string(u32(function + 4)) if function else None,
                module=string(u32(module + 4)) if module else None)
class FunctionProbe(gdb.Breakpoint):
    def stop(self):
        functions.append(context())
        return False
class RealProbe(gdb.Breakpoint):
    def stop(self):
        reals.append(dict(context(), name=string(int(gdb.parse_and_eval('$esi'))),
                          optimizer_index=len(functions) - 1))
        return False
class SectionProbe(gdb.Breakpoint):
    def stop(self):
        descriptor = int(gdb.parse_and_eval('$edi'))
        if u32(descriptor + 8) == 0x40001040:
            sections.append(dict(context(), index=int(gdb.parse_and_eval('$ebp')),
                                 name=string(u32(descriptor + 4)),
                                 section_record=int(gdb.parse_and_eval('$ebx'))))
        return False
FunctionProbe('*%#x' % (0x107bb399 + delta))
RealProbe('*%#x' % (0x107a32f5 + delta))
SectionProbe('*%#x' % (0x1074cb2d + delta))
class DoneProbe(gdb.Breakpoint):
    def stop(self):
        contributions = []
        section = u32(0x4aa334)
        group = u32(section + 12)
        while group:
            if string(u32(group)) == '.rdata':
                contribution = u32(group + 12)
                while contribution:
                    fields = words(contribution, 15)
                    contributions.append(dict(address=contribution, fields=fields))
                    contribution = fields[8]
            group = u32(group + 8)
        for row in sections:
            row['final_fields'] = words(row['section_record'], 18)
        with open(TRACE_PATH, 'w') as output:
            json.dump(dict(c2_base=base, functions=functions, real_creations=reals,
                           readonly_sections=sections, linker_contributions=contributions),
                      output, indent=2)
        return False
DoneProbe('*0x457369')
end
continue
quit
'''


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def winpath(path):
    return 'Z:' + str(path.resolve()).replace('/', '\\')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True, help='Completed build_nemu.py build.json')
    parser.add_argument('--output', type=Path, required=True, help='New directory under ~/.ai-tmp')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(Path.home() / '.ai-tmp') or output.exists():
        parser.error('--output must be a new directory under ~/.ai-tmp')
    build = json.loads(args.build.read_text())
    artifact = Path(build['immutable_artifact_dir']).resolve()
    project, original = artifact / 'build-tree', artifact / 'nemu64.exe'
    prefix = Path(build['wine_prefix']).resolve()
    if not project.is_dir() or not original.is_file() or not prefix.is_dir():
        parser.error('The immutable artifact, build tree, and original Wine prefix must exist')
    toolchain = ROOT / 'toolchains/vc7'
    for name, expected in TOOL_HASHES.items():
        if sha(toolchain / 'bin' / name) != expected:
            parser.error(f'Unsupported binary: {name}')
    command = next(row['command'] for row in reversed(build['commands']) if row['stage'] == 'link')
    if command[2:4] != ['vc7', 'link']:
        parser.error('Expected the tracked VC7 runner link command')
    output.mkdir(parents=True)
    arguments = [('/OUT:' + winpath(output / 'trace.exe') if item.upper().startswith('/OUT:')
                  else '/MAP:' + winpath(output / 'trace.map') if item.upper().startswith('/MAP:')
                  else item) for item in command[4:]]
    environment = dict(os.environ, WINEPREFIX=str(prefix), WINEARCH='win32', WINEDEBUG='-all',
                       WINEPATH=winpath(toolchain / 'bin'),
                       LIB=';'.join(winpath(toolchain / part)
                                    for part in ['lib', 'atlmfc/lib', 'platformsdk/lib']))
    debugger = ['winedbg', '--gdb', '--no-start', str(toolchain / 'bin/link.exe'), *arguments]
    (output / 'command.json').write_text(json.dumps(dict(command=debugger, cwd=str(project)), indent=2) + '\n')
    server = None
    try:
        with (output / 'server.log').open('w') as log:
            server = subprocess.Popen(debugger, cwd=project, env=environment, stdout=log,
                                      stderr=subprocess.STDOUT, start_new_session=True)
            deadline = time.monotonic() + 20
            while time.monotonic() < deadline:
                match = re.search(r'target remote ([^\r\n]+)', (output / 'server.log').read_text())
                if match:
                    break
                if server.poll() is not None:
                    raise RuntimeError('Wine debugger exited; see server.log')
                time.sleep(0.1)
            else:
                raise RuntimeError('Wine debugger did not publish a connection within 20 seconds')
            script = GDB_SCRIPT.replace('TARGET', match[1]).replace('TRACE_PATH', repr(str(output / 'trace.json')))
            (output / 'trace.gdb').write_text(script)
            with (output / 'gdb.log').open('w') as gdb_log:
                result = subprocess.run(['gdb', '-nx', '-batch', '-x', str(output / 'trace.gdb')],
                                        stdout=gdb_log, stderr=subprocess.STDOUT, timeout=180)
            if result.returncode:
                raise RuntimeError('GDB failed; see gdb.log')
            server.wait(timeout=15)
    finally:
        if server is not None and server.poll() is None:
            os.killpg(server.pid, signal.SIGTERM)
            server.wait(timeout=15)
    traced = PE32(output / 'trace.exe')
    baseline = PE32(original)
    if [s['name'] for s in traced.sections] != [s['name'] for s in baseline.sections]:
        raise RuntimeError('Trace output section list differs from the immutable artifact')
    sections = []
    for left, right in zip(baseline.sections, traced.sections):
        before = baseline.data[left['offset']:left['offset'] + left['raw_size']]
        after = traced.data[right['offset']:right['offset'] + right['raw_size']]
        sections.append(dict(name=left['name'], layout_equal=left == right,
                             bytes_equal=before == after,
                             differing_bytes=sum(a != b for a, b in zip(before, after)) + abs(len(before) - len(after))))
    trace = json.loads((output / 'trace.json').read_text())
    if not trace['real_creations'] or not trace['readonly_sections'] or not trace['linker_contributions']:
        raise RuntimeError('Trace is incomplete')
    report = dict(build_sha256=sha(args.build), baseline_sha256=sha(original),
                  traced_sha256=sha(output / 'trace.exe'), tool_sha256=TOOL_HASHES,
                  script_sha256=sha(Path(__file__)), reused_wine_prefix=str(prefix),
                  program_sections=sections, optimizer_function_visits=len(trace['functions']),
                  real_creations=trace['real_creations'],
                  linked_reals=[row for row in read_map(output / 'trace.map') if row['name'].startswith('__real@')],
                  readonly_section_count=len(trace['readonly_sections']),
                  linker_contribution_count=len(trace['linker_contributions']),
                  trace_sha256=sha(output / 'trace.json'),
                  qualification='Readonly section context does not alone bind a named symbol to a section; '
                                'the separate provider trace in nemu-c2-float-emission.json establishes that association.')
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    if not all(row['layout_equal'] and row['bytes_equal'] for row in sections):
        raise RuntimeError('Traced program sections differ from the immutable artifact; see report.json')
    print(f"Observed {len(trace['real_creations'])} real creations; all program sections preserved. {output / 'report.json'}")


if __name__ == '__main__':
    main()
