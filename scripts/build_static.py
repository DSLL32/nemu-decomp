#!/usr/bin/env python3
"""Build nemu64-static.exe with native MSVC object/library linking.

Generated source adaptations and OMF/COFF intermediates live in obj/static.
The reconstructed DLL projects are independent of this ordinary user build.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from omf_to_coff import Converter, convert, import_library_aliases
from static_objects import Glue, symbols, namespace
from static_resources import aggregate

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'obj/static'
PASCAL = WORK / 'pascal'
DEFAULT_FILES = ('config.ini', 'nemu64.ini', 'LemD3DCombine.dat', 'LemD3DCombine.new')
PROFILE_APIS = {'GetPrivateProfileStringA': 24, 'GetPrivateProfileStructA': 20,
                'WritePrivateProfileStringA': 16, 'WritePrivateProfileStructA': 20}
THEME_APIS = {'GetSysColor': 4, 'GetSysColorBrush': 4,
              'DrawFrameControl': 16, 'DrawEdge': 16}
STATIC_APIS = dict(PROFILE_APIS, **THEME_APIS)
COMPONENTS = [('kailleraclient.dll', False), ('gs_dll.dll', False),
              ('Plugin/audio.dll', True), ('Plugin/Input.dll', True),
              ('Plugin/LemD3D8.dll', True), ('Plugin/debugger.dll', True),
              ('Plugin/dialogs.dll', True), ('Plugin/profile.dll', True),
              ('Plugin/LemCombineDebugger.dll', True)]
PROJECTS = [('input', 'Input'), ('debugger', 'debugger'), ('dialogs', 'dialogs'),
            ('profile', 'profile'), ('lemcombinedebugger', 'LemCombineDebugger')]


def win(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')


class Build:
    def __init__(self):
        WORK.mkdir(parents=True, exist_ok=True)
        self.env = os.environ.copy()
        if not self.env.get('WINEPREFIX'):
            parent = Path.home() / '.ai-tmp'
            parent.mkdir(exist_ok=True)
            scratch = Path(tempfile.mkdtemp(prefix='nemu-static-build.', dir=parent))
            self.env['WINEPREFIX'] = str(scratch / 'wine')
            print('Wine prefix:', self.env['WINEPREFIX'], flush=True)
        self.env.setdefault('WINEARCH', 'win32')
        self.env.setdefault('WINEDEBUG', '-all')
        self.log = (WORK / 'build.log').open('w')
        self.glue = Glue()
        self.apis = {}
        self.objects = []
        self.resource_inputs = []
        self.report = {}

    def tool(self, tool, args, cwd=ROOT, chain='vc7'):
        command = [sys.executable, str(ROOT / 'scripts/run_toolchain.py'), chain, tool, *map(str,args)]
        self.log.write(' '.join(command) + '\n'); self.log.flush()
        result = subprocess.run(command, env=self.env, cwd=cwd, stdout=self.log, stderr=subprocess.STDOUT)
        if result.returncode:
            self.log.flush()
            print((WORK / 'build.log').read_text()[-9000:], file=sys.stderr)
            raise RuntimeError(f'{tool} failed; see obj/static/build.log')

    def pascal(self):
        print('Compiling shared Delphi RTL/VCL and built-in Pascal plugins...', flush=True)
        PASCAL.mkdir(exist_ok=True)
        # Regenerate the unit cache as a group; all units must use the same RTL.
        for pattern in ('*.dcu', '*.obj'):
            for p in PASCAL.glob(pattern): p.unlink()
        base = ROOT / 'toolchains/d6u2'
        for directory in ('rtl/sys', 'rtl/common', 'rtl/win', 'vcl'):
            for p in (base / 'source' / directory).iterdir():
                if p.suffix.lower() in ('.pas', '.inc'):
                    shutil.copy2(p, PASCAL / p.name.lower())
        for name in ('StaticFiles', 'StaticVclTheme'):
            shutil.copy2(ROOT / ('src/static/' + name + '.pas'), PASCAL / (name.lower() + '.pas'))
        for p in (base / 'lib').glob('*.res'): shutil.copy2(p, PASCAL / p.name)
        flags = ['-JPN', '-$D-', '-$L-', '-$Y-', '-N.', '-E.', '-U.', '-I.', '-R.']
        self.tool('dcc32-personal', ['-y', *flags, 'system.pas'], PASCAL, 'd6u2')
        # DCC emits SysInit.obj while compiling System but its reserved unit
        # interface must come from the matching compiler's shipped DCU.
        shutil.copy2(base / 'lib/sysinit.dcu', PASCAL / 'sysinit.dcu')
        helper = WORK / 'dfm'
        helper.mkdir(exist_ok=True)
        self.tool('dcc32-personal', ['-Q', '-N.', '-E.', win(ROOT / 'scripts/delphi_dfm.dpr')], helper, 'd6u2')
        roots, exports = ['StaticVclTheme'], {}
        shared_sources = {}
        for folder, project in PROJECTS:
            source = ROOT / 'src' / folder
            local = list(source.glob('*.pas')) + list((source / 'components').glob('*.pas'))
            mapping = {p.stem.lower(): 'Static_' + folder + '_' + p.stem for p in local}
            # These recovered component units are shared by both projects
            # (ThemeMgr differs only in its provenance comment).
            if folder in ('debugger', 'dialogs'):
                mapping.update({p.stem.lower(): 'Static_Shared_' + p.stem
                                for p in (source / 'components').glob('*.pas')})
            entry = 'Static_' + project
            roots.append(entry)
            for p in local:
                text = rename_pascal(p.read_text(encoding='latin1'), mapping)
                if folder == 'lemcombinedebugger' and p.stem in ('UnitCombineEditor', 'UnitCombiners'):
                    text = re.sub(r'(?i)\buses\b', 'uses StaticFiles,', text, count=1)
                    text = text.replace('Reset(CombineFile)', 'StaticReset(CombineFile, CombineFilename)')
                if folder == 'debugger' and p.stem in ('UnitDebugCommands', 'UnitDebugMemory'):
                    text = re.sub(r'(?i)\buses\b', 'uses StaticVclTheme,', text, count=1)
                    # Adapt painters and register-color resets, including their
                    # comparisons. Keep all original colors in light mode.
                    text = re.sub(r'(?i)(?<![\w$])(clWhite|clBlack|clBlue|clRed|clGray|'
                                  r'\$00FFCACA|\$AEFFFF|\$FFAEAE|\$D7FFAE|\$40FFFB)(?!\w)',
                                  r'StaticThemeColor(\1)', text)
                    if p.stem == 'UnitDebugMemory':
                        text = text.replace('    Brush.Color := StaticThemeColor(clWhite);\n    FillRect',
                            '    Font.Color := StaticThemeColor(clBlack);\n'
                            '    Brush.Color := StaticThemeColor(clWhite);\n    FillRect', 1)
                        # VCL's bsClear brush normalizes its stored color to
                        # literal white. Restore the palette before opaque text
                        # (ASCII/address columns and subsequent rows).
                        text = text.replace('          Brush.Style := bsSolid;',
                            '          Brush.Style := bsSolid;\n'
                            '          Brush.Color := StaticThemeColor(clWhite);')
                if p.stem.lower() == 'themesrv':
                    # The recovered XP theme painter otherwise paints light
                    # backgrounds over the static build's dark VCL palette.
                    text = text.replace('  SysUtils;', '  SysUtils;\n\n'
                        'function NemuStaticThemeIsDark: LongBool; cdecl;\n'
                        "  external 'nemu64-static.exe' name 'NemuStaticThemeIsDark';")
                    text = text.replace('Result := FThemesAvailable and FUseThemes;',
                        'Result := not NemuStaticThemeIsDark and FThemesAvailable and FUseThemes;')
                target = PASCAL / (mapping[p.stem.lower()].lower() + '.pas')
                if target.stem.startswith('static_shared_'):
                    signature = pascal_signature(text)
                    if target in shared_sources and shared_sources[target] != signature:
                        raise ValueError('Shared component sources differ: ' + str(p))
                    shared_sources[target] = signature
                target.write_text(text, encoding='latin1')
                dfm = p.with_suffix('.dfm')
                if dfm.exists():
                    dest = target.with_suffix('.dfm')
                    command = ['wine', str(helper / 'delphi_dfm.exe'), 'resource', win(dfm), win(dest)]
                    subprocess.run(command, env=self.env, cwd=ROOT, stdout=self.log, stderr=self.log, check=True)
                    self.resource_inputs.append((dest, 0))
            for p in (source / 'components').glob('*.inc'):
                shutil.copy2(p, PASCAL / p.name)
            text = (source / (project + '.dpr')).read_text(encoding='latin1')
            text = rename_pascal(text, mapping)
            if folder == 'lemcombinedebugger':
                # Keep the editor's on-disk write/rename path, but don't create
                # empty files at startup: reads have embedded fallbacks.
                text = re.sub(r'  if not FileExists\((?:New)?CombineFilename\) then Rewrite\((?:CombineFile|NewFile)\);\n', '', text)
            text = re.sub(r"\s+in\s+'[^']*'", '', text, flags=re.I)
            text = re.sub(r'\{\$R[^}]*\}', '', text, flags=re.I)
            text = re.sub(r'^library\s+\w+;', 'unit ' + entry + ';\ninterface', text, flags=re.I)
            exp = re.search(r'\bexports\s+(.*?);\s*begin\s*end\.', text, re.I | re.S)
            names = re.findall(r'(\w+)(?:\s+index\s+\d+)?\s*(?:,|$)', exp[1].strip(), re.I)
            exports[project] = names
            # Keep the original declarations/types in the interface, then the
            # entry point signatures; implementations retain their source body.
            first = re.search(r'^(?:procedure|function)\s+\w+', text, re.M | re.I).start()
            headers = []
            for name in names:
                match = re.search(r'^(?:procedure|function)\s+' + name + r'\b.*?;\s*cdecl;', text, re.M | re.I | re.S)
                if not match: raise ValueError('Missing Pascal export ' + name)
                headers.append(match[0])
            text = text[:first] + '\n'.join(headers) + '\nimplementation\n' + text[first:exp.start()] + '\nend.\n'
            (PASCAL / (entry.lower() + '.pas')).write_text(text, encoding='latin1')
        # Compile one root so the compiler reports its real unit dependencies.
        (PASCAL / 'static_all.pas').write_text('unit static_all; interface uses ' + ','.join(roots) + '; implementation end.\n')
        self.tool('dcc32-personal', [*flags, '-DNEMU_FIX_HID_ENUMERATION', 'static_all.pas'], PASCAL, 'd6u2')
        libraries = []
        for name in ('kernel32', 'user32', 'gdi32', 'advapi32', 'ole32', 'oleaut32', 'comctl32', 'comdlg32', 'shell32', 'winmm', 'imm32', 'version', 'wininet', 'wsock32', 'mpr', 'uuid', 'winspool'):
            for directory in ('lib', 'platformsdk/lib'):
                path = ROOT / 'toolchains/vc7' / directory / (name + '.lib')
                if path.exists(): libraries.append(path); break
        aliases = import_library_aliases(libraries)
        aliases.update({name: '_NemuStatic' + name + '@' + str(size) for name, size in STATIC_APIS.items()})
        aliases['NemuStaticDefaultData'] = '_NemuStaticDefaultData'
        aliases['NemuStaticThemeIsDark'] = '_NemuStaticThemeIsDark'
        bridge = Converter((PASCAL / 'staticvcltheme.obj').read_bytes())
        bridge.declarations()
        for symbol in bridge.symbols:
            if symbol.name.lower().startswith('@staticvcltheme@nemustaticvclthemerefresh$'):
                aliases[symbol.name] = '_NemuStaticVclThemeRefresh'
        metadata = {}
        for path in sorted(PASCAL.glob('*.obj')):
            image = Converter(path.read_bytes())
            image.declarations()
            if path.stem in ('system', 'sysinit'):
                for s in image.symbols:
                    for name in ('VclInit', 'VclExit', 'TlsIndex'):
                        if s.name.startswith('@Sysinit@' + name + '$') or s.name == '@Sysinit@' + name:
                            aliases[s.name] = '_StaticDelphi' + name
            output = WORK / 'coff' / path.name
            metadata[path.stem.lower()] = convert(path, output, aliases)
            self.objects.append(output)
        for folder, project in PROJECTS:
            unit = 'Static_' + project
            image = Converter((PASCAL / (unit.lower() + '.obj')).read_bytes())
            image.declarations()
            entries = {}
            for name in exports[project]:
                candidates = [s.name for s in image.symbols if s.section and s.name.lower().startswith(('@' + unit + '@' + name + '$').lower())]
                if len(candidates) != 1: raise ValueError((name, candidates))
                alias = '_StaticAPI_' + project + '_' + name
                self.glue.jump(alias, candidates[0]); entries[name] = alias
            self.apis[project] = entries
        order, seen = [], set()
        def visit(name):
            key = Path(name.replace('\\','/')).stem.lower()
            if key in seen: return
            seen.add(key)
            if key not in metadata: raise ValueError('Missing object dependency ' + key)
            for dependency in reversed(metadata[key]['dependencies']): visit(dependency)
            order.append(key)
        visit('static_all')
        self.report['delphi'] = list(metadata.values())
        self.report['initialization_order'] = order
        for item in metadata.values():
            for name in item['resources']:
                if Path(name).suffix.lower() == '.res':
                    matches = [p for p in PASCAL.iterdir() if p.name.lower() == name.lower()]
                    if len(matches) != 1: raise ValueError('Missing resource ' + name)
                    self.resource_inputs.append((matches[0], 0))
        return metadata, order


    def native(self):
        print('Compiling native components into private COFF symbol namespaces...', flush=True)
        settings = [
            ('gs', 'gs_dll', 4096, 'Addsubcode gs_dll gs_exeDlg GS_interpret NewCodeDlg PropertiesDlg RW_INI_files StdAfx', 'gs_dll.rc'),
            ('audio', 'audio/ahle_ds', 8192, 'ABI1 ABI2 AudioHLE ABI3 AudioLegacy DirectSound AudioLog CPUFeatures Jpeg Mp3 AudioProcessing AudioResample AudioPlugin RSP', 'Audio.rc'),
            ('kaillera', 'kailleraclient', 12288, 'reliableconnection config message udpconnection jnetlib/asyncdns jnetlib/connection jnetlib/httpget jnetlib/util listview resizer connect kailleraclient session serverbrowser', 'kaillera.rc'),
            ('gfx', 'lemd3d', 16384, '3DFunctions DListParser Direct3d INIFile LemNemu StdAfx blender combiner prerender vendor/amd3d/detect vendor/amd3d/math_lib', 'LemNemu.rc')]
        for component, folder, offset, units, rc in settings:
            directory = WORK / component
            source = directory / 'src'
            shutil.copytree(ROOT / 'src' / folder, source, dirs_exist_ok=True)
            for path in source.rglob('*'):
                if path.suffix.lower() not in ('.c', '.cpp', '.h'): continue
                text = path.read_text(encoding='latin1')
                if component == 'gfx' and path.name == 'combiner.cpp':
                    text = '#include "' + win(ROOT / 'src/static/StaticFiles.h').replace('\\', '/') + '"\n' + text
                    begin = text.index('void LoadDatFile()')
                    end = text.index('void ConfigureCombiner', begin)
                    body = text[begin:end].replace('FILE *in;', 'NemuStaticText *in;')
                    body = re.sub(r'fopen\(DatFilename, "rt?"\)', 'NemuStaticTextOpen(DatFilename)', body)
                    body = body.replace('while (!feof(in)){\n\t\tfscanf(in, "%s\\n", str);', 'while (NemuStaticTextToken(in, str, sizeof(str)) == 1){')
                    body = body.replace('feof(in)', 'NemuStaticTextEof(in)').replace('fgets(str, 256, in)', 'NemuStaticTextLine(in, str, sizeof(str))').replace('fclose(in)', 'NemuStaticTextClose(in)')
                    text = text[:begin] + body + text[end:]
                if path.name.lower() == 'resource.h':
                    text = re.sub(r'(#define\s+ID[DRIBS]_[A-Za-z_0-9]+\s+)(\d+)',
                                  lambda m: m[1] + str(int(m[2]) + offset), text)
                text = re.sub(r'(MAKEINTRESOURCE[A-W]*\s*\(\s*)(\d+)(\s*\))',
                              lambda m: m[1] + str(int(m[2]) + offset) + m[3], text)
                path.write_text(text, encoding='latin1')
            objects = []
            for unit in units.split():
                path = source / (unit + '.cpp')
                if not path.exists(): path = source / (unit + '.c')
                obj = directory / (unit.replace('/', '_') + '.obj')
                flags = ['/nologo', '/c', '/O2', '/MT', '/GX', '/Gy', '/DWIN32', '/DNDEBUG', '/D_WINDOWS', '/D_USRDLL', '/W3']
                if component == 'gfx':
                    flags += ['/D_MBCS', '/DLEMNEMU_EXPORTS', '/I' + win(source / 'vendor/amd3d'), '/I' + win(ROOT / 'toolchains/vc7/platformsdk/include/prerelease')]
                self.tool('cl', [*flags, '/Fo' + win(obj), win(path)])
                objects.append(obj)
            if component == 'gfx':
                for name, tool, filename in [('matrix-c', 'cl', 'matrix.c'), ('matrix-asm', 'ml', 'matrix.asm')]:
                    obj = directory / (name + '.obj')
                    args = ['/nologo', '/c', *(['/coff'] if tool == 'ml' else ['/O2', '/MT']), '/Fo' + win(obj), win(source / 'vendor/amd3d' / filename)]
                    self.tool(tool, args); objects.append(obj)
            mapping = {}
            for path in objects:
                for _, name, section, value, storage in symbols(path.read_bytes()):
                    if storage == 2 and (section != 0 or value): mapping[name] = '__static_' + component + '_' + name
            for name, size in STATIC_APIS.items():
                original = '_' + name + '@' + str(size)
                target = '_NemuStatic' + name + '@' + str(size)
                mapping[original] = target
                mapping['__imp_' + original] = '__imp_' + target
            private = directory / 'private'
            private.mkdir(exist_ok=True)
            for path in objects:
                obj = private / path.name
                obj.write_bytes(namespace(path.read_bytes(), mapping))
                self.objects.append(obj)
            self.report[component + '_symbols'] = mapping
            if component in ('audio', 'gfx'):
                if component == 'audio':
                    names = re.findall(r'^\s+(\w+)\s+@\d+', (source / 'audio.def').read_text(), re.M)
                else:
                    # These are precisely the plugin ABI functions queried by
                    # the host; optional functions simply remain absent.
                    names = ['GetDllInfo', 'InitiateGFX', 'CloseDLL', 'DllAbout', 'DllConfig', 'DllTest', 'ChangeWindow', 'MoveScreen', 'DrawScreen', 'ProcessDList', 'RomClosed', 'RomOpen', 'UpdateScreen', 'ViStatusChanged', 'ViWidthChanged', 'VISetUCodeID', 'SetResolution', 'FillRectHack', 'GetCombinerPointer']
                entries = {}
                for name in names:
                    target = mapping.get('_' + name)
                    if target:
                        alias = '_StaticAPI_' + component + '_' + name
                        self.glue.jump(alias, target); entries[name] = alias
                self.apis['LemD3D8' if component == 'gfx' else 'audio'] = entries
            if component == 'kaillera':
                for name, target in mapping.items():
                    if re.fullmatch(r'_kaillera\w+@\d+', name): self.glue.pointer('__imp_' + name, target)
            if component == 'gs':
                for name, target in mapping.items():
                    if name.startswith(('?GSInitialize@@', '?GSUpdate@@', '?GSConfigure@@')): self.glue.jump(name, target)
            # Compile original resource IDs, then relocate their resource names
            # as one unit (control IDs inside each window stay unchanged).
            resource = directory / 'original.res'
            self.tool('rc', ['/I' + win(ROOT / 'src' / folder), '/dNDEBUG', '/fo' + win(resource), win(ROOT / 'src' / folder / rc)])
            self.resource_inputs.append((resource, offset))


    def defaults(self):
        declarations, entries = [], []
        self.report['embedded_defaults'] = {}
        for index, name in enumerate(DEFAULT_FILES):
            data = (ROOT / 'master' / name).read_bytes()
            symbol = 'defaultData' + str(index)
            values = [str(byte) for byte in data] + ['0']
            rows = [', '.join(values[pos:pos + 32]) for pos in range(0, len(values), 32)]
            declarations.append('static const unsigned char ' + symbol + '[] = {\n' + ',\n'.join(rows) + '\n};')
            entries.append('    {"' + name + '", (const char *)' + symbol + ', ' + str(len(data)) + '}')
            self.report['embedded_defaults'][name] = dict(size=len(data), sha256=hashlib.sha256(data).hexdigest())
        text = '\n'.join(declarations) + '\nstruct DefaultFile { const char *name, *data; unsigned int size; };\n'
        text += 'static const DefaultFile defaultFiles[] = {\n' + ',\n'.join(entries) + '\n};\n'
        (WORK / 'StaticDefaults.inc').write_text(text)
        output = WORK / 'StaticFiles.obj'
        self.tool('cl', ['/nologo', '/c', '/O2', '/MT', '/GX', '/I' + win(WORK), '/Fo' + win(output), win(ROOT / 'src/static/StaticFiles.cpp')])
        self.objects.append(output)


    def link(self, metadata, order, host_objects):
        print('Generating static bindings and linking nemu64-static.exe...', flush=True)
        declarations, units, registry = [], [], []
        for name, size in STATIC_APIS.items():
            target = '_NemuStatic' + name + '@' + str(size)
            self.glue.pointer('__imp_' + target, target)
        for key in order:
            item = metadata[key]
            image = Converter(Path(item['source']).read_bytes())
            image.declarations()
            values = []
            for kind, names in [('.dinit', item['initialization_symbols']), ('.dfini', item['finalization_symbols'])]:
                if not names: values.extend(['NULL', '0']); continue
                if len(names) != 1: raise ValueError('Multiple initialization tables')
                alias = '_StaticTable_' + key + ('_init' if kind == '.dinit' else '_fini')
                # An address alias must preserve data, so expose the actual
                # table through one pointer cell in the generated object.
                self.glue.pointer(alias, names[0])
                size = sum(section.size for section in image.sections if section.name == kind)
                if size % 6: raise ValueError('Malformed Delphi initialization table')
                count = size // 6
                declarations.append('extern "C" DelphiInit *' + alias[1:] + ';')
                values.extend([alias[1:], str(count)])
            if values[1] != '0' or values[3] != '0': units.append('    {' + ', '.join(values) + '}')
        for project, entries in self.apis.items():
            array = []
            for name, alias in entries.items():
                declarations.append('extern "C" void __cdecl ' + alias[1:] + '(void);')
                array.append('    {"' + name + '", (FARPROC)' + alias[1:] + '}')
            registry.append('static const StaticProcedure procedures_' + project + '[] = {\n' + ',\n'.join(array) + '\n};')
        plugin_order = ['audio', 'Input', 'LemD3D8', 'debugger', 'dialogs', 'profile', 'LemCombineDebugger']
        plugin_rows = ['    {"' + name + '.dll", procedures_' + name + ', sizeof(procedures_' + name + ') / sizeof(StaticProcedure)}' for name in plugin_order]
        native_entries = []
        for component in ['kaillera', 'audio', 'gfx']:
            mapping = self.report[component + '_symbols']
            candidates = [target for name, target in mapping.items() if name == '_DllMain@12']
            if len(candidates) != 1: raise ValueError(('DllMain', component, candidates))
            alias = '_StaticEntry_' + component + '@12'
            self.glue.jump(alias, candidates[0])
            declarations.append('extern "C" BOOL WINAPI StaticEntry_' + component + '(HINSTANCE, DWORD, LPVOID);')
            native_entries.append('StaticEntry_' + component)
        text = '\n'.join(declarations) + '\n'
        text += 'struct StaticUnit { DelphiInit *init; unsigned int initCount; DelphiInit *fini; unsigned int finiCount; };\n'
        text += 'static const StaticUnit delphiUnits[] = {\n' + ',\n'.join(units) + '\n};\n'
        text += 'struct StaticProcedure { const char *name; FARPROC address; };\n'
        text += 'struct StaticPlugin { const char *filename; const StaticProcedure *procedures; unsigned int count; };\n'
        text += '\n'.join(registry) + '\nstatic const StaticPlugin plugins[] = {\n' + ',\n'.join(plugin_rows) + '\n};\n'
        text += 'static BOOL (WINAPI *const nativeEntries[])(HINSTANCE, DWORD, LPVOID) = {' + ', '.join(native_entries) + '};\n'
        (WORK / 'StaticBindings.inc').write_text(text)
        glue = WORK / 'StaticGlue.obj'
        glue.write_bytes(self.glue.emit_coff())
        self.objects.append(glue)
        runtime = WORK / 'StaticRuntime.obj'
        self.tool('cl', ['/nologo', '/c', '/O2', '/MT', '/GX', '/I' + win(WORK), '/Fo' + win(runtime), win(ROOT / 'src/static/StaticRuntime.cpp')])
        self.objects.append(runtime)
        theme = WORK / 'StaticTheme.obj'
        self.tool('cl', ['/nologo', '/c', '/O2', '/MT', '/Fo' + win(theme), win(ROOT / 'src/static/StaticTheme.cpp')])
        self.objects.append(theme)
        self.defaults()
        resource = WORK / 'static.res'
        self.report['resources'] = aggregate([(ROOT / 'obj/nemu64/nemu64.res', 0), *self.resource_inputs], resource)
        library = WORK / 'builtins.lib'
        response = WORK / 'archive.rsp'
        response.write_text('/nologo /OUT:"' + win(library) + '"\n' + '\n'.join('"' + win(p) + '"' for p in self.objects))
        self.tool('lib', ['@' + win(response)])
        response = WORK / 'link.rsp'
        options = ['/nologo', '/LTCG', '/SUBSYSTEM:WINDOWS', '/INCREMENTAL:NO', '/MACHINE:I386', '/OUT:"' + win(ROOT / 'bin/nemu64-static.exe') + '"', '/MAP:"' + win(WORK / 'nemu64-static.map') + '"', '/LIBPATH:"' + win(ROOT / 'toolchains/vc7/platformsdk/lib/prerelease') + '"', '/DELAYLOAD:d3d8.dll', '/DELAYLOAD:dsound.dll']
        libraries = ['nafxcw.lib', 'libcmt.lib', 'delayimp.lib', 'd3dx8.lib', 'd3d8.lib', 'dxerr8.lib', 'dsound.lib', 'winmm.lib', 'kernel32.lib', 'user32.lib', 'gdi32.lib', 'advapi32.lib', 'ole32.lib', 'oleaut32.lib', 'comctl32.lib', 'comdlg32.lib', 'shell32.lib', 'imm32.lib', 'version.lib', 'wininet.lib', 'wsock32.lib', 'ws2_32.lib', 'mpr.lib', 'uuid.lib', 'winspool.lib']
        # Delphi constructor/initializer records and plugin entry tables root
        # their implementations through ordinary relocations in the archive.
        options += ['/INCLUDE:' + self.report['gs_symbols']['?theApp@@3VCGs_dllApp@@A']]
        options += ['"' + win(p) + '"' for p in host_objects] + ['"' + win(library) + '"', '"' + win(resource) + '"', *libraries]
        response.write_text('\n'.join(options) + '\n')
        (ROOT / 'bin').mkdir(exist_ok=True)
        self.tool('link', ['@' + win(response)])
        (WORK / 'link-report.json').write_text(json.dumps(self.report, indent=2) + '\n')


def pascal_signature(text):
    token = re.compile(r"'(?:(?:'')|[^'])*'|\{[^}]*\}|\(\*.*?\*\)|//[^\n]*|\s+|.", re.S)
    return [m[0] for m in token.finditer(text) if not m[0].isspace()
            and not m[0].startswith(('//', '(*'))
            and (not m[0].startswith('{') or m[0].startswith('{$'))]


def rename_pascal(text, mapping):
    # Identifiers only: preserve comments, directives and string literals.
    token = re.compile(r"'(?:(?:'')|[^'])*'|\{[^}]*\}|\(\*.*?\*\)|//[^\n]*|[A-Za-z_][A-Za-z_0-9]*", re.S)
    return token.sub(lambda m: mapping.get(m[0].lower(), m[0]), text)


def main():
    build = Build()
    metadata, order = build.pascal()
    build.native()
    build.link(metadata, order, [Path(p) for p in sys.argv[1:]])


if __name__ == '__main__':
    main()
