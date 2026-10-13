"""WinMain 4025b0's settings span (4025b0..402c29, up to CoInitialize) against
its original instructions, patched as the pinned mod loader patches it: a CALL
at 402614 to a SetLanguage fixture (stores VoiceLanguage 3b0ef38 and
TextLanguage 3b0f0e8 from the case's loader settings) and a JMP at 40297a to
402a90.

The whole PE is mapped. Win32 imports are IAT fixtures: GetPrivateProfileIntA
and GetPrivateProfileStringA answer from the case's scripted INI table (absent
keys return the fallback; strings truncated to size-1), wsprintfA formats the
"%d" templates, LoadLibraryA/GetProcAddress/ShowCursor return scripted values,
MessageBoxA and the ImmDisableIME procedure record. 401a20, 40ff20, 40ccf0,
40e8a0 and 40e8b0 are recording fixtures; any nested call can stop the source.
Compared: completion, every global the span writes (3b0ef38, 3b0f0e8, 89295c,
909eb4, 3abdcf0, 38f6ec0, 3abdfac, 3a7cf3c, 909f28, 892950, 3b0ef28, 3abdf90, 3b0eae0,
the 13 words at 389d634), the imm32 module local, the ordered profile-read trace (site, section,
key, fallback) and the nested trace (callee, site, arguments). Not covered: the
INI parsing itself (Win32Profile has unit tests; its Win32 semantics are a
reported re-implementation) and the unported nested owners.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
IAT={0x7db040:('LoadLibraryA',4),0x7db044:('GetProcAddress',8),0x7db1e0:('MessageBoxA',0x10),0x7db034:('GetPrivateProfileIntA',0x10),
     0x7db038:('GetPrivateProfileStringA',0x18),0x7db228:('wsprintfA',0),0x7db1dc:('ShowCursor',4)}
STUB={slot:CODE+0x400+0x10*i for i,slot in enumerate(IAT)};PROC=CODE+0x500;SETLANG=CODE+0x510
GAME=(0x401a20,0x40e8b0)
SITES=[0x4025c9,0x4025e1,0x4025e8,0x402621,0x40269a,0x402961,0x402c09]
WORDS=[(0x909eb4,1,6),(0x38f6ec0,4,7),(0x3abdfac,4,8),(0x3a7cf3c,4,9),(0x89295c,4,10),(0x3ab918c,4,11),(0x3ab9190,4,12),(0x8929b0,4,13),(0x8929c4,4,14)]+\
      [(0x8929d8+4*i,4,15+i) for i in range(17)]+[(0x389d634+2*i,2,32+i) for i in range(13)]+[(0x3b0ef38,4,45),(0x3b0f0e8,4,46),(0x3abdcf0,4,47),(0x909f28,4,48),(0x892950,4,49),(0x3b0ef28,4,50),(0x3abdf90,4,51),(0x3b0eae0,4,52)]
KEYS=['framerate','fogemulation','sound3d','cliplevel','sevoice','voicev','bgm','bgmv','screen','mousemode']+[f'cmd{i}' for i in range(5)]
def cstr(m,a):
    out=b''
    while True:
        c=bytes(m.mem_read(a+len(out),1))
        if c==b'\0':return out.decode('latin-1')
        out+=c
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x27f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    for slot,stub in STUB.items():
        pop=IAT[slot][1];m.mem_write(stub,b'\xc3' if pop==0 else b'\xc2'+struct.pack('<H',pop));m.mem_write(slot,struct.pack('<I',stub))
    m.mem_write(PROC,b'\xc2\x04\x00');m.mem_write(SETLANG,b'\xc3')
    m.mem_write(0x402614,b'\xe8'+struct.pack('<i',SETLANG-(0x402614+5)))           # loader WriteCall(402614, SetLanguage)
    m.mem_write(0x40297a,b'\xe9'+struct.pack('<i',0x402a90-(0x40297a+5)))           # loader WriteJump(40297a, 402a90)
    returns={}
    for site in SITES:
        op=image.read(site,2);returns[site+(5 if op[0]==0xe8 else 6 if op==b'\xff\x15' else 2)]=site
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def nested(mu,callee,site,xs):
        st['nested'].append([callee,site]+xs)
        if st['fail'] and len(st['nested'])==st['fail']:st['stopped']=True;mu.emu_stop();return True
        return False
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP);ret=word(mu,esp);D=0xdeadbeef;c=st['case']
        if address==SETLANG:mu.mem_write(0x3b0ef38,struct.pack('<I',c[4]));mu.mem_write(0x3b0f0e8,struct.pack('<I',c[5]));return
        if address==PROC:nested(mu,PROC,returns.get(ret,ret),[word(mu,esp+4),D,D,D]);return
        if address in GAME:
            n=2 if address==0x40e8b0 else 0 if address==0x401a20 else 1
            xs=[word(mu,esp+4+4*k) for k in range(n)]+[D]*(4-n)
            if not nested(mu,address,returns.get(ret,ret),xs):mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
            return
        name=[n for s_,(n,_) in IAT.items() if STUB[s_]==address][0]
        if name=='LoadLibraryA':
            if not nested(mu,0x7db040,returns.get(ret,ret),[D]*4):mu.reg_write(UC_X86_REG_EAX,c[1])
        elif name=='GetProcAddress':
            if not nested(mu,0x7db044,returns.get(ret,ret),[word(mu,esp+4),word(mu,esp+8),D,D]):mu.reg_write(UC_X86_REG_EAX,c[2])
        elif name=='MessageBoxA':nested(mu,0x7db1e0,returns.get(ret,ret),[word(mu,esp+4+4*k) for k in range(4)])
        elif name=='ShowCursor':
            if not nested(mu,0x7db1dc,returns.get(ret,ret),[word(mu,esp+4),D,D,D]):st['cursor']-=1;mu.reg_write(UC_X86_REG_EAX,st['cursor']&0xffffffff)
        elif name=='GetPrivateProfileIntA':
            section,key=cstr(mu,word(mu,esp+4)),cstr(mu,word(mu,esp+8));fallback=struct.unpack('<i',mu.mem_read(esp+12,4))[0]
            st['trace'].append(f'{ret-2}|{section}|{key}|{fallback}')
            v=st['ini'].get((section.lower(),key.lower()));mu.reg_write(UC_X86_REG_EAX,(int(v) if v is not None else fallback)&0xffffffff)
        elif name=='GetPrivateProfileStringA':
            section,key,fallback=cstr(mu,word(mu,esp+4)),cstr(mu,word(mu,esp+8)),cstr(mu,word(mu,esp+12));buf,size=word(mu,esp+16),word(mu,esp+20)
            st['trace'].append(f'{ret-6}|{section}|{key}|{fallback}')
            v=st['ini'].get((section.lower(),key.lower()),fallback)[:size-1];mu.mem_write(buf,v.encode('latin-1')+b'\0');mu.reg_write(UC_X86_REG_EAX,len(v))
        elif name=='wsprintfA':
            buf,fmt,arg=word(mu,esp+4),cstr(mu,word(mu,esp+8)),struct.unpack('<i',mu.mem_read(esp+12,4))[0]
            v=fmt.replace('%d',str(arg));mu.mem_write(buf,v.encode()+b'\0');mu.reg_write(UC_X86_REG_EAX,len(v))
    for addr in list(STUB.values())+[PROC,SETLANG]+list(GAME):m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_winmain
    fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_char_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_char_p,ctypes.c_size_t];fn.restype=ctypes.c_int
    pad_bits=[struct.unpack('<i',image.read(0x8929d8+4*i,4))[0] for i in range(17)]
    rng=random.Random(0x4025b0);bad=[];outcomes={};cases=0
    for n in range(3000):
        c=[0]*53;c[0]=0 if rng.random()<0.8 else rng.randrange(1,8);c[1]=rng.choice([0,0x10000000,0x6f000000]);c[2]=rng.choice([PROC,PROC,PROC,0]);c[3]=rng.choice([0,1,2,3])
        c[4]=rng.choice([0,1,2,5]);c[5]=rng.choice([0,1,3]);c[6]=rng.getrandbits(8);c[7]=rng.getrandbits(32);c[8]=rng.getrandbits(32);c[9]=rng.getrandbits(32);c[10]=rng.getrandbits(32)
        c[11]=rng.choice([0,1,0x1234]);c[12]=rng.choice([0,1]);c[13]=0x7db4a8;c[14]=0x7db2c8
        for i in range(17):c[15+i]=pad_bits[i] if rng.random()<0.9 else rng.choice([-1,0,12,5])
        for i in range(13):c[32+i]=rng.getrandbits(16)
        for k in range(45,53):c[k]=rng.getrandbits(32)
        ini={}
        for k in KEYS:
            if rng.random()<0.6:ini[('sonicdx',k)]=str(rng.choice([-5,-1,0,1,2,3,4,50,99,100,101,0x7fffffff,-0x80000000,rng.randrange(-200,200)]))
        if rng.random()<0.5:
            section=rng.choice(['pad_default','custom','PAD_DEFAULT']);
            if section!='pad_default' or rng.random()<0.5:ini[('sonicdx','padconfig')]=section
            sec=ini.get(('sonicdx','padconfig'),'pad_default').lower()
            ini[(sec,'buttons')]=str(rng.choice([0,1,5,9,13,14,20,-3]))
            for i in range(14):
                if rng.random()<0.5:ini[(sec,f'pad{i}')]=str(rng.choice([-2,-1,0,1,5,9,0x11,0x12,rng.randrange(-3,20)]))
        blob='\n'.join(f'{s}\x01{k}\x01{v}' for (s,k),v in ini.items()).encode('latin-1')
        out=(ctypes.c_uint32*256)();trace=ctypes.create_string_buffer(65536)
        if fn((ctypes.c_uint32*len(c))(*c),blob,out,trace,len(trace))!=1:raise RuntimeError('Native rejected')
        got=list(out);got_trace=trace.value.decode('latin-1').strip('\n').split('\n') if trace.value else []
        m.context_restore(clean);st.update(nested=[],trace=[],stopped=False,fail=c[0],case=c,cursor=c[3],ini={(s,k):v for (s,k),v in ini.items()})
        for at,w,k in WORDS:m.mem_write(at,(c[k]&((1<<(8*w))-1)).to_bytes(w,'little'))
        m.mem_write(STACK,struct.pack('<5I',STOP,0x400000,0,0,1));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x4025b0,0x402c29,count=500000)
        except unicorn.UcError as err: # ImmDisableIME through a null GetProcAddress result: the source faults at 4025e8 as the native owner stops
            if not(m.reg_read(UC_X86_REG_EIP)==0 and st['nested'] and st['nested'][-1][0]==0x7db044):raise RuntimeError(f'case {n}: {err} at {m.reg_read(UC_X86_REG_EIP):#x}')
            st['stopped']=True
        if not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=0x402c29:raise RuntimeError(f'case {n}: ended at {m.reg_read(UC_X86_REG_EIP):#x}')
        e=[0]*256;e[0]=int(not st['stopped'])
        e[1]=struct.unpack('<I',m.mem_read(STACK-0x6c-0x10+0x10,4))[0] if False else got[1] # the local is compared through LoadLibraryA's scripted result below
        for at,w,k in WORDS:
            if k in(11,12,13,14) or 15<=k<32:continue
            e[k]=int.from_bytes(m.mem_read(at,w),'little')
        got_cmp=list(got)
        for k in(11,12,13,14):got_cmp[k]=0
        for k in range(15,32):got_cmp[k]=0
        e[60]=len(st['nested'])
        for i,t in enumerate(st['nested'][:24]):e[61+6*i:67+6*i]=t
        if got[0] and got[1]!=c[1]:bad.append(dict(case=n,why='imm32 module',native=hex(got[1])))
        got_cmp[1]=e[1]
        cases+=1;key=f"completed{e[0]}:nested{e[60]}:reads{len(st['trace'])}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(256) if got_cmp[i]!=e[i]]
        if d or got_trace!=st['trace']:bad.append(dict(case=n,fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got_cmp[i]) for i in d[:12]],trace_match=got_trace==st['trace'],
            source_trace=st['trace'][:6] if got_trace!=st['trace'] else None,native_trace=got_trace[:6] if got_trace!=st['trace'] else None))
    names=['src/game/game_winmain.hpp','src/game/game_winmain.cpp','tests/probes/native_game_winmain_probe.cpp','tools/oracles/verify_original_game_winmain.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_range_sha256={'0x4025b0-0x402c29':sha(image.read(0x4025b0,0x402c29-0x4025b0))},loader_patches={'0x402614':'call SetLanguage','0x40297a':'jmp 0x402a90'},
        cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
