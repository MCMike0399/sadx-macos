"""Private original FileSelect initialization caller with ordered resource receipts."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESI,UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
WORK=DATA+0x1000
GLOBALS=[(0x3b2a2f9,1,1),(0x3b290d8,2,1),(0x3b290e0,3,1),(0x3b2919c,4,4),(0x3c5e8a4,5,2),(0x3b29198,6,4),(0x3c5e8b4,7,1),(0x10d77ad,8,1)]
CALLS={0x506010:1,0x505990:2,0x505a40:3,0x505050:4,0x421de0:5,0x420a00:6,0x420a50:7,0x42d770:8}
NATIVE_PATHS=['src/game/file_select.cpp', 'src/game/file_select.hpp', 'tests/probes/native_file_select_probe.cpp']
def native_sources():
    root=Path(__file__).resolve().parents[2]
    return {name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in NATIVE_PATHS}

class Source:
    def __init__(self,image):
        self.m=Original(image,0x7f).machine
        for page in {a&~4095 for a,_,_ in GLOBALS}:
            if not any(x<=page<=y for x,y,_ in self.m.mem_regions()):self.m.mem_map(page,4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
    def write(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
    def snapshot(self):
        out=[self.word(WORK+0x24)]+[self.word(a,n) for a,_,n in GLOBALS]
        out += [self.word(WORK+x) for x in (0x20,0x1c,0x28,0x2c,0x18)]
        out += [self.word(WORK+0x30,2),self.word(WORK+0x32,1),self.word(WORK+0x34)]
        return out
    def hook(self,m,a,size,user):
        if a not in CALLS:return
        sp=m.reg_read(UC_X86_REG_ESP);kind=CALLS[a];arg=self.word(sp+4) if kind in (1,2,3) else 0
        if kind in (6,7):assert self.word(sp+4)==0x90e494;arg=1
        self.trace.append([kind,arg]+self.snapshot()+[0])
        if len(self.trace)==self.v[14]:self.stopped=True;m.emu_stop();return
        m.reg_write(UC_X86_REG_EAX,self.v[13] if kind==4 else 0);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
    def run(self,v):
        self.v=v;self.trace=[];self.stopped=False;self.m.context_restore(self.initial)
        for a,i,n in GLOBALS:self.write(a,v[i],n)
        for offset,index in ((0x24,0),(0x20,9),(0x1c,10),(0x28,11),(0x2c,12),(0x18,15),(0x30,16),(0x32,17),(0x34,18)):
            self.write(WORK+offset,v[index],2 if offset==0x30 else 1 if offset==0x32 else 4)
        self.m.mem_write(STACK-0x100,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.reg_write(UC_X86_REG_ESI,WORK);self.m.emu_start(0x505320,STOP,timeout=0)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=self.snapshot()+[0]*239;out[24]=len(self.trace);out[25]=int(self.stopped)
        for n,row in enumerate(self.trace):out[32+n*20:52+n*20]=row
        return out

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1500);p.add_argument('--seed',type=int,default=50532);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image)
    lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_file_select_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2
    vectors=[];base=[17,18,19,0,0xabadbeef,27,1,29,30,31,32,33,34,0,0,0x7fa12345,0xabcd,127,0x80000000]
    for ready in (0,1,2,255):
     for selected in (0,1,127,255):
      for count in (0,1,2,99,255,256):
       for stop in range(9):v=base.copy();v[3]=ready;v[2]=selected;v[13]=count;v[14]=stop;vectors.append(v)
    rng=random.Random(a.seed)
    for _ in range(a.cases):
        v=[rng.getrandbits(32) for _ in base];v[13]=rng.randrange(256);v[14]=rng.randrange(9);vectors.append(v)
    bad=[]
    for n,v in enumerate(vectors):
        expected=source.run(v);out=(ctypes.c_uint32*256)();probe((ctypes.c_uint32*19)(*v),out)
        if list(out)!=expected:
            bad.append({'case':n,'input':v,'diff':[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]})
            if len(bad)>=8:break
    result={'passed':not bad,'cases':len(vectors),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_ranges':{'0x505320-0x5053e2':hashlib.sha256(image.read(0x505320,0xc2)).hexdigest()},'source_sha256':image.sha256,'native_sources':native_sources(),'source_functions':{f'0x{x:x}-0x{x+n:x}':hashlib.sha256(image.read(x,n)).hexdigest() for x,n in [(5264160, 194), (4378800, 38)]},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'limits':['Initialization caller only; texture/music/voice/save/font callees are exact ordered receipts. Save discovery and real load backends are not claimed by this caller judge.']}
    Path(a.output).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
