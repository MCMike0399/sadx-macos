"""Private original menu-voice queue/task constructors and updater; allocations/destruction receipts."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK,OTHER,WORK=DATA+0x1000,DATA+0x2000,DATA+0x3000
NATIVE_PATHS=['src/game/menu_voice.cpp', 'src/game/menu_voice.hpp', 'tests/probes/native_menu_voice_probe.cpp']
def native_sources():
    root=Path(__file__).resolve().parents[2]
    return {name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in NATIVE_PATHS}

class Source:
    def __init__(self,image):
        self.image=image;self.m=Original(image,0x7f).machine
        for p in (0x3c5e000,0x912000,0x3b2a000):self.m.mem_map(p,4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def snapshot(self):
        pointer=self.word(TASK+0x2c);global_task=self.word(0x3c5e8cc)
        return [0 if not global_task else 1 if global_task==TASK else 2,self.word(0x912694),int(bool(pointer)),self.word(WORK) if pointer else 0,self.word(WORK+4) if pointer else 0]
    def hook(self,m,a,size,user):
        if a not in (0x40b860,0x40b2d0,0x40b570):return
        sp=m.reg_read(UC_X86_REG_ESP);arg=lambda n:self.word(sp+4*n)
        if a==0x40b860:assert [arg(n) for n in (1,2,3)]==[0,4,0x5059b0];row=[1,0,4,1];result=0 if self.v[3] else TASK
        elif a==0x40b2d0:assert [arg(n) for n in (1,2)]==[1,8];row=[2,1,8,0];result=WORK;self.m.mem_write(WORK,bytes(8))
        else:row=[3,1 if arg(1)==TASK else 2,0,0];result=0
        self.trace.append(row+self.snapshot()+[0]*3)
        if len(self.trace)==self.v[10]:self.stopped=True;m.emu_stop();return
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_EIP,arg(0));m.reg_write(UC_X86_REG_ESP,sp+4)
    def run(self,v):
        self.v=v;self.trace=[];self.stopped=False;self.m.context_restore(self.initial);self.m.mem_write(TASK,bytes(0x3000));self.write(0x3c5e8cc,0 if not v[2] else TASK if v[2]==1 else OTHER);self.write(0x912694,v[7]);self.write(0x912690,v[8]);self.write(0x3c5e8e0,v[5]);self.m.mem_write(0x3b2a2f2,bytes([v[6]&255]))
        if v[0]==3:self.write(TASK+0x2c,WORK);self.write(WORK,v[1]);self.write(WORK+4,v[4])
        parameter=v[9] if v[0]==2 else TASK if v[0]==3 else v[1]
        self.m.mem_write(STACK-0x100,struct.pack('<2I',STOP,parameter));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.emu_start((0x425710,0x5059f0,0x505a40,0x5059b0)[v[0]],STOP,timeout=0)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=self.snapshot()+[0]*59;out[7]=len(self.trace);out[8]=0 if self.stopped or v[0] in (0,3) else self.m.reg_read(UC_X86_REG_EAX);out[9]=int(self.stopped)
        for n,row in enumerate(self.trace):out[16+n*12:28+n*12]=row
        return out

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1000);p.add_argument('--seed',type=int,default=50590);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image)
    vectors=[]
    for op in range(4):
     for canonical in (0,1,2):
      for fail in (0,1):
       for elapsed in (0,1,2,0xffffffff,0x7fffffff,0x80000000):
        for ready in (0,1,2):
         for enabled in (0,1,0xffffffff):
          for stop in (0,1,2):vectors.append([op,123,canonical,fail,elapsed,ready,0,456,enabled,1,stop,1])
    rng=random.Random(a.seed)
    for _ in range(a.cases):
        op=rng.randrange(4);v=[op,rng.getrandbits(32),rng.randrange(3),rng.randrange(2),rng.getrandbits(32),rng.getrandbits(32),0,rng.getrandbits(32),rng.getrandbits(32),rng.randrange(9),rng.randrange(3),0];v[11]=image.unpack('<i',0x7ee930+v[9]*4)[0];vectors.append(v)
    lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_menu_voice_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;bad=[]
    for n,v in enumerate(vectors):
        expected=source.run(v);out=(ctypes.c_uint32*64)();probe((ctypes.c_uint32*12)(*v),out)
        if list(out)!=expected:
            bad.append({'case':n,'input':v,'diff':[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]})
            if len(bad)>=8:break
    report={'passed':not bad,'cases':len(vectors),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'native_sources':native_sources(),'source_functions':{f'0x{x:x}-0x{x+n:x}':hashlib.sha256(image.read(x,n)).hexdigest() for x,n in [(4347664, 16), (5265840, 62), (5265904, 75), (5265984, 48)]},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'limits':['Actual queue/constructor/map/updater instructions execute; task creation/CAlloc/destruction are ordered receipts. CAlloc null faults and unsupported imported request indices remain explicit native boundaries.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
