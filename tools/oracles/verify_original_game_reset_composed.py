"""40bf40 with its ported leaves running as real source code, over fourteen randomized pages.

Real source: 40bf40, 422440, 4224b0 (PrintDebug 401000 skipped after checking
its constant format), 40cd10, 401410, 785330, 403e60, 4257c0, 4257e0, 424460,
410100 (audio-uninitialized return), 40cff0, 40d490(0), 4228f0, 421fb0, the
51f680 RETs and srand. Fixtures (recorded callee, site, argument or ECX; may
fail at call N): 4208e0, 40b1a0, 40b460, 411af0, 780fb0, the
3b291b4 callback (its `call *%eax` at 42291e is skipped and recorded), 504ed0, 411760, 411720. When the audio system is initialized
the source stops at 41010d, which native reports as its boundary. Pages are
compared byte for byte (stray source writes count as mismatches).
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
PAGES=[0x3abd000,0x892000,0x3ab9000,0x912000,0x3b29000,0x3b11000,0x3c5e000,0x3b0e000,0x3d08000,0x389d000,0x2bcb000,0x3c4e000,0x3a7c000,0x38f6000]
CALLBACK=CODE+0x800
FIXTURES={0x4208e0:'none',0x40b1a0:'none',0x40b460:'none',0x411af0:'none',0x780fb0:'arg',0x401000:'print',0x42291e:'callback',0x504ed0:'tail',0x411760:'ecx',0x411720:'ecx',0x41010d:'stop'}
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
def put(m,a,n,v):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x7f).machine
        for p in PAGES:
            try:m.mem_map(p,0x1000)
            except unicorn.UcError:pass
        m.mem_write(CALLBACK,b'\xc3');self.clean=m.context_save()
        for a in FIXTURES:m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=a,end=a)
    def code(self,m,address,size,user):
        kind=FIXTURES[address]
        if kind=='stop':self.stop=0x41010d;m.emu_stop();return
        if kind=='print': # PrintDebug: constant formats only; skip vsprintf
            esp=m.reg_read(UC_X86_REG_ESP);fmt=bytes(m.mem_read(get(m,esp+4,4),32)).split(b'\0')[0]
            if b'%' in fmt or fmt not in(b'ReleaseSetFile\n',b'ReleaseCamFile\n'):raise RuntimeError('unexpected PrintDebug format')
            m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,get(m,esp,4));return
        if kind=='callback': # call *%eax: record the callback identity and skip the call
            self.trace.append([m.reg_read(unicorn.x86_const.UC_X86_REG_EAX),0x42291e,0])
            if self.fail and len(self.trace)==self.fail:self.stop=0x42291e;m.emu_stop();return
            m.reg_write(UC_X86_REG_EIP,0x422920);return
        esp=m.reg_read(UC_X86_REG_ESP);ret=get(m,esp,4)
        site=0x421fc6 if kind=='tail' else ret-5
        arg=get(m,esp+4,4) if kind=='arg' else (m.reg_read(UC_X86_REG_ECX) if kind=='ecx' else 0)
        self.trace.append([address,site,arg])
        if self.fail and len(self.trace)==self.fail:self.stop=site;m.emu_stop();return
        m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
    def run(self,pages,fail):
        m=self.m;m.context_restore(self.clean);self.trace=[];self.stop=None;self.fail=fail
        for i,p in enumerate(PAGES):m.mem_write(p,pages[i*4096:(i+1)*4096])
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40bf40,STOP,count=20000)
        if self.stop is None and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        out=[int(self.stop is None),self.stop or 0,len(self.trace)]
        for e in self.trace[:60]:out+=e
        return b''.join(bytes(m.mem_read(p,4096)) for p in PAGES),out+[0]*(200-len(out))
def fixtures():
    rng=random.Random(0x40bf41);extra_rng=random.Random(0x40bf42);cases=[]
    for n in range(300):
        pages=bytearray(rng.getrandbits(8) for _ in range(12*4096))
        struct.pack_into('<I',pages,4*4096+0x1b4,rng.choice([0,0x592370,0x401234]))       # 3b291b4
        struct.pack_into('<I',pages,7*4096+0xf2c,0 if n%6 else 1)                       # 3b0ef2c audio initialized
        struct.pack_into('<I',pages,6*4096+0x8b8,rng.choice([0,1]))                     # 3c5e8b8
        for off in(0xf94,0xfa0):struct.pack_into('<I',pages,off,rng.choice([0,rng.getrandbits(32)]))
        for i in range(12):struct.pack_into('<I',pages,0xf28+4*i,rng.choice([0,0,rng.getrandbits(32)]))   # 3abdf28..3abdf54
        struct.pack_into('<I',pages,8*4096+0x428,rng.choice([0,2,3,0xffffffff]))        # 3d08428
        # Preserve every existing byte and failure index. Independent RNG adds the
        # two configuration pages that the native probe always reads at construction.
        pages.extend(extra_rng.getrandbits(8) for _ in range(2*4096))
        cases.append((bytes(pages),0 if n%4 else rng.randrange(1,30)))
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_reset_composed
    fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.c_uint32,ctypes.POINTER(ctypes.c_ubyte),ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);bad=[];outcomes={}
    for n,(pages,fail) in enumerate(fixtures()):
        ep,eo=source.run(pages,fail);op=(ctypes.c_ubyte*len(pages))();oo=(ctypes.c_uint32*200)()
        if fn((ctypes.c_ubyte*len(pages)).from_buffer_copy(pages),len(pages),fail,op,oo,200)!=1:raise RuntimeError('Native rejected')
        key=f"completed{eo[0]}:fixtures{eo[2]}";outcomes[key]=outcomes.get(key,0)+1
        diff=[i for i in range(len(pages)) if op[i]!=ep[i]]
        if diff or list(oo)!=eo:bad.append(dict(case=n,fail=fail,bytes=[hex(PAGES[i//4096]+i%4096) for i in diff[:12]],fields=[i for i,(x,y) in enumerate(zip(oo,eo)) if x!=y][:8],source=eo[:8],native=list(oo)[:8]))
    names=['src/game/game_reset.cpp','src/game/game_reset_leaves.hpp','src/game/game_reset_leaves.cpp','tests/probes/native_game_reset_composed_probe.cpp','tools/oracles/verify_original_game_reset_composed.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={k:sha((pathlib.Path(__file__).resolve().parents[2]/k).read_bytes()) for k in names},
        cases=300,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],pages_trace_boundaries_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=300,mismatches=len(bad),outcomes=outcomes)));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
