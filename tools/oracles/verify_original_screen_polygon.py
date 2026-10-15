"""77dfc0 (BeginScene's 78b922 polygon) against its original instructions, with
the real 791940, 78bba0, 78b780, 78ea10, 78b800, 78dbe0 and the D3DX8 math
772615 (Vec3Project), 774310 (MatrixMultiply) and 774150 (Vec3TransformCoord).

The whole PE is mapped. The D3DX dispatch slots are written as 77350c leaves
them on an SSE CPU without 3DNow! (389b784 = 774310, 389b7a0 = 774150) and
389b7b0 = 772615 (the default every path keeps); 77350c's CPU and registry
probes are not executed. RCPPS is Unicorn's (QEMU's float division), which is
not a real CPU's estimate: the native probe uses the same division model, so
this checks everything around RCPPS, not the reference CPU's table. MXCSR
choices keep FTZ/DAZ clear. Cases keep at most one NaN source (the argument or one matrix word): Unicorn
applies QEMU's x87 NaN rule (larger significand) to SSE operands, where hardware
SSE returns the first operand's NaN. MXCSR is compared without its six exception flags:
Unicorn 2.1.4 raises none, which hardware and the native SseContext do. The device is a COM fixture recording
SetRenderState (+0xc8), SetTextureStageState (+0xfc), SetTexture (+0xf4),
SetVertexShader (+0x130) and DrawPrimitiveUP (+0x120, with the vertex bytes it
is given). A null device faults at the vtable load (page 0 is unmapped), as the
native owner stops (null_device). Compared: completion, 3d0fdf4, whether
3d129a0 was cleared, the device trace, the drawn vertices and MXCSR.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_MXCSR
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
DEVICE=DATA+0x100;VTABLE=DATA+0x400;BLOCK=DATA+0x800;POINTS=DATA+0x900;COLORS=DATA+0xa00;BUFFER=DATA+0x1000
METHODS={0xc8:(CODE+0x300,0xc),0xfc:(CODE+0x310,0x10),0xf4:(CODE+0x320,0xc),0x130:(CODE+0x330,8),0x120:(CODE+0x340,0x14)}
NATIVE_BLOCK,NATIVE_POINTS,NATIVE_COLORS=0x10000800,0x10000900,0x10000a00
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    for off,(stub,pop) in METHODS.items():m.mem_write(stub,b'\x31\xc0\xc2'+struct.pack('<H',pop));m.mem_write(VTABLE+off,struct.pack('<I',stub))
    m.mem_write(DEVICE,struct.pack('<I',VTABLE))
    m.mem_write(0x389b784,struct.pack('<I',0x774310));m.mem_write(0x389b7a0,struct.pack('<I',0x774150));m.mem_write(0x389b7b0,struct.pack('<I',0x772615))
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==METHODS[0xc8][0]:st['calls'].append([0xc8,word(mu,esp+8),word(mu,esp+12)]);return
        if address==METHODS[0xfc][0]:st['calls'].append([0xfc,word(mu,esp+12),word(mu,esp+16)]);return
        if address==METHODS[0xf4][0]:st['calls'].append([0xf4,word(mu,esp+8),int(word(mu,esp+12)!=0)]);return
        if address==METHODS[0x130][0]:st['calls'].append([0x130,word(mu,esp+8),0]);return
        if address==METHODS[0x120][0]:
            t,count,ptr,stride=(word(mu,esp+8+4*i) for i in range(4));st['calls'].append([0x120,t,count]);n=min(30,(count+2)*stride//4)
            st['draw']=[t,count,stride,int(ptr==BUFFER)]+list(struct.unpack(f'<{n}I',mu.mem_read(ptr,4*n)));return
    for stub,_ in METHODS.values():m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=stub,end=stub)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_screen_polygon;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    matrix=list(struct.unpack('<16I',image.read(0x389d318,64)))
    fb=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
    rng=random.Random(0x77dfc0);bad=[];outcomes={};cases=0;corpus=[]
    for n in range(3000):
        c=[0]*63;c[0]=rng.choice([0x7f,0x27f,0x37f,0xc7f,0x47f,0x87f]);c[1]=rng.choice([0x1f80,0x1f80,0x3f80,0x5f80,0x7f80,0x1f81])
        c[2]=rng.choice([4,4,4,3,5,6,2,1,0,0x80000004]);c[3]=rng.choice([0xbf800000,0xbf800000,0x3f800000,0xc0000000,0xbf000000,0,0x80000000,0x7fc00000,rng.getrandbits(32)])
        c[4]=rng.choice([0x22,0x40,0x62,0,0xffffffff,rng.getrandbits(32)]);c[5]=rng.choice([0xffffffff,0xffffffff,0,7]);c[6]=int(rng.random()<0.93)
        for i in range(12):c[7+i]=fb(rng.choice([0.0,640.0,480.0,320.0,rng.uniform(-1000,1000)]))
        for i in range(6):c[19+i]=rng.getrandbits(32)
        c[25]=rng.choice([0,0,10,0x80000000]);c[26]=rng.choice([0,0,20]);c[27]=rng.choice([640,1280,0,0x80000000,rng.getrandbits(32)]);c[28]=rng.choice([480,720,0,0x80000001,rng.getrandbits(32)])
        c[29]=rng.choice([0,0,fb(0.25)]);c[30]=rng.choice([0x3f800000,0x3f800000,fb(0.75)])
        kind=rng.random()
        for i in range(16):
            if kind<0.5:proj=[1.3,0,0,0, 0,1.7,0,0, 0,0,1.0001,1.0, 0,0,-0.1,0][i]
            elif kind<0.8:proj=rng.uniform(-2,2)
            else:proj=None
            c[31+i]=fb(proj) if proj is not None else rng.getrandbits(32)
            c[47+i]=matrix[i] if rng.random()<0.85 else fb(rng.uniform(-2,2))
        # One NaN source at most: Unicorn applies QEMU's x87 rule (larger significand) to SSE operands, where
        # hardware SSE returns the first operand's NaN, so two different NaNs meeting would compare the emulator.
        isnan=lambda w:(w&0x7fffffff)>0x7f800000
        for i in range(31,63):
            if isnan(c[i]):c[i]&=~0x40000000
        if not isnan(c[3]) and rng.random()<0.05:c[31+rng.randrange(32)]=rng.choice([0x7fc00000,0xffc00001,0x7f800001,0xff91bb64])
        corpus.append(c.copy())
        out=(ctypes.c_uint32*200)()
        if fn((ctypes.c_uint32*len(c))(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);st.update(calls=[],draw=[])
        m.mem_write(BLOCK,struct.pack('<4I',POINTS,COLORS,0x5555,0x6666));m.mem_write(POINTS,struct.pack('<12I',*c[7:19]));m.mem_write(COLORS,struct.pack('<6I',*c[19:25]))
        m.mem_write(BUFFER,bytes(0x100));m.mem_write(0x3d128ac,struct.pack('<I',BUFFER))
        m.mem_write(0x3d0fdf4,struct.pack('<I',c[5]));m.mem_write(0x3d129a0,struct.pack('<I',0x7777))
        m.mem_write(0x3d12780,struct.pack('<6I',*c[25:31]));m.mem_write(0x3d129c0,struct.pack('<16I',*c[31:47]));m.mem_write(0x389d318,struct.pack('<16I',*c[47:63]))
        m.mem_write(0x3d128b0,struct.pack('<I',DEVICE if c[6] else 0))
        m.reg_write(UC_X86_REG_FPCW,c[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.reg_write(UC_X86_REG_MXCSR,c[1])
        m.mem_write(STACK,struct.pack('<5I',STOP,BLOCK,c[2],c[3],c[4]));m.reg_write(UC_X86_REG_ESP,STACK)
        faulted=False
        try:m.emu_start(0x77dfc0,STOP,count=400000)
        except unicorn.UcError:faulted=True
        if not faulted and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'case {n}: ended at {m.reg_read(UC_X86_REG_EIP):#x}')
        e=[0]*200;e[0]=int(not faulted);e[1]=word(m,0x3d0fdf4);e[2]=int(word(m,0x3d129a0)==0)
        e[3]=len(st['calls'])
        for i,t in enumerate(st['calls'][:24]):e[4+3*i:7+3*i]=t
        if st['draw']:
            d_=st['draw'];e[80],e[81],e[82]=d_[0],d_[1],d_[2]
            if not d_[3]:raise RuntimeError(f'case {n}: DrawPrimitiveUP was not given 3d128ac')
            for i,w in enumerate(d_[4:34]):e[84+i]=w
        e[120]=m.reg_read(UC_X86_REG_MXCSR)&~0x3f;got[120]&=~0x3f # Unicorn raises no SSE exception flags
        cases+=1;key=f"completed{e[0]}:calls{e[3]}:drawn{int(bool(st['draw']))}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(200) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got[i]) for i in d[:12]]))
    names=['src/engine/ninja_screen.hpp','src/engine/ninja_screen.cpp','src/math/d3dx_math.hpp','src/math/d3dx_math.cpp','src/cpu/sse_math.cpp','tests/probes/native_screen_polygon_probe.cpp','tools/oracles/verify_original_screen_polygon.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x77dfc0-0x77e04e':sha(image.read(0x77dfc0,0x8e)),'0x78ea10-0x78eaaf':sha(image.read(0x78ea10,0x9f)),'0x78dbe0-0x78dc49':sha(image.read(0x78dbe0,0x69)),
            '0x772615-0x7727a2':sha(image.read(0x772615,0x18d)),'0x774150-0x774243':sha(image.read(0x774150,0xf3)),'0x774310-0x774456':sha(image.read(0x774310,0x146)),
            '0x791940-0x79198b':sha(image.read(0x791940,0x4b)),'0x78b780-0x78b83d':sha(image.read(0x78b780,0xbd)),'0x78bba0-0x78bbd0':sha(image.read(0x78bba0,0x30))},
        cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    path=a.output.with_suffix('.cases.bin');path.write_bytes(b''.join(struct.pack('<63I',*c) for c in corpus))
    report['corpus']=dict(path=str(path),cases=cases,words_per_case=63,entry='77dfc0')
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
