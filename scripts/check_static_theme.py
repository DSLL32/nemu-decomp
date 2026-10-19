"""Compatibility probe used by check_static.py --components."""
import shutil
import sys
from build_static import ROOT, WORK, win
from pe_fingerprints import PE32
from omf_to_coff import Converter, convert


def check(run, scratch):
    fixture = scratch / 'theme'
    fixture.mkdir()
    shutil.copy2(ROOT / 'scripts/fixtures/static_theme/probe.cpp', fixture / 'probe.cpp')
    run('compile-theme', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
        'vc7', 'cl', '/nologo', '/MT', '/I' + win(ROOT / 'src/static'), 'probe.cpp',
        '/link', 'user32.lib', 'gdi32.lib', 'comctl32.lib', '/out:probe.exe'], cwd=fixture)
    imports = {row['dll'].lower() for row in PE32(fixture / 'probe.exe').imports()}
    assert not imports.intersection({'dwmapi.dll', 'uxtheme.dll'})
    output = run('theme-compatibility', ['wine', 'probe.exe'], cwd=fixture)
    assert output.count('PASS:') == 5


def check_ui(run, scratch):
    fixture = scratch / 'theme-ui'
    fixture.mkdir()
    for name in ('forced.cpp', 'ui_probe.c', 'StaticDebuggerProbe.pas'):
        shutil.copy2(ROOT / 'scripts/fixtures/static_theme' / name, fixture / name)
    shutil.copy2(ROOT / 'scripts/nemu_probe.c', fixture / 'nemu_probe.c')
    run('compile-theme-ui', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
        'vc7', 'cl', '/nologo', '/c', '/O2', '/MT', '/I' + win(ROOT / 'src/static'),
        'forced.cpp'], cwd=fixture)
    for unit in (WORK / 'pascal').glob('*.dcu'):
        shutil.copy2(unit, fixture / unit.name)
    run('compile-debugger-pascal', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
        'd6u2', 'dcc32-personal', '-JPN', '-$D-', '-$L-', '-$Y-', '-N.', '-E.',
        '-U' + win(WORK / 'pascal'), 'StaticDebuggerProbe.pas'], cwd=fixture)
    source = fixture / 'StaticDebuggerProbe.obj'
    converter = Converter(source.read_bytes()); converter.declarations()
    name = next(s.name for s in converter.symbols if s.section and '@ProbeDebugger$' in s.name)
    convert(source, fixture / 'debugger-probe.obj', {name: '_ProbeDebugger'})
    options = []
    for line in (WORK / 'link.rsp').read_text().splitlines():
        if line.upper().startswith('/MAP:'):
            line = '/MAP:themed.map'
        if line.upper().startswith('/PDB:'):
            line = '/PDB:themed.pdb'
        if line.upper().startswith('/OUT:'):
            line = '/OUT:themed.exe'
        if line.lower().endswith('builtins.lib"'):
            options.extend(['forced.obj', 'debugger-probe.obj'])
        options.append(line)
    (fixture / 'link.rsp').write_text('\n'.join(options) + '\n')
    run('link-theme-ui', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
        'vc7', 'link', '@link.rsp'], cwd=fixture)
    run('compile-theme-ui-probe', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
        'vc7', 'cl', '/nologo', '/MT', '/DNEMU_STATIC_PROBE', 'ui_probe.c', '/link',
        'user32.lib', 'gdi32.lib', '/out:ui_probe.exe'], cwd=fixture)
    (fixture / 'roms').mkdir()
    config = (ROOT / 'master/config.ini').read_text(encoding='latin1')
    (fixture / 'config.ini').write_text('\n'.join(
        'Folder=.\\roms\\' if line.startswith('Folder=') else line
        for line in config.splitlines()) + '\n', encoding='latin1')
    output = run('theme-client-ui', ['setarch', 'i386', '-R', 'wine', 'ui_probe.exe', 'themed.exe'], cwd=fixture)
    assert 'PASS: full static UI' in output
