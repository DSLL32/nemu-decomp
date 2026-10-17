#!/usr/bin/env python3
"""Build the recovered audio sources with the original VC7 compiler/runtime.

The default attempts a complete DLL link and requires strict original-file
identity. --unit/--objects-only are explicitly incomplete COFF diagnostics.
Every run preserves a source snapshot, exact commands, logs and tool hashes.
Missing implementations are never replaced by stubs or original machine code.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from verify_audio import compare_objects, load_symbols, ORIGINAL
from verify_gs import compare_dll

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'src/audio/ahle_ds'
FLAGS = ['/nologo','/c','/O2','/Ob1','/MT','/W3','/DWIN32','/DNDEBUG','/D_WINDOWS','/D_USRDLL']
ORDER = ['ABI1','ABI2','AudioHLE','ABI3','AudioLegacy',
         'DirectSound','AudioLog','CPUFeatures','Jpeg','Mp3',
         'AudioProcessing','AudioSSE','AudioEnvelope','AudioProfile','AudioADPCM',
         'AudioPoleFilter','AudioResample','AudioADPCMWide','AudioADPCMLegacy',
         'AudioPlugin','RSP']


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work',type=Path)
    parser.add_argument('--report',type=Path)
    parser.add_argument('--objects-only',action='store_true')
    parser.add_argument('--unit',action='append',default=[],help='compile selected source basename; diagnostic only')
    args=parser.parse_args()
    parent=Path.home()/'.ai-tmp'; parent.mkdir(exist_ok=True)
    scratch=args.work.resolve() if args.work else Path(tempfile.mkdtemp(prefix='nemu-audio-build.',dir=parent))
    if not scratch.is_relative_to(parent.resolve()) or scratch==parent.resolve():
        parser.error('--work must be a folder below ~/.ai-tmp')
    snapshot=scratch/'runs'/next(tempfile._get_candidate_names())
    snapshot.mkdir(parents=True)
    staged=snapshot/'source'; shutil.copytree(SOURCE,staged)
    work=snapshot/'build'; shutil.copytree(staged,work)
    symbols=snapshot/'symbols'; symbols.mkdir()
    for path in [*sorted((ROOT/'analysis').glob('audio*addresses.json')), ROOT/'analysis/audio-libraries.json']:
        if path.is_file(): shutil.copyfile(path,symbols/path.name)
    records=load_symbols(symbols)
    toolchain_manifest={str(p.relative_to(ROOT)):sha(p)
        for p in sorted((ROOT/'toolchains/vc7').rglob('*')) if p.is_file()}
    manifest=snapshot/'toolchain-sha256.json'
    manifest.write_text(json.dumps(toolchain_manifest,indent=2)+'\n')
    env=dict(os.environ,WINEPREFIX=str(scratch/'wine'),WINEARCH='win32',WINEDEBUG='-all')
    report=dict(original=str(ORIGINAL),original_sha256=sha(ORIGINAL),full_dll_byte_exact=False,
                diagnostic_only=args.objects_only or bool(args.unit),work=str(work),wine_prefix=env['WINEPREFIX'],
                source_sha256={str(p.relative_to(staged)):sha(p) for p in staged.rglob('*') if p.is_file()},
                symbol_sha256={p.name:sha(p) for p in sorted(symbols.iterdir())},
                toolchain_manifest=dict(path=str(manifest),sha256=sha(manifest),files=len(toolchain_manifest)),
                tool_sha256={str(p.relative_to(ROOT)):sha(p) for p in [ROOT/'toolchains/vc7/bin/cl.exe',
                    ROOT/'toolchains/vc7/bin/c1.dll',ROOT/'toolchains/vc7/bin/c1xx.dll',ROOT/'toolchains/vc7/bin/c2.dll',
                    ROOT/'toolchains/vc7/bin/link.exe',ROOT/'toolchains/vc7/lib/libcmt.lib',
                    *[ROOT/'scripts'/name for name in ['build_audio.py','verify_audio.py','verify_gs.py','run_toolchain.py']]]},
                commands=[],artifact=str(snapshot))
    print('Audio build:',work,'\nWine prefix:',env['WINEPREFIX'],flush=True)
    def tool(label,name,*options):
        command=[sys.executable,str(ROOT/'scripts/run_toolchain.py'),'vc7',name,*options]
        log=snapshot/(label+'.log')
        with log.open('w') as output:
            process=subprocess.run(command,cwd=work,env=env,stdout=output,stderr=subprocess.STDOUT,timeout=180)
        report['commands'].append(dict(label=label,argv=command,exit_code=process.returncode))
        if process.returncode:
            raise RuntimeError(log.read_text(errors='replace'))
    result=1
    try:
        sources=[p for p in staged.rglob('*') if p.suffix.lower() in ('.c','.cpp','.asm')]
        sources.sort(key=lambda p:(ORDER.index(p.stem) if p.stem in ORDER else len(ORDER),str(p)))
        if args.unit:
            sources=[p for p in sources if p.name in args.unit or p.stem in args.unit]
            if len(sources)!=len(args.unit): raise RuntimeError('Selected unit missing or ambiguous')
        objects=[]
        for source in sources:
            relative=source.relative_to(staged); output=relative.with_suffix('.obj')
            if source.suffix=='.asm': tool('compile-'+source.stem,'ml','/nologo','/c','/coff','/Fo'+str(output),str(relative))
            else: tool('compile-'+source.stem,'cl',*FLAGS,'/Fo'+str(output),str(relative))
            objects.append(work/output)
        report['objects']=compare_objects(objects,records)
        if report['diagnostic_only']:
            result=0 if all(f.get('relocation_masked_equal',False) and not f.get('relocation_target_mismatch_count',0)
                for obj in report['objects'] for f in obj['functions']) else 1
        else:
            resources=[]
            for source in staged.glob('*.rc'):
                output=source.with_suffix('.res').name
                tool('resource-'+source.stem,'rc','/l','0x409','/fo'+output,source.name); resources.append(output)
            tool('link','link','/nologo','/dll','/incremental:no','/debug','/opt:noref','/opt:noicf',
                 '/subsystem:windows','/machine:I386','/base:0x10000000','/align:4096','/filealign:4096',
                 '/out:audio.dll','/pdb:audio.pdb','/map:audio.map','/def:audio.def',
                 '/libpath:Z:'+str(ROOT/'toolchains/vc7/platformsdk/lib/prerelease').replace('/','\\'),
                 *[str(p.relative_to(work)) for p in objects],*resources,
                 'dsound.lib','winmm.lib','kernel32.lib','user32.lib','gdi32.lib')
            shutil.copyfile(work/'audio.dll',work/'audio.unstamped.dll')
            report.update(compare_dll(work/'audio.dll',ORIGINAL))
            result=0 if report['full_dll_byte_exact'] else 1
    except (RuntimeError,subprocess.TimeoutExpired) as error:
        report['build_error']=str(error)
        print(str(error),file=sys.stderr)
    finally:
        report['result_code']=result
        report['toolchain_changed_during_build']=[name for name,digest in toolchain_manifest.items()
            if sha(ROOT/name)!=digest]
        if report['toolchain_changed_during_build']:
            result=1
            report['result_code']=result
            report['full_dll_byte_exact']=False
        data=json.dumps(report,indent=2)+'\n'
        (snapshot/'comparison.json').write_text(data)
        if args.report: args.report.write_text(data)
        for obj in report.get('objects',[]):
            print('%s: %d/%d COFF body diagnostics match'%(Path(obj['object']).name,
                obj['relocation_masked_matches'],len(obj['functions'])))
        print('Complete DLL byte exact:',report['full_dll_byte_exact'])
        print('Frozen evidence:',snapshot/'comparison.json')
    return result


if __name__=='__main__':
    raise SystemExit(main())
