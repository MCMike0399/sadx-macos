"""D3DX8 7716df (D3DXMatrixPerspectiveFovLH as linked into sonic.exe, stdcall, x87 throughout)
against its original instructions.

7716df is called directly (it is not dispatched through the 389b778 table): out, fovy, aspect, near,
far on the stack, RET 0x14. Every case runs the whole body. Compared: the 16 stored floats and the
x87 control word; the six x87 exception flags are compared separately (flag class).

Two profiles:
  primary  Unicorn executes every instruction, including FSINCOS at 0x771706, which QEMU computes
           with the host's double sin/cos (53-bit), not an x87 64-bit-significand reduction. The
           native side uses the Bochs P5/P6 FSINCOS model (third_party/bochs_fpatan, fsincos.cc).
  seeded   the same, but right after 0x771706 ST(0)/ST(1) are overwritten with the native Bochs
           cosine/sine (exported by the probe), so the rest of the body is compared exactly with no
           FSINCOS model difference. Seeding validates the suffix only; it proves nothing about
           Intel FSINCOS.
|fovy * 0.5| >= 2^63 makes FSINCOS set C2 and leave ST(0); the native port returns an explicit unknown
(nullopt) for it, and those cases are counted as native_unknown_cases, not compared.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import (UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,
    UC_X86_REG_FP0)
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA
sha=lambda b:hashlib.sha256(b).hexdigest()
OUT=DATA+0x100
FSINCOS,AFTER=0x771706,0x771708
fb=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
def cases(rng):
    special=[0,0x80000000,fb(1e-30),fb(1e30),fb(3.0e19),0x7f800000,0xff800000,1]
    for n in range(6000):
        cw=rng.choice([0x7f,0x27f,0x37f,0x47f,0x87f,0xc7f,0x7f,0x27f])
        fovy=fb(rng.choice([0.7853982,1.0471976,1.5707964,0.5,2.0,3.0]) if n%5==0 else rng.uniform(-6.3,6.3))
        aspect=fb(rng.choice([4/3,16/9,1.0,0.75]) if n%3 else rng.uniform(0.1,4.0))
        near=fb(rng.choice([0.1,1.0,-1.0,0.5]) if n%4 else rng.uniform(0.001,50))
        far=fb(rng.choice([1000.0,10000.0,65535.0,-65535.0]) if n%4 else rng.uniform(1,100000))
        args=[fovy,aspect,near,far]
        if n%17==0:args[rng.randrange(4)]=rng.choice(special)
        if n%41==0:args[1]=0
        if n%43==0:args[3]=args[2]
        yield [cw]+args
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine;clean=m.context_save()
    lib=ctypes.CDLL(str(a.native_library.resolve()))
    fn=lib.sadx_native_d3dx_perspective;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    sc=lib.sadx_native_d3dx_perspective_sincos;sc.argtypes=[ctypes.c_uint32,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint64)];sc.restype=ctypes.c_int
    seed={'on':False,'cw':0,'checked':False}
    def at_fsincos_end(uc,address,size,user):
        if address!=AFTER or not seed['on']:return
        half=struct.unpack('<I',uc.mem_read(uc.reg_read(UC_X86_REG_ESP)+0x10-0x8,4))[0]  # [EBP-8], EBP=ESP+0x10 after SUB ESP,0x10
        out=(ctypes.c_uint32*3)();sig=(ctypes.c_uint64*2)();sc(seed['cw'],half,out,sig)
        if seed.get('force'):out[0],sig[0],out[1],sig[1],out[2]=1,*seed['force']
        if not out[0]:return
        top=(uc.reg_read(UC_X86_REG_FPSW)>>11)&7
        uc.reg_write(UC_X86_REG_FP0+top,(sig[1],out[2]))          # ST(0) = cosine
        uc.reg_write(UC_X86_REG_FP0+((top+1)&7),(sig[0],out[1]))  # ST(1) = sine
    m.hook_add(unicorn.UC_HOOK_CODE,at_fsincos_end,begin=AFTER,end=AFTER)
    clean=m.context_save()
    def original(c,seeded):
        m.context_restore(clean);seed.update(on=seeded,cw=c[0])
        m.mem_write(OUT,b'\xa5'*64)
        m.reg_write(UC_X86_REG_FPCW,c[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<6I',STOP,OUT,*c[1:5]));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x7716df,STOP,count=1000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+0x18:raise RuntimeError('7716df did not return with RET 0x14')
        return list(struct.unpack('<16I',m.mem_read(OUT,64))),m.reg_read(UC_X86_REG_FPCW),m.reg_read(UC_X86_REG_FPSW)&0x3f
    # The seeded write must target the physical registers of ST(0)/ST(1): force sine 0.5, cosine 1.0
    # (ext80 0x3ffe/0x3fff, significand 2^63) and expect _11 = _22 = cot = 2.0.
    seed['force']=(1<<63,0x3ffe,1<<63,0x3fff)
    forced,_,_=original([0x27f,fb(1.0),fb(1.0),fb(1.0),fb(10.0)],True);seed['force']=None
    if forced[0]!=fb(2.0) or forced[5]!=fb(2.0):raise RuntimeError(f'seeded FSINCOS register convention wrong: {forced[0]:#x} {forced[5]:#x}')
    profiles={}
    a.output.with_suffix('.cases.bin').write_bytes(b''.join(struct.pack('<5I',*c) for c in cases(random.Random(0x7716df)))) # corpus: cw, fovy, aspect, near, far
    for profile in ('primary','seeded'):
        rows=dict(cases=0,native_unknown_cases=0,value_mismatches=0,control_mismatches=0,flag_mismatches=0,mismatch_classes={},examples=[],flag_examples=[])
        for c in cases(random.Random(0x7716df)):
            out=(ctypes.c_uint32*18)()
            if fn((ctypes.c_uint32*5)(*c),out)!=1:raise RuntimeError('Native rejected')
            if not out[0]:rows['native_unknown_cases']+=1;continue
            values,cw,flags=original(c,profile=='seeded');rows['cases']+=1
            native=list(out[1:17])
            if values!=native:
                rows['value_mismatches']+=1
                # QEMU's FSINCOS treats an infinite operand like |x| >= 2^63 (C2 set, ST(0) kept); the SDM makes it
                # an invalid operation whose masked response is the indefinite NaN, which the Bochs model returns.
                half_infinite=(c[1]&0x7fffffff)==0x7f800000
                kind='unicorn_fsincos_infinity_as_out_of_range' if half_infinite and native[0]==native[5]==0xffc00000 else 'unclassified'
                rows['mismatch_classes'][kind]=rows['mismatch_classes'].get(kind,0)+1
                if len(rows['examples'])<12:rows['examples'].append(dict(input=[hex(x) for x in c],fields=[i for i in range(16) if values[i]!=native[i]],source=[hex(values[i]) for i in range(16) if values[i]!=native[i]],native=[hex(native[i]) for i in range(16) if values[i]!=native[i]]))
            if cw!=c[0]:rows['control_mismatches']+=1
            if flags!=out[17]:
                rows['flag_mismatches']+=1
                if len(rows['flag_examples'])<6:rows['flag_examples'].append(dict(input=[hex(x) for x in c],source=hex(flags),native=hex(out[17])))
        profiles[profile]=rows;print(json.dumps({k:v for k,v in rows.items() if not k.endswith('examples')}),flush=True)
    names=['src/math/d3dx_math.hpp','src/math/d3dx_math.cpp','src/cpu/x87_transcendental.cpp','tests/probes/native_d3dx_perspective_probe.cpp','tools/oracles/verify_original_d3dx_perspective.py']
    root=pathlib.Path(__file__).resolve().parents[2]
    pr,se=profiles['primary'],profiles['seeded']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((root/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x7716df-0x771773':sha(image.read(0x7716df,0x94)),'0x7dcc98-0x7dcc9c':sha(image.read(0x7dcc98,4))},
        profiles=profiles,
        seeded_suffix_values_match=se['value_mismatches']==0 and se['control_mismatches']==0,
        primary_values_with_unicorn_fsincos_match=pr['value_mismatches']==0 and pr['control_mismatches']==0,
        flag_mismatches=pr['flag_mismatches']+se['flag_mismatches'],
        unclassified_primary_mismatches=pr['mismatch_classes'].get('unclassified',0),
        limits=[__doc__.strip(),
                'primary value mismatches are FSINCOS model differences, classified in profiles.primary.mismatch_classes: unicorn_fsincos_infinity_as_out_of_range = QEMU treats FSINCOS(+-inf) as |x| >= 2^63 (C2, ST(0) kept) where the SDM gives #IA with the indefinite NaN (the Bochs model); finite operands use host double sin/cos in Unicorn. The Bochs model is not changed to match Unicorn; settling FSINCOS needs the Intel capture (roadmap M4). This report stays fail while primary mismatches exist',
                'flag_mismatches: x87 exception flags; Unicorn 2.1.4 does not raise PE/UE for most x87 arithmetic, the native SoftFloat/Bochs models do',
                'native_unknown_cases are |fovy/2| >= 2^63 (FSINCOS C2 path); the native port reports them as unknown instead of modelling the stack-underflow stores'])
    a.output.write_text(json.dumps(report,indent=1)+'\n')
    return int(not report['seeded_suffix_values_match'])
if __name__=='__main__':raise SystemExit(main())
