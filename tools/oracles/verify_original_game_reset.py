"""Game-state reset 40bf40 against original instructions.

Every callee except the 51f680 RET and srand 6443b5 is an owner fixture that
records (callee, site, first argument) and may fail at call N. Compared: call
order/sites/arguments, 3abdf80, word 3b29d08, the conditional free-and-zero of
3b290d0 and 3b0e330, and the CRT seed 2bcbdf8.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
OWNERS=[0x40cd10,0x401410,0x785330,0x403e60,0x4257c0,0x4257e0,0x424460,0x410100,0x40cff0,0x4208e0,0x40d490,0x40b1a0,0x40b460,0x4228f0,0x421fb0,0x780fb0,0x411af0]
ARGS={0x401410,0x785330,0x410100,0x40cff0,0x40d490,0x4228f0,0x780fb0}
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
def put(m,a,n,v):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x7f).machine
        for p in(0x3abd000,0x3b29000,0x3b0e000,0x2bcb000):m.mem_map(p,0x1000)
        self.clean=m.context_save()
        for a in OWNERS:m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=a,end=a)
    def code(self,m,address,size,user):
        esp=m.reg_read(UC_X86_REG_ESP);ret=get(m,esp,4);self.trace.append([address,ret-5,get(m,esp+4,4) if address in ARGS else 0])
        if self.c[5] and len(self.trace)==self.c[5]:self.stop=(3,ret-5);m.emu_stop();return
        m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.c=c;self.trace=[];self.stop=None
        put(m,0x3abdf80,4,c[0]);put(m,0x3b29d08,2,c[1]);put(m,0x3b290d0,4,c[2]);put(m,0x3b0e330,4,c[3]);put(m,0x2bcbdf8,4,c[4])
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40bf40,STOP,count=5000)
        if not self.stop and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        f,b=self.stop or(0,0);out=[f,b,int(self.stop is None),get(m,0x3abdf80,4),get(m,0x3b29d08,2),get(m,0x3b290d0,4),get(m,0x3b0e330,4),get(m,0x2bcbdf8,4),len(self.trace)]
        for e in self.trace[:36]:out+=e
        return out+[0]*(128-len(out))
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_reset;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);rng=random.Random(0x40bf40);bad=[];cases=[]
    for n in range(300):cases.append([rng.getrandbits(32),rng.getrandbits(16),rng.choice([0,rng.getrandbits(32)]),rng.choice([0,rng.getrandbits(32)]),rng.getrandbits(32),0 if n%3 else rng.randrange(1,32)])
    for n,c in enumerate(cases):
        e=source.run(c);out=(ctypes.c_uint32*128)()
        if fn((ctypes.c_uint32*6)(*c),6,out,128)!=1:raise RuntimeError('Native rejected')
        if list(out)!=e:bad.append(dict(case=n,input=[hex(x) for x in c],fields=[i for i,(x,y) in enumerate(zip(out,e)) if x!=y][:10]))
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),original_range_sha256=sha(image.read(0x40bf40,0xe3)),cases=len(cases),mismatch_count=len(bad),mismatches=bad[:20],order_stores_seed_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(bad))));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
