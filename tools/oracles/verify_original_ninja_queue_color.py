"""Render-queue colour 402f40 (with its 77ff90 tail jump) against original instructions.

402f40 (cdecl, pointer to four floats) copies the colour into 3ab9864 word by
word, clamps each component to [-1, 1] with FLD/FCOMP against 7dcb10 (1.0) and
7dcb0c (-1.0), then jumps to 77ff90(&3ab9864), which copies the four words into
3d0f7f0. Cases: finite values around the clamp edges, signed zeros, infinities,
denormals, quiet and signalling NaNs (any number per case: nothing is computed,
so no NaN-propagation rule applies), under the four masked x87 control words
the game uses. Compared: the eight stored words (bitwise), and EAX/ECX/EDX and
the x87 condition bits after the call against the values the source leaves
(EAX = 3ab9864, ECX/EDX = the last two copied words, C3/C2/C0 = the last FCOMP).
x87 exception flags are compared separately: Unicorn 2.1.4 raises neither IE
for a NaN FCOMP operand / SNaN FLD nor DE for a denormal FLD m32, which the
native model raises as the Intel SDM documents, so those cases differ in flags
only (verdict partial). Unmasked-exception control words are not exercised.
The corpus (CW, the input words, the prior 3ab9864 words) goes to
ninja_queue_color.cases.bin for the guest override-equivalence test.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA
sha=lambda b:hashlib.sha256(b).hexdigest()
fb=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
INPUT=DATA+0x100
EDGES=[fb(1.0),fb(-1.0),0x3f800001,0x3f7fffff,0xbf800001,0xbf7fffff,0,0x80000000,0x7f800000,0xff800000,0x7fc00000,0xffc00000,
    0x7f800001,0xff91bb64,0x7fa00000,1,0x807fffff,0x7f7fffff,0xff7fffff,fb(0.5),fb(-0.5),fb(2.0),fb(-2.0)]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    clean=m.context_save()
    one,negative=(struct.unpack('<I',image.read(x,4))[0] for x in(0x7dcb10,0x7dcb0c))
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_queue_color;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x402f40);value=lambda:rng.choice(EDGES) if rng.random()<0.45 else fb(rng.uniform(-3,3)) if rng.random()<0.8 else rng.getrandbits(32)
    cases=[];bad=[];flag_mismatches=0;outcomes={}
    for cw in(0x7f,0x27f,0x37f,0xc7f):
        for n in range(1000):cases.append([cw]+[value() for _ in range(4)]+[rng.getrandbits(32) for _ in range(4)])
    for c in cases:
        m.context_restore(clean)
        m.mem_write(INPUT,struct.pack('<4I',*c[1:5]));m.mem_write(0x3ab9864,struct.pack('<4I',*c[5:9]));m.mem_write(0x3d0f7f0,b'\xa5'*16)
        m.reg_write(UC_X86_REG_FPCW,c[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<2I',STOP,INPUT));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x402f40,STOP,count=1000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'case {c}: ended at {m.reg_read(UC_X86_REG_EIP):#x}')
        src=list(struct.unpack('<4I',m.mem_read(0x3ab9864,16)))+list(struct.unpack('<4I',m.mem_read(0x3d0f7f0,16)))
        sw=m.reg_read(UC_X86_REG_FPSW)
        regs=[m.reg_read(UC_X86_REG_EAX),m.reg_read(UC_X86_REG_ECX),m.reg_read(UC_X86_REG_EDX)]
        out=(ctypes.c_uint32*11)()
        if fn((ctypes.c_uint32*11)(c[0],one,negative,*c[1:9]),out)!=1:raise RuntimeError('Native rejected')
        # The source's register/condition leftovers, from the native result (the bridge sets them the same way).
        last=c[4];expected_regs=[0x3ab9864,src[2],src[3]]
        fields=[]
        if not out[0]:fields.append('completed')
        if list(out[2:10])!=src:fields.append('stores')
        if regs!=expected_regs:fields.append('registers')
        if (out[10]&0x3f)!=(sw&0x3f):flag_mismatches+=1
        key=('>1' if src[3]==fb(1.0) and last!=fb(1.0) else '<-1' if src[3]==fb(-1.0) and last!=fb(-1.0) else 'kept')+f':cw{c[0]:#x}'
        outcomes[key]=outcomes.get(key,0)+1
        if fields:bad.append(dict(case=[hex(x) for x in c],fields=fields,source=[hex(x) for x in src+regs],native=[hex(x) for x in out]))
    a.output.with_suffix('.cases.bin').write_bytes(b''.join(struct.pack('<9I',*c) for c in cases)) # corpus: cw, input[4], prior 3ab9864[4]
    names=['src/engine/ninja_queue_consumer.hpp','src/engine/ninja_queue_consumer.cpp','src/cpu/x87_math.cpp','tests/probes/native_ninja_queue_color_probe.cpp','tools/oracles/verify_original_ninja_queue_color.py']
    root=pathlib.Path(__file__).resolve().parents[2]
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((root/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x402f40-0x40306f':sha(image.read(0x402f40,0x12f)),'0x77ff90-0x77ffbc':sha(image.read(0x77ff90,0x2c))},
        cases=len(cases),source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,
        fp_flag_mismatches=flag_mismatches,exception_flags_match=flag_mismatches==0,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(bad),fp_flag_mismatches=flag_mismatches,outcomes=outcomes)))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
