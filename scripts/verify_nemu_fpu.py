#!/usr/bin/env python3
"""Compare original and reconstructed FPU translator behavior in an x86 sandbox.

Compile DynarecFPU.cpp with tracked VC7 /O2 /Ob0 /ML, without /GL, then pass
that ordinary COFF object. Each original and candidate function executes in
Unicorn. Hooks decode original custom-register arguments and candidate cdecl
arguments separately, recording emitter/helper calls and selected global writes.
The diagnostic covers FR selection, register formats, rounding modes, exception
check guards, dispatch profiling, and delay-slot state changes. It certifies
neither machine-code equality nor the behavior of the hooked callees.

No original executable code is copied into reconstruction sources or outputs.
"""

import argparse
from pathlib import Path
import json,sys,re,struct,random,hashlib
from collections import defaultdict
sys.path.insert(0, str(Path(__file__).resolve().parent))
from verify_gs import COFF
from pe_fingerprints import PE32
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UcError
from unicorn.x86_const import *
ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--object', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
cli_args = parser.parse_args()
report_path = cli_args.report
pe=PE32(ROOT/'master/nemu64.exe');obj=COFF(cli_args.object)
meta=json.load(open(ROOT/'analysis/nemu-dynarec-fpu.json'));functions=meta['functions'];byname={r['name']:r for r in functions}
values={};globalmap={}
def num(v):return int(v,16) if isinstance(v,str) else v
def walk(x):
 if isinstance(x,dict):
  name=x.get('name',x.get('symbol'));va=x.get('original_va',x.get('va',x.get('address')))
  if name and va is not None:
   try: values[name]=num(va)
   except(ValueError,TypeError):pass
  for v in x.values():walk(v)
 elif isinstance(x,list):
  for v in x:walk(v)
analysis_files = ['nemu-cpu.json','nemu-dynarec.json','nemu-cpu-control.json','nemu-plugins.json','nemu-dynarec-fpu.json']
for f in analysis_files:
 walk(json.load(open(ROOT/'analysis'/f)))
values.update(Dynarec_Cop1FunctionDispatch=0x477ca0,OpcodeStats=0x97ae68,OpcodeStatsEnabled=0x1240e6c,
              CPU_LastPC=0x2f73484,SetFpuRounding=0x414fa0,EmitBranchTarget=0x4138f0,RecompileDelaySlot=0x413cf0,
              FinishInstruction=0x414010,LoadGPRLow=0x413160,LoadGPR64=0x413190,StoreGPRSigned32=0x4131e0,
              StoreGPR64=0x413240,EmitLabel=0x407400,EmitCompareImmediate=0x407600,EmitConditionalJump=0x4076f0,
              EmitJump=0x407760,EmitAddMemory=0x4074e0,ReportCompilerError=0x414f50)
wrappers=json.loads((ROOT/'analysis/nemu-dynarec.json').read_text())['wrapper_argument_evidence']
hooks={}
for r in wrappers:
 hooks[num(r['va'])]=(r['name'],[p['location'] for p in r['inputs']],[p['type'] for p in r['inputs']]);values[r['name']]=num(r['va'])
for r in functions:
 hooks[num(r['original_va'])]=(r['name'],[p['location'] for p in r.get('inputs',[])],[p['type'] for p in r.get('inputs',[])])
custom={
 'EmitLabel':(['EAX'],['unsigned']), 'EmitCompareImmediate':(['CL','DL','EAX'],['unsigned char','unsigned char','unsigned']),
 'EmitConditionalJump':(['S4'],['unsigned char']),'EmitJump':([],[]),
 'EmitAddMemory':(['S4','S8','CL','S12','AL'],['unsigned char','unsigned char','unsigned char','unsigned','unsigned char']),
 'StoreGPRSigned32':(['EAX'],['unsigned']),'StoreGPR64':(['S4'],['unsigned char']),
 'LoadGPRLow':(['ESI'],['unsigned']),'LoadGPR64':(['EAX'],['unsigned']),
 'EmitBranchTarget':(['ESI'],['unsigned']),'RecompileDelaySlot':([],[]),'FinishInstruction':([],[]),
 'ReportCompilerError':(['S4'],['const char*'])}
for name,(loc,types) in custom.items():hooks[values[name]]=(name,loc,types)
# Invalid/default dispatch entries are observed as opaque calls, not executed.
for i in range(64):
 va=struct.unpack_from('<I',pe.data,pe.offset(0x477ca0-pe.base)+i*4)[0]
 hooks.setdefault(va,('original_dispatch_%08x'%va,[],[]))
regs={'EAX':UC_X86_REG_EAX,'EBX':UC_X86_REG_EBX,'ECX':UC_X86_REG_ECX,'EDX':UC_X86_REG_EDX,
      'ESI':UC_X86_REG_ESI,'EDI':UC_X86_REG_EDI,'EBP':UC_X86_REG_EBP,'ESP':UC_X86_REG_ESP}
low={'AL':'EAX','CL':'ECX','DL':'EDX','BL':'EBX'}
def base_name(name):
 if name.startswith('?') and '@@' in name:return name[1:name.index('@@')]
 return name[1:] if name.startswith('_') and not name.startswith('__') else name
# Relocate every candidate section, keeping actual source-local labels and tables.
sectionbases={s['index']:0x08000000+s['index']*0x1000 for s in obj.sections}
functions_byname={base_name(f['name']):f for f in obj.functions()}
def resolve(s):
 name=base_name(s['name'])
 if name in values:return values[name]
 if s['section']>0:
  if s['name'].startswith('??_C@'):
   section=obj.sections[s['section']-1];data=section['data'][s['value']:].split(b'\0')[0]+b'\0'
   pos=pe.data.find(data)
   if pos>=0:
    for sec in pe.sections:
     if sec['offset']<=pos<sec['offset']+sec['raw_size']:return pe.base+sec['rva']+pos-sec['offset']
  return sectionbases[s['section']]+s['value']
 if s['section']==-1:return s['value']
 raise ValueError('unresolved '+s['name'])
sections={}
for s in obj.sections:
 if s['name']=='.drectve' or s['name'].startswith('.debug'):continue
 data=bytearray(s['data'])
 for rel in s['relocations']:
  off=rel['offset'];target=resolve(rel['symbol']);addend=struct.unpack_from('<I',data,off)[0]
  if rel['type']==6:value=target+addend
  elif rel['type']==20:value=target+addend-(sectionbases[s['index']]+off+4)
  else:raise ValueError('rel type '+str(rel['type']))
  struct.pack_into('<I',data,off,value&0xffffffff)
 sections[s['index']]=bytes(data)
class Engine:
 def __init__(self,candidate):
  self.candidate=candidate;self.u=Uc(UC_ARCH_X86,UC_MODE_32)
  end=max(s['rva']+max(s['virtual_size'],s['raw_size']) for s in pe.sections)
  self.u.mem_map(pe.base,(end+0xfff)&~0xfff)
  for sec in pe.sections:self.u.mem_write(pe.base+sec['rva'],pe.data[sec['offset']:sec['offset']+sec['raw_size']])
  self.u.mem_map(0x08000000,0x200000);self.u.mem_map(0x0e000000,0x1000);self.u.mem_write(0x0e000000,b'\xf4')
  self.u.mem_map(0x0f000000,0x100000)
  for i,data in sections.items():self.u.mem_write(sectionbases[i],data)
  self.u.hook_add(UC_HOOK_CODE,self.hook)
 def word(self,a):return struct.unpack('<I',self.u.mem_read(a,4))[0]
 def write(self,a,v):self.u.mem_write(a,struct.pack('<I',v&0xffffffff))
 def location(self,where):
  if where.startswith('S'):return self.word(self.u.reg_read(UC_X86_REG_ESP)+int(where[1:]))
  return self.u.reg_read(regs[low.get(where,where)]) & (255 if where in low else 0xffffffff)
 def hook(self,u,va,size,data):
  if va==0x0e000000:u.emu_stop();return
  if va not in hooks or va==self.start:return
  name,loc,types=hooks[va]
  if self.candidate:loc=['S'+str(4+4*i) for i in range(len(loc))]
  args=[self.location(loc[i])&(255 if types[i]=='unsigned char' else 0xffffffff) for i in range(len(loc))]
  self.trace.append([name,args])
  result=0
  if name in ['EmitJump','EmitConditionalJump']:
   self.label+=1;result=self.label
  if name=='GetFpuRegisterAddress':
   reg,fmt,full=args
   if fmt in (16,17,20,21):result=0x406f0d0+reg*8 if full else 0x3e69488+(reg&(~1 if fmt in(17,21)else~0))*4
  if name=='RecompileDelaySlot' and self.mutate_delay:
   self.write(values['CPU_Instruction'],0x12345678);self.write(values['CPU_LastPC'],0x80001004)
  u.reg_write(UC_X86_REG_EAX,result)
  sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,self.word(sp));u.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,name,case,args):
  self.trace=[];self.label=100;self.mutate_delay=case.get('mutate_delay',False)
  for r in regs.values():self.u.reg_write(r,0)
  self.u.reg_write(UC_X86_REG_EFLAGS,2)
  for key,value in dict(CPU_Instruction=case['opcode'],CPU_FRMode=case['fr'],Dynarec_DynamicFpuMode=case['dynamic'],
    Dynarec_FpuChecked=case.get('checked',0),Dynarec_FpuEndLabel=0xaaaa,Dynarec_FpuModeLabel=0xbbbb,
    Dynarec_BlockFinished=0,Dynarec_DelaySlot=case.get('delay',0),CPU_LastPC=case.get('pc',0x80001000),
    OpcodeStatsEnabled=case.get('profile',0)).items():self.write(values[key],value)
  sp=0x0f080000;self.u.reg_write(UC_X86_REG_ESP,sp);self.write(sp,0x0e000000)
  f=functions_byname[name]
  self.start=sectionbases[f['section']]+f['value'] if self.candidate else values[name]
  locations=['S'+str(4+4*i) for i in range(len(args))] if self.candidate else hooks[values[name]][1]
  for where,val in zip(locations,args):
   if where.startswith('S'):self.write(sp+int(where[1:]),val)
   else:self.u.reg_write(regs[low.get(where,where)],val)
  try:self.u.emu_start(self.start,0x0e000000,count=10000)
  except UcError as e:raise RuntimeError(name+' at '+hex(self.u.reg_read(UC_X86_REG_EIP))+' '+str(e))
  if self.u.reg_read(UC_X86_REG_EIP)!=0x0e000000:raise RuntimeError('did not return '+name)
  result=dict(trace=self.trace,state={n:self.word(values[n])for n in ['Dynarec_FpuChecked','Dynarec_FpuEndLabel','Dynarec_FpuModeLabel','Dynarec_BlockFinished']})
  if name=='GetFpuRegisterAddress':result['return']=self.u.reg_read(UC_X86_REG_EAX)
  return result
original,candidate=Engine(False),Engine(True);rng=random.Random(13);tests=[];failures=[]
for f in functions:
 name=f['name']
 count=192 if name.startswith('Recompile_') else 128
 for i in range(count):
  fmt=(16,17,20,21,0,31)[(i >> 2)%6];ft=(0,1,2,3,31)[i%5];fs=(0,1,30,31)[(i >> 4)%4];fd=rng.randrange(32);op=i%64
  case=dict(opcode=(17<<26)|(fmt<<21)|(ft<<16)|(fs<<11)|(fd<<6)|op,fr=(i>>1)&1,dynamic=i&1,
            checked=(i>>3)&1,delay=(i>>2)&1,profile=(i>>4)&1,mutate_delay=bool((i>>5)&1))
  args=[]
  for inp in f.get('inputs',[]):
   field=inp['name'];args.append({'fullMode':i%2,'format':fmt,'mode':i%7,'condition':(2,4,6)[i%3],'reg':i%32}[field])
  a,b=original.run(name,case,args),candidate.run(name,case,args)
  tests.append(dict(name=name,case=i,match=a==b))
  if a!=b:
   failures.append(dict(name=name,case=case,args=args,original=a,candidate=b))
   if sum(x['name']==name for x in failures)>=2:break
report=dict(source_sha256=hashlib.sha256((ROOT/'src/nemu64/DynarecFPU.cpp').read_bytes()).hexdigest(),object_sha256=hashlib.sha256(obj.data).hexdigest(),tests=len(tests),passed=sum(x['match']for x in tests),functions=len(functions),failed_function_names=sorted({x['name']for x in failures}),failures=failures,description='Run original and unlinked compiled C++ function bodies in Unicorn; callee hooks record typed emitter/helper calls and selected globals. Original register ABI and candidate cdecl ABI are decoded separately. This semantic diagnostic does not certify linked bytes.')
report.update(
 command_line=[sys.executable, *sys.argv],
 tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
 original_sha256=hashlib.sha256(pe.data).hexdigest(),
 object_path=str(cli_args.object.resolve()),
 metadata_inputs=[dict(path='analysis/'+name, sha256=hashlib.sha256((ROOT/'analysis'/name).read_bytes()).hexdigest()) for name in analysis_files],
 source_binding='The source hash identifies the current working-tree source. The separately supplied object must be tied to that source by its compilation record; this runner does not compile it.'
)
report_path.write_text(json.dumps(report,indent=2)+'\n');print('tests',len(tests),'passed',report['passed'],'failures',report['failed_function_names'])
for f in failures[:8]:
 print(f['name'],f['args'],f['case']);print('  orig',f['original']);print('  cand',f['candidate'])

raise SystemExit(1 if failures else 0)
