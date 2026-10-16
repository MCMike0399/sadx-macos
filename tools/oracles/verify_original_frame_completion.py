"""411d00 frame completion with the actual Speedrun 2.0 hook, against original instructions.

Updated profile: the pinned sadx-speedrun-mod.dll (SHA f3fc68a3...) is mapped at
its preferred base 0x10000000 and 411d1c is patched with `jmp 0x100011d0`, the
hook observed live in the Windows reference snapshot. The DLL stub calls +0x11c0
(EAX=max(0,[3b11188]-1)) and jumps through its file-initialized pointer to
411da3. Original profile: 411d1c lag recovery is unported, so the source run
stops there and native must report that required boundary.
The 781010..781060 thunks (renderer/limiter owners) are explicit fixtures; the
781030 fixture may rewrite 3b28114 to exercise the reread at 411e01.
"""
import argparse,ctypes,hashlib,json,pathlib,struct
import unicorn
from unicorn import UC_PROT_ALL
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
SPEEDRUN_SHA256='f3fc68a3058fb726142547018da860b170f137248e54b82712b52527e7b74802'
OWNERS=[0x781010,0x781020,0x781030,0x781040,0x781050,0x781060]
def put(m,a,size,v):m.mem_write(a,(v&((1<<(8*size))-1)).to_bytes(size,'little'))
def get(m,a,size):return int.from_bytes(m.mem_read(a,size),'little')
class Source:
    def __init__(self,image,dll,patched):
        self.m=m=Original(image,0x7f).machine
        for page in(0x892000,0x3abd000,0x3b11000,0x3b28000):m.mem_map(page,0x1000)
        if patched:
            for s in dll.sections:
                base=dll.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
                m.mem_map(base,size,UC_PROT_ALL);m.mem_write(base,dll.slice(s['file_offset'],min(s['raw_size'],size)))
            m.mem_protect(0x411000,0x1000,UC_PROT_ALL);m.mem_write(0x411d1c,b'\xe9'+struct.pack('<i',0x100011d0-(0x411d1c+5)))
        self.patched=patched;self.prev=0;self.clean=m.context_save();m.hook_add(unicorn.UC_HOOK_CODE,self.code)
    def code(self,m,address,size,user):
        if address in OWNERS:
            esp=m.reg_read(UC_X86_REG_ESP);ret=get(m,esp,4);site=self.prev if ret==STOP else ret-5
            self.trace.append([address,site]);c=self.case
            if c[7] and len(self.trace)==c[7]:self.failure=(4,site,0x123);m.emu_stop();return
            if address==0x781030 and c[6]:put(m,0x3b28114,4,c[6]-1)
            m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret);return
        if not self.patched and address==0x411d1c:self.failure=(2,0x411d1c,0);m.emu_stop();return
        self.prev=address
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.case=c;self.trace=[];self.failure=None
        put(m,0x3abdf68,1,c[1]);put(m,0x89295c,4,c[2]);put(m,0x3b11188,4,c[3]);put(m,0x3b11198,4,c[4]);put(m,0x3b28114,4,c[5]);put(m,0x3b11180,4,0x5a5a5a5a);put(m,0x3b11194,4,0x6b6b6b6b)
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x411d00,STOP,count=5000)
        if not self.failure and(m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+4):raise RuntimeError('Source return ABI')
        f,b,n=self.failure or(0,0,0);out=[0]*40;out[0:4]=[f,b,n,int(self.failure is None)]
        out[4:10]=[get(m,0x3b11180,4),get(m,0x3b11194,4),get(m,0x3b11198,4),get(m,0x3b28114,4),get(m,0x3abdf68,1),len(self.trace)]
        for i,e in enumerate(self.trace[:8]):out[10+2*i:12+2*i]=e
        return out
def fixtures():
    cases=[]
    for policy in(0,1):
        for resync in(0,1,5):
            for factor in(0,1,2,0x80000000):
                for multiplier in(0xffffffff,0,1,2,3,0x80000000,0x80000001,0x7fffffff):
                    for active in(0,1,7):
                        for mode in(0,1,2,3,4):
                            c=[0]*16;c[0:6]=[policy,resync,factor,multiplier,active,mode];cases.append(c)
    for mode in(0,3,2):
        for script in(0,1,2,6):
            for fail in range(0,7):c=[0]*16;c[0:8]=[0,0,1,2,1,mode,script,fail];cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('speedrun_dll',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    image=PEImage(a.exe);dll=PEImage(a.speedrun_dll)
    if image.sha256!=SOURCE_SHA256 or dll.sha256!=SPEEDRUN_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/mod/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_frame_completion;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    sources={0:Source(image,dll,True),1:Source(image,dll,False)};cases=fixtures();bad=[];outcomes={}
    for number,c in enumerate(cases):
        expected=sources[c[0]].run(c);out=(ctypes.c_uint32*40)()
        if fn((ctypes.c_uint32*16)(*c),16,out,40)!=1:raise RuntimeError('Native fixture rejected')
        key=f"policy{c[0]}:failure{expected[0]}:owners{expected[9]}";outcomes[key]=outcomes.get(key,0)+1
        if list(out)!=expected:bad.append(dict(case=number,input=c[:8],fields=[i for i,(x,y) in enumerate(zip(out,expected)) if x!=y],source=expected,native=list(out)))
    blob=b''.join(struct.pack('<16I',*c) for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/game/game_loop.hpp','src/guest/game_loop.cpp','tests/probes/native_frame_completion_probe.cpp','tools/oracles/verify_original_frame_completion.py']
    report=dict(source_sha256=image.sha256,speedrun_dll_sha256=dll.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},
        original_range_sha256=sha(image.read(0x411d00,0x118)),hook_bytes={'100011c0':dll.read(0x100011c0,0x20).hex(),'10010018':dll.read(0x10010018,4).hex()},cases=len(cases),case_sha256=sha(blob),source_outcomes=outcomes,
        mismatch_count=len(bad),mismatches=bad[:30],stores_order_boundaries_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(bad),outcomes=outcomes)));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
