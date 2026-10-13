"""Projection range 403180 + 7815e0 against original instructions.

The D3D8 device is a COM fixture: GetTransform (+0x98) writes the case matrix,
SetTransform (+0x94) records the written matrix. Each case runs with a random
cache and again with the cache equal to the first run's stored values (the
unchanged path). x87 status flags are reported separately.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
DEVICE=DATA+0x100;VTABLE=DATA+0x400;GET=CODE+0x300;SET=CODE+0x310
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
def put(m,a,n,v):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x7f).machine
        for p in(0x3d0f000,0x900000,0x3aad000,0x3d12000):
            try:m.mem_map(p,0x1000)
            except unicorn.UcError:pass
        m.mem_write(GET,b'\x31\xc0\xc2\x0c\x00');m.mem_write(SET,b'\x31\xc0\xc2\x0c\x00');put(m,VTABLE+0x98,4,GET);put(m,VTABLE+0x94,4,SET);put(m,DEVICE,4,VTABLE)
        self.clean=m.context_save()
        for a in(GET,SET):m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=a,end=a)
    def code(self,m,address,size,user):
        esp=m.reg_read(UC_X86_REG_ESP);state,ptr=get(m,esp+8,4),get(m,esp+12,4)
        if address==GET:self.gets+=1;m.mem_write(ptr,struct.pack('<16I',*self.matrix))
        else:self.written=list(struct.unpack('<16I',m.mem_read(ptr,64))) if state==3 else None
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.gets=0;self.written=None;self.matrix=c[7:23]
        put(m,0x3d128b0,4,DEVICE);put(m,0x3aad0c0,4,c[2]);put(m,0x3aad0c4,4,c[3])
        for a in(0x3d0f808,0x3d0f968,0x90082c,0x900830):put(m,a,4,0xdeadbeef)
        m.reg_write(UC_X86_REG_FPCW,c[4]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<III',STOP,c[0],c[1]));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x403180,STOP,count=5000)
        out=[1,get(m,0x3d0f808,4),get(m,0x3d0f968,4),get(m,0x90082c,4),get(m,0x900830,4),get(m,0x3aad0c0,4),get(m,0x3aad0c4,4),self.gets,int(self.written is not None)]
        out+=(self.written or [0]*16)+[m.reg_read(UC_X86_REG_FPSW)&0x3f]
        return out+[0]*(40-len(out))
def f32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    k=[struct.unpack('<I',image.read(0x7dcb0c,4))[0],struct.unpack('<I',image.read(0x7dce50,4))[0]]
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_projection_range;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);rng=random.Random(0x403180);bad=[];flags=0;n=0;specials=[f32(-1.0),f32(-3000.0),f32(-0.5),f32(0.0),0x80000000,f32(-65535.0),f32(-70000.0),f32(5.0),0x7fc00000,0xff800000,0x7f800000,f32(-1.0000001)]
    for i in range(1500):
        c=[0]*23;c[0]=rng.choice(specials+[f32(rng.uniform(-80000,10))]);c[1]=rng.choice(specials+[f32(rng.uniform(-80000,10))]);c[2]=rng.getrandbits(32);c[3]=rng.getrandbits(32);c[4]=rng.choice([0x7f,0x27f,0x37f,0xc7f]);c[5:7]=k
        c[7:23]=[f32(rng.uniform(-3,3)) for _ in range(16)]
        for second in(0,1):
            e=source.run(c);out=(ctypes.c_uint32*40)()
            if fn((ctypes.c_uint32*23)(*c),23,out,40)!=1:raise RuntimeError('Native rejected')
            got=list(out);diff=[j for j in range(25) if got[j]!=e[j]];n+=1
            if diff:bad.append(dict(case=i,second=second,input=[hex(x) for x in c[:5]],fields=diff[:8],source=[hex(e[j]) for j in diff[:8]],native=[hex(got[j]) for j in diff[:8]]))
            if got[25]!=e[25]:flags+=1
            c[2],c[3]=e[5],e[6]
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),original_range_sha256=sha(image.read(0x403180,0xc6)+image.read(0x7815e0,0x8c)),cases=n,mismatch_count=len(bad),fp_flag_mismatches=flags,mismatches=bad[:20],stores_transform_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=n,mismatches=len(bad),fp_flag_mismatches=flags)));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
