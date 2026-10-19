#!/usr/bin/env python3
"""Exercise the static XP-era crash reporter with real optimized x86 faults."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from build_static import ROOT, win
from pe_fingerprints import PE32
from static_crash_symbols import checked_link
from static_resources import aggregate
from check_static_defaults import archive_options
from omf_to_coff import Converter, convert


def worker(scratch):
    prefix=scratch/'wine'
    env=dict(os.environ,WINEPREFIX=str(prefix),WINEARCH='win32',WINEDEBUG='-all',WINEDLLOVERRIDES='winemenubuilder.exe=d')
    def run(label,command,cwd=scratch,timeout=90):
        with (scratch/(label+'.log')).open('w') as log:
            p=subprocess.run(command,cwd=cwd,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=timeout)
        if p.returncode:
            raise RuntimeError(f'{label}: exit {p.returncode}; see {scratch/(label+".log")}')
        return (scratch/(label+'.log')).read_text(errors='replace')
    def tool(label,name,*args):
        return run(label,[sys.executable,str(ROOT/'scripts/run_toolchain.py'),'vc7',name,*args])
    try:
        run('xp-profile',['wine','reg','add',r'HKCU\Software\Wine','/v','Version','/t','REG_SZ','/d','winxp','/f'])
        for name in ('probe.cpp','ui_probe.c','VclCrashProbe.pas','vcl-main.cpp','external.c'):
            shutil.copy2(ROOT/'scripts/fixtures/static_crash'/name,scratch/name)
        tool('compile-crash','cl','/nologo','/c','/O2','/Z7','/MT','/I'+win(ROOT/'src/static'),
             '/FoStaticCrash.obj',win(ROOT/'src/static/StaticCrash.cpp'))
        # /GL + /Zi matches the real core and intentionally retains /Oy.
        tool('compile-probe','cl','/nologo','/c','/O2','/GL','/Zi','/MT','/I'+win(ROOT/'src/static'),
             '/Fdprobe-compile.pdb','probe.cpp')
        options=['/nologo','/LTCG','/DEBUG','/OPT:REF','/OPT:ICF','/INCREMENTAL:NO','/OUT:probe.exe',
                 '/PDB:probe.pdb','/MAP:probe.map','probe.obj','StaticCrash.obj','crash.res','user32.lib','gdi32.lib','kernel32.lib']
        (scratch/'link.rsp').write_text('\n'.join(options)+'\n')
        count=0
        def link():
            nonlocal count
            count+=1;tool('link-'+str(count),'link','@link.rsp')
        checked_link(link,scratch/'probe.map',scratch/'probe.exe',scratch/'symbols.res',scratch/'fpo.res',scratch/'probe.pdb',
            lambda:aggregate([(scratch/'symbols.res',0),(scratch/'fpo.res',0)],scratch/'crash.res'),'crash-probe-v001')
        imports={entry['dll'].lower() for entry in PE32(scratch/'probe.exe').imports()}
        assert imports <= {'kernel32.dll','user32.dll','gdi32.dll'},imports
        tool('compile-ui','cl','/nologo','/O2','/MT','ui_probe.c','/link','user32.lib','gdi32.lib')
        # Nothing beside the EXE in the launch directory: no maps/PDBs/DLLs.
        fixture=scratch/'isolated';fixture.mkdir();shutil.copy2(scratch/'probe.exe',fixture/'probe.exe')
        for mode in ('main','dark','worker','badbp','caught'):
            output=run('crash-'+mode,['setarch','i386','-R','wine',str(scratch/'ui_probe.exe'),'probe.exe',mode],fixture)
            assert 'PASS:' in output
            print(output.strip(),flush=True)
        # Exercise the real generated Delphi VCL hardware-exception bridge.
        for unit in (ROOT/'obj/static/pascal').glob('*.dcu'):
            shutil.copy2(unit,scratch/unit.name)
        run('compile-vcl-pascal',[sys.executable,str(ROOT/'scripts/run_toolchain.py'),'d6u2','dcc32-personal',
            '-JPN','-$D-','-$L-','-$Y-','-N.','-E.','-U'+win(ROOT/'obj/static/pascal'),'VclCrashProbe.pas'])
        converter=Converter((scratch/'VclCrashProbe.obj').read_bytes());converter.declarations()
        symbol=next(s.name for s in converter.symbols if s.section and '@RunVclCrash$' in s.name)
        convert(scratch/'VclCrashProbe.obj',scratch/'vcl-pascal.obj',{symbol:'_RunVclCrash','PostMessageA':'_PostMessageA@16',
            'GetMessageA':'_GetMessageA@16','TranslateMessage':'_TranslateMessage@4','DispatchMessageA':'_DispatchMessageA@4'})
        tool('compile-vcl-main','cl','/nologo','/c','/O2','/Z7','/MT','/I'+win(ROOT/'src/static'),'vcl-main.cpp')
        aggregate([(ROOT/'obj/static/static.res',0)],scratch/'base.res',
                  exclude={(10,'NEMU_CRASH_SYMBOLS'),(10,'NEMU_CRASH_FPO')})
        options=[line for line in archive_options() if not line.lower().endswith('static.res"')]
        options+=['/SUBSYSTEM:CONSOLE','/OUT:probe.exe','/PDB:probe.pdb','/MAP:probe.map',
                  'vcl-main.obj','vcl-pascal.obj','crash.res']
        (scratch/'link.rsp').write_text('\n'.join(options)+'\n')
        checked_link(link,scratch/'probe.map',scratch/'probe.exe',scratch/'symbols.res',scratch/'fpo.res',scratch/'probe.pdb',
            lambda:aggregate([(scratch/'base.res',0),(scratch/'symbols.res',0),(scratch/'fpo.res',0)],scratch/'crash.res'),'vcl-crash-probe01')
        shutil.copy2(scratch/'probe.exe',fixture/'probe.exe')
        print(run('crash-vcl',['setarch','i386','-R','wine',str(scratch/'ui_probe.exe'),'probe.exe','vcl'],fixture).strip(),flush=True)
        # A real external tool crashes after normal static UI/plugin bootstrap.
        tool('compile-external','cl','/nologo','/LD','/O2','/MT','external.c','/link','/OUT:external.dll')
        plugin=fixture/'Plugin';plugin.mkdir();shutil.copy2(scratch/'external.dll',plugin/'external.dll')
        shutil.copy2(ROOT/'bin/nemu64-static.exe',fixture/'nemu64-static.exe')
        print(run('crash-external',['setarch','i386','-R','wine',str(scratch/'ui_probe.exe'),'nemu64-static.exe','external'],fixture).strip(),flush=True)
    finally:
        subprocess.run(['wineserver','-k'],env=env,timeout=15,check=False)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--worker',type=Path,help=argparse.SUPPRESS)
    args=parser.parse_args()
    if args.worker:
        worker(args.worker);return
    parent=Path.home()/'.ai-tmp';parent.mkdir(exist_ok=True)
    scratch=Path(tempfile.mkdtemp(prefix='nemu-crash-probe.',dir=parent));print('Crash probe scratch:',scratch,flush=True)
    command=['xvfb-run','-a','-s','-screen 0 1280x1024x24',sys.executable,str(Path(__file__).resolve()),'--worker',str(scratch)]
    raise SystemExit(subprocess.run(command,env=dict(os.environ,TMPDIR=str(scratch))).returncode)

if __name__=='__main__': main()
