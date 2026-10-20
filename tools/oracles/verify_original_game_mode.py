"""Game-mode state machine 40c090 against original instructions.

Every callee is an explicit owner fixture with a scripted EAX (callees whose
result the source ignores return 0x5a5a5a5a); up to two scripted callees
rewrite a source global when called, to exercise the source's rereads after
calls; one call can fail. The level pointer at 3b0ef4c targets a mapped word or
is null (source faults at the MOVSX). This compares call order, call sites,
arguments, stores and failure boundaries; it does not show any owner subsystem
(menus, saves, file loads, cutscenes, audio) is implemented.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA
sha=lambda b:hashlib.sha256(b).hexdigest()
RETURNING=[0x421f70,0x421f80,0x42e890,0x413c00,0x42f240,0x416610,0x4173b0,0x415af0,0x42f020,0x42caa0,0x5033b0,0x4144e0,0x412d20,0x642640,0x6428d0,0x413bc0,0x641c60,0x640f20,0x590650]
ARGC={0x4221e0:2,0x411e30:2,0x402f10:3,0x414570:2}
for a in(0x4212e0,0x414470,0x413380,0x4133e0,0x503380,0x5032e0,0x5033b0,0x77ed40,0x4228f0,0x780fb0,0x42f020,0x412d20,0x40ef70,0x785330,0x40d490,0x642640,0x6428d0,0x642170,0x641c60,0x641a80,0x640f20,0x4147f0,0x412c00):ARGC[a]=1
TARGETS={0:(0x3abdc7c,4),1:(0x3abdcfd,1),2:(0x3b28114,4),4:(0x3abdcf8,4)}
LEVEL=DATA+0x800
GLOBALS=[(0x3abdc7c,4),(0x3b28114,4),(0x3b291a4,4),(0x3b291ad,1),(0x3abdcf8,4),(0x3abdf74,1),(0x3abdcfc,1),(0x3abdcfd,1),(0x3b18db4,1),(0x3b2a2fa,1),(0x3b22e1e,1),(0x3abdf78,1),(0x3b2c45c,4),(0x3b290e4,4)]
OUT_GLOBALS=[(0x3abdc7c,4),(0x3b28114,4),(0x3b291a4,4),(0x3b291ad,1),(0x3abdcf8,4),(0x3abdf74,1),(0x3abdcfc,1),(0x3abdcfd,1),(0x3b18db4,1),(0x3b2c45c,4)]
def put(m,a,n,v):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
def callees(image):
    import re,subprocess
    code=image.read(0x40c090,0x92d);found=set()
    for i in range(len(code)-4):
        if code[i]==0xe8:
            target=(0x40c090+i+5+struct.unpack('<i',code[i+1:i+5])[0])&0xffffffff
            if 0x401000<=target<0x7db000:found.add(target)
    return found
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x7f).machine
        for page in(0x3abd000,0x3b0e000,0x3b18000,0x3b22000,0x3b28000,0x3b29000,0x3b2a000,0x3b2c000):m.mem_map(page,0x1000)
        self.clean=m.context_save();self.callees=sorted(set(ARGC)|set(RETURNING)|{0x415000,0x42cea0,0x4f88a0,0x42c7b0,0x42cc70,0x42f590,0x40fdc0,0x40ef40,0x40bf30,0x40bf40,0x42f080,0x42ca10,0x420f40,0x420ef0,0x406f00,0x5034a0,0x5033f0,0x424c80,0x42e780,0x42c630,0x427ee0,0x427ec0,0x42d770,0x412ac0,0x42c820,0x42c730,0x785330,0x424320,0x590660,0x42c4f0,0x426050})
        for a in self.callees:m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=a,end=a)
    def code(self,m,address,size,user):
        c=self.c;esp=m.reg_read(UC_X86_REG_ESP);ret=get(m,esp,4);site=ret-5
        args=[get(m,esp+4+4*i,4) for i in range(ARGC.get(address,0))]+[0,0,0]
        self.trace.append([address,site,args[0],args[1],args[2]])
        if c[41] and len(self.trace)==c[41]:self.stop=(3,site);m.emu_stop();return
        for k in range(2):
            callee,target,value=c[35+3*k:38+3*k]
            if callee!=address:continue
            if target in TARGETS:put(m,*TARGETS[target],value)
            elif target==3:put(m,LEVEL+4,2,value)
        eax=c[16+RETURNING.index(address)] if address in RETURNING else 0x5a5a5a5a
        m.reg_write(UC_X86_REG_EAX,eax);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.c=c;self.trace=[];self.stop=None
        for i,(a,n) in enumerate(GLOBALS):put(m,a,n,c[i])
        put(m,LEVEL+4,2,c[14]);put(m,0x3b0ef4c,4,0 if c[15] else LEVEL)
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x40c090,STOP,count=20000)
        except unicorn.UcError as e:
            if e.errno!=unicorn.UC_ERR_READ_UNMAPPED:raise
            self.stop=(1,m.reg_read(UC_X86_REG_EIP))
        if not self.stop and(m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+4 or m.reg_read(UC_X86_REG_EAX)!=0):raise RuntimeError('Source return ABI')
        out=[0]*256;f,b=self.stop or(0,0);out[0:3]=[f,b,int(self.stop is None)]
        out[3:13]=[get(m,a,n) for a,n in OUT_GLOBALS];out[13]=min(len(self.trace),48)
        for i,e in enumerate(self.trace[:48]):out[14+5*i:19+5*i]=e
        return out
def fixtures():
    rng=random.Random(0x40c090);cases=[]
    interesting=[0,1,2,3,4,5,6,7,10,0xfffffffe,0xffffffff,100,101,102,103,104,105,106,107,108,0x10001,0xffff,0xfffe,0x6d,0x6c,0x80000000]
    for _ in range(24000):
        c=[0]*48
        c[0]=rng.choice(list(range(0x1c))*8+[0x7fffffff,0xffffffff,0x80000000])
        c[1]=rng.choice([0,1,2,3]);c[2]=rng.choice([0,0,1,0xffffffff]);c[3]=rng.randrange(256);c[4]=rng.choice([0,0,0x11223344])
        for i in(5,6,7,8,9,10,11):c[i]=rng.choice([0,1,2,rng.randrange(256)])
        c[12]=rng.choice([0,3,7]);c[13]=rng.choice([0,1]);c[14]=rng.choice([0,5,0xffff,0x8000]);c[15]=int(rng.random()<0.08)
        for i in range(19):c[16+i]=rng.choice(interesting)
        for k in range(2):
            if rng.random()<0.3:c[35+3*k:38+3*k]=[rng.choice([0x77ed40,0x641a80,0x642170,0x42c630,0x4228f0,0x4221e0,0x424c80,0x5034a0,0x40bf40,0x42d770,0x427ee0,0x780fb0,0x424320]),rng.choice([0,1,2,3,4]),rng.choice([0,1,2,10,0x15,0x7fffffff])]
        if rng.random()<0.1:c[41]=rng.randrange(1,40)
        cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_mode;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);missing=callees(image)-set(source.callees)
    if missing:raise RuntimeError('Unhooked source callees: '+','.join(hex(x) for x in sorted(missing)))
    cases=fixtures();bad=[];outcomes={};paths=set()
    for n,c in enumerate(cases):
        e=source.run(c);out=(ctypes.c_uint32*256)()
        if fn((ctypes.c_uint32*48)(*c),48,out,256)!=1:raise RuntimeError('Native rejected')
        key=f"mode{c[0]:#x}:failure{e[0]}";outcomes[key]=outcomes.get(key,0)+1;paths.add(tuple(e[15+5*i] for i in range(e[13])))
        if list(out)!=e:bad.append(dict(case=n,input=c[:42],fields=[i for i,(x,y) in enumerate(zip(out,e)) if x!=y][:12],source=[hex(v) for v in e[:30]],native=[hex(v) for v in list(out)[:30]]))
    blob=b''.join(struct.pack('<48I',*c) for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/game/game_mode.hpp','src/game/game_mode_services.hpp','src/game/game_mode.cpp','src/game/game_mode_internal.hpp','src/game/game_mode_native.cpp','src/guest/game_mode_reference.hpp','src/guest/game_mode_reference.cpp','tests/probes/native_game_mode_probe.cpp','tools/oracles/verify_original_game_mode.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={k:sha((pathlib.Path(__file__).resolve().parents[2]/k).read_bytes()) for k in names},original_range_sha256=sha(image.read(0x40c090,0x92d)),
        cases=len(cases),distinct_call_site_paths=len(paths),case_sha256=sha(blob),source_outcomes=dict(sorted(outcomes.items())),mismatch_count=len(bad),mismatches=bad[:30],order_arguments_stores_boundaries_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),paths=len(paths),mismatches=len(bad))));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
