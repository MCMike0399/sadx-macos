"""Original one-shot request, channel search and connected title-confirmation wrapper.

Compares all36 full_SEcallbuf records, cursor, alternating pan and return. No audio
backend is mocked successful: these entrypoints enqueue only and do not play PCM.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
QUEUE=0x3b292f8
class Source:
    def __init__(self,image):
        self.m=Original(image,0x7f).machine
        for page in (0x3b29000,0x910000):
            if not any(a<=page<=b for a,b,_ in self.m.mem_regions()):self.m.mem_map(page,4096)
        self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def run(self,v):
        self.m.context_restore(self.initial);self.write(0x3b29b7c,v[5]);self.write(0x910290,v[6])
        self.m.mem_write(QUEUE,struct.pack('<504I',*v[7:]));sp=STACK-0x100
        self.m.mem_write(sp,struct.pack('<5I',STOP,*v[1:5]));self.m.reg_write(UC_X86_REG_ESP,sp)
        self.m.reg_write(UC_X86_REG_EDI,v[3]);entry=(0x423d10,0x423d70,0x505810)[v[0]]
        self.m.emu_start(entry,STOP,count=10000);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
        result=0 if v[0]==2 else self.m.reg_read(UC_X86_REG_EAX)
        return [result,self.word(0x3b29b7c),self.word(0x910290)]+list(struct.unpack('<504I',self.m.mem_read(QUEUE,2016)))
def cases(seed,count):
    rng=random.Random(seed);out=[]
    record=[1,1,101,7,123,8,9,10,11,0x7fa12345,0x80000000,0x7f800000,333,444]
    for operation in (0,1,2):
     for cursor in (0,1,33,34,35):
      for tone in (2,7,0xffffffff):
       for slot in (0,1,33,34,35,None):
        for priority in (0,1,0xffffffff,0x80000000):
         v=[operation,tone,222,priority,0xffffffff,cursor,8]+record*36
         if slot is not None:v[7+slot*14+1]=0
         out.append(v)
    for _ in range(count):
        v=[rng.randrange(3),rng.choice([2,7,rng.getrandbits(32)]),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(36),rng.getrandbits(32)]
        for channel in range(36):
            row=[rng.getrandbits(32) for _ in range(14)];row[1]=rng.choice([0,1,120,0xffffffff]);v+=row
        out.append(v)
    return out
def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',default='build/oracles/sound_effects.json');p.add_argument('--seed',type=int,default=42370);p.add_argument('--cases',type=int,default=1500);a=p.parse_args()
    image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image)
    lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_sound_effects_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2
    bad=[];vectors=cases(a.seed,a.cases)
    for index,v in enumerate(vectors):
        expected=source.run(v);output=(ctypes.c_uint32*507)();probe((ctypes.c_uint32*511)(*v),output)
        if list(output)!=expected:
            bad.append({'case':index,'input':v,'diff':[[n,x,y] for n,(x,y) in enumerate(zip(expected,output)) if x!=y]})
            if len(bad)>=8:break
    paths=['src/game/sound_effects.hpp','src/game/sound_effects.cpp','src/game/sound_initial_state.hpp','src/game/sound_bank.hpp','tests/probes/native_sound_effects_probe.cpp']
    report={'passed':not bad,'cases':len(vectors),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'source_ranges':{f'0x{start:x}-0x{end:x}':hashlib.sha256(image.read(start,end-start)).hexdigest() for start,end in [(0x423d10,0x423d67),(0x423d70,0x423e19),(0x505810,0x505821)]},'native_sources':{n:hashlib.sha256(Path(n).read_bytes()).hexdigest() for n in paths},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'limits':['Cursor inputs0..35 are covered. Out-of-range source-array access is an explicit native boundary. Queueing only; sound-server/sample/PCM playback is outside these entrypoints.']}
    Path(a.output).parent.mkdir(parents=True,exist_ok=True);Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
