"""D3DX8 7757e0 (D3DXMatrixInverse, the SSE cofactor inverse) against its original instructions.

The call goes through the thunk 770d10 (JMP [389b7c0]) with dispatch slot 18 (389b7c0) written as
77350c leaves it on an SSE CPU without 3DNow!: both SSE tables (7735f0, 773700) put 7757e0 there.
77350c itself is not executed. Arguments: out, pDeterminant (NULL or not), pM; RET 0xc. The source
matrix sits at a 16-byte aligned or misaligned (+8) address (7757e0 loads it with movlps/movhps only,
so alignment must not matter). Compared: the return (out or NULL), the 16 stored floats, the stored
determinant, untouched sentinels on the NULL path, and MXCSR without its six exception flags.

RCPSS is Unicorn's (QEMU's IEEE single division), not a real CPU's estimate; the native probe uses the
same division model (sse_reciprocal_by_division), so this proves the transcription, not the estimate.
MXCSR control profiles vary the rounding mode only: Unicorn 2.1.4 ignores FZ and DAZ (see the sse_math
report), so they are not exercised here. Cases keep at most one NaN source: Unicorn applies QEMU's x87
NaN rule to SSE operands where hardware returns the first operand's NaN.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_MXCSR
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA
sha=lambda b:hashlib.sha256(b).hexdigest()
OUT=DATA+0x100;DET=DATA+0x180;MATRIX=DATA+0x200
fb=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
def matrices(rng):
    for n in range(8000):
        kind=n%10
        if kind<5:m=[fb(rng.uniform(-3,3)) for _ in range(16)]
        elif kind==5:  # affine transforms (the game's typical case)
            m=[fb(rng.uniform(-2,2)) for _ in range(12)]+[fb(rng.uniform(-500,500)) for _ in range(3)]+[fb(1.0)];m[3]=m[7]=m[11]=0
        elif kind==6:  # singular: a repeated row, or a zero row
            m=[fb(rng.uniform(-3,3)) for _ in range(16)];r=rng.randrange(4);s=(r+1+rng.randrange(3))%4
            m[4*s:4*s+4]=m[4*r:4*r+4] if rng.random()<0.7 else [0,0,0,0]
        elif kind==7:  # diagonal / scale, including tiny and huge scales
            m=[0]*16
            for i in range(4):m[5*i]=fb(rng.choice([1.0,-1.0,2.0,0.5,1e-20,1e20,rng.uniform(-10,10)]))
        elif kind==8:  # small integers (exact cofactors)
            m=[fb(float(rng.randrange(-4,5))) for _ in range(16)]
        else:          # edge values in one element
            m=[fb(rng.uniform(-3,3)) for _ in range(16)]
            m[rng.randrange(16)]=rng.choice([0,0x80000000,1,0x80000001,0x7f7fffff,0x7f800000,0xff800000,0x7fc00000,0xff91bb64])
        yield n,m
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    m.mem_write(0x389b7c0,struct.pack('<I',0x7757e0))
    clean=m.context_save()
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_d3dx_matrix_inverse;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x7757e0);bad=[];outcomes={};cases=0;flag_mismatches=0
    for n,mat in matrices(rng):
        mxcsr=rng.choice([0x1f80,0x1f80,0x3f80,0x5f80,0x7f80]);with_det=rng.random()<0.8;aligned=rng.random()<0.5
        out=(ctypes.c_uint32*19)()
        if fn((ctypes.c_uint32*17)(mxcsr,*mat),out)!=1:raise RuntimeError('Native rejected')
        m.context_restore(clean)
        at=MATRIX+(0 if aligned else 8)
        m.mem_write(OUT,b'\xa5'*64);m.mem_write(DET,b'\x5a'*4);m.mem_write(at,struct.pack('<16I',*mat))
        m.reg_write(UC_X86_REG_MXCSR,mxcsr)
        m.mem_write(STACK,struct.pack('<4I',STOP,OUT,DET if with_det else 0,at));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x770d10,STOP,count=10000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+0x10:raise RuntimeError(f'case {n}: 7757e0 did not return with RET 0xc')
        eax=m.reg_read(UC_X86_REG_EAX);stored=list(struct.unpack('<16I',m.mem_read(OUT,64)));det=struct.unpack('<I',m.mem_read(DET,4))[0]
        source=dict(returned=int(eax==OUT),null=int(eax==0),matrix=stored,det=det,mxcsr=m.reg_read(UC_X86_REG_MXCSR)&~0x3f)
        if out[0]:native=dict(returned=1,null=0,matrix=list(out[2:18]),det=out[1] if with_det else 0x5a5a5a5a,mxcsr=out[18]&~0x3f)
        else:native=dict(returned=0,null=1,matrix=[0xa5a5a5a5]*16,det=0x5a5a5a5a,mxcsr=out[18]&~0x3f)
        if (m.reg_read(UC_X86_REG_MXCSR)&0x3f)!=(out[18]&0x3f):flag_mismatches+=1
        cases+=1;key=('inverse' if source['returned'] else 'null' if source['null'] else 'other')+(':det' if with_det else '')+(':aligned' if aligned else ':misaligned')
        outcomes[key]=outcomes.get(key,0)+1
        fields=[k for k in ('returned','null','matrix','det','mxcsr') if source[k]!=native[k]]
        if fields:bad.append(dict(case=n,mxcsr=hex(mxcsr),input=[hex(x) for x in mat],fields=fields,source={k:(hex(source[k]) if isinstance(source[k],int) else [hex(x) for x in source[k]]) for k in fields},native={k:(hex(native[k]) if isinstance(native[k],int) else [hex(x) for x in native[k]]) for k in fields}))
    names=['src/math/d3dx_math.hpp','src/math/d3dx_math.cpp','src/cpu/sse_math.cpp','tests/probes/native_d3dx_matrix_inverse_probe.cpp','tools/oracles/verify_original_d3dx_matrix_inverse.py']
    root=pathlib.Path(__file__).resolve().parents[2]
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((root/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x770d10-0x770d16':sha(image.read(0x770d10,6)),'0x7757e0-0x775a99':sha(image.read(0x7757e0,0x2b9))},
        dispatch_seed={'0x389b7c0':'0x7757e0'},cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],
        values_match=not bad,flag_mismatches=flag_mismatches,passed=not bad and not flag_mismatches,
        limits=[__doc__.strip(),'flag_mismatches: MXCSR exception flags; Unicorn 2.1.4 raises no SSE exception flags, the native SoftFloat SSE model does. Hardware flags need the Intel capture (roadmap M4)'])
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcomes=outcomes,mismatches=len(bad),flag_mismatches=flag_mismatches)))
    for b in bad[:3]:print(json.dumps(b)[:600])
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
