"""Per-frame GameLoop owners 40d4d0 (402f00, 77ed00/77ed20, 402ed0 with the real
7815c0, 40bcf0), 40eab0, 42f990, 40d3c0, 420840 (77ed00/77ed20, 4128f0 with
the real 402f40/77ff90) and the key queries 40ef30/40ef20, against original
instructions. SetRenderState is a recording COM fixture; 40e2c0 and 780fb0
are recording fixtures (one can stop the source). The draw-list, effect and
release-walk bodies are native nested owners: the source stops when it reaches
their first body instruction (40d520, 40eac0, 42f9cc, 40d3f5). Compared:
completion, EAX for the key queries, every touched global (FOV words, the
3d0f9e0 projection scale, counters, list/buffer words, 3b280a8, the 3ab9864
and 3d0f7f0 colours), the render-state trace and the nested-owner trace.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
DEVICE=DATA+0x100;VTABLE=DATA+0x400;METHOD=CODE+0x300
CALLEES=[0x40d4d0,0x40eab0,0x42f990,0x40d3c0,0x420840,0x40ef30,0x40ef20]
FIXTURES=(0x40e2c0,0x780fb0);BODIES=(0x40d520,0x40eac0,0x42f9cc,0x40d3f5)
GLOBALS={4:(0x3b280a8,1),5:(0x3b0e3b8,1),6:(0x3b0e338,4),7:(0x3b0e998,4),8:(0x3b2cbb4,4),9:(0x3ab98ec,4),10:(0x3aad0bc,4),11:(0x3d0f9e0,4),12:(0x3d0f9e4,4),
    13:(0x3b2c584,4),14:(0x3b0e334,4),15:(0x3abdf88,4),16:(0x3abdf84,4)}
def f32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    m.mem_map(0,0x1000) # fs:0 SEH chain (fs base 0)
    m.mem_write(METHOD,b'\x31\xc0\xc2\x0c\x00');m.mem_write(VTABLE+0xc8,struct.pack('<I',METHOD));m.mem_write(DEVICE,struct.pack('<I',VTABLE))
    clean=m.context_save();st={}
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==METHOD:st['render'].append(list(struct.unpack('<II',mu.mem_read(esp+8,8)))+[struct.unpack('<I',mu.mem_read(esp,4))[0]-6]);return
        if address in BODIES:
            arg=mu.reg_read(UC_X86_REG_EAX) if address in(0x40d520,0x42f9cc) else 0xdeadbeef
            st['nested'].append([address,address,arg]);st['stopped']=True;mu.emu_stop();return
        ret,arg=struct.unpack('<II',mu.mem_read(esp,8));st['nested'].append([address,ret-5,arg])
        if st['fail'] and len(st['nested'])==st['fail']:st['stopped']=True;mu.emu_stop();return
        mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
    for addr in (METHOD,)+FIXTURES+BODIES:m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    table=image.read(0x88af78,4100)+image.read(0x88ad9c,32)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_frame_owners;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x40d4d0);bad=[];outcomes={};cases=0
    for n in range(2100):
        callee=CALLEES[n%len(CALLEES)];c=[0]*(48+0x302)
        c[1]=rng.choice([0,1,2,5,255,256,257]) if callee==0x40ef20 else 0;c[2]=rng.choice([0x7f,0x27f,0x37f,0xc7f]);c[3]=0 if rng.random()<0.8 else rng.randrange(1,3)
        c[4]=rng.getrandbits(8);c[5]=rng.getrandbits(8);c[6]=rng.choice([0,0,1,3,0xffffffff]);c[7]=rng.choice([0,0,1,0xffffffff,0x7fffffff])
        fovs=[0x31c7,0x2000,0x31c7,rng.getrandbits(16)];c[8]=rng.choice([0,0x31c7,rng.getrandbits(16)]);c[9]=rng.choice(fovs);c[10]=rng.choice(fovs)
        c[11]=f32(rng.uniform(100,500));c[12]=f32(rng.choice([320.0,640.0,rng.uniform(100,1000)]))
        c[13]=rng.choice([0,0,0x5000]);c[14]=rng.choice([0,0,1,0xffffffff]);c[15]=rng.choice([0,1,2,0xffffffff]);c[16]=rng.choice([0,0x1000])
        for i in range(4):c[32+i]=f32(rng.choice([0.5,1.0,-2.0,2.0,rng.uniform(-3,3)]));c[36+i]=rng.getrandbits(32);c[40+i]=rng.getrandbits(32)
        for j in range(0x302):c[48+j]=rng.getrandbits(8)
        c[0]=callee
        out=(ctypes.c_uint32*128)()
        if fn((ctypes.c_uint32*len(c))(*c),(ctypes.c_uint8*len(table)).from_buffer_copy(table),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);st.update(render=[],nested=[],stopped=False,fail=c[3])
        for k,(addr,w) in GLOBALS.items():m.mem_write(addr,(c[k]&((1<<(8*w))-1)).to_bytes(w,'little'))
        for i in range(4):m.mem_write(0x3b18230+4*i,struct.pack('<I',c[32+i]));m.mem_write(0x3ab9864+4*i,struct.pack('<I',c[36+i]));m.mem_write(0x3d0f7f0+4*i,struct.pack('<I',c[40+i]))
        m.mem_write(0x3b0e3e0,bytes(c[48:48+0x302]));m.mem_write(0x3d128b0,struct.pack('<I',DEVICE))
        m.reg_write(UC_X86_REG_FPCW,c[2]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<II',STOP,c[1]));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(callee,STOP,count=200000)
        except unicorn.UcError as err:raise RuntimeError(f'case {n} {callee:#x}: {err} at {m.reg_read(UC_X86_REG_EIP):#x} input {c[:17]}')
        if not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        g=lambda addr,w=4:int.from_bytes(m.mem_read(addr,w),'little')
        e=[0]*128;e[0]=int(not st['stopped'])
        if callee in(0x40ef30,0x40ef20) and not st['stopped']:e[1]=m.reg_read(UC_X86_REG_EAX);e[2]=1
        else:e[1]=0xdeadbeef;e[2]=0
        for k,(addr,w) in GLOBALS.items():
            if k in(5,7,12):continue
            e[k]=g(addr,w)
        for i in range(4):e[36+i]=g(0x3ab9864+4*i);e[40+i]=g(0x3d0f7f0+4*i)
        e[60]=len(st['render']);[e.__setitem__(slice(64+3*i,67+3*i),t) for i,t in enumerate(st['render'][:8])]
        e[90]=len(st['nested']);[e.__setitem__(slice(92+3*i,95+3*i),t) for i,t in enumerate(st['nested'][:8])]
        got[5]=got[7]=got[12]=0
        if callee==0x40ef20 and c[1]>256:e[1]=0xdeadbeef;e[2]=0;e[0]=0 # index past the edges table: native explicit
        cases+=1;key=f"{callee:#x}:completed{e[0]}:nested{e[90]}:render{e[60]}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(128) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,callee=hex(callee),fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got[i]) for i in d[:12]]))
    names=['src/game/game_frame_owners.hpp','src/game/game_frame_owners.cpp','tests/probes/native_game_frame_owners_probe.cpp','tools/oracles/verify_original_game_frame_owners.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={hex(x):sha(image.read(x,0x40)) for x in CALLEES+[0x40bcf0,0x402ed0,0x402f00,0x4128f0]},cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
