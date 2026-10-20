"""Projected depth 78dbe0 (stdcall, float w) against original instructions.

78dbe0 compares w with 7e9b40 (FCOMP; below it, or unordered, z = 65535.0, else
z = 1.0f / w through FDIV and FSTP m32), then D3DXVec3Project((0, 0, z),
viewport 3d12780, projection 3d129c0, view = world = 389d318) through thunk
770295 and returns FLD out.z in ST(0) (ret 4). The original runs 770295, 772615,
774310 and 774150 as real code with dispatch slots 3/10/14 written as 77350c
leaves them on an SSE CPU without 3DNow! (774310, 774150, 772615); 77350c is not
executed. Cases: w at the 7e9b40 edge and its neighbours, signed zeros,
negatives, infinities, denormals, NaNs, and typical depths; integer viewports
including zero/negative extents; perspective-like and random projections; the
image's 389d318 or random words; four masked x87 control words and the MXCSR
rounding modes. Compared: the returned ext80 (bitwise), that exactly one value
was pushed (TOP and tag), EAX (772615's pOut, entry ESP - 0xc). RCPPS is
Unicorn's QEMU division, the same model the native port uses; cases keep at
most one NaN source (Unicorn applies the x87 NaN rule to SSE operands). x87
exception flags are compared separately: Unicorn 2.1.4 raises no PE for an
inexact FDIV/FSTP (1/w for most w) nor IE/DE for NaN/denormal operands, which
the native model raises as the SDM documents, so nearly every case differs in
flags only (verdict partial); MXCSR flags are not compared. ECX/EDX (772615's
leftovers) and the x87 condition bits C0/C2/C3 (undefined after the trailing
FLD/FDIV) are not compared. The case words go to ninja_projected_depth.cases.bin.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_MXCSR,UC_X86_REG_FP0
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
fb=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
isnan=lambda w:(w&0x7fffffff)>0x7f800000
EDGES=[0x37800080,0x3780007f,0x37800081,0,0x80000000,0x7f800000,0xff800000,1,0x807fffff,fb(1.0),fb(-1.0),fb(65535.0),0x7f7fffff,fb(1e-30)]
NANS=[0x7fc00000,0xffc00001,0x7f800001,0xff91bb64]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    for slot,target in((0x389b784,0x774310),(0x389b7a0,0x774150),(0x389b7b0,0x772615)):m.mem_write(slot,struct.pack('<I',target))
    clean=m.context_save();identity=list(struct.unpack('<16I',image.read(0x389d318,64)))
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_projected_depth;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x78dbe0);cases=[];bad=[];flag_mismatches=0;outcomes={}
    for n in range(4000):
        c=[0]*41;c[0]=rng.choice([0x7f,0x27f,0x37f,0xc7f]);c[1]=rng.choice([0x1f80,0x1f80,0x3f80,0x5f80,0x7f80])
        c[2]=rng.choice(EDGES) if rng.random()<0.35 else fb(rng.uniform(-50,5000)) if rng.random()<0.9 else rng.getrandbits(32)
        c[3:9]=[rng.choice([0,0,10,rng.getrandbits(32)]),rng.choice([0,20,0xfffffff0]),rng.choice([640,1280,0,rng.getrandbits(32)]),rng.choice([480,0,rng.getrandbits(32)]),
            rng.choice([0,fb(0.25),fb(-1.0)]),rng.choice([fb(1.0),fb(0.75),fb(100.0)])]
        if rng.random()<0.7:
            f=rng.uniform(0.5,3);q=rng.uniform(0.5,1.2);proj=[fb(x) for x in(f,0,0,0, 0,f*rng.uniform(0.7,1.4),0,0, 0,0,q,1, 0,0,-q*rng.uniform(0.1,10),0)]
        else:proj=[fb(rng.uniform(-3,3)) if rng.random()<0.9 else rng.choice([0,0x80000000,fb(1e20),rng.getrandbits(32)]) for _ in range(16)]
        c[9:25]=proj;c[25:41]=identity if rng.random()<0.8 else [fb(rng.uniform(-2,2)) for _ in range(16)]
        for i in range(2,41):
            if isnan(c[i]):c[i]&=~0x40000000
        if rng.random()<0.05:c[rng.choice([2]+list(range(7,41)))]=rng.choice(NANS)
        cases.append(c)
    for c in cases:
        m.context_restore(clean)
        m.mem_write(0x3d12780,struct.pack('<6I',*c[3:9]));m.mem_write(0x3d129c0,struct.pack('<16I',*c[9:25]));m.mem_write(0x389d318,struct.pack('<16I',*c[25:41]))
        m.reg_write(UC_X86_REG_FPCW,c[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.reg_write(UC_X86_REG_MXCSR,c[1])
        m.mem_write(STACK,struct.pack('<2I',STOP,c[2]));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x78dbe0,STOP,count=100000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'case {c[:3]}: ended at {m.reg_read(UC_X86_REG_EIP):#x}')
        sw=m.reg_read(UC_X86_REG_FPSW);top=(sw>>11)&7;tag=m.reg_read(UC_X86_REG_FPTAG)
        mant,exp=m.reg_read(UC_X86_REG_FP0+top)
        pushed=top==7 and m.reg_read(UC_X86_REG_ESP)==STACK+8
        out=(ctypes.c_uint32*6)()
        if fn((ctypes.c_uint32*41)(*c),out)!=1:raise RuntimeError('Native rejected')
        native=(out[2]|out[3]<<32,out[4])
        fields=[]
        if not out[0]:fields.append('completed')
        elif native!=(mant,exp&0xffff):fields.append('st0')
        if not pushed:fields.append('stack')
        if m.reg_read(UC_X86_REG_EAX)!=STACK-0xc:fields.append('eax')
        if (out[5]&0x3f)!=(sw&0x3f):flag_mismatches+=1
        key=('near' if c[2] in(0x37800080,0x3780007f,0x37800081) else 'nan' if isnan(c[2]) else 'small' if struct.unpack('<f',struct.pack('<I',c[2]))[0]<struct.unpack('<f',struct.pack('<I',0x37800080))[0] else 'divided')
        outcomes[key]=outcomes.get(key,0)+1
        if fields:bad.append(dict(case=[hex(x) for x in c[:9]],fields=fields,source=[hex(mant),hex(exp),hex(sw),hex(tag)],native=[hex(x) for x in out]))
    a.output.with_suffix('.cases.bin').write_bytes(b''.join(struct.pack('<41I',*c) for c in cases)) # corpus: the probe's 41 input words
    root=pathlib.Path(__file__).resolve().parents[2]
    names=['src/engine/ninja_screen.hpp','src/guest/ninja_screen.cpp',
        'src/engine/ninja_projected_depth.hpp','src/engine/ninja_projected_depth.cpp',
        'src/math/d3dx_math.hpp','src/math/d3dx_math.cpp',
        'src/cpu/fidelity_float.hpp','src/cpu/x87_math.hpp','src/cpu/x87_math.cpp',
        'src/cpu/sse_math.hpp','src/cpu/sse_math.cpp',
        'tests/probes/native_ninja_projected_depth_probe.cpp',
        'tools/oracles/verify_original_ninja_projected_depth.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((root/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x78dbe0-0x78dc49':sha(image.read(0x78dbe0,0x69)),'0x772615-0x7727a2':sha(image.read(0x772615,0x18d)),'0x774150-0x774243':sha(image.read(0x774150,0xf3)),'0x774310-0x774456':sha(image.read(0x774310,0x146)),'0x770295':sha(image.read(0x770295,6))},
        cases=len(cases),source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,fp_flag_mismatches=flag_mismatches,exception_flags_match=flag_mismatches==0,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(bad),fp_flag_mismatches=flag_mismatches,outcomes=outcomes)))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
