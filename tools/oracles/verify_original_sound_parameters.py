"""Original game sound volume/pan/pitch conversion; final DirectSound calls are receipts."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
CALLEES={0x4101a0:1,0x410260:2,0x410200:3,0x4102c0:4}
class Source:
    def __init__(self,image):
        self.m=Original(image,0x7f).machine
        self.m.mem_map(0x3b29000,4096);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def hook(self,m,a,size,user):
        if a not in CALLEES:return
        sp=m.reg_read(UC_X86_REG_ESP);kind=CALLEES[a]
        args=[self.word(sp+4),self.word(sp+8),0,0]
        if kind==4:args[2:]=[self.word(sp+12),self.word(sp+16)]
        self.receipts.append([kind,*args]);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
    def run(self,v):
        self.receipts=[];self.m.context_restore(self.initial);self.m.reg_write(UC_X86_REG_FPCW,v[4]);self.m.reg_write(UC_X86_REG_FPSW,0)
        self.m.mem_write(0x3b29304+v[2]*56,struct.pack('<I',v[3]));self.m.mem_write(STACK-0x100,struct.pack('<3I',STOP,v[1],v[2]));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
        self.m.emu_start((0x423c20,0x423c80,0x423cc0)[v[0]],STOP,count=100000);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=[len(self.receipts),self.m.reg_read(UC_X86_REG_FPSW)&63,0,0]+[0]*12
        for n,r in enumerate(self.receipts):out[4+n*5:9+n*5]=r
        return out
def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1000);p.add_argument('--seed',type=int,default=42320);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image)
    lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_sound_parameter_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2
    vectors=[]
    for control in (0x7f,0x47f,0x87f,0xc7f,0x27f,0x67f,0xa7f,0xe7f,0x37f,0x77f,0xb7f,0xf7f):
     for operation in range(3):
      for value in (0,1,-1,-100,-101,-99,100,126,127,128,-126,-127,-128,65535,0x7fffffff,-0x80000000,0x66666666,-14634,14634,65534):
       for flags in (0,16384):vectors.append([operation,value&0xffffffff,17,flags,control])
    rng=random.Random(a.seed)
    for _ in range(a.cases):vectors.append([rng.randrange(3),rng.getrandbits(32),rng.randrange(36),rng.getrandbits(32),rng.choice([0x7f,0x27f,0x37f,0x87f])])
    bad=[];flag_mismatches=0
    for n,v in enumerate(vectors):
        expected=source.run(v);out=(ctypes.c_uint32*16)();probe((ctypes.c_uint32*5)(*v),out)
        if expected[1]!=out[1]:flag_mismatches+=1
        diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if i!=1 and x!=y]
        if diff:
            bad.append({'case':n,'input':v,'diff':diff})
            if len(bad)>=8:break
    paths=['src/game/sound_effects.hpp','src/game/sound_effects.cpp','tests/probes/native_sound_effects_probe.cpp']
    report={'passed':not bad and not flag_mismatches,'values_match':not bad,'cases':len(vectors),'mismatches':len(bad),'fp_flag_mismatches':flag_mismatches,'first_mismatches':bad,'source_sha256':image.sha256,'source_ranges':{f'0x{x:x}-0x{y:x}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in [(0x423c20,0x423c79),(0x423c80,0x423cc0),(0x423cc0,0x423d0b)]},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'native_sources':{n:hashlib.sha256(Path(n).read_bytes()).hexdigest() for n in paths},'limits':['Final volume/pan/frequency/3D-position callees are traced resource receipts; no hardware output is claimed. Low-six x87 exception flags are separately reported.']}
    Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
