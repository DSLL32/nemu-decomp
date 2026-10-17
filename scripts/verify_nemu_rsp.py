#!/usr/bin/env python3
"""Execute original and rebuilt RSP vector machine code on identical guest state.

This is differential behavioral evidence, not a test of architectural correctness
or complete-image byte equality. No C++ result expression is reproduced here.
The real RSP routines, memory breakpoint checks and linked CRT helpers run in
Unicorn. Unexpected application callbacks stop the case and are reported.
"""
import argparse
from bisect import bisect_right
from collections import Counter
import hashlib
import json
from pathlib import Path
import random
import struct
from compare_nemu_functions import aliases, read_map
from pe_fingerprints import PE32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)
ROOT = Path(__file__).resolve().parents[1]
STACK, RETURN = 0x70000000, 0x71000000
TARGET_SHA = '794fdc8830c5c87d770926c67291c8140cd3660576d4f0ae1cc51742f3c7cc4a'
SIZES = dict(RSPRegisters=128, RSPVectors=512, RSPFlags=6,
    RSPAccumulator=64, SPMemory=0x4000, RSPDelayPC=4, RSPMemoryAddress=4,
    RSPCompareNotEqual=8, RSPCompareCarry=8, CPU_Instruction=4,
    CPU_ActionFlags=4, SPReg=32, MemoryBreakpointCount=4,
    MemoryBreakpoints=800, RSPBreakpointInhibit=4)
EXTERNS = dict(CPU_Instruction=0x3e5ae08, CPU_ActionFlags=0x3e69368,
    SPReg=0x3e69534, ReportCPUError=0x414eb0,
    ReportCPUErrorFormat=0x414db0, ReadPhysicalWord=0x417180,
    WritePhysicalWord=0x417c70)
FORBIDDEN = ('ReportCPUError', 'ReportCPUErrorFormat', 'ReadPhysicalWord',
             'WritePhysicalWord')
EDGES16 = (0, 1, 0x7fff, 0x8000, 0xffff, 0x7ffe, 0x8001, 0xfffe,
           0x4000, 0xc000, 0x5555, 0xaaaa, 0x20, 0xffe0, 0x100, 0xff00)
EDGES48 = (0, 1, -1, 0x7fffffff, 0x80000000, -0x80000000,
    -0x80000001, 0x7fffffffffff, -0x800000000000, 0xffffffff,
    -0x100000000, 0x1f0000, 0x200000, -0x200000, 0x3f0000, -0x3f0000)

def sha(data): return hashlib.sha256(data).hexdigest()
def pack(values, kind): return struct.pack('<' + str(len(values)) + kind, *values)
def word(value): return struct.pack('<I', value & 0xffffffff)

class Image:
    def __init__(self, path, symbols):
        self.pe, self.symbols = PE32(path), symbols
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        end = max(s['rva'] + max(s['raw_size'], s['virtual_size']) for s in self.pe.sections)
        self.uc.mem_map(self.pe.base, (end + 0xfff) & ~0xfff)
        for s in self.pe.sections:
            self.uc.mem_write(self.pe.base+s['rva'], self.pe.data[s['offset']:s['offset']+s['raw_size']])
        self.uc.mem_map(STACK, 0x10000)
        self.uc.mem_map(RETURN, 0x1000)
        self.unexpected = None
        self.write_ranges = sorted((symbols[name], symbols[name]+size, name)
                                   for name,size in SIZES.items())
        self.write_starts = [start for start,end,name in self.write_ranges]
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self.check_write)
        for name in FORBIDDEN:
            if name in symbols:
                self.uc.hook_add(UC_HOOK_CODE, self.callback, name,
                                 begin=symbols[name], end=symbols[name])
    def check_write(self, uc, access, address, size, value, user_data):
        if STACK <= address and address+size <= STACK+0x10000:
            return
        index = bisect_right(self.write_starts, address)-1
        if index >= 0:
            start,end,name = self.write_ranges[index]
            if address+size <= end:
                return
        self.unexpected = dict(write_address=hex(address), size=size,
                               value=hex(value & ((1 << (size*8))-1)))
        uc.emu_stop()
    def callback(self, uc, address, size, name):
        self.unexpected = dict(name=name, address=hex(address))
        uc.emu_stop()
    def run(self, function, state):
        for name, data in state.items():
            self.uc.mem_write(self.symbols[name], data)
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.uc.reg_write(reg, 0x5a5a5a5a)
        self.uc.mem_write(STACK, b'\xa5' * 0x10000)
        self.uc.reg_write(UC_X86_REG_ESP, STACK+0xffe0)
        self.uc.mem_write(STACK+0xffe0, word(RETURN))
        self.uc.reg_write(UC_X86_REG_EFLAGS, 2)
        self.unexpected = None
        self.uc.emu_start(self.symbols[function], RETURN, count=100000)
        if self.unexpected:
            raise RuntimeError('unexpected execution effect: ' + json.dumps(self.unexpected))
        if self.uc.reg_read(UC_X86_REG_EIP) != RETURN:
            raise RuntimeError('instruction budget exhausted')
        if self.uc.reg_read(UC_X86_REG_ESP) != STACK+0xffe4:
            raise RuntimeError('unbalanced stack')
        return {name:bytes(self.uc.mem_read(self.symbols[name], size))
                for name,size in SIZES.items()}

def initial(rng, variant):
    s = {name:bytes(size) for name,size in SIZES.items()}
    s['RSPVectors'] = pack([rng.getrandbits(16) for _ in range(256)], 'H')
    s['RSPAccumulator'] = pack([rng.getrandbits(64) for _ in range(8)], 'Q')
    s['RSPRegisters'] = pack([rng.getrandbits(32) for _ in range(32)], 'I')
    s['RSPFlags'] = pack([rng.getrandbits(16) for _ in range(3)], 'H')
    s['SPMemory'] = rng.randbytes(0x4000)
    s['SPReg'] = pack([rng.getrandbits(32) for _ in range(8)], 'I')
    s['CPU_ActionFlags'] = word(rng.getrandbits(32))
    s['RSPMemoryAddress'] = word(0xffc)
    s['RSPDelayPC'] = word(rng.getrandbits(32))
    s['RSPCompareNotEqual'] = rng.randbytes(8)
    s['RSPCompareCarry'] = rng.randbytes(8)
    if variant < 16:
        s['RSPAccumulator'] = pack([((EDGES48[(variant+i)%16] & ((1<<48)-1)) |
            (rng.getrandbits(16)<<48)) for i in range(8)], 'Q')
    return s

def vector_case(rng, name, opcode, element, variant, alias):
    s = initial(rng, variant)
    vs,vt,vd = ((3,17,29),(3,17,3),(3,17,17),(3,3,29),(3,3,3))[alias]
    vectors = list(struct.unpack('<256H', s['RSPVectors']))
    if variant < 16:
        for i in range(8):
            vectors[vs*8+i] = EDGES16[(variant+i)%16]
            vectors[vt*8+i] = EDGES16[(variant*3+7-i)%16]
    s['RSPVectors'] = pack(vectors,'H')
    s['CPU_Instruction'] = word((0x12<<26)|((16+element)<<21)|(vt<<16)|(vs<<11)|(vd<<6)|opcode)
    if variant < 8:
        s['RSPFlags'] = pack([(0,0xffff,0xff,0xff00,0x5555,0xaaaa,0x8080,0x0101)[(variant+i)%8] for i in range(3)],'H')
    return dict(function=name, opcode=opcode, element=element, variant=variant, alias=alias),s

def cases(seed, random_count, memory=True):
    rng = random.Random(seed)
    # Minimal real-execution regressions: VMADN with zero multiplicands and an
    # accumulator just beyond either 32-bit clamp boundary. Initial upper 16
    # storage bits are deliberately nonzero and must be preserved.
    for accumulator in (0xa55a000080000000, 0x5aaaffff7fffffff):
        state = {name:bytes(size) for name,size in SIZES.items()}
        state['CPU_Instruction'] = word(0x4a0208ce)
        state['RSPAccumulator'] = pack([accumulator]+[0]*7,'Q')
        yield dict(function='RSP_VectorMultiply', opcode=14,
                   kind='vmadn_clamp_boundary', accumulator=hex(accumulator)),state
    names = [('RSP_VectorMultiply',i) for i in range(16)] + [
        ('RSP_VADD',16),('RSP_VSUB',17),('RSP_VSUT',18),
        ('RSP_VABS',19),('RSP_VADDC',20),('RSP_VSUBC',21),('RSP_VCL',36)]
    for name,opcode in names:
        for element in range(16):
            for variant in range(16):
                yield vector_case(rng,name,opcode,element,variant,(variant+element)%5)
        for i in range(random_count):
            yield vector_case(rng,name,opcode,rng.randrange(16),16+i,rng.randrange(5))
    # Exhaust all five VCL input flag bits for each lane. Rotated patterns
    # ensure every lane sees all 32 combinations and both preserved flag halves.
    for flags in range(32):
        for element in range(16):
            desc,s = vector_case(rng,'RSP_VCL',36,element,flags%16,flags%5)
            f=[0,0,0]
            for i in range(8):
                bits=(flags+i)%32
                f[0] |= ((bits&1)<<i) | (((bits>>1)&1)<<(i+8))
                f[1] |= (((bits>>2)&1)<<i) | (((bits>>3)&1)<<(i+8))
                f[2] |= ((bits>>4)&1)<<i
            s['RSPFlags']=pack(f,'H')
            desc.update(flag_pattern=flags)
            yield desc,s
    if not memory: return
    addresses=(0,1,7,8,15,0xff0,0xff7,0xff8,0xfff,0x1000,0xfffffff8)
    for name,primary in [('RSP_LoadVector',0x32),('RSP_StoreVector',0x3a)]:
        for kind in (*range(10),11):
            for element in range(16):
                for n,address in enumerate(addresses):
                    s=initial(rng,16)
                    # Leave a following vector for original subword operations
                    # whose element indexes cross their nominal 16-byte register.
                    vt=(n*7+element)%30
                    offset=(-64,-1,0,1,63)[(n+element)%5]
                    s['CPU_Instruction']=word((primary<<26)|(5<<21)|(vt<<16)|(kind<<11)|(element<<7)|(offset&127))
                    regs=bytearray(s['RSPRegisters']);struct.pack_into('<I',regs,20,address)
                    s['RSPRegisters']=bytes(regs)
                    bp=(n+element+kind)%4
                    s['MemoryBreakpointCount']=word(0 if bp==0 else 2)
                    s['MemoryBreakpoints']=pack([0x04000ffc,3,0x05000ffc,3],'I')+bytes(784)
                    s['RSPBreakpointInhibit']=word(bp==3)
                    s['RSPMemoryAddress']=word(0xff8 if bp==2 else 0xffc)
                    yield dict(function=name,kind=kind,element=element,base=address,
                        offset=offset,vt=vt,breakpoint_mode=bp),s

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--image',type=Path,required=True)
    p.add_argument('--map',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True)
    p.add_argument('--seed',type=lambda v:int(v,0),default=0x525350)
    p.add_argument('--random-count',type=int,default=128)
    p.add_argument('--no-memory',action='store_true')
    p.add_argument('--limit',type=int)
    p.add_argument('--function',action='append',help='select functions; repeat to select more than one')
    p.add_argument('--max-failures',type=int,default=20)
    p.add_argument('--replay',type=Path,help='JSON fixture list with function and state hex fields; omitted state is zero')
    args=p.parse_args()
    metadata_bytes=(ROOT/'analysis/nemu-rsp.json').read_bytes()
    metadata=json.loads(metadata_bytes)
    map_bytes=args.map.read_bytes()
    original={r['name']:int(r['va'],0) if isinstance(r['va'],str) else r['va']
              for key in ('functions','globals','shared_externs') for r in metadata[key]}
    original.update(EXTERNS)
    rebuilt={alias:r['va'] for r in read_map(args.map) for alias in aliases(r['name'])}
    a,b=Image(ROOT/'master/nemu64.exe',original),Image(args.image,rebuilt)
    if args.map.read_bytes()!=map_bytes:
        raise RuntimeError('candidate MAP changed while reading the inputs')
    if sha(a.pe.data)!=TARGET_SHA: raise RuntimeError('unexpected original image hash')
    for name,size in [('RSPElementLaneMask',32),('RSPElementIndexMask',32)]:
        if a.uc.mem_read(original[name],size)!=b.uc.mem_read(rebuilt[name],size):
            raise RuntimeError('different selector table: '+name)
    counts,failcounts,subfailures,failures=Counter(),Counter(),Counter(),[]
    digests=[hashlib.sha256(),hashlib.sha256()]
    total=0
    tests = cases(args.seed,args.random_count,not args.no_memory)
    if args.replay:
        fixtures = json.loads(args.replay.read_text())
        if isinstance(fixtures,dict):
            fixtures = fixtures['failures']
        tests = []
        for fixture in fixtures:
            state = {name:bytes(size) for name,size in SIZES.items()}
            for name,data in fixture.get('state',fixture.get('initial_state',{})).items():
                if name not in SIZES or len(bytes.fromhex(data)) != SIZES[name]:
                    raise ValueError('invalid fixture state field: '+name)
                state[name] = bytes.fromhex(data)
            name=fixture['function'] if 'function' in fixture else fixture['case']['function']
            tests.append((dict(function=name),state))
    for index,(desc,state) in enumerate(tests):
        if args.function and desc['function'] not in args.function:
            continue
        if args.limit is not None and total>=args.limit:
            break
        total+=1;name=desc['function'];counts[name]+=1
        results, errors = [], {}
        for label, image in [('original',a),('rebuilt',b)]:
            try:
                results.append(image.run(name,state))
            except Exception as error:
                results.append(None)
                errors[label] = dict(message=str(error),
                    eip=hex(image.uc.reg_read(UC_X86_REG_EIP)))
        differences = {}
        if not errors:
            one,two = results
            for result,digest in zip(results,digests):
                for field in sorted(result):
                    digest.update(result[field])
            differences = {
                field:[dict(offset=i,original=x,rebuilt=y)
                       for i,(x,y) in enumerate(zip(one[field],two[field])) if x!=y]
                for field in one if one[field]!=two[field]}
        if errors or differences:
            failcounts[name] += 1
            subfailures[str((name,desc.get('opcode',desc.get('kind'))))] += 1
            if len(failures) < args.max_failures:
                failures.append(dict(index=index,case=desc,differences=differences,
                    execution_errors=errors,
                    initial_state={field:data.hex() for field,data in state.items()
                                   if any(data)}))
    if not total:
        raise RuntimeError('no test cases selected')
    report=dict(status='passed' if not failcounts else 'different',
        evidence_type='actual_PE_behavioral_differential_only_not_byte_equality',
        original_sha256=sha(a.pe.data),rebuilt_sha256=sha(b.pe.data),
        rebuilt_map_sha256=sha(map_bytes),script_sha256=sha(Path(__file__).read_bytes()),
        metadata_sha256=sha(metadata_bytes),
        seed=args.seed,random_cases_per_opcode=args.random_count,
        selected_functions=args.function,tests=total,passed=total-sum(failcounts.values()),
        cases_by_function=dict(counts),failures_by_function=dict(failcounts),
        failures_by_opcode=dict(subfailures),
        original_results_sha256=digests[0].hexdigest(),rebuilt_results_sha256=digests[1].hexdigest(),
        write_coverage='Every emulated write is checked against the complete compared state objects or the emulated stack.',
        external_callbacks='No callbacks are simulated. Reaching error reporting or physical-memory dependency routines fails the case.',
        limitations=['Differential equality can preserve original bugs; this is not an RSP architectural conformance test.',
          'Only the selected vector handlers and reachable helpers are exercised, not the complete RSP dispatcher or host UI.',
          'Invalid vector memory opcode 10 and vector-register-array overrun are excluded.',
          'The standalone candidate may contain dependency stubs, but reaching the listed dependency functions is a test failure.'],
        failures=failures)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','tests','passed','cases_by_function','failures_by_function')},indent=2))
    for f in failures[:3]: print(json.dumps({k:v for k,v in f.items() if k!='initial_state'},indent=2))
    return bool(failcounts)
if __name__=='__main__':raise SystemExit(main())
