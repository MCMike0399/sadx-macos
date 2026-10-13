"""Input 2.6.1 DreamPad::poll (+0x4080, incl. its convert_axes calls) against the pinned DLL's instructions.

convert_axes alone is covered by verify_original_input_mod_axes.py.

The pinned sadx-input-mod.dll (SHA c5ccdcc7...) runs at its preferred base. Only
the dynamically resolved SDL entry points are fixtures: the GetAxis pointer at
+0x1000e354 and GetButton pointer at +0x1000e358 return scripted values. The
keyboard Dreamcast pad globals (1000e8e8..1000e90e, 1000e744) and the demo byte
1000e708 are set per case. MXCSR is part of each case. This compares stored
record bytes, persistent E-held/half-press bytes, normalized magnitudes and
MXCSR; it is not physical controller or SDL backend equivalence. MXCSR differences
are expected: Unicorn 2.1.4 does not raise the SSE precision flag (0x20) on
inexact results, which hardware and the native SseContext do.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn import UC_PROT_ALL,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_MXCSR
from pe_image import PEImage
sha=lambda b:hashlib.sha256(b).hexdigest()
INPUT_SHA256='c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5'
CONSTANTS=[];STUB=0x20000000;AXIS=STUB;BUTTON=STUB+0x10;STOP=STUB+0x100;STACK=0x21000000+0xf000;PAD=0x22000000
def put(m,a,size,v):m.mem_write(a,(v&((1<<(8*size))-1)).to_bytes(size,'little'))
def get(m,a,size):return int.from_bytes(m.mem_read(a,size),'little')
class Source:
    def __init__(self,dll):
        m=self.m=unicorn.Uc(UC_ARCH_X86,UC_MODE_32)
        for s in dll.sections:
            base=dll.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
            m.mem_map(base,size,UC_PROT_ALL);m.mem_write(base,dll.slice(s['file_offset'],min(s['raw_size'],size)))
        for base in(STUB,0x21000000,PAD):m.mem_map(base,0x10000,UC_PROT_ALL)
        m.mem_write(AXIS,b'\xc3');m.mem_write(BUTTON,b'\xc3');put(m,0x1000e354,4,AXIS);put(m,0x1000e358,4,BUTTON)
        m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=STUB,end=STUB+0x20);self.clean=m.context_save()
    def code(self,m,address,size,user):
        esp=m.reg_read(UC_X86_REG_ESP);handle,index=get(m,esp+4,4),get(m,esp+8,4)
        if handle!=0x5a5a:raise RuntimeError('SDL handle not from the pad')
        if address==AXIS:m.reg_write(UC_X86_REG_EAX,self.c[10+index]&0xffff)
        else:m.reg_write(UC_X86_REG_EAX,(self.c[16]>>index)&1)
    def poll(self,c):
        m=self.m;m.context_restore(self.clean);self.c=c
        raw=bytearray(0x60)
        for w in range(13):struct.pack_into('<I',raw,4*w,c[28+w])
        struct.pack_into('<I',raw,0x2c,c[26]);struct.pack_into('<I',raw,0x34,0x5a5a);raw[0x3c]=c[0]&1;raw[0x3d]=c[9]&1;raw[0x3e]=c[8]&1
        struct.pack_into('<II',raw,0x44,0xdeadbeef,0xdeadbeef);raw[0x4c]=c[1]&1;struct.pack_into('<h',raw,0x4e,c[2]-(1<<16) if c[2]>>15 else c[2]);raw[0x50]=c[4]&1
        struct.pack_into('<h',raw,0x52,c[3]-(1<<16) if c[3]>>15 else c[3]);raw[0x54]=c[5]&1;raw[0x55]=c[6]&255;m.mem_write(PAD,bytes(raw))
        put(m,0x1000e8f4,4,c[17]);put(m,0x1000e904,2,c[18]);put(m,0x1000e906,2,c[19]);put(m,0x1000e908,2,c[20]);put(m,0x1000e90a,2,c[21]);put(m,0x1000e90c,2,c[22]);put(m,0x1000e90e,2,c[23])
        put(m,0x1000e8e8,4,c[24]);put(m,0x1000e744,4,c[25]);put(m,0x1000e708,1,c[7])
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_ECX,PAD);m.reg_write(UC_X86_REG_MXCSR,c[27])
        m.emu_start(0x10004080,STOP,count=5000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Source return ABI')
        out=[0]*24
        for w in range(13):out[1+w]=get(m,PAD+4*w,4)
        out[14]=m.mem_read(PAD+0x3c,1)[0];out[15]=m.mem_read(PAD+0x3d,1)[0];out[16]=m.mem_read(PAD+0x3e,1)[0];out[17]=get(m,PAD+0x44,4);out[18]=get(m,PAD+0x48,4);out[19]=m.reg_read(UC_X86_REG_MXCSR)
        return out
def poll_fixtures():
    rng=random.Random(0x10004080);cases=[]
    def base():
        c=[0]*48;c[0]=1;c[1]=1;c[2]=7849;c[3]=8689;c[4]=1;c[6]=30;c[27]=0x1f80;c[24]=0x3f800000;c[25]=0x3e800000;c[28:41]=[rng.getrandbits(32) for _ in range(13)];c[41:48]=CONSTANTS;return c
    for connected in(0,1):
        for allow in(0,1):
            for kbstick in((0,0),(127,0),(0,-89)):
                for trig in((0,0),(255,0),(0,255)):
                    for demo in(0,1):
                        for half in(0,1):
                            c=base();c[0]=connected;c[1]=allow;c[20],c[21]=kbstick;c[22],c[23]=kbstick[::-1];c[18],c[19]=trig;c[7]=demo;c[8]=half
                            c[10:16]=[20000,-12000,-32768,32767,128,32767];c[16]=0x7fff;c[17]=0x606;c[26]=0x8;cases.append(c)
    for _ in range(900):
        c=base();c[0]=rng.randrange(2);c[1]=rng.randrange(2);c[2]=rng.choice([0,7849,16000,32766]);c[3]=rng.choice([0,8689,1]);c[4]=rng.randrange(2);c[5]=rng.randrange(2);c[6]=rng.choice([0,30,255,128])
        c[7]=int(rng.random()<.1);c[8]=rng.randrange(2);c[9]=rng.randrange(2);c[10:16]=[rng.choice([0,1,-1,7848,7849,-7849,32767,-32768,rng.randrange(-32768,32768)])&0xffff for _ in range(6)]
        c[16]=rng.getrandbits(15);c[17]=rng.getrandbits(32);c[18]=rng.choice([0,255,rng.randrange(65536)]);c[19]=rng.choice([0,255]);c[20:24]=[rng.choice([0,127,-127,89,-1])&0xffff for _ in range(4)]
        c[26]=rng.getrandbits(32);c[27]=rng.choice([0x1f80,0x9fc0,0x7f80,0x1fa1]);cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('input_dll',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    dll=PEImage(a.input_dll)
    if dll.sha256!=INPUT_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned mod/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));poll=lib.sadx_native_input_mod_pad;poll.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];poll.restype=ctypes.c_int
    CONSTANTS.extend(struct.unpack('<I',dll.read(a_,4))[0] for a_ in(0x1000bb6c,0x1000bbb0,0x1000bba0,0x1000bba4,0x1000bb7c,0x1000bba8,0x1000bb78))
    source=Source(dll);bad=[];value_bad=0;mxcsr_bad=0
    pcases=poll_fixtures()
    for n,c in enumerate(pcases):
        e=source.poll(c);out=(ctypes.c_uint32*24)()
        if poll((ctypes.c_uint32*48)(*c),48,out,24)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        diff=[i for i in range(24) if i!=19 and got[i]!=e[i]]
        if diff:value_bad+=1;bad.append(dict(kind='poll',case=n,fields=diff,source=[hex(v) for v in e[:20]],native=[hex(v) for v in got[:20]]))
        if got[19]!=e[19]:mxcsr_bad+=1
    names=['src/input/input_mod_pad.hpp','src/input/input_mod_pad.cpp','tests/probes/native_input_mod_pad_probe.cpp','tools/oracles/verify_original_input_mod_pad.py']
    report=dict(input_dll_sha256=dll.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},
        poll_bytes_sha256=sha(dll.read(0x10004080,0x379)),convert_bytes_sha256=sha(dll.read(0x10004400,0x1e0)),poll_cases=len(pcases),poll_value_mismatches=value_bad,poll_mxcsr_mismatches=mxcsr_bad,
        mismatches=bad[:60],values_match=not value_bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps({k:report[k] for k in('poll_cases','poll_value_mismatches','poll_mxcsr_mismatches')}));return int(bool(value_bad))
if __name__=='__main__':raise SystemExit(main())
