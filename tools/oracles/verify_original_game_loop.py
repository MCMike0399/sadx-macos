"""GameLoop 40cad0 local orchestration against original instructions.

Every callee (40c030, 51f680, 401410, 77f090, 40f460, 781010/781060, 420830,
407ab0, 77ed00, 40c090, 4250d0, 411b60, 403180, 4086f0, 40be40, 40e2c0, 4326f0,
40d4d0, 40eab0, 42f990, 40d3c0, 420840, 411d00, 421fd0, 40ef30, 40ef20, 7853c0,
6fbf10, 410630, 420f40, 420ef0 and the 77fc50 tail) is an explicit owner fixture:
the 77f090 fixture writes scripted per-frame globals standing in for the real
producers, 4250d0 may set the exit flag, 411b60 may replace the near fog range,
40ef30/40ef20 return scripted values. From the 4th frame the fixture forces focus and
an exit key so every case terminates. This compares the local call order, call
arguments, global loads/stores and failure boundaries only; it does not show that
any owner subsystem, Windows message pump or frame pacing is implemented.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
CALLEES=[0x40c030,0x51f680,0x401410,0x77f090,0x40f460,0x781010,0x781060,0x420830,0x407ab0,0x77ed00,0x40c090,0x4250d0,0x411b60,0x403180,0x4086f0,0x40be40,0x40e2c0,0x4326f0,
         0x40d4d0,0x40eab0,0x42f990,0x40d3c0,0x420840,0x411d00,0x421fd0,0x40ef30,0x40ef20,0x7853c0,0x6fbf10,0x410630,0x420f40,0x420ef0,0x77fc50]
ARGS={0x401410:1,0x77ed00:1,0x411b60:1,0x403180:2,0x40e2c0:1,0x40ef20:1}
GLOBALS=[(0x3abdf7c,4),(0x909eb4,1),(0x3abdcfe,1),(0x3abdca0,4),(0x3abdca4,4),(0x3abdc70,4),(0x3abdc74,4),(0x3b291ad,1),(0x3b291b2,1),(0x3b291a4,4),(0x3abdf76,1),(0x3abdf79,1),
         (0x3abdf80,4),(0x3b28114,4),(0x3abdf58,4),(0x3abdf5c,4),(0x3b22de4,2),(0x3c85804,4),(0x8929d4,4),(0x3abdcf8,4),(0x3b11198,4)]
def put(m,a,size,v):m.mem_write(a,(v&((1<<(8*size))-1)).to_bytes(size,'little'))
def get(m,a,size):return int.from_bytes(m.mem_read(a,size),'little')
class Source:
    def __init__(self,image):
        self.o=Original(image,0x7f);m=self.m=self.o.machine
        for page in(0x8929000,0x909000,0x3abd000,0x3b11000,0x3b22000,0x3b28000,0x3b29000,0x3c85000):
            try:m.mem_map(page,0x1000)
            except unicorn.UcError:pass
        m.mem_map(0x892000,0x1000)
        m.hook_add(unicorn.UC_HOOK_CODE,self.code);self.clean=m.context_save();self.case=None
    def code(self,m,address,size,user):
        if self.case is None or address not in self.callee_set:return
        c=self.case;esp=m.reg_read(UC_X86_REG_ESP);ret=get(m,esp,4)
        site=0x40cce5 if address==0x77fc50 else ret-5
        args=[get(m,esp+4+4*i,4) for i in range(ARGS.get(address,0))]+[0,0]
        if address==0x51f680 and ret==0x40ccb9:args=[get(m,esp+4,4),0]
        self.trace.append([address,site,args[0],args[1]])
        if c[22] and len(self.trace)==c[22]:self.failure=(3,site,0x123);m.emu_stop();return
        index=min(self.iteration-1,3) if self.iteration else 0;w=c[24+index*10:34+index*10];eax=0
        if address==0x77f090:
            self.iteration+=1;w=c[24+min(self.iteration-1,3)*10:34+min(self.iteration-1,3)*10]
            put(m,0x8929d4,4,1 if self.iteration>=4 else w[0]&1);put(m,0x909eb4,1,w[0]>>1&3);put(m,0x3abdcfe,1,w[0]>>3&1);put(m,0x3b291ad,1,w[0]>>4&1);put(m,0x3abdf79,1,w[0]>>5&1);put(m,0x3abdf80,4,w[0]>>6&1)
            put(m,0x3b291a4,4,w[1]);put(m,0x3b28114,4,w[2]);put(m,0x3b22de4,2,w[3])
        elif address==0x4250d0 and w[6]:put(m,0x3abdf7c,4,1)
        elif address==0x411b60:put(m,0x3abdca0,4,w[7])
        elif address==0x40ef30:eax=0x44 if self.iteration>=4 else w[4]
        elif address==0x40ef20:eax=1 if self.iteration>=4 else w[5]
        if address==0x77fc50:self.finished=True;m.emu_stop();return
        if self.iteration>8:self.failure=(99,0,0);m.emu_stop();return
        m.reg_write(UC_X86_REG_EAX,eax);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.case=c;self.trace=[];self.iteration=0;self.failure=None;self.finished=False;self.callee_set=set(CALLEES)
        for i,(a,size) in enumerate(GLOBALS):put(m,a,size,c[i])
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40cad0,STOP,count=200000)
        failure,boundary,nested=self.failure or(0,0,0)
        out=[0]*512;out[0:5]=[failure,boundary,nested,int(self.finished and not self.failure),self.iteration]
        values=[get(m,0x3abdf7c,4),get(m,0x909eb4,1),get(m,0x3abdcfe,1),get(m,0x3abdca0,4),get(m,0x3b291ad,1),get(m,0x3b291b2,1),get(m,0x3b291a4,4),get(m,0x3abdf76,1),get(m,0x3abdf79,1),
                get(m,0x3b28114,4),get(m,0x3abdf58,4),get(m,0x3abdf5c,4),get(m,0x3abdcf8,4),get(m,0x3b11198,4)]
        out[5:19]=values;out[19]=min(len(self.trace),120)
        for i,entry in enumerate(self.trace[:120]):out[20+4*i:24+4*i]=entry
        return out
def fixtures():
    rng=random.Random(0x40cad0);cases=[]
    def base():
        c=[0]*64;c[3:7]=[0x11,0x22,0x33,0x44];c[8]=0xfe;c[14]=0xfffffffe;c[15]=7;c[16]=4
        for i in range(4):c[24+i*10:34+i*10]=[1,0,0,4,0,0,0,0x55,0,0]
        return c
    cases.append(base())
    c=base();c[0]=1;cases.append(c)
    for flags in range(128):
        for phase in(0,1,0xffffffff,2):
            c=base();c[24]=flags;c[25]=phase;c[26]=rng.choice([0,1,2,3]);c[27]=rng.choice([0x10,4,0]);c[28]=rng.choice([0,0x44,4,0x40]);c[29]=rng.choice([0,1]);c[30]=rng.randrange(2);cases.append(c)
    for keys in(0,4,0x40,0x44,0xffffffff):
        for key in(0,1,0xffffffff):c=base();c[28]=keys;c[29]=key;cases.append(c)
    for stage in(0x10,0x11,0xffff,0x10010):c=base();c[16]=stage;c[27]=stage;cases.append(c)
    for counter in(0,0xff):c=base();c[8]=counter;c[24]=0x11;cases.append(c)
    for frames in(0xffffffff,0x7fffffff):c=base();c[14]=frames;c[15]=frames;cases.append(c)
    for shutdown in(0,1):c=base();c[0]=1;c[17]=shutdown;cases.append(c)
    for _ in range(80):
        c=base()
        for i in range(4):c[24+i*10:34+i*10]=[rng.randrange(128),rng.choice([0,1,0xffffffff,2]),rng.randrange(4),rng.choice([0x10,3,0x10010]),rng.choice([0,0x44,0x40]),rng.randrange(2),int(rng.random()<.15),rng.getrandbits(32),0,0]
        cases.append(c)
    base_count=len(cases)
    for n in range(1,70):
        c=list(cases[2+(n*7)%(base_count-2)]);c[22]=n;cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_loop;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);cases=fixtures();bad=[];outcomes={}
    for number,case in enumerate(cases):
        expected=source.run(case)
        if expected[0]==99:raise RuntimeError(f'Fixture {number} did not terminate')
        out=(ctypes.c_uint32*512)()
        if fn((ctypes.c_uint32*64)(*case),64,out,512)!=1:raise RuntimeError('Native fixture rejected')
        key=f"failure{expected[0]}:iterations{expected[4]}:finished{expected[3]}";outcomes[key]=outcomes.get(key,0)+1
        if list(out)!=expected:bad.append(dict(case=number,fields=[i for i,(x,y) in enumerate(zip(out,expected)) if x!=y][:20],source=expected[:60],native=list(out)[:60]))
    blob=b''.join(struct.pack('<64I',*c) for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/game/game_loop.hpp','src/game/game_loop.cpp','tests/probes/native_game_loop_probe.cpp','tools/oracles/verify_original_game_loop.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},original_range_sha256=sha(image.read(0x40cad0,0x21a)),
        cases=len(cases),case_sha256=sha(blob),source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:30],order_arguments_stores_boundaries_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(bad),outcomes=len(outcomes))));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
