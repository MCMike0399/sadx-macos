"""Full original title input caller; named music/input/transition callees are receipts.

Every receipt includes the current title/shared stores. Optional stopped prefixes check
publication order, including per-task music_started before request and submode after sound.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ESI,UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
WORK,TASK,SEQUENCE=DATA+0x1000,DATA+0x2000,DATA+0x3000
FIELDS=[(WORK+0x18,0),(WORK+0x24,1),(WORK+0x28,2),(WORK+0x2c,3),(WORK+8,4),
        (0x3b1118c,5),(0x3b11184,6),(0x3b1117c,7),(0x3b29198,9),
        (0x3b291a0,10),(0x3b2919c,11),(0x3abdf7c,12),(0x3b0e354,13),
        (0x3c5e8e0,14),(0x3a7cf3c,15),(0x3c5fefc,16),(SEQUENCE+0x10,22)]
SNAP=[WORK+0x18,WORK+0x24,WORK+0x28,WORK+0x2c,0x3b1118c,0x3b11184,0x3b1117c,
      None,0x3b29198,0x3b291a0,0x3b2919c,0x3abdf7c,0x3c5fefc,SEQUENCE+0x10]
CALLS={0x413920:1,0x505990:2,0x40ef10:3,0x505b40:4,0x505810:5,0x40ff10:6,0x5058b0:7}
class Source:
    def __init__(self,image):
        self.m=Original(image,0x7f).machine
        for page in sorted({a & ~4095 for a,_ in FIELDS} | {0x3c5e000,0x3c5f000,0x3b2a000}):
            if not any(a<=page<=b for a,b,_ in self.m.mem_regions()):self.m.mem_map(page,4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def snapshot(self):return [self.word(a) if a else self.m.mem_read(0x3b2a2f3,1)[0] for a in SNAP]
    def hook(self,m,a,size,user):
        if getattr(self,'transition',False):
            if a!=0x505870:return
            self.receipts=[[1,self.m.mem_read(SEQUENCE+0x1e,1)[0],self.word(SEQUENCE+0x18)]]
            if self.v[3]:self.stopped=True;m.emu_stop();return
            sp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
            return
        if a not in CALLS:return
        kind=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP)
        arg=self.word(sp+4) if kind in (2,3,4) else 0
        self.receipts.append([kind,arg]+self.snapshot())
        if self.v[21]==len(self.receipts):self.stopped=True;m.emu_stop();return
        if len(self.v)>23 and self.v[23]==kind:
            self.write(0x3b0e354,self.v[24]);self.write(0x3c5e8e0,self.v[25]);self.write(0x3a7cf3c,self.v[26])
        result=0
        if kind==1:self.write(0x3b1118c,self.v[20])
        if kind==3:result=self.v[17]
        if kind==6:result=self.v[18]
        if kind==7:result=self.v[19]
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
    def run_transition(self,v):
        self.transition=True;self.v=v;self.receipts=[];self.stopped=False;self.m.context_restore(self.initial)
        self.write(0x3c5e8d0,TASK);self.write(TASK+0x2c,SEQUENCE)
        self.write(SEQUENCE+4,5);self.write(SEQUENCE+8,0);self.write(SEQUENCE+0x1c,0)
        self.write(SEQUENCE+0x18,v[1]);self.m.mem_write(SEQUENCE+0x1e,bytes([v[0]&255]))
        self.m.mem_write(STACK-0x100,struct.pack('<2I',STOP,v[2]));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
        self.m.emu_start(0x505b40,STOP,count=10000)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        if not self.stopped and v[4]:
            self.m.mem_write(STACK-0x100,struct.pack('<2I',STOP,TASK));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
            self.m.emu_start(0x505c20,STOP,count=10000)
            assert self.m.reg_read(UC_X86_REG_EIP)==STOP
        return [self.m.mem_read(SEQUENCE+0x1e,1)[0],self.word(SEQUENCE+0x18)]+self.receipts[0]+[int(self.stopped),self.word(SEQUENCE+0x1c)&65535,self.word(SEQUENCE+4)]+[0]*4
    def run(self,v):
        self.transition=False
        self.v=v;self.receipts=[];self.stopped=False;self.m.context_restore(self.initial)
        self.m.mem_write(WORK,bytes(0x3000))
        for a,index in FIELDS:self.write(a,v[index])
        self.m.mem_write(0x3b2a2f3,bytes([v[8]&255]));self.write(0x3c5e8d0,TASK);self.write(TASK+0x2c,SEQUENCE)
        self.m.mem_write(STACK-0x100,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
        self.m.reg_write(UC_X86_REG_ESI,WORK);self.m.emu_start(0x50e300,STOP,count=10000)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=self.snapshot()+[len(self.receipts),int(self.stopped)]+[0]*144
        for index,receipt in enumerate(self.receipts):out[32+index*16:48+index*16]=receipt
        return out

def cases(seed,count):
    base=[2,150,123,1,6,1,1,9,7,8,9,10,0,0,1,1,0,0,1,0,3,0,99]
    out=[]
    for mode in (0,1,2,3,0xffffffff):
      for timer in (0,149,150,151,0x7fffffff,0x80000000,0xffffffff):
       for music in (0,1,0xffffffff):
        for loop in (0,1,6,0x7fffffff,0x80000000,0xffffffff):
         v=base.copy();v[0]=mode;v[1]=timer;v[3]=music;v[6]=loop;out.append(v)
    for buttons in (0,8,0xffffffff):
     for ready in (0,1,2,0xffffffff):
      for audio in (0,1,0xffffffff):
       for bgm in (0,1,0xffffffff):
        for state in (0,4,5,6,0xffffffff):
         for wait in (0,298,299,300,0x7fffffff,0xffffffff):
          v=base.copy();v[13]=buttons;v[14]=ready;v[18]=audio;v[15]=bgm;v[19]=state;v[16]=wait;out.append(v)
    for key in (0,1,0xffffffff):
     v=base.copy();v[17]=key;out.append(v)
    # Every callee/store prefix for both initialization and active control.
    for mode in (0,2):
     for start in (0,8):
      for stop in range(1,7):
       v=base.copy();v[0]=mode;v[3]=0;v[13]=start;v[21]=stop;out.append(v)
    rng=random.Random(seed)
    for _ in range(count):
      v=[rng.getrandbits(32) for _ in base];v[0]=rng.choice([0,1,2,2,2,3]);v[1]=rng.choice([149,150,rng.getrandbits(32)])
      v[3]=rng.choice([0,1,rng.getrandbits(32)]);v[14]=rng.choice([0,1,2]);v[15]=rng.choice([0,1]);v[18]=rng.choice([0,1]);v[19]=rng.choice([0,5,6]);v[21]=rng.choice([0,0,0,1,2,3,4,5]);out.append(v)
    # Added reentry vectors retain all previous case words and expectations.
    for callback in (3,6):
     for buttons in (0,8):
      for ready in (0,1,2):
       for bgm in (0,1):
        for new_buttons in (0,8):
         for new_ready in (0,1,2):
          for new_bgm in (0,1):
           v=base.copy();v[13]=buttons;v[14]=ready;v[15]=bgm;v[16]=299
           out.append(v+[callback,new_buttons,new_ready,new_bgm])
    return out

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',default='build/oracles/title_input.json');p.add_argument('--seed',type=int,default=50300);p.add_argument('--cases',type=int,default=1500);a=p.parse_args()
    assert hashlib.sha256(Path(a.exe).read_bytes()).hexdigest()==SOURCE_SHA256
    source=Source(PEImage(a.exe));lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_title_input_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2
    bad=[];vectors=cases(a.seed,a.cases)
    for index,v in enumerate(vectors):
      expected=source.run(v);out=(ctypes.c_uint32*160)();probe((ctypes.c_uint32*27)(*(v+[0]*(27-len(v)))),out);actual=list(out)
      if actual!=expected:
       bad.append({'case':index,'input':v,'differences':[[i,x,y] for i,(x,y) in enumerate(zip(expected,actual)) if x!=y]})
       if len(bad)>=8:break
    transition_probe=lib.sadx_native_title_transition_probe;transition_probe.argtypes=probe.argtypes
    transition_vectors=[]
    for fade in (0,1,255):
     for previous in (0,5,14,0xffffffff):
      for next_mode in (0,6,14,0xffffffff):
       for stop in (0,1):
        for composed in (0,1):transition_vectors.append([fade,previous,next_mode,stop,composed])
    for index,v in enumerate(transition_vectors):
      expected=source.run_transition(v);out=(ctypes.c_uint32*12)();transition_probe((ctypes.c_uint32*5)(*v),out)
      if list(out)!=expected:bad.append({'transition_case':index,'input':v,'expected':expected,'actual':list(out)})
    paths=['src/game/title_menu.cpp','src/game/title_menu.hpp','tests/probes/native_title_input_probe.cpp']
    report={'cases':len(vectors)+len(transition_vectors),'mismatches':len(bad),'examples':bad,'seed':a.seed,'passed':not bad,'values_match':not bad,'source_sha256':SOURCE_SHA256,'source_ranges':{'0x50e300-0x50e435':hashlib.sha256(PEImage(a.exe).read(0x50e300,0x135)).hexdigest(),'0x505b40-0x505b5b':hashlib.sha256(PEImage(a.exe).read(0x505b40,0x1b)).hexdigest(),'0x505c20-0x505d65':hashlib.sha256(PEImage(a.exe).read(0x505c20,0x145)).hexdigest()},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'native_sources':{s:hashlib.sha256(Path(s).read_bytes()).hexdigest() for s in paths},'scope':'Full title input caller with input/audio/transition snapshot receipts; transition wrapper also executes with the real sequence update to check waiting/countdown composition.'}
    Path(a.output).parent.mkdir(parents=True,exist_ok=True);Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
