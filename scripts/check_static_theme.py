"""Compatibility probe used by check_static.py --components."""
import shutil
import sys
from build_static import ROOT, win
from pe_fingerprints import PE32


def check(run, scratch):
    fixture = scratch / 'theme'
    fixture.mkdir()
    shutil.copy2(ROOT / 'scripts/fixtures/static_theme/probe.cpp', fixture / 'probe.cpp')
    run('compile-theme', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'),
        'vc7', 'cl', '/nologo', '/MT', '/I' + win(ROOT / 'src/static'), 'probe.cpp',
        '/link', 'user32.lib', '/out:probe.exe'], cwd=fixture)
    imports = {row['dll'].lower() for row in PE32(fixture / 'probe.exe').imports()}
    assert not imports.intersection({'dwmapi.dll', 'uxtheme.dll'})
    output = run('theme-compatibility', ['wine', 'probe.exe'], cwd=fixture)
    assert output.count('PASS:') == 4
