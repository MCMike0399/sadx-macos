"""D3DX8 772615 (Vec3Project) against its original instructions, with the
SSE-path 774310 (MatrixMultiply) and 774150 (Vec3TransformCoord).

The dispatch slots are written as 77350c leaves them on an SSE CPU without
3DNow! (389b784 = 774310, 389b7a0 = 774150); 77350c is not executed. Each case
calls 772615 (stdcall) with any of the projection, view, world and viewport
pointers NULL, and the matrices at 16-byte aligned or misaligned (+8)
addresses, which selects 774150's movaps or movlps path. RCPPS is Unicorn's
(QEMU's float division), not a real CPU's estimate; the native probe uses the
same division model. Cases keep at most one NaN source: Unicorn applies QEMU's
x87 NaN rule (larger significand) to SSE operands, where hardware SSE returns
the first operand's NaN. MXCSR is compared without its six exception flags,
which Unicorn 2.1.4 does not raise. Compared: the three output floats.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_MXCSR
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA
sha=lambda b:hashlib.sha256(b).hexdigest()
OUT=DATA+0x100;V=DATA+0x120;VP=DATA+0x140;MATRICES=(DATA+0x200,DATA+0x300,DATA+0x400)
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    m.mem_write(0x389b784,struct.pack('<I',0x774310));m.mem_write(0x389b7a0,struct.pack('<I',0x774150))
    clean=m.context_save()
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_d3dx_project;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    fb=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
    isnan=lambda w:(w&0x7fffffff)>0x7f800000
    rng=random.Random(0x772615);bad=[];outcomes={};cases=0
    for n in range(4000):
        c=[0]*61;c[0]=rng.choice([0x7f,0x27f,0x37f,0xc7f,0x47f,0x87f]);c[1]=rng.choice([0x1f80,0x1f80,0x3f80,0x5f80,0x7f80])
        c[2]=rng.randrange(16);c[3]=rng.randrange(8)
        for i in range(3):c[4+i]=fb(rng.uniform(-100,100)) if rng.random()<0.9 else rng.choice([0,0x80000000,fb(1e30),fb(1e-30)])
        c[7]=rng.choice([0,10,0x80000000,rng.getrandbits(32)]);c[8]=rng.choice([0,20,0xfffffff0]);c[9]=rng.choice([640,1280,0,0x80000000,rng.getrandbits(32)]);c[10]=rng.choice([480,0,0x80000001,rng.getrandbits(32)])
        c[11]=rng.choice([0,fb(0.25),fb(-1.0)]);c[12]=rng.choice([0x3f800000,fb(0.75),fb(100.0)])
        for i in range(48):c[13+i]=fb(rng.uniform(-3,3)) if rng.random()<0.9 else rng.choice([0,0x80000000,fb(1e20),rng.getrandbits(32)])
        for i in list(range(4,13))+list(range(13,61)):
            if isnan(c[i]):c[i]&=~0x40000000
        if rng.random()<0.05:c[rng.choice(list(range(4,7))+list(range(11,61)))]=rng.choice([0x7fc00000,0xffc00001,0x7f800001,0xff91bb64])
        out=(ctypes.c_uint32*8)()
        if fn((ctypes.c_uint32*len(c))(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean)
        addresses=[]
        for k in range(3):
            at=MATRICES[k]+(0 if c[3]>>k&1 else 8);addresses.append(at if c[2]>>k&1 else 0);m.mem_write(at,struct.pack('<16I',*c[13+16*k:29+16*k]))
        m.mem_write(V,struct.pack('<3I',*c[4:7]));m.mem_write(VP,struct.pack('<6I',*c[7:13]));m.mem_write(OUT,bytes(12))
        m.reg_write(UC_X86_REG_FPCW,c[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.reg_write(UC_X86_REG_MXCSR,c[1])
        m.mem_write(STACK,struct.pack('<7I',STOP,OUT,V,VP if c[2]&8 else 0,*addresses));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x772615,STOP,count=100000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'case {n}: ended at {m.reg_read(UC_X86_REG_EIP):#x}')
        e=list(struct.unpack('<3I',m.mem_read(OUT,12)))+[m.reg_read(UC_X86_REG_MXCSR)&~0x3f];got[3]&=~0x3f
        cases+=1;key=f"arm{c[2]&7}:viewport{c[2]>>3}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(4) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,fields=d,source=[hex(e[i]) for i in d],native=[hex(got[i]) for i in d]))
    names=['src/d3dx/d3dx_math.hpp','src/d3dx/d3dx_math.cpp','src/cpu/sse_math.cpp','tests/probes/native_d3dx_project_probe.cpp','tools/oracles/verify_original_d3dx_project.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x772615-0x7727a2':sha(image.read(0x772615,0x18d)),'0x774150-0x774243':sha(image.read(0x774150,0xf3)),'0x774310-0x774456':sha(image.read(0x774310,0x146))},
        cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
