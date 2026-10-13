"""401830 (system init) against its original instructions, as the pinned
Input mod patches it (NOPs over 77f03e and 77f205 written in), with the real
77f240, 780ea0, 780870, 7808c0, 780850/78e720, 7803b0, 77f610, 780140, 77ff60,
402f10/780c00, 78c5a0, 77f150 (and its 785240 tail), 77efd0, 77ffc0 and 40bb20.

Fixtures: HeapCreate/HeapAlloc/HeapFree (njMalloc; GuestHeap's identity scheme),
the CRT malloc 645002 (a second GuestHeap scheme: handle 0x520000, blocks from
0x60000000), QueryPerformanceCounter (the case's count), DirectInput8Create
(IAT 7db014) and the interface/device COM objects (CreateDevice, SetDataFormat,
SetCooperativeLevel, Acquire). 780180, 77f0a0, 793c70 and 794540 are
recording fixtures (any nested call can stop the source). Compared: completion,
every global the native owners hold (see FIELDS), the 0x22000-byte texture pool,
the 3ce6f48/3ce6e78/3ce6d80 record blocks, 3d0fae0, the callback list, the
nested trace (callee, site, ECX, EDX, arguments) and the heap call traces. Not
covered: 77f0a0 under the Input mod (its own oracle and unit test), the
nested owners, and the reference's actual heap addresses.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
IAT={0x7db078:0xc,0x7db074:0xc,0x7db0f8:0xc,0x7db158:4,0x7db014:0x14}
STUB={slot:CODE+0x400+0x10*i for i,slot in enumerate(IAT)}
DI=0x11000800;DEV=0x11000900;DIVT=DATA+0xa00;DEVVT=DATA+0xb00;M={'create':CODE+0x600,'format':CODE+0x610,'coop':CODE+0x620,'acquire':CODE+0x630}
NESTED=(0x780180,0x77f0a0,0x793c70,0x794540)
FIELDS=[(0x3d0f83c,4),(0x3d0f810,4),(0x3ce71b0,4),(0x3ce71a4,4),(0x3ce715c,4),(0x3ce7154,4),(0x3ce7158,4),(0x3d127a0,4),(0x3d0f740,4),(0x3ce7150,4),(0x3ce7130,4),(0x3ce7134,4),
        (0x3d0f9c8,4),(0x3abdc10,4),(0x3abdc14,4),(0x3d128ac,4),(0x3d0f844,4),(0x3d0f80c,4),(0x3d0f830,4),(0x3d0fa14,4),(0x3d0f960,4),(0x3d0f860,1),(0x389d628,4)]
ZERO=[(0x3d0f83c,4),(0x3d0f810,4),(0x3ce71a4,4),(0x3ce715c,4),(0x3ce7154,4),(0x3ce7158,4),(0x3d127a0,4),(0x3d0f740,4),(0x3ce7150,4),(0x3ce7130,8),(0x3d0f9c8,4),(0x3abdc10,8),(0x3d128ac,4),
      (0x3d0f844,4),(0x3d0f80c,4),(0x3d0f830,4),(0x3d0fa14,4),(0x3d0f960,4),(0x3d0f860,1),(0x3d0f7c0,32),(0x3d0fa24,4),(0x3ce7128,4),(0x3ce7174,16)]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    for base in(0x50000000,0x60000000):m.mem_map(base,0x1000000)  # the DirectInput objects live in DATA (0x11000000)
    m.mem_write(0x77f03e,b'\x90'*5);m.mem_write(0x77f205,b'\x90'*5)   # the Input mod's Init NOPs
    for slot,stub in STUB.items():m.mem_write(stub,b'\xc2'+struct.pack('<H',IAT[slot]));m.mem_write(slot,struct.pack('<I',stub))
    for name,pop in(('create',0x10),('format',8),('coop',0xc),('acquire',4)):m.mem_write(M[name],b'\x31\xc0\xc2'+struct.pack('<H',pop))
    m.mem_write(DIVT+0xc,struct.pack('<I',M['create']));m.mem_write(DI,struct.pack('<I',DIVT))
    m.mem_write(DEVVT+0x2c,struct.pack('<I',M['format']));m.mem_write(DEVVT+0x34,struct.pack('<I',M['coop']));m.mem_write(DEVVT+0x1c,struct.pack('<I',M['acquire']));m.mem_write(DEV,struct.pack('<I',DEVVT))
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def record(mu,callee,site,ecx,edx,args):
        D=0xdeadbeef;st['nested'].append([callee,site,ecx,edx]+(args+[D]*4)[:4])
        if st['fail'] and len(st['nested'])==st['fail']:st['stopped']=True;mu.emu_stop();return True
        return False
    def heap_alloc(kind,size):
        h=st[kind];addr=h['next'];span=(max(size,1)+7)&~7;h['next']+=span+8;h['trace'].append(f'H|A|{size}|{addr}');return addr
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP);ret=word(mu,esp);D=0xdeadbeef
        if address==STUB[0x7db078]:st['heap']['trace'].append(f'H|C|{0x510000}|0');mu.reg_write(UC_X86_REG_EAX,0x510000);return
        if address==STUB[0x7db074]:mu.reg_write(UC_X86_REG_EAX,heap_alloc('heap',word(mu,esp+12)));return
        if address==STUB[0x7db0f8]:st['heap']['trace'].append(f'H|F|{word(mu,esp+12)}|0');mu.reg_write(UC_X86_REG_EAX,1);return
        if address==STUB[0x7db158]:mu.mem_write(word(mu,esp+4),struct.pack('<II',st['case'][15],st['case'][16]));mu.reg_write(UC_X86_REG_EAX,1);return
        if address==STUB[0x7db014]: # DirectInput8Create(instance, version, riid, ppv, outer)
            args=[word(mu,esp+4+4*k) for k in range(4)]
            if record(mu,0x77dbb0,0x40bb36,D,D,args):return
            mu.mem_write(args[3],struct.pack('<I',DI));mu.reg_write(UC_X86_REG_EAX,0);return
        if address==M['create']:
            args=[word(mu,esp+4),word(mu,esp+8)]
            if record(mu,0x40bb4f,0x40bb4f,D,D,args):return
            mu.mem_write(word(mu,esp+12),struct.pack('<I',DEV));return
        if address==M['format']:record(mu,0x40bb5f,0x40bb5f,D,D,[word(mu,esp+4),word(mu,esp+8)]);return
        if address==M['coop']:record(mu,0x40bb71,0x40bb71,D,D,[word(mu,esp+4),word(mu,esp+8),word(mu,esp+12)]);return
        if address==M['acquire']:record(mu,0x40bb7c,0x40bb7c,D,D,[word(mu,esp+4)]);return
        if address==0x645002: # CRT malloc
            h=st['crt'];h['trace'].append(f'H|C|{0x520000}|0');mu.reg_write(UC_X86_REG_EAX,heap_alloc('crt',word(mu,esp+4)));mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        site=ret-5;ecx=edx=D;args=[]
        if address==0x780180:ecx,edx=mu.reg_read(UC_X86_REG_ECX),mu.reg_read(UC_X86_REG_EDX);args=[word(mu,esp+4)]
        elif address==0x793c70:ecx,edx=mu.reg_read(UC_X86_REG_ECX),mu.reg_read(UC_X86_REG_EDX);args=[word(mu,esp+4)]
        elif address==0x794540:ecx,edx=mu.reg_read(UC_X86_REG_ECX),mu.reg_read(UC_X86_REG_EDX);args=[word(mu,esp+4+4*k) for k in range(3)]
        if record(mu,address,site,ecx,edx,args):return
        pop={0x780180:4,0x793c70:4,0x794540:0xc}.get(address,0) # fastcall callees clean their stack arguments
        mu.reg_write(UC_X86_REG_ESP,esp+4+pop);mu.reg_write(UC_X86_REG_EIP,ret)
    for addr in list(STUB.values())+list(M.values())+list(NESTED)+[0x645002]:m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_system_init
    fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.c_char_p,ctypes.c_size_t];fn.restype=ctypes.c_int
    tables=list(struct.unpack('<27I',image.read(0x389d888,108)));sizes=list(struct.unpack('<9I',image.read(0x389d864,36))) if False else [0x1f40,0xfa0,0x1f40,0x2ee0,0x5dc0,0xfa0,0xfa0,0x2ee0,0x7d00]
    rng=random.Random(0x401830);bad=[];outcomes={};cases=0
    for n in range(600):
        c=[0]*53;c[0]=0 if rng.random()<0.7 else rng.randrange(1,24);c[1]=rng.choice([0x7f,0x27f,0x37f,0xc7f]);c[2]=rng.choice([0,0,0x50000000]);c[3]=rng.choice([0xffffffff,0xffffffff,0x510000])
        c[4]=rng.choice([0,0,1,3]);
        for i in range(8):c[5+i]=rng.getrandbits(32)
        c[13]=rng.getrandbits(32);c[14]=rng.choice([0x400000,rng.getrandbits(32)]);c[15]=rng.getrandbits(32);c[16]=rng.getrandbits(32)
        for i in range(9):c[17+i]=sizes[i] if rng.random()<0.8 else rng.getrandbits(16)
        for i in range(27):c[26+i]=tables[i] if rng.random()<0.8 else rng.getrandbits(32)
        if c[2]==0x50000000 and c[3]==0xffffffff:c[3]=0x510000 # a live pool implies a created heap
        out=(ctypes.c_uint32*320)();pool=(ctypes.c_uint8*0x22000)();blocks=(ctypes.c_uint8*(0x1e0+0xd0+0xb8+0x200))();text=ctypes.create_string_buffer(8192)
        if fn((ctypes.c_uint32*53)(*c),out,pool,blocks,text,len(text))!=1:raise RuntimeError('Native rejected')
        got=list(out);got_text=[x for x in text.value.decode().split('\n') if x]
        m.context_restore(clean);st.update(case=c,nested=[],stopped=False,fail=c[0],heap=dict(next=0x50000000,trace=[]),crt=dict(next=0x60000000,trace=[]))
        for at,w in ZERO:m.mem_write(at,bytes(w))
        m.mem_write(0x3ce71b0,struct.pack('<I',c[2]));m.mem_write(0x389d628,struct.pack('<I',c[3]));m.mem_write(0x3ce712c,struct.pack('<I',c[4]));m.mem_write(0x3d0fa40,struct.pack('<8I',*c[5:13]))
        m.mem_write(0x3d0fa60,bytes(32))
        for i in range(4):m.mem_write(0x3d08428+0x18*i,struct.pack('<I',i+1))
        m.mem_write(0x3d0fd30,struct.pack('<II',c[13],c[14]));m.mem_write(0x389d864,struct.pack('<9I',*c[17:26]));m.mem_write(0x389d888,struct.pack('<27I',*c[26:53]))
        m.mem_write(0x3ce6f48,b'\x11'*0x1e0);m.mem_write(0x3ce6e78,b'\x22'*0xd0);m.mem_write(0x3ce6d80,b'\x33'*0xb8);m.mem_write(0x3d0fae0,b'\x44'*0x200);m.mem_write(0x3a8b0a0,b'\x55'*0x22000)
        if c[2]:st['heap']['next']=0x50000000;m.mem_write(0x50000000,bytes(8))
        m.reg_write(UC_X86_REG_FPCW,c[1]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x401830,STOP,count=3000000)
        except unicorn.UcError as err:raise RuntimeError(f'case {n}: {err} at {m.reg_read(UC_X86_REG_EIP):#x} nested {[[hex(x) for x in t[:2]] for t in st["nested"]]} heap {st["heap"]["trace"]} crt {st["crt"]["trace"]}')
        if not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        g=lambda at,w=4:int.from_bytes(m.mem_read(at,w),'little');e=[0]*320;e[0]=int(not st['stopped'])
        for i,(at,w) in enumerate(FIELDS):e[1+i]=g(at,w)
        for i in range(8):e[30+i]=g(0x3d0f7c0+4*i)
        e[38]=g(0x3d0fa24);e[39]=g(0x3ce7128)
        for i in range(4):e[40+i]=g(0x3ce7174+4*i)
        e[44]=g(0x3ce712c);count=e[44]
        for i in range(16):e[45+i]=g(0x3d0fa40+4*i) if i<max(8,min(count,16)) else 0
        for i in range(16):
            if got[45+i]==0 and i>=8 and e[45+i]==0:pass
        for i in range(4):e[66+i]=g(0x3d08428+0x18*i)
        e[70]=len(st['nested'])
        for i,t in enumerate(st['nested'][:28]):e[71+8*i:79+8*i]=t
        source_text=st['heap']['trace']+st['crt']['trace']
        # Native slots past the list's vector are not output (0); the source's memory past the 8 seeded slots is zero.
        problems=[i for i in range(320) if got[i]!=e[i]]
        if bytes(pool)!=bytes(m.mem_read(0x3a8b0a0,0x22000)):problems.append('pool')
        src_blocks=bytes(m.mem_read(0x3ce6f48,0x1e0))+bytes(m.mem_read(0x3ce6e78,0xd0))+bytes(m.mem_read(0x3ce6d80,0xb8))+bytes(m.mem_read(0x3d0fae0,0x200))
        if bytes(blocks)!=src_blocks:problems.append('blocks')
        if got_text!=source_text:problems.append('heap traces')
        cases+=1;key=f"completed{e[0]}:nested{e[70]}";outcomes[key]=outcomes.get(key,0)+1
        if problems:bad.append(dict(case=n,problems=[hex(x) if isinstance(x,int) else x for x in problems[:12]],source=[hex(e[x]) for x in problems[:12] if isinstance(x,int)],native=[hex(got[x]) for x in problems[:12] if isinstance(x,int)],
            native_text=got_text[:6] if 'heap traces' in problems else None,source_text=source_text[:6] if 'heap traces' in problems else None))
    names=['src/engine/ninja_system_init.hpp','src/engine/ninja_system_init.cpp','src/engine/ninja_texture_pool.cpp','src/engine/ninja_memory.cpp','src/loader/guest_heap.cpp','tests/probes/native_system_init_probe.cpp','tools/oracles/verify_original_system_init.py']
    ranges=((0x401830,0x4018ff),(0x77f240,0x77f25a),(0x780ea0,0x780f15),(0x780870,0x7808a8),(0x7808c0,0x7808d4),(0x780850,0x780865),(0x78e720,0x78e791),(0x7803b0,0x7803b7),(0x77f610,0x77f6d5),
            (0x780140,0x780156),(0x77ff60,0x77ff6a),(0x77f150,0x77f21f),(0x785240,0x785268),(0x77efd0,0x77f053),(0x40bb20,0x40bb80))
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={f'{x:#x}-{y:#x}':sha(image.read(x,y-x)) for x,y in ranges},profile_patches={'0x77f03e':'5 x nop','0x77f205':'5 x nop'},
        cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
