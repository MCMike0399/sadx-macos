"""InitGameLoop 40c030 against original instructions, over six randomized source pages.

Real source code runs for 40c030, 420810, 40f350, 424c20 (local part), 40f3a0,
40a8a0 and 781000->789930. Fixtures: 408630 (queue arena alloc/reset), 411e30,
410510 (audio init), 40bf40 (state reset), 4253c0->40d0d0 (audio objects) and the
78c5a0 RET, recorded as owner calls; one call can fail. Pages 3abd000, 3b0e000,
3b29000, 900000, 903000 and 3d08000 are compared byte for byte, so source writes
outside the native model also count as mismatches. Vanilla InitControllers only;
the Input mod's 40f451 hook is covered by input_mod_init_raw_controllers tests.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
PAGES=[0x3abd000,0x3b0e000,0x3b29000,0x900000,0x903000,0x3d08000]
OWNERS={0x408630:0,0x78c5a0:1,0x411e30:2,0x410510:0,0x40bf40:0,0x4253c0:0}
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x7f).machine
        for p in PAGES:
            try:m.mem_map(p,0x1000)
            except unicorn.UcError:pass
        self.clean=m.context_save()
        for a in OWNERS:m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=a,end=a)
    def code(self,m,address,size,user):
        esp=m.reg_read(UC_X86_REG_ESP);ret=get(m,esp,4);site=ret-5 if address!=0x408630 else 0x420810
        args=[get(m,esp+4+4*i,4) for i in range(OWNERS[address])]+[0,0]
        self.trace.append([address,site,args[0],args[1]])
        if self.fail and len(self.trace)==self.fail:self.stop=(3,site);m.emu_stop();return
        m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
    def run(self,pages,fail):
        m=self.m;m.context_restore(self.clean);self.trace=[];self.stop=None;self.fail=fail
        for i,p in enumerate(PAGES):m.mem_write(p,pages[i*4096:(i+1)*4096])
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40c030,STOP,count=20000)
        if not self.stop and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        f,b=self.stop or(0,0);out=[f,b,int(self.stop is None),len(self.trace)]
        for e in self.trace[:12]:out+=e
        return b''.join(bytes(m.mem_read(p,4096)) for p in PAGES),out+[0]*(64-len(out))
def fixtures():
    rng=random.Random(0x40c030);cases=[]
    for n in range(400):
        pages=bytearray(rng.getrandbits(8) for _ in range(6*4096)) if n else bytearray(6*4096)
        if n%3==0:pages[3*4096+0xe88]=0   # 900e88==0: block copy path
        if n%5==0:                        # full callback list
            for i in range(32):struct.pack_into('<I',pages,5*4096+0x4b0+4*i,i+1)
        elif n%5==1:struct.pack_into('<I',pages,5*4096+0x4b0+4*rng.randrange(32),0)
        pages[2*4096+0xce8]=rng.choice([0,1,2,3,4,5,0xff])
        cases.append((bytes(pages),0 if n%7 else rng.randrange(1,7)))
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_startup
    fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_ubyte),ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);bad=[];outcomes={}
    for n,(pages,fail) in enumerate(fixtures()):
        epages,eout=source.run(pages,fail);opages=(ctypes.c_ubyte*len(pages))();out=(ctypes.c_uint32*64)()
        if fn((ctypes.c_ubyte*len(pages)).from_buffer_copy(pages),len(pages),(ctypes.c_uint32*4)(fail,0,0,0),4,opages,out,64)!=1:raise RuntimeError('Native rejected')
        key=f"failure{eout[0]}:owners{eout[3]}";outcomes[key]=outcomes.get(key,0)+1
        diff=[i for i in range(len(pages)) if opages[i]!=epages[i]]
        if diff or list(out)!=eout:bad.append(dict(case=n,fail=fail,byte_diffs=[hex(PAGES[i//4096]+i%4096) for i in diff[:12]],trace_fields=[i for i,(x,y) in enumerate(zip(out,eout)) if x!=y][:10]))
    names=['src/game/game_startup.hpp','src/guest/game_startup.cpp','src/engine/game_controller.cpp','tests/probes/native_game_startup_probe.cpp','tools/oracles/verify_original_game_startup.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={k:sha((pathlib.Path(__file__).resolve().parents[2]/k).read_bytes()) for k in names},
        original_ranges_sha256={h:sha(image.read(int(h,16),n)) for h,n in(('40c030',0x57),('40f350',0x4a),('424c20',0x56),('40a8a0',0x50),('789930',0x27),('420810',0xa))},
        cases=len(fixtures()),source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],pages_order_boundaries_match=not bad,limits=__doc__)
    # Replay the input page at the independent 40f350 entry too, including cases
    # whose composed startup fixture stops before reaching it.
    corpus=a.output.with_suffix('.cases.bin')
    corpus.write_bytes(b''.join(pages[4096:8192] for pages,_ in fixtures()))
    report['corpus']=dict(path=str(corpus),cases=len(fixtures()),bytes_per_case=4096,entry='40f350')
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=report['cases'],mismatches=len(bad),outcomes=outcomes)));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
