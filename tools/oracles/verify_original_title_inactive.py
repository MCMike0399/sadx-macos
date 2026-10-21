"""Judge original inactive menu exec paths, including pre-switch canonical side effects.

Active initialization is stopped before its first call; active phase bodies remain explicitly
unrecovered in the native callback result. This does not judge those bodies as implemented.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP
ENTRIES={1:0x5122d0,2:0x5053f0,3:0x50b220,4:0x511270,6:0x50bde0,
         7:0x5096f0,8:0x50e110,9:0x50a450,10:0x50dea0,11:0x50d310,12:0x508300}
DRAW_ENTRIES={1:0x512450,2:0x505550,3:0x50b410,4:0x511390,6:0x50bcb0,
              7:0x509810,8:0x50e230,9:0x50a640,10:0x50dfd0,11:0x50d430,12:0x508450}
FIRST_CALLS={1:0x414570,2:0x505320,3:0x50a9b0,4:0x5105a0,6:0x50bd20,
             7:0x508580,8:0x506010,9:0x509bb0,10:0x506010,11:0x506010,12:0x506010}
TASK,WORK,SEQ,SEQUENCE=DATA+0x1000,DATA+0x2000,DATA+0x3000,DATA+0x4000
ANIMATION=((0x40,0x3e),(0x41,0x3f),(0x28,0x29),(0x2a,0x2b),(0x2c,0x2d),(0x43,0x42),(0x45,0x44))
class Source:
    def __init__(self,image):
        self.m=Original(image,0x7f).machine
        regions=list(self.m.mem_regions())
        for page in (0x3c5e000,0x3b2a000):
            if not any(a<=page<=b for a,b,_ in regions):self.m.mem_map(page,4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def hook(self,m,a,size,user):
        if len(self.v)>16 and self.v[16]==2:
            if a==0x40b220:
                self.allocated=1
                if self.v[4]:self.result=0;self.puti_stopped=True;m.emu_stop();return
                sp=m.reg_read(UC_X86_REG_ESP)
                assert self.word(sp+4)==16
                m.reg_write(__import__('unicorn.x86_const',fromlist=['UC_X86_REG_EAX']).UC_X86_REG_EAX,WORK)
                m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
            elif a in (0x406f00,0x6fbec0,0x506040):
                self.result={0x406f00:1,0x6fbec0:4,0x506040:5}[a];m.emu_stop()
            return
        if len(self.v)>16 and self.v[16]==1:
            if a==0x433170:self.result=1;m.emu_stop()
            return
        if a==FIRST_CALLS[self.v[0]]:
            self.result=1;m.emu_stop()
    def run_puti(self,v):
        self.v=v;self.result=0;self.allocated=0;self.puti_stopped=False;self.m.context_restore(self.initial)
        self.m.mem_write(TASK,bytes(0x4000));self.write(TASK+0x2c,WORK if v[0] else 0)
        self.write(0x3c5e8d0,SEQ);self.write(SEQ+0x2c,SEQUENCE);self.write(SEQUENCE+4,v[3])
        self.write(WORK,v[1]);self.write(WORK+12,v[2]);self.write(WORK+8,0xabadcafe)
        self.m.mem_write(STACK-0x100,struct.pack('<2I',STOP,TASK));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
        self.m.emu_start(0x50c010,STOP,count=10000)
        assert self.result or self.puti_stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        work=self.word(TASK+0x2c);out=[0]*32
        out[:9]=[self.result,int(bool(work)),self.word(WORK) if work else 0,0,self.word(WORK+12) if work else 0,
                 int(self.word(TASK+24)==0x50bfc0),int(self.word(TASK+20)==0x51f680),self.allocated,int(self.puti_stopped)]
        return out
    def run_draw(self,v):
        self.v=v;self.result=0;self.m.context_restore(self.initial)
        self.m.mem_write(TASK,bytes(0x4000));self.write(TASK+0x2c,WORK)
        color=0x18 if v[0]==6 else 0x14
        self.write(WORK+color,v[5]);self.write(WORK+0x18,v[8])
        if v[0]==6:self.write(WORK+color,v[5])
        self.m.mem_write(STACK-0x100,struct.pack('<2I',STOP,TASK));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
        self.m.emu_start(DRAW_ENTRIES[v[0]],STOP,count=10000)
        assert self.result or self.m.reg_read(UC_X86_REG_EIP)==STOP
        return [self.result]+[0]*31
    def run(self,v):
        if len(v)>16 and v[16]==2:return self.run_puti(v)
        if len(v)>16 and v[16]==1:return self.run_draw(v)
        self.v=v;self.result=0;self.m.context_restore(self.initial)
        self.m.mem_write(TASK,bytes(0x4000));self.write(TASK+0x2c,WORK)
        self.write(0x3c5e8d0,SEQ);self.write(SEQ+0x2c,SEQUENCE)
        self.write(SEQUENCE+4,v[2]);self.write(SEQUENCE+8,v[3])
        self.write(WORK,v[1]);fade=0x14 if v[0]==6 else 0x10;color=fade+4
        self.write(WORK+fade,v[4]);self.write(WORK+color,v[5])
        self.m.mem_write(0x3b2a2fb,bytes([v[6]&255]));self.m.mem_write(0x3b2a2fd,bytes([v[7]&255]))
        if v[0]==1:self.write(WORK+0x20,v[8])
        if v[0]==7:
            self.write(WORK+0x24,v[8])
            for (frames,frame),value in zip(ANIMATION,v[9:16]):
                self.m.mem_write(WORK+frames,bytes([value&255]));self.m.mem_write(WORK+frame,bytes([(value>>8)&255]))
        self.m.mem_write(STACK-0x100,struct.pack('<2I',STOP,TASK));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
        self.m.emu_start(ENTRIES[v[0]],STOP,count=10000)
        assert self.result or self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=[0]*32;out[:6]=[self.result,self.word(WORK),self.word(WORK+fade),self.word(WORK+color),
                              self.m.mem_read(0x3b2a2fb,1)[0],self.m.mem_read(0x3b2a2fd,1)[0]]
        if v[0]==1:out[6]=self.word(WORK+0x20)
        if v[0]==7:
            out[6]=self.word(WORK+0x24)
            for index,(frames,frame) in enumerate(ANIMATION):
                out[7+index]=self.m.mem_read(WORK+frames,1)[0]|(self.m.mem_read(WORK+frame,1)[0]<<8)
        return out

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('native_library',type=Path)
    p.add_argument('--output',required=True,type=Path);p.add_argument('--seed',type=int,default=423723);p.add_argument('--cases',type=int,default=1500)
    args=p.parse_args();image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise ValueError('requires pinned original PC executable')
    source=Source(image);library=ctypes.CDLL(str(args.native_library.resolve()));native=library.sadx_native_title_inactive_probe
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;native.restype=None
    rng=random.Random(args.seed);rows=[]
    for mode in ENTRIES:
        for phase in (0,4,0xffffffff):
            for current in (0,mode,5,13):
                for byte in (0,1,7,8,11,12,31,32,127,128,254,255):
                    rows.append([mode,phase,current,mode,rng.getrandbits(32),rng.getrandbits(32),byte,byte,byte]+[byte|(byte<<8)]*7)
    for _ in range(args.cases):
        mode=rng.choice(tuple(ENTRIES));rows.append([mode,rng.choice((0,4,0xffffffff)),rng.randrange(14),rng.randrange(14)]+[rng.getrandbits(32) for _ in range(12)])
    for mode in DRAW_ENTRIES:
        for color in (0,1,0xffffffff):
            for submode in (0,1,2,3,0xffffffff):
                row=[mode,0,5,0,0,color,0,0,submode]+[0]*7+[1]
                rows.append(row)
    for present in (0,1):
        for phase in (0,1,2,3,4,0xffffffff):
            for frames in (0,119,120,121,0x7fffffff,0x80000000,0xffffffff):
                for current in (0,5,13):
                    for stop in (0,1):rows.append([present,phase,frames,current,stop]+[0]*11+[2])
    bad=[]
    puti_native=library.sadx_native_title_puti_probe
    puti_native.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;puti_native.restype=None
    draw_native=library.sadx_native_title_draw_probe
    draw_native.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;draw_native.restype=None
    for index,row in enumerate(rows):
        expected=source.run(row);out=(ctypes.c_uint32*32)()
        fn=puti_native if len(row)>16 and row[16]==2 else draw_native if len(row)>16 and row[16]==1 else native
        fn((ctypes.c_uint32*len(row))(*row),out)
        difference=[(i,a,b) for i,(a,b) in enumerate(zip(expected,out)) if a!=b]
        if difference:
            bad.append({'case':index,'input':row,'differences':difference})
            if len(bad)==8:break
    root=Path(__file__).resolve().parents[2];paths=['src/game/title_menu.hpp','src/game/title_menu.cpp','tests/probes/native_title_inactive_probe.cpp']
    report={'passed':not bad,'values_match':not bad,'cases':len(rows),'mismatches':len(bad),'first_mismatches':bad,'seed':args.seed,
            'source_sha256':image.sha256,'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},
            'limits':['Judges inactive phase0 and source invalid-phase returns; active initialization stops before its first call. Active phases1/2/3 remain tagged missing branches. All menu display wrappers compare their guard or stop before the first draw call. Puti first exec/MAlloc callbacks and wait-phase stores are compared; its unused indeterminate exit countdown is not read/claimed.']}
    spans = [(5317328, 5317792), (5264368, 5264896), (5288480, 5289104), (5313136, 5313536), (5291184, 5291280), (5291488, 5291824), (5281520, 5281888), (5277408, 5277644), (5300496, 5300848), (5284944, 5285568), (5299872, 5300256), (5296912, 5297264), (5276416, 5276832), (5292048, 5292326)]
    report['source_ranges'] = {f'{start:#x}-{end:#x}':hashlib.sha256(image.read(start,end-start)).hexdigest() for start,end in spans}
    report['native_library_sha256'] = hashlib.sha256(args.native_library.read_bytes()).hexdigest()
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
