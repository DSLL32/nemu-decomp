#!/usr/bin/env python3
"""Execute original/rebuilt PE function bodies under controlled Unicorn inputs.

Only data inputs/IAT binding are written. Machine code is never modified.
CRT file/format and SetStatusText/lstrcatA boundaries are explicitly modeled.
This is finite behavioral evidence, not executable-byte or source-identity proof.
The history-slot check requires the observed 0x218 local frame and EAX input ABI;
incompatible candidate frames fail rather than silently inspecting another slot.
"""
from pathlib import Path
import argparse
import json, struct, sys, hashlib, time
from collections import Counter
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_BLOCK, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_INVALID, UC_MEM_READ, UC_MEM_WRITE
from unicorn.x86_const import *
from pe_fingerprints import PE32
from compare_nemu_functions import read_map, aliases, map_function_spans

ROOT=Path(__file__).resolve().parents[1]
TARGET_SHA='794fdc8830c5c87d770926c67291c8140cd3660576d4f0ae1cc51742f3c7cc4a'
RAM=0x10000000; RAMSIZE=0x200000; TEXT=0x11000000
STACK=0x18000000; STACKSIZE=0x100000; ENTRY_SP=STACK+STACKSIZE-0x1000
STOP=0x20000000; STRCAT=STOP+0x1000; FILE_TOKEN=0x22000010
MASK=0xffffffff
ORIGINAL_SYMBOLS={
 'SaveFunctionSignatures':0x41ffa0,'HashSignatureInstruction':0x41ff00,
 'ReplaceExtension':0x41e620,'SetStatusText':0x41a6c0,
 'fopen':0x427c2c,'fprintf':0x427c3f,'fflush':0x42885c,
 'sprintf':0x427c71,'fclose':0x427e09,
 'RuntimeSignatures':0x460d18,'RuntimeRelocations':0x461aa0,
 'RuntimeReplacementCount':0x408a7c0,'RuntimeReplacements':0x408a7c8,
 'RDRAM':0x3e69554,'ApplicationDirectory':0x4089cc0,'CurrentRom':0x408a008}

def digest(data):return hashlib.sha256(data).hexdigest()
def p32(v):return struct.pack('<I',v&MASK)
def u32(data):return struct.unpack('<I',data)[0]
def cstr_file(pe,rva):
    off=pe.offset(rva);end=pe.data.index(b'\0',off);return pe.data[off:end].decode('ascii')
def imported_slot(pe,wanted):
    rva,_=pe.directory(1);offset=pe.offset(rva)
    while True:
        lookup,stamp,forwarder,dll,table=struct.unpack_from('<5I',pe.data,offset)
        if not any((lookup,stamp,forwarder,dll,table)):break
        at=lookup or table;i=0
        while True:
            value=u32(pe.data[pe.offset(at+i*4):pe.offset(at+i*4)+4])
            if not value:break
            if not value&0x80000000 and cstr_file(pe,value+2)==wanted:
                return pe.base+table+i*4
            i+=1
        offset+=20
    raise ValueError(wanted)

def signature_hash(words,relocations=()):
    # Independent scalar specification used only to prepare synthetic table
    # hashes. The emulator executes the real HashSignatureInstruction body.
    h=0
    for i,word in enumerate(words):
        ordinary=True
        for offset in relocations:
            if offset!=i*4:continue
            ordinary=False;op=word>>26
            if op in (2,3):h^=op
            elif op==15 and 0xa400<=word&0xffff<=0xafff:h+=word
            elif op in (15,13,35):h^=word>>16
            else:h+=word>>16
            h&=MASK
        if ordinary:h=(h+word if word else h^0xdeadbeef)&MASK
    return h

def entry(name,words,offset,**extra):
    d={'name':name,'words':words,'length':len(words)*4,'relocations':[],
       'first_word':words[0]>>16,'match_offset':offset}
    d.update(extra)
    d.setdefault('hash',signature_hash(words[:min(d['length'],256)//4],d['relocations']))
    return d

def cases():
    a=[0x24210001,0,0x03e00008,0x34220002]
    b=[0x3c081234,0x35085678,0x01000008,0]
    c=[0x27bdffe0,0xafbf001c,0x03e00008,0x27bd0020]
    result=[]
    def add(name,entries,seeds,**extra):
        x={'name':name,'entries':entries,'seeds':seeds,'start_offset':0,
           'directory':'C:\\Nemu','rom':'Demo.z64','fopen_failure':False,'initial_count':77}
        x.update(extra);result.append(x)
    add('three_matches_rescan_from_base',[entry('alpha',a,0x40),entry('beta',b,0x200),entry('gamma',c,0x104)],[(0x40,a),(0x200,b),(0x104,c)])
    add('match_miss_match',[entry('alpha',a,0x40),entry('absent',b,None),entry('gamma',c,0x104)],[(0x40,a),(0x104,c)],directory='C:\\Nemu\\')
    add('all_zero_ram',[entry('alpha',a,None),entry('beta',b,None),entry('gamma',c,None)],[])
    add('strict_end_boundary',[entry('at_end',[0x24210001],None),entry('before_end',[0x34220002],0xffff8)],[(0xffffc,[0x24210001]),(0xffff8,[0x34220002])])
    add('firstword_and_hash_mismatch_then_match',[entry('alpha',a,0x100)],[(0x20,[0x11112222]),(0x40,[0x24210001,0x12345678,0x03e00008,0x34220002]),(0x100,a)])
    add('overlapping_hash_retry',[entry('alpha',a,0x44)],[(0x40,[a[0]]+a)])
    add('input_clamped_to_rdram',[entry('alpha',a,0x40)],[(0x40,a)],start_offset=-0x100)
    add('nonzero_scan_base',[entry('beta',b,0x180)],[(0x40,a),(0x180,b)],start_offset=0x100)
    longwords=[0x27bdffe0]+[0x24010000+i for i in range(1,75)]
    add('length_capped_at_256',[entry('long',longwords,0x80)],[(0x80,longwords)])
    reloc=[0x08001234,0x0c005678,0x3c08a400,0x3c098000,0x350a1234,0x8d4b0020,0x256c0001,0,0x3c0cb000,0x03e00008]
    add('relocation_hash_paths',[entry('relocated',reloc,0x80,relocations=[0,4,8,12,16,20,24,32])],[(0x80,reloc)])
    add('zero_hash_firstword_mismatch',[entry('zero_hash',[0x99990000],0x40,hash=0)],[(0x40,[0x12345678])])
    add('fopen_failure',[entry('alpha',a,0x40),entry('beta',b,0x200)],[(0x40,a),(0x200,b)],fopen_failure=True)
    add('empty_signature_table',[],[])
    add('ignored_output_error_returns',[entry('alpha',a,0x40),entry('beta',b,0x200)],[(0x40,a),(0x200,b)],file_error_returns=True)
    return result

class Execution:
    def __init__(self,label,case,artifact,inject=None):
        self.label=label;self.case=case;self.inject=inject
        self.path=ROOT/'master/nemu64.exe' if label=='original' else artifact/'nemu64.exe'
        self.pe=PE32(self.path)
        self.artifact=artifact
        self.symbols=ORIGINAL_SYMBOLS.copy()
        lengths={'SaveFunctionSignatures':672,'HashSignatureInstruction':160,'ReplaceExtension':48}
        if label!='original':
            rows=read_map(artifact/'nemu64.map')
            for name in ORIGINAL_SYMBOLS:
                addresses={row['va'] for row in rows if name in aliases(row['name'])}
                if len(addresses)!=1:raise RuntimeError('ambiguous/missing MAP identity: '+name)
                self.symbols[name]=addresses.pop()
            spans=map_function_spans(self.pe,rows)
            lengths={name:spans[self.symbols[name]] for name in lengths}
        self.code_ranges={name:(self.symbols[name],self.symbols[name]+size) for name,size in lengths.items()}
        self.original_code={name:self.pe.data[self.pe.offset(start-self.pe.base):self.pe.offset(start-self.pe.base)+end-start] for name,(start,end) in self.code_ranges.items()}
        if not self.original_code['SaveFunctionSignatures'].startswith(bytes.fromhex('81ec1802000053558bd8')):
            raise RuntimeError('unsupported Save frame/entry ABI; review history-slot coordinates')
        self.uc=Uc(UC_ARCH_X86,UC_MODE_32)
        image_size=u32(self.pe.data[self.pe.opt+56:self.pe.opt+60])
        self.uc.mem_map(self.pe.base,(image_size+0xfff)&~0xfff)
        headers=u32(self.pe.data[self.pe.opt+60:self.pe.opt+64])
        self.uc.mem_write(self.pe.base,self.pe.data[:headers])
        for section in self.pe.sections:
            raw=self.pe.data[section['offset']:section['offset']+section['raw_size']]
            if raw:self.uc.mem_write(self.pe.base+section['rva'],raw)
        for address,size in ((RAM,RAMSIZE),(TEXT,0x10000),(STACK,STACKSIZE),(STOP,0x2000)):
            self.uc.mem_map(address,size)
        self.uc.mem_write(STACK,b'\xcc'*STACKSIZE)
        self.records=[];self.status=[];self.boundaries=[];self.history=[]
        self.hash_calls=0;self.extension_calls=0;self.done=False;self.invalid=[]
        self.file_closed=False
        self.block_counts=Counter();self.blocks=set();self.write_counts=Counter()
        self.guard_violations=[]
        self.setup_inputs()
        self.install_hooks()

    def word(self,address):return u32(bytes(self.uc.mem_read(address,4)))
    def string(self,address):
        data=bytearray()
        while len(data)<4096:
            b=bytes(self.uc.mem_read(address+len(data),1))
            if b==b'\0':return data.decode('ascii')
            data+=b
        raise AssertionError('unterminated controlled string')
    def put_string(self,address,value):self.uc.mem_write(address,value.encode('ascii')+b'\0')
    def setup_inputs(self):
        c=self.case;s=self.symbols
        self.uc.mem_write(s['RDRAM'],p32(RAM))
        self.put_string(s['ApplicationDirectory'],c['directory'])
        self.put_string(s['CurrentRom'],c['rom']) # FileName is the first member.
        self.uc.mem_write(s['RuntimeReplacementCount'],p32(c['initial_count']))
        self.initial_replacements=b'\xa5'*4096
        self.uc.mem_write(s['RuntimeReplacements'],self.initial_replacements)
        self.uc.mem_write(s['RuntimeSignatures'],b'\0'*(173*20))
        self.uc.mem_write(s['RuntimeRelocations'],b'\0'*(173*516))
        for i,e in enumerate(c['entries']):
            name=TEXT+i*256;self.put_string(name,e['name'])
            token=0x60100000+i*0x100 # Copied opaque replacement value; never called.
            e['replacement_token']=token
            self.uc.mem_write(s['RuntimeSignatures']+i*20,struct.pack('<5I',name,e['length'],e['hash'],e['first_word'],token))
            reloc=e['relocations']
            self.uc.mem_write(s['RuntimeRelocations']+i*516,p32(len(reloc))+b''.join(struct.pack('<H',x) for x in reloc))
        for offset,words in c['seeds']:
            self.uc.mem_write(RAM+offset,b''.join(p32(w) for w in words))
        self.ram_before=bytes(self.uc.mem_read(RAM,RAMSIZE))
        self.uc.mem_write(ENTRY_SP,p32(STOP))
        self.initial_saved={UC_X86_REG_EBX:0xb0b0b0b0,UC_X86_REG_EBP:0xb1b1b1b1,UC_X86_REG_ESI:0xb2b2b2b2,UC_X86_REG_EDI:0xb3b3b3b3}
        for reg,val in self.initial_saved.items():self.uc.reg_write(reg,val)
        self.uc.reg_write(UC_X86_REG_ESP,ENTRY_SP)
        self.uc.reg_write(UC_X86_REG_EAX,RAM+c['start_offset'])
        self.uc.reg_write(UC_X86_REG_ECX,0xc1c1c1c1);self.uc.reg_write(UC_X86_REG_EDX,0xd1d1d1d1)
        self.uc.reg_write(UC_X86_REG_EFLAGS,2)
        self.frame=ENTRY_SP-0x228;self.history_slot=self.frame+0x18
        self.iat_slot=imported_slot(self.pe,'lstrcatA')
        self.uc.mem_write(self.iat_slot,p32(STRCAT))

    def return_from_model(self,result=0,pop_args=0):
        sp=self.uc.reg_read(UC_X86_REG_ESP);target=self.word(sp)
        # Challenge volatile-register liveness at every modeled boundary.
        self.uc.reg_write(UC_X86_REG_EAX,result&MASK)
        self.uc.reg_write(UC_X86_REG_ECX,0xcacacaca)
        self.uc.reg_write(UC_X86_REG_EDX,0xdadadada)
        self.uc.reg_write(UC_X86_REG_ESP,sp+4+pop_args)
        self.uc.reg_write(UC_X86_REG_EIP,target)

    def boundary(self,name):
        sp=self.uc.reg_read(UC_X86_REG_ESP)
        args=lambda i:self.word(sp+4+i*4)
        if name=='fopen':
            filename,mode=self.string(args(0)),self.string(args(1))
            assert mode=='wb'
            self.boundaries.append({'function':name,'filename':filename,'mode':mode})
            self.return_from_model(0 if self.case['fopen_failure'] else FILE_TOKEN)
        elif name in ('fprintf','sprintf'):
            dest=args(0);fmt=self.string(args(1));address=args(2);text=self.string(args(3))
            assert fmt in ('0x%X,%s\r\n','Searching... 0x%X, %s'),fmt
            rendered=fmt.replace('%X',format(address,'X')).replace('%s',text)
            if name=='fprintf':
                assert dest==FILE_TOKEN
                self.records.append(rendered)
            else:
                assert self.frame+0x128<=dest and dest+len(rendered)<self.frame+0x228
                self.put_string(dest,rendered)
            self.boundaries.append({'function':name,'format':fmt,'address':address,'name':text,'text':rendered})
            self.return_from_model(-1 if self.case.get('file_error_returns') and name=='fprintf' else len(rendered))
        elif name in ('fflush','fclose'):
            assert args(0)==FILE_TOKEN
            self.boundaries.append({'function':name})
            if name=='fclose':assert not self.file_closed;self.file_closed=True
            self.return_from_model(-1 if self.case.get('file_error_returns') else 0)
        elif name=='SetStatusText':
            part=self.uc.reg_read(UC_X86_REG_ECX);text=self.string(self.uc.reg_read(UC_X86_REG_EAX))
            self.status.append({'part':part,'text':text})
            self.boundaries.append({'function':name,'part':part,'text':text})
            self.return_from_model()
        elif name=='lstrcatA':
            dest,source=args(0),args(1)
            suffix=self.string(source)
            assert dest==self.frame+0x24 and suffix=='.cod'
            assert len(self.string(dest))+len(suffix)<260
            self.put_string(dest,self.string(dest)+suffix)
            self.boundaries.append({'function':name,'suffix':suffix})
            self.return_from_model(dest,8)
        else:raise AssertionError(name)

    def install_hooks(self):
        model_entries={self.symbols[name] for name in ('fopen','fprintf','fflush','sprintf','fclose','SetStatusText')}
        model_entries.update((STOP,STRCAT))
        def code_guard(uc,address,size,user):
            if address in model_entries:return
            for name,(start,end) in self.code_ranges.items():
                if start<=address and address+size<=end:
                    self.block_counts[name]+=1;self.blocks.add((name,address-start))
                    return
            self.guard_violations.append({'kind':'unexpected_code','address':address,'size':size})
            raise RuntimeError('execution left the three reviewed application bodies')
        self.uc.hook_add(UC_HOOK_BLOCK,code_guard)
        write_ranges=[(self.frame-0x1000,ENTRY_SP+4,'stack'),
                      (self.symbols['RuntimeReplacements'],self.symbols['RuntimeReplacements']+4096,'RuntimeReplacements'),
                      (self.symbols['RuntimeReplacementCount'],self.symbols['RuntimeReplacementCount']+4,'RuntimeReplacementCount')]
        def write_guard(uc,access,address,size,value,user):
            for start,end,name in write_ranges:
                if start<=address and address+size<=end:
                    self.write_counts[name]+=1
                    return
            self.guard_violations.append({'kind':'unexpected_write','address':address,'size':size,'eip':uc.reg_read(UC_X86_REG_EIP)})
            raise RuntimeError('write outside private stack and declared replacement state')
        self.uc.hook_add(UC_HOOK_MEM_WRITE,write_guard)
        def stop(uc,address,size,user):self.done=True;uc.emu_stop()
        self.uc.hook_add(UC_HOOK_CODE,stop,begin=STOP,end=STOP)
        for name in ('fopen','fprintf','fflush','sprintf','fclose','SetStatusText','lstrcatA'):
            address=STRCAT if name=='lstrcatA' else self.symbols[name]
            def boundary(uc,address,size,user,name=name):self.boundary(name)
            self.uc.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
        def hash_call(uc,address,size,user):self.hash_calls+=1
        self.uc.hook_add(UC_HOOK_CODE,hash_call,begin=self.symbols['HashSignatureInstruction'],end=self.symbols['HashSignatureInstruction'])
        def extension(uc,address,size,user):self.extension_calls+=1
        self.uc.hook_add(UC_HOOK_CODE,extension,begin=self.symbols['ReplaceExtension'],end=self.symbols['ReplaceExtension'])
        def history(uc,access,address,size,value,user):
            assert address==self.history_slot and size==4,(hex(address),size)
            if access==UC_MEM_READ:
                if self.inject is not None:uc.mem_write(address,p32(self.inject))
                value=self.word(address)
            self.history.append({'kind':'read' if access==UC_MEM_READ else 'write','save_offset':uc.reg_read(UC_X86_REG_EIP)-self.symbols['SaveFunctionSignatures'],'esp_from_frame':uc.reg_read(UC_X86_REG_ESP)-self.frame,'value':value&MASK})
        self.uc.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,history,begin=self.history_slot,end=self.history_slot+3)
        def invalid(uc,access,address,size,value,user):
            self.invalid.append({'access':access,'address':address,'size':size});return False
        self.uc.hook_add(UC_HOOK_MEM_INVALID,invalid)

    def run(self):
        started=time.monotonic()
        self.uc.emu_start(self.symbols['SaveFunctionSignatures'],STOP+1,count=30000000)
        assert self.done,'instruction limit reached'
        assert not self.invalid,self.invalid
        assert not self.guard_violations,self.guard_violations
        assert self.uc.reg_read(UC_X86_REG_ESP)==ENTRY_SP+4
        assert all(self.uc.reg_read(reg)==value for reg,value in self.initial_saved.items())
        assert bytes(self.uc.mem_read(RAM,RAMSIZE))==self.ram_before
        assert self.extension_calls==1
        for name,(start,end) in self.code_ranges.items():
            assert bytes(self.uc.mem_read(start,end-start))==self.original_code[name],name
        table=bytes(self.uc.mem_read(self.symbols['RuntimeReplacements'],4096))
        result={'records':self.records,'statuses':self.status,'count':self.word(self.symbols['RuntimeReplacementCount']),
                'replacements_sha256':digest(table),'replacements_prefix':[list(struct.unpack_from('<II',table,i*8)) for i in range(len(self.case['entries'])+1)],
                'boundary_calls':self.boundaries,'file_closed':self.file_closed}
        expected_table=bytearray(self.initial_replacements)
        expected_records=[];expected_status=[]
        if not self.case['fopen_failure']:
            for i,e in enumerate(self.case['entries']):
                offset=e['match_offset']
                if offset is None:struct.pack_into('<II',expected_table,i*8,0,0)
                else:
                    address=0x80000000+offset
                    struct.pack_into('<II',expected_table,i*8,address,e['replacement_token'])
                    expected_records.append('0x%X,%s\r\n'%(address,e['name']))
                    expected_status.append({'part':0,'text':'Searching... 0x%X, %s'%(address,e['name'])})
            expected_status.append({'part':0,'text':'Nemulating...'})
        expected_count=self.case['initial_count'] if self.case['fopen_failure'] or not self.case['entries'] else len(self.case['entries'])
        assert self.records==expected_records,(self.case['name'],self.label,'records',self.records,expected_records)
        assert self.status==expected_status,(self.case['name'],self.label,'status')
        assert result['count']==expected_count,(self.case['name'],self.label,'count',result['count'])
        assert table==expected_table,(self.case['name'],self.label,'replacement bytes')
        expected_filename=self.case['directory'].rstrip('\\')+'\\cod\\Demo.cod'
        assert next(x for x in self.boundaries if x['function']=='fopen')['filename']==expected_filename
        assert self.file_closed==(not self.case['fopen_failure'])
        return {'observables':result,'history_slot_trace':self.history,'hash_machine_code_calls':self.hash_calls,'extension_machine_code_calls':self.extension_calls,
                'callee_saved_registers_preserved':True,'stack_restored':True,'rdram_unchanged':True,'invalid_memory_accesses':self.invalid,
                'code_unchanged':True,'guard_violations':self.guard_violations,'writes_by_permitted_region':dict(self.write_counts),
                'executed_basic_blocks':{name:sorted(offset for block_name,offset in self.blocks if block_name==name) for name in self.code_ranges},
                'basic_block_visits':dict(self.block_counts),
                'actual_body_spans':{name:{'va':start,'size':end-start,'sha256':digest(self.original_code[name])} for name,(start,end) in self.code_ranges.items()},
                'seconds':time.monotonic()-started}

def main():
    if not __debug__:raise RuntimeError('validation requires Python assertions; do not use -O')
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifact',type=Path,required=True,help='Complete fixture with nemu64.exe, nemu64.map and diagnostic.json')
    parser.add_argument('--source',type=Path,help='Associated RuntimeSignatures.cpp snapshot; defaults to artifact/src/nemu64')
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    artifact=args.artifact.resolve()
    source=args.source or artifact/'src/nemu64/RuntimeSignatures.cpp'
    source=source.resolve()
    proof_paths=[Path(__file__).resolve(),ROOT/'scripts/compare_nemu_functions.py',ROOT/'scripts/pe_fingerprints.py',
                 ROOT/'master/nemu64.exe',artifact/'nemu64.exe',artifact/'nemu64.map',artifact/'diagnostic.json',source]
    input_hashes={str(path):digest(path.read_bytes()) for path in proof_paths}
    manifest=json.loads((artifact/'diagnostic.json').read_text())
    if digest((ROOT/'master/nemu64.exe').read_bytes())!=TARGET_SHA:
        raise RuntimeError('original image does not match the recorded target hash')
    if manifest.get('never_forced') is not True or manifest.get('link_returncode')!=0:
        raise RuntimeError('requires a complete unforced linked fixture')
    if manifest.get('sha256')!=digest((artifact/'nemu64.exe').read_bytes()):
        raise RuntimeError('candidate image hash does not match fixture manifest')
    source_text=source.read_text()
    source_begin=source_text.index('void SaveFunctionSignatures(')
    source_end=source_text.index('void ResetRuntimeReplacements()',source_begin)
    allcases=cases();runs=[]
    for case in allcases:
        pair={}
        for label in ('original','rebuilt'):
            pair[label]=Execution(label,case,artifact).run()
        assert pair['original']['observables']==pair['rebuilt']['observables'],case['name']
        assert pair['original']['hash_machine_code_calls']==pair['rebuilt']['hash_machine_code_calls']
        assert [(x['kind'],x['value']) for x in pair['original']['history_slot_trace']]==[(x['kind'],x['value']) for x in pair['rebuilt']['history_slot_trace']],case['name']
        for access in pair['original']['history_slot_trace']:
            if access['kind']=='read':
                assert access['save_offset']==0x1c9 and access['esp_from_frame']==-20
            else:
                assert access['save_offset'] in (0x13,0x1d2,0x1d8)
        runs.append({'case':case,'matched':True,**pair})
        print(case['name'],'matched; hashes',pair['original']['hash_machine_code_calls'],'history writes',sum(x['kind']=='write' for x in pair['original']['history_slot_trace']),flush=True)
    perturbed=[]
    for label in ('original','rebuilt'):
        for injected in (0,0xffffffff):
            result=Execution(label,allcases[0],artifact,injected).run()
            assert result['observables']==runs[0][label]['observables']
            perturbed.append({'image':label,'forced_history_read':injected,'observables_unchanged':True,'execution':result})
    # Exercise the guards themselves without adding or modifying machine code.
    guard_checks=[]
    for failure in ('unexpected_code','unexpected_write'):
        execution=Execution('original',allcases[0],artifact)
        start=execution.symbols['SaveFunctionSignatures']
        if failure=='unexpected_code':start+=672 # Next real PE function, outside permitted bodies.
        else:execution.uc.reg_write(UC_X86_REG_ESP,ENTRY_SP-0x3000)
        try:
            execution.uc.emu_start(start,STOP+1,count=20)
        except RuntimeError:
            assert execution.guard_violations and execution.guard_violations[0]['kind']==failure
            guard_checks.append({'guard':failure,'rejected':True,'observation':execution.guard_violations[0]})
        else:raise AssertionError('guard self-check failed: '+failure)
    for path,expected_hash in input_hashes.items():
        if digest(Path(path).read_bytes())!=expected_hash:
            raise RuntimeError('proof input changed during execution: '+path)
    report={'kind':'controlled_original_and_rebuilt_full_pe_machine_code_signature_writer_validation',
      'status':'passed','paired_case_count':len(runs),'history_perturbation_count':len(perturbed),
      'original':{'path':str(ROOT/'master/nemu64.exe'),'sha256':digest((ROOT/'master/nemu64.exe').read_bytes())},
      'rebuilt':{'path':str(artifact/'nemu64.exe'),'sha256':digest((artifact/'nemu64.exe').read_bytes()),'map_sha256':digest((artifact/'nemu64.map').read_bytes()),'diagnostic_sha256':digest((artifact/'diagnostic.json').read_bytes())},
      'script_sha256':digest(Path(__file__).read_bytes()),'unicorn_version':unicorn.__version__,
      'proof_input_hashes':input_hashes,
      'source_snapshot':{'path':str(source),'sha256':digest(source.read_bytes()),'save_body_sha256':digest(source_text[source_begin:source_end].encode()),'scope':'Associated compiler-input snapshot; hash association is not source-operand provenance.'},
      'reproduce_argv':[sys.executable,str(Path(__file__).resolve()),'--artifact',str(artifact),'--source',str(source),'--report',str(args.report)],
      'code_mutations':False,'host_file_writes_from_modeled_functions':False,
      'executed_actual_bodies':['SaveFunctionSignatures','HashSignatureInstruction','ReplaceExtension'],
      'modeled_boundaries':['fopen','fprintf','fflush','sprintf','fclose','SetStatusText','KERNEL32.lstrcatA'],
      'entry_abi':'candidate pointer in EAX; actual x86 call/return stack and callee-saved registers',
      'input_policy':'PE loaded at preferred base; only synthetic data/stack/RDRAM/table/name inputs and resolved lstrcatA IAT binding are changed; code bytes retained.',
      'guards':['Original hash and complete unforced candidate-manifest hash checked.','Unique MAP symbols and original-compatible frame/entry ABI required.','Every executed basic block must lie in the three reviewed bodies or begin at a modeled boundary.','Every emulated write must lie in the private stack, replacement table or count.','Full4096-byte replacement state checked against explicit expected values; RDRAM and code bytes unchanged.','Return stack and all callee-saved registers checked; instruction limit and unmapped-memory guards active.'],
      'boundary_model_scope':'records are formatted fprintf attempts captured by the model, including when a negative return is injected; no real FILE buffering, filesystem persistence or GUI behavior is claimed. Volatile registers are deliberately clobbered at each modeled call.',
      'original_history_slot':{'relative_to_entry_esp':-0x210,'relative_to_frame_esp':0x18,'initialization_offset':0x13,'read_offset':0x1c9,'read_esp_delta':-20,'read_displacement':0x2c,'write_offsets':[0x1d2,0x1d8],
          'dataflow':'Read value is tested for zero; it selects whether the same private slot retains start or is overwritten with the advanced hash cursor. No use reaches records, status, count, replacement table, or scan cursor.'},
      'cases':runs,'history_perturbations':perturbed,'guard_self_checks':guard_checks,
      'limitations':['Finite controlled inputs are not a universal equivalence proof.','CRT file/format functions, SetStatusText and lstrcatA are boundary models; real OS, CRT FILE internals, GUI and filesystem behavior are not exercised.','Only the stated normal bounded strings and synthetic tables are tested; undefined buffer-overflow inputs are excluded.','Preferred-base execution does not establish rebasing or source-operand provenance.','Behavioral agreement does not certify exact compiled bytes or recover original source declarations.']}
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS',len(runs),'paired cases and',len(perturbed),'history-perturbation executions; report',args.report,flush=True)

if __name__=='__main__':main()
