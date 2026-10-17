"""Lightweight declarations used by Nemu source storage inventory and staging.

This deliberately recognizes the reconstructed source subset, not arbitrary C++.
"""
import re

def clean(s):
    s=re.sub(r'/\*.*?\*/',lambda m:'\n'*m[0].count('\n'),s,flags=re.S)
    s=re.sub(r'//[^\n]*','',s)
    return re.sub(r'^\s*#.*','',s,flags=re.M)

def statements(path):
    s=clean(path.read_text()); start=0; depth=0; skip=False
    for i,c in enumerate(s):
        if c=='{':
            if depth==0:
                prefix=s[start:i]
                skip='=' not in prefix
            depth+=1
        elif c=='}':
            depth-=1
            if depth==0 and skip:
                start=i+1;skip=False
        elif c==';' and depth==0:
            raw=s[start:i].strip(); line=s.count('\n',0,start)+1
            while start<len(s) and s[start].isspace():
                if s[start]=='\n':line+=1
                start+=1
            if raw:yield raw,line
            start=i+1

def parts(s):
    start=0;depth=0
    for i,c in enumerate(s):
        if c in '([':depth+=1
        elif c in ')]':depth-=1
        elif c==',' and depth==0:yield s[start:i];start=i+1
    yield s[start:]

def decls(path):
    for raw,line in statements(path):
        raw=' '.join(raw.split())
        if raw.startswith(('typedef ','struct ','union ','enum ','return ')):continue
        ext=raw.startswith('extern ')
        static=raw.startswith('static ')
        text=re.sub(r'^(extern|static)\s+','',raw)
        left,sep,initializer=text.partition('=')
        fp=re.search(r'\(\s*(?:__cdecl\s+|__stdcall\s+)?\*\s*(\w+)\s*\)',left)
        if '(' in left:
            if not fp:continue
            name=fp[1];typ=left[:fp.start()]+left[fp.start():].replace(name,'',1)
            yield name,typ.strip(),initializer.strip() if sep else None,ext,static,line,raw
            continue
        base=None
        for bit in parts(left):
            bit=bit.strip();m=re.fullmatch(r'(.*?)\b([A-Za-z_]\w*)\s*((?:\[[^\]]*\]\s*)*)',bit)
            if not m:continue
            lead,name,arr=m.groups()
            if base is None:
                base=lead.rstrip(' *');pointer=lead[len(base):]
                if not base:break
            else:pointer=lead
            typ=(base+' '+pointer+arr).strip()
            yield name,typ,initializer.strip() if sep else None,ext,static,line,raw

aliases={'CpuU8':'unsigned char','CpuS8':'signed char','CpuU16':'unsigned short','CpuS16':'short','CpuU32':'unsigned','CpuS32':'int','CpuU64':'unsigned __int64','CpuS64':'__int64','WORD':'unsigned short','DWORD':'unsigned long','BOOL':'int'}
sizes={'char':1,'signed char':1,'unsigned char':1,'bool':1,'short':2,'unsigned short':2,'int':4,'unsigned':4,'long':4,'unsigned long':4,'__int64':8,'unsigned __int64':8,'float':4,'double':8,'CPUFloatingRegisters':128,'TLBEntry':68,'AIRegisters':36,'MIRegisters':12,'SPRegisters':32,'VIRegisters':56,'PIRegisters':52,'RIRegisters':32,'DPCRegisters':32,'RDRAMRegisters':40,'RSPMemoryBreakpoint':8,'DebugWatch':8,'DebugBreakpoint':12,'DebugOpcodeBreakpoint':8,'PluginDescriptor':476,'ToolDescriptor':1020,'PluginInfo':112,'RecompiledInstruction':64,'DynarecJumpFixup':8,'DynarecAddressMap':8,'DynarecSavedInstruction':12,'DynarecOpcodeBreakpoint':8,'RuntimeReplacement':8,'FunctionSignature':20,'FunctionRelocations':516,'RomInformation':704,'PIFCommandLength':8,'ControllerState':8}
sizes['POINT']=8
handles={'HWND','HINSTANCE','HIMAGELIST','HMENU','HANDLE','HMODULE','HDC','HBITMAP','HFONT','HBRUSH','HICON','CPUOpcodeHandler','RSPOpcodeHandler','DynarecOpcodeHandler','LegacyCallback'}
def canon(t):
    t=' '.join(t.split()).replace('unsigned int','unsigned').replace('signed int','int')
    t=re.sub(r'\bsigned short\b','short',t);t=re.sub(r'\bsigned __int64\b','__int64',t)
    for a,b in aliases.items():t=re.sub(r'\b'+a+r'\b',b,t)
    return re.sub(r'\s*([*\[\]])\s*',r'\1',t).replace('[MAX_PATH]','[260]')
def extent(t):
    t=canon(t);dims=re.findall(r'\[([^\]]*)\]',t);base=re.sub(r'\[.*','',t).strip();base=re.sub(r'^const\s+','',base)
    n=4 if '*' in base or base in handles else sizes.get(base)
    if n is None:return None
    for dim in dims:
        if dim=='MAX_PATH':d=260
        elif re.fullmatch(r'(0x[0-9a-fA-F]+|[0-9]+)',dim):d=int(dim,0)
        else:return None
        n*=d
    return n
