#!/usr/bin/env python3
"""Map reconstructed application globals and audit their original storage.

This is a source/owner-evidence inventory, not a C++ parser or a claim that an
inferred name or extent is original. Unknown image ranges are reported, never
converted into padding objects. --verify-storage independently compiles the
shared typed definitions with VC7 and checks their complete initial contents.
"""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, struct, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report',type=Path,default=ROOT/'analysis/nemu-globals.json')
parser.add_argument('--verify-storage',action='store_true',help='compile Globals.cpp and compare every defined object with original initial storage')
parser.add_argument('--work',type=Path,help='reusable verification directory under ~/.ai-tmp')
parser.add_argument('--layout-map',type=Path,help='compare known original object addresses with a Microsoft linker MAP file')
args=parser.parse_args()
previous_verification=None
if args.report.exists():
    previous_verification=json.loads(args.report.read_text()).get('globals_cpp_verification')
sys.path.insert(0,str(ROOT/'scripts'))
from pe_fingerprints import PE32
PE=PE32(ROOT/'master/nemu64.exe')

from nemu_storage_source import decls, canon, extent

entries={}; conflicts=[]
for p in sorted((ROOT/'src/nemu64').glob('*')):
    if p.suffix not in ['.h','.cpp','.inc']:continue
    for n,t,init,ext,static,line,raw in decls(p):
        e=entries.setdefault(n,dict(symbol=n,spelling='inferred',declarations=[],observations=[]))
        e['declarations'].append(dict(file=str(p.relative_to(ROOT)),line=line,type=t,canonical_type=canon(t),extent_bytes=extent(t),definition=p.suffix in ('.cpp','.inc') and not ext,internal_linkage=static,initializer=init))

def add(name,va,source,**kw):
    e=entries.setdefault(name,dict(symbol=name,spelling='inferred',declarations=[],observations=[]))
    if isinstance(va,str):va=int(va,16) if not va.startswith('0x') else int(va,0)
    e['observations'].append(dict(va=hex(va),source=source,**kw))

# These two extents have independent consumer evidence beyond their headers.
# Capacity 100 is explicitly an inference, not a recovered original sizeof.
add('MemoryBreakpoints',0xa3fb20,'Original Address/Access indexing and next live object at 0xA3FE40',extent_bytes=800,extent_basis='INFERRED capacity100 from next-object layout and natural round count; no producer bound survives',confidence='inferred')
add('UndoData',0x47ae38,'src/debugger/UnitDebugCommands.pas: reverse read wraps offset0 to0x3FFFFC and reads4 bytes',extent_bytes=0x400000,extent_basis='observed usable ring length; trailing linker padding is not defined as part of this object')
for p in (ROOT/'src/nemu64').glob('*.cpp'):
    for m in re.finditer(r'// Original (?:table|VA) (?:0x)?([0-9A-Fa-f]+)\.?\s*(?:CPUOpcodeHandler|RSPOpcodeHandler|DynarecOpcodeHandler)\s+(\w+)\[(\d+)\]',p.read_text()):
        add(m[2],int(m[1],16),str(p.relative_to(ROOT)),extent_bytes=int(m[3])*4,kind='function_pointer_table')

sources=[];function_addresses={}
for label in ['cpu','cpu-control','memory','rsp','plugins','pif','frontend','browser','dynarec','dynarec-fpu','dynarec-opcodes','dynarec-memory','legacy-data']:
    p=ROOT/'analysis'/('nemu-'+label+'.json')
    if not p.exists():continue
    obj=json.loads(p.read_text());sources.append(str(p.relative_to(ROOT)))
    fs=obj.get('functions',[])
    if isinstance(fs,dict):fs=[dict(name=n,**v) for n,v in fs.items() if isinstance(v,dict)]
    for f in fs:
        fn=f.get('name',f.get('symbol'));fv=f.get('va',f.get('address',f.get('original_va')))
        if fn and fv is not None:
            function_addresses[fn]=(int(fv,16) if isinstance(fv,str) else fv)
    gs=obj.get('globals',{})
    if isinstance(gs,dict):gs=[dict(symbol=n,**v) if isinstance(v,dict) else dict(symbol=n,va=v) for n,v in gs.items()]
    gs=list(gs)+list(obj.get('assets',[]))+list(obj.get('shared_externs',[]))
    for g in gs:
        n=g.get('symbol',g.get('name'));va=g.get('va',g.get('original_va',g.get('address')))
        if n and va is not None:add(n,va,str(p.relative_to(ROOT)),**{k:x for k,x in g.items() if k not in ['symbol','name','va','original_va','address','source']})

# The optimized image folds several empty application functions onto RET.
# CPU table comments, the plugin map, and the empty C++ body corroborate this
# alias independently of its inferred source spelling.
empty=ROOT/'src/nemu64/Dynarec.cpp'
if empty.exists() and re.search(r'void\s+DynarecNoOperation\(\)\s*\{\s*\}',empty.read_text()):
    if PE.data[PE.offset(0x41fef0-PE.base)]==0xc3:
        function_addresses['DynarecNoOperation']=0x41fef0

for n in ['PifResponseTable1','PifResponseTable0','PifResponseHistory']:
    if n in entries:
        original=entries[n]['declarations'][0]
        entries[n]['declarations'].append(dict(original, file='PifTables.inc (build generated by res/generate_pif_tables.py)',line=0,definition=True,initializer='algorithmic generated table'))

def image_observation(va,size):
    sec=next((s for s in PE.sections if PE.base+s['rva']<=va<PE.base+s['rva']+s['virtual_size']),None)
    if sec is None:return dict(status='outside_image')
    delta=va-PE.base-sec['rva'];raw=max(0,min(size,sec['raw_size']-delta))
    data=PE.data[sec['offset']+delta:sec['offset']+delta+raw]
    obs=dict(section=sec['name'],file_backed_bytes=raw,loader_zero_filled_bytes=size-raw,all_zero=not any(data))
    if raw and any(data):
        obs['sha256']=hashlib.sha256(data+b'\0'*(size-raw)).hexdigest()
        if size in (1,2,4,8):obs['unsigned_little_endian']=int.from_bytes(data,'little')
        else:obs['nonzero_byte_count']=sum(x!=0 for x in data)
    return obs

for n,e in entries.items():
    ds=e['declarations'];types=sorted(set(d['canonical_type'] for d in ds));defs=[d for d in ds if d['definition']]
    if len(types)>1 and not (len(set(re.sub(r'\[[^\]]*\]', '[]', t) for t in types))==1 and any('[]' in t for t in types)):conflicts.append(dict(kind='declaration_type',symbol=n,types=types))
    if len(defs)>1:conflicts.append(dict(kind='multiple_definitions',symbol=n,locations=[dict(file=d['file'],line=d['line']) for d in defs]))
    vas=sorted(set(int(o['va'],16) for o in e['observations']))
    if len(vas)>1:conflicts.append(dict(kind='address',symbol=n,addresses=[hex(v) for v in vas]))
    ex=sorted(set(d['extent_bytes'] for d in ds if d['extent_bytes'] is not None))
    if not ex:
        ex=sorted(set(o.get('extent_bytes',o.get('size')) for o in e['observations'] if isinstance(o.get('extent_bytes',o.get('size')),int)))
    if len(vas)==1:e['va']=hex(vas[0])
    if len(ex)==1:e['extent_bytes']=ex[0]
    if len(types)==1:e['canonical_type']=types[0]
    e['definition_status']='defined' if defs else 'declaration_only'
    if n=='ROMAlternateBase':
        e['storage_kind']='compile_time_guest_address_constant'
        e['original_address_required']=False
        e['note']='0x13FCB4D0 is the guest-ROM mirror adjustment used as an immediate. No separately allocated host object is required.'
    if len(vas)==len(ex)==1:e['original_storage']=image_observation(vas[0],ex[0])
    if len(vas)==len(ex)==1 and defs and e.get('original_storage',{}).get('all_zero') is False:
        val=defs[0]['initializer']
        if val and re.fullmatch(r'(?:0x[0-9a-fA-F]+|[0-9]+|true|false)',val) and ex[0] in (1,2,4,8):
            scalar={'true':1,'false':0}.get(val)
            if scalar is None:scalar=int(val,0)
            e['initializer_validation']=dict(kind='scalar',source_value=scalar,equal=scalar==e['original_storage'].get('unsigned_little_endian'))
        elif val and ('OpcodeHandler' in defs[0]['type'] or 'LegacyCallback' in defs[0]['type'] or 'Dispatch' in n):
            names=[x.strip() for x in val.strip('{} ').split(',') if x.strip()]
            off=PE.offset(vas[0]-PE.base);target=list(struct.unpack_from('<'+'I'*(ex[0]//4),PE.data,off))
            items=[dict(index=i,source_symbol=fn,original_target=hex(target[i]),source_target=hex(function_addresses[fn]) if fn in function_addresses else None) for i,fn in enumerate(names) if i<len(target)]
            e['initializer_validation']=dict(kind='function_pointer_table',entries=items,equal=len(names)==len(target) and all(x['source_target']==x['original_target'] for x in items),unresolved_symbols=sorted(set(fn for fn in names if fn not in function_addresses)))
    if e.get('original_storage',{}).get('all_zero') is False and defs and all(d['initializer'] is None for d in defs):
        conflicts.append(dict(kind='missing_nonzero_initializer',symbol=n,va=e['va'],observation=e['original_storage']))

spans=sorted((int(e['va'],16),int(e['va'],16)+e['extent_bytes'],n) for n,e in entries.items() if 'va'in e and 'extent_bytes'in e)
for i,(a,b,n) in enumerate(spans):
    for c,d,m in spans[i+1:]:
        if c>=b:break
        if n!=m:conflicts.append(dict(kind='overlapping_objects',symbols=[n,m],addresses=[hex(a),hex(c)],intersection=[hex(c),hex(min(b,d))]))
unresolved=[dict(symbol=n,reasons=(["original_address_missing"] if 'va'not in e else [])+(["extent_missing"] if 'extent_bytes'not in e else [])+(["definition_missing"] if e['definition_status']=='declaration_only' else [])) for n,e in sorted(entries.items()) if e.get('original_address_required',True) and ('va'not in e or 'extent_bytes'not in e or e['definition_status']=='declaration_only')]
out=dict(schema_version=1,original='master/nemu64.exe',original_sha256=hashlib.sha256(PE.data).hexdigest(),scope='Application globals only; CRT, zlib, compiler jump tables, and untyped byte regions are excluded.',status='In progress; declarations are inventoried without treating absent source or unknown extents as verified.',generator='scripts/map_nemu_globals.py',source_inventories=sources,globals=sorted(entries.values(),key=lambda e:(int(e.get('va','0xffffffff'),16),e['symbol'])),conflicts=conflicts,unresolved_declarations=unresolved)
data_section=next(s for s in PE.sections if s['name']=='.data')
ds=PE.base+data_section['rva'];de=ds+data_section['virtual_size']
known=[(a,b,'application') for a,b,n in spans]
coverage_sources=[]
for filename in ['nemu-library-addresses.json','nemu-vendor-data.json']:
    libraries=ROOT/'analysis'/filename
    if not libraries.exists():continue
    coverage_sources.append(str(libraries.relative_to(ROOT)))
    for s in json.loads(libraries.read_text()).get('data_sections',[]):
        if not s.get('comparison_only') and s.get('va') and s.get('size'):
            known.append((s['va'],s['va']+s['size'],'library'))
pos=ds;unmapped=[]
for a,b,kind in sorted(known):
    if b<=ds or a>=de:continue
    a=max(a,ds);b=min(b,de)
    if a>pos:unmapped.append(dict(start_va=hex(pos),end_va_exclusive=hex(a),size=a-pos))
    pos=max(pos,b)
if pos<de:unmapped.append(dict(start_va=hex(pos),end_va_exclusive=hex(de),size=de-pos))
code_map=ROOT/'analysis/nemu-code-map.json'
memory_references=json.loads(code_map.read_text()).get('global_memory_references',[]) if code_map.exists() else []
for gap in unmapped:
    start=int(gap['start_va'],16);end=int(gap['end_va_exclusive'],16)
    before=[(b,n) for a,b,n in spans if b<=start]
    after=[(a,n) for a,b,n in spans if a>=end]
    gap['preceding_application_symbol']=max(before)[1] if before else None
    gap['following_application_symbol']=min(after)[1] if after else None
    gap['original_storage']=image_observation(start,end-start)
    gap['direct_memory_references']=[x for x in memory_references if start<=x['va']<end]
    gap['status']='unresolved; no padding or unnamed storage object synthesized'
    gap['evidence_limit']='Absence of direct memory operands does not exclude pointer-derived access, dead retained globals, unrecognized library storage, or linker alignment. Neighboring source extents can also be inferred.'
out['data_section_coverage']=dict(start_va=hex(ds),size=data_section['virtual_size'],file_backed_size=data_section['raw_size'],library_coverage_sources=coverage_sources,unmapped_bytes=sum(x['size']for x in unmapped),unmapped_intervals=unmapped,note='Unmapped intervals are not asserted to be objects or padding. No storage definitions are synthesized for them. Verified CRT and vendor data contributions are excluded from these intervals.')
if args.layout_map:
    # C++ public data symbols have the source name before @@. Internal objects
    # can be absent from the map; absence is reported instead of guessed.
    linked={}
    for name,address in re.findall(r'^\s+\w{4}:\w{8}\s+(\S+)\s+([0-9a-fA-F]{8})',args.layout_map.read_text(),re.M):
        linked[name.lstrip('?').split('@@')[0]]=int(address,16)
    groups={};missing=[]
    for e in out['globals']:
        if 'va' not in e or 'extent_bytes' not in e:continue
        name=e['symbol']
        if name not in linked:
            missing.append(name);continue
        original=int(e['va'],16);compiled=linked[name]
        storage=e.get('original_storage',{})
        group=storage.get('section','unknown')+(' zero' if storage.get('all_zero') else ' initialized')
        groups.setdefault(group,[]).append(dict(symbol=name,original_va=hex(original),compiled_va=hex(compiled),extent_bytes=e['extent_bytes'],delta=compiled-original))
    for name,rows in list(groups.items()):
        transitions=[row for index,row in enumerate(rows) if index==0 or row['delta']!=rows[index-1]['delta']]
        groups[name]=dict(objects=len(rows),exact_addresses=sum(row['delta']==0 for row in rows),relative_order_exact=[row['symbol'] for row in rows]==[row['symbol'] for row in sorted(rows,key=lambda row:int(row['compiled_va'],16))],delta_transitions=transitions,objects_by_original_va=rows)
    out['compiled_layout']=dict(map=str(args.layout_map),map_sha256=hashlib.sha256(args.layout_map.read_bytes()).hexdigest(),groups=groups,missing_map_symbols=missing,scope='Known source object addresses only; matching placement does not prove original types, inferred extents, code bytes, or unknown storage boundaries.')
if args.verify_storage:
    from verify_gs import COFF
    parent=Path.home()/'.ai-tmp';parent.mkdir(exist_ok=True)
    work=args.work.resolve() if args.work else Path(tempfile.mkdtemp(prefix='nemu-globals.',dir=parent))
    if not work.is_relative_to(parent.resolve()):parser.error('--work must be under ~/.ai-tmp')
    project=work/'project';project.mkdir(parents=True,exist_ok=True)
    inputs=[]
    for source in [ROOT/'src/nemu64/Globals.cpp',*sorted((ROOT/'src/nemu64').glob('*.h'))]:
        data=source.read_bytes();(project/source.name).write_bytes(data)
        inputs.append(dict(path=str(source.relative_to(ROOT)),sha256=hashlib.sha256(data).hexdigest()))
    generator=ROOT/'src/nemu64/res/generate_pif_tables.py'
    inputs.append(dict(path=str(generator.relative_to(ROOT)),sha256=hashlib.sha256(generator.read_bytes()).hexdigest()))
    subprocess.run([sys.executable,str(ROOT/'src/nemu64/res/generate_pif_tables.py'),str(project/'PifTables.inc')],check=True,stdout=subprocess.PIPE)
    inputs.append(dict(path='PifTables.inc (generated)',sha256=hashlib.sha256((project/'PifTables.inc').read_bytes()).hexdigest()))
    env=dict(os.environ,WINEPREFIX=str(work/'wine'),WINEARCH='win32',WINEDEBUG='-all')
    commands=[]
    for label,extra in [('ltcg',['/GL']),('ordinary',[])]:
        cmd=[sys.executable,str(ROOT/'scripts/run_toolchain.py'),'vc7','cl','/nologo','/c','/O2','/Ob1','/ML','/W3','/DWIN32','/DNDEBUG','/D_WINDOWS',*extra,'/FoGlobals-'+label+'.obj','Globals.cpp']
        run=subprocess.run(cmd,cwd=project,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180)
        (work/(label+'.log')).write_bytes(run.stdout)
        commands.append(dict(command=cmd,returncode=run.returncode))
        if run.returncode:raise RuntimeError(run.stdout.decode(errors='replace'))
    obj=COFF(project/'Globals-ordinary.obj');checks=[]
    for sym in obj.symbols:
        name=sym['name'].lstrip('?').split('@@')[0]
        if name not in entries:continue
        entry=entries[name];size=entry.get('extent_bytes')
        if size is None or 'va' not in entry:raise RuntimeError('Missing storage evidence for '+name)
        va=int(entry['va'],16)
        if sym['section']==0:
            equal=sym['value']==size and entry['original_storage']['all_zero'];kind='COFF common zero'
        else:
            sec=obj.sections[sym['section']-1];data=sec['data'][sym['value']:sym['value']+size]
            if entry['original_storage']['all_zero']:target=bytes(size)
            else:
                off=PE.offset(va-PE.base);target=PE.data[off:off+size]
            equal=data==target;kind=sec['name']
        check=dict(symbol=name,size=size,kind=kind,equal=equal)
        if name=='MemoryBreakpoints':check['extent_status']='capacity100 inferred; equality of initialized bytes does not prove original sizeof'
        checks.append(check)
    out['globals_cpp_verification']=dict(compiler='VC7 13.00.9466 /O2 /Ob1 /ML; ordinary COFF byte/size checks plus /GL compilation',inputs=inputs,work=str(work),commands=commands,scope='Object sizes and complete initial storage; final linker placement and inferred original extents are not proven by this check.',all_equal=all(x['equal']for x in checks),objects=checks)
    (work/'storage-verification.json').write_text(json.dumps(out['globals_cpp_verification'],indent=2)+'\n')
elif previous_verification:
    out['globals_cpp_verification']=previous_verification
    out['globals_cpp_verification']['source_inputs_match_current']=all(
        (ROOT/x['path']).exists() and hashlib.sha256((ROOT/x['path']).read_bytes()).hexdigest()==x['sha256']
        for x in previous_verification.get('inputs',[]) if not x['path'].endswith('(generated)'))
    out['globals_cpp_verification']['reused_snapshot_verification']=True
args.report.parent.mkdir(parents=True,exist_ok=True)
args.report.write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(dict(globals=len(entries),mapped=sum('va'in e for e in entries.values()),conflicts=conflicts,unresolved=len(unresolved)),indent=2))
