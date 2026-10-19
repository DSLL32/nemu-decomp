"""Archive fixture used by check_static.py --components; no standalone runner."""
import json
from pathlib import Path
import shutil
import sys
from build_static import ROOT, WORK, PASCAL, win
from omf_to_coff import Converter, convert
from static_objects import Glue


def archive_options():
    return [line for line in (WORK / 'link.rsp').read_text().splitlines()
            if not line.upper().startswith(('/OUT:', '/MAP:', '/PDB:', '/SUBSYSTEM:', '/LTCG'))
            and not line.lower().endswith('.obj"')]


def check(run, scratch):
    fixture = scratch / 'defaults'
    fixture.mkdir()
    source = ROOT / 'scripts/fixtures/static_defaults'
    for name in ('probe.c', 'StaticDefaultsProbe.pas'):
        shutil.copy2(source / name, fixture / name)
    for unit in PASCAL.glob('*.dcu'):
        shutil.copy2(unit, fixture / unit.name)
    run('compile-defaults-pascal', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
        'd6u2', 'dcc32-personal', '-JPN', '-$D-', '-$L-', '-$Y-', '-N.', '-E.', '-U' + win(PASCAL),
        'StaticDefaultsProbe.pas'], cwd=fixture)
    path = fixture / 'StaticDefaultsProbe.obj'
    converter = Converter(path.read_bytes()); converter.declarations()
    aliases = {}
    for name in ('ReadText', 'IniValue', 'IniWrite'):
        symbol = next(s.name for s in converter.symbols if s.section and ('@' + name.lower() + '$') in s.name.lower())
        aliases[symbol] = '_Probe' + name
    convert(path, fixture / 'pascal.obj', aliases)
    glue = Glue()
    mapping = json.loads((WORK / 'link-report.json').read_text())['gfx_symbols']
    glue.jump('_ProbeLoadDatFile', mapping['?LoadDatFile@@YAXXZ'])
    glue.pointer('_ProbeDatFilename', mapping['?DatFilename@@3PADA'])
    glue.pointer('_ProbeScripts', mapping['?CombineScripts@@3UTCombineScripts@@A'])
    (fixture / 'glue.obj').write_bytes(glue.emit_coff())
    run('compile-defaults-c', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
        'vc7', 'cl', '/nologo', '/c', '/MT', '/I' + win(ROOT / 'src/static'), 'probe.c'], cwd=fixture)
    options = archive_options() + ['/SUBSYSTEM:CONSOLE', '/OUT:probe.exe', 'probe.obj', 'pascal.obj', 'glue.obj']
    (fixture / 'link.rsp').write_text('\n'.join(options) + '\n')
    run('link-defaults', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
        'vc7', 'link', '@link.rsp'], cwd=fixture)
    output = run('embedded-defaults-archive', ['setarch', 'i386', '-R', 'wine', 'probe.exe'], cwd=fixture)
    assert 'PASS: embedded defaults' in output
