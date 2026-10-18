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
import struct
import subprocess
import sys
import tempfile

from verify_audio import compare_objects, compare_native_link_layout, load_symbols, ORIGINAL
from verify_gs import compare_dll
from pe_fingerprints import PE32
from seed_audio_pdb import seed_pdb

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'src/audio/ahle_ds'
DSOUND_LIBRARY = ROOT/'toolchains/vc6/lib/dsound.lib'
FLAGS = ['/nologo','/c','/O2','/Ob1','/MT','/W3','/DWIN32','/DNDEBUG','/D_WINDOWS','/D_USRDLL']
ORDER = ['ABI1','ABI2','AudioHLE','ABI3','AudioLegacy',
         'DirectSound','AudioLog','CPUFeatures','Jpeg','Mp3',
         'AudioProcessing','AudioResample','AudioPlugin','RSP']


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
    toolchain_manifest[str(DSOUND_LIBRARY.relative_to(ROOT))] = sha(DSOUND_LIBRARY)
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
                    ROOT/'toolchains/vc7/bin/link.exe',ROOT/'toolchains/vc7/bin/lib.exe',
                    ROOT/'toolchains/vc7/lib/libcmt.lib',
                    *[ROOT/'scripts'/name for name in ['build_audio.py','verify_audio.py','verify_gs.py','run_toolchain.py','seed_audio_pdb.py']]]},
                commands=[],artifact=str(snapshot))
    print('Audio build:',work,'\nWine prefix:',env['WINEPREFIX'],flush=True)
    def tool(label,name,*options,extra_env=None):
        command=[sys.executable,str(ROOT/'scripts/run_toolchain.py'),'vc7',name,*options]
        log=snapshot/(label+'.log')
        with log.open('w') as output:
            process=subprocess.run(command,cwd=work,env=dict(env,**(extra_env or {})),stdout=output,stderr=subprocess.STDOUT,timeout=180)
        report['commands'].append(dict(label=label,argv=command,exit_code=process.returncode))
        if process.returncode:
            raise RuntimeError(log.read_text(errors='replace'))
    result=1
    clock_file=None
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
            link_options=['/nologo','/dll','/incremental:no','/debug','/opt:noref','/opt:noicf',
                 '/subsystem:windows','/machine:I386','/base:0x10000000','/align:4096','/filealign:4096',
                 '/out:audio.dll','/pdb:audio.pdb','/map:audio.map','/def:audio.def',
                 *[str(p.relative_to(work)) for p in objects],*resources,
                 'Z:'+str(DSOUND_LIBRARY).replace('/','\\'),
                 'winmm.lib','kernel32.lib','user32.lib','gdi32.lib']
            tool('link','link',*link_options)
            shutil.copyfile(work/'audio.dll',work/'audio.unstamped.dll')
            shutil.copyfile(work/'audio.pdb',snapshot/'audio.initial.pdb')
            shutil.copyfile(work/'audio.exp',snapshot/'audio.initial.exp')

            # Recover only the historical PDB identity. LINK generates the
            # complete final image, including all clock fields and RSDS bytes.
            seeded,identity=seed_pdb((snapshot/'audio.initial.pdb').read_bytes())
            (snapshot/'audio.seeded.pdb').write_bytes(seeded)
            (snapshot/'pdb-identity-seed.json').write_text(json.dumps(identity,indent=2)+'\n')
            drive=snapshot/'E'
            historical_directory=drive/'nemu/audiohle/ahle_ds/ahle_ds___Win32_Release_nemu8'
            historical_directory.mkdir(parents=True)
            mapping=Path(env['WINEPREFIX'])/'dosdevices/e:'
            if mapping.is_symlink(): mapping.unlink()
            elif mapping.exists(): raise RuntimeError('Private Wine e: mapping is not a symbolic link')
            mapping.symlink_to(drive)
            historical_pdb=historical_directory/'audio.pdb'
            historical_pdb.write_bytes(seeded)
            clock_file=snapshot/'link-clock.txt'
            clock_file.write_text('+0\n')
            clock_env=dict(LD_PRELOAD='/usr/$LIB/faketime/libfaketime.so.1',
                FAKETIME_TIMESTAMP_FILE=str(clock_file),FAKETIME_NO_CACHE='1',
                FAKETIME_DONT_FAKE_MONOTONIC='1',FAKETIME_DISABLE_SHM='1',NO_FAKE_STAT='1',TZ='UTC')
            report['link_identity']=dict(seed=identity,clock_environment=clock_env,
                warmup_clock='+0',export_clock='2003-03-23 04:56:25',
                final_clock='2003-03-23 04:56:26',
                initial_dll_sha256=sha(work/'audio.unstamped.dll'),
                initial_pdb_sha256=sha(snapshot/'audio.initial.pdb'),
                seeded_pdb_sha256=sha(snapshot/'audio.seeded.pdb'),
                faketime_libraries={str(p):sha(p) for p in Path('/usr/lib').glob('*/faketime/libfaketime.so.1')})
            def wine_control(label,command,extra=None,accepted=(0,)):
                with (snapshot/(label+'.log')).open('w') as output:
                    process=subprocess.run(command,cwd=work,env=dict(env,**(extra or {})),
                        stdout=output,stderr=subprocess.STDOUT,timeout=60)
                report['commands'].append(dict(label=label,argv=command,exit_code=process.returncode))
                if process.returncode not in accepted: raise RuntimeError('Wine clock preparation failed: '+label)
            wine_control('clock-stop-server',['wineserver','-k'],accepted=(0,1))
            wine_control('clock-wait-server',['wineserver','-w'])
            wine_control('clock-start-server',['wineserver','-p'],clock_env)
            wine_control('clock-warm-wine',['wineboot','-u'],clock_env)
            next_clock=clock_file.with_suffix('.new')
            # The original export directory predates the COFF/debug clocks by
            # one second. Generate its ordinary .exp input with native LIB at
            # that time; LINK then emits the DLL at the later recorded time.
            # Both export data and the final image remain tool-generated.
            next_clock.write_text('2003-03-23 04:56:25\n')
            next_clock.replace(clock_file)
            tool('exports-historical','lib','/nologo','/def:audio.def',
                 '/machine:ix86','/out:audio.lib',extra_env=clock_env)
            shutil.copyfile(work/'audio.exp',snapshot/'audio.exports.exp')
            report['link_identity']['export_object_sha256']=sha(snapshot/'audio.exports.exp')
            next_clock.write_text('2003-03-23 04:56:26\n')
            next_clock.replace(clock_file)
            historical_path=r'e:\nemu\audiohle\ahle_ds\ahle_ds___Win32_Release_nemu8\audio.pdb'
            final_options=[('/pdb:'+historical_path) if x=='/pdb:audio.pdb' else x
                           for x in link_options if x!='/def:audio.def'] + ['audio.exp']
            tool('link-historical','link',*final_options,extra_env=clock_env)
            shutil.copyfile(historical_pdb,snapshot/'audio.final.pdb')
            def link_metadata(path):
                pe=PE32(path)
                rva,size=pe.directory(6)
                if size!=28: raise RuntimeError('Expected one native debug-directory entry')
                entry=struct.unpack_from('<IIHHIIII',pe.data,pe.offset(rva))
                export_rva,_=pe.directory(0)
                return dict(coff_timestamp=struct.unpack_from('<I',pe.data,pe.pe+8)[0],
                    debug_timestamp=entry[1],
                    export_timestamp=struct.unpack_from('<I',pe.data,pe.offset(export_rva)+4)[0],
                    codeview=pe.data[entry[-1]:entry[-1]+entry[-3]].hex())
            actual=link_metadata(work/'audio.dll');expected=link_metadata(ORIGINAL)
            report['link_identity'].update(final_pdb_sha256=sha(snapshot/'audio.final.pdb'),
                emitted_metadata=actual,expected_metadata=expected,metadata_exact=actual==expected)
            if actual!=expected: raise RuntimeError('Native final LINK did not reproduce historical metadata')
            report.update(compare_dll(work/'audio.dll',ORIGINAL))
            report['native_link_layout'] = compare_native_link_layout(report['objects'], work/'audio.map')
            result=0 if report['full_dll_byte_exact'] else 1
    except (RuntimeError,subprocess.TimeoutExpired) as error:
        report['build_error']=str(error)
        print(str(error),file=sys.stderr)
    finally:
        if clock_file is not None:
            clock_file.write_text('+0\n')
            subprocess.run(['wineserver','-k'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=30)
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
