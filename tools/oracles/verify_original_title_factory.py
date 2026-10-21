"""Compare native menu constructors with their unmodified original PC instructions."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP

# Entry, canonical task pointer, exec, display, source allocation extent.
FACTORIES = {
    1: (0x5124a0,0x3c5ff08,0x5122d0,0x512450,56),
    2: (0x505600,0x3c5e8c0,0x5053f0,0x505550,56),
    3: (0x50b490,0x3c5fee0,0x50b220,0x50b410,208),
    4: (0x511400,0x3c5ff04,0x511270,0x511390,72),
    7: (0x509860,0x3c5fed8,0x5096f0,0x509810,84),
    9: (0x50a6c0,0x3c5fedc,0x50a450,0x50a640,208),
    10:(0x50e020,0x3c5fef4,0x50dea0,0x50dfd0,60),
    12:(0x5084a0,0x3c5fed4,0x508300,0x508450,56),
    6: (0x50bf30,0x3c5fee4,0x50bde0,0x50bcb0,40),
    8: (0x50e270,0x3c5fef8,0x50e110,0x50e230,32),
    11:(0x50d470,0x3c5feec,0x50d310,0x50d430,40),
    13:(0x50c140,0x3c5fee8,0x50c010,0,0),
}
TASK, WORK, SELECTION = DATA+0x1000, DATA+0x2000, DATA+0x3000

class Source:
    def __init__(self,image):
        self.m=Original(image,0x7f).machine
        regions=list(self.m.mem_regions())
        for page in (0x3c5f000,0x3c5e000,0x3b2a000,0x3b29000,0x3b18000):
            if not any(a<=page<=b for a,b,_ in regions):self.m.mem_map(page,4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
        self.initial=self.m.context_save()
    def word(self,address):return struct.unpack('<I',self.m.mem_read(address,4))[0]
    def write(self,address,value):self.m.mem_write(address,struct.pack('<I',value&0xffffffff))
    def hook(self,m,address,size,user):
        if address not in (0x40b860,0x40b2d0):return
        sp=m.reg_read(UC_X86_REG_ESP)
        if address==0x40b860:
            flags,level,callback=(self.word(sp+4+4*i) for i in range(3))
            assert callback==self.factory[2]
            self.trace.extend([1,flags,level,self.v[0]])
            result=0 if self.v[8] else TASK
        else:
            count,size=self.word(sp+4),self.word(sp+8)
            assert (count,size)==(1,self.factory[4])
            self.trace.extend([2,size,0,0]);result=WORK
        if len(self.trace)//4==self.v[9]:
            self.stopped=True;m.emu_stop();return
        if address==0x40b860 and result:self.write(TASK+0x10,self.factory[2])
        m.reg_write(UC_X86_REG_EAX,result)
        m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
    def run(self,v):
        self.v=v;self.factory=FACTORIES[v[0]];self.trace=[];self.stopped=False
        self.m.context_restore(self.initial)
        self.m.mem_write(TASK,bytes(0x2100))
        self.write(self.factory[1],TASK+0x100)
        self.m.mem_write(0x3b2a2fb,bytes([v[4]&255]))
        self.m.mem_write(0x3b2a2f9,bytes([v[5]&255]))
        self.m.mem_write(0x3b290e0,bytes([v[6]&255]))
        self.m.mem_write(0x3b290d8,struct.pack('<H',(v[6]>>16)&65535))
        self.m.mem_write(0x3b2a2fa,bytes([v[7]&255]))
        self.m.mem_write(0x3b2a2fd,bytes([v[10]]))
        self.m.mem_write(0x3b2a2f0,bytes([v[11]&255]))
        self.m.mem_write(0x3b18250,struct.pack('<86I',*v[256:342]))
        self.m.mem_write(SELECTION,struct.pack('<3I',*v[1:4]))
        self.m.mem_write(STACK-0x100,struct.pack('<2I',STOP,SELECTION))
        self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
        self.m.emu_start(self.factory[0],STOP,count=10000)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=[0]*128
        pointer=self.word(self.factory[1])
        out[0]=0 if self.stopped or not self.m.reg_read(UC_X86_REG_EAX) else 1
        out[1]=0 if not pointer else 1 if pointer==TASK else 2
        out[2]=int(self.word(TASK+0x2c)==WORK)
        out[3]=int(self.word(TASK+0x10)==self.factory[2])
        display=self.word(TASK+0x14)
        assert display in (0,self.factory[3])
        out[4]=int(display!=0)
        out[5]=self.m.mem_read(0x3b2a2f9,1)[0]
        out[6]=self.factory[4] if out[2] else 0
        out[7]=int(self.stopped)
        out[8:8+self.factory[4]//4]=struct.unpack('<'+str(self.factory[4]//4)+'I',self.m.mem_read(WORK,self.factory[4]))
        if out[2] and v[0] in (3,9):
            assert out[20]==WORK+0x1c
            out[20]=1
        if out[2] and v[0]==7:
            assert out[26]==0x7ef728
            out[26]=1
        out[64:64+len(self.trace)]=self.trace
        return out

def cases(count,seed,image):
    rng=random.Random(seed)
    rows=[]
    for mode in FACTORIES:
        for fail in (0,1):
            for stop in (0,1,2,3):
                for byte in (0,1,2,3,4,127,128,255):
                    rows.append([mode,0xffffffff,5,13,byte,byte,byte|(byte*257<<16),byte,fail,stop])
    for _ in range(count):
        rows.append([rng.choice(tuple(FACTORIES))]+[rng.getrandbits(32) for _ in range(7)]+[rng.randrange(2),rng.randrange(4)])
    content=[]
    for base in (0x7ef5ec,0x7ef624):
        for character in range(7):
            pointer,size=struct.unpack('<IB3x',image.read(base+character*8,8))
            pairs=list(struct.unpack('<'+str(size)+'H',image.read(pointer,size*2)))
            content.extend([size]+pairs+[0]*(10-size))
    content.extend(struct.unpack('<7H',image.read(0x7ef6b4,14)))
    content.extend(image.read(0x7ef688,44))
    for row in rows:
        row += [rng.randrange(7),rng.choice([0,1,255])]
        row += [0]*(512-len(row))
        row[16:16+len(content)]=content
        flags=bytes(rng.choice([0,0,0,1,255]) for _ in range(344))
        row[256:342]=struct.unpack('<86I',flags)
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--seed',type=int,default=423723)
    parser.add_argument('--cases',type=int,default=1500)
    args=parser.parse_args();image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise ValueError('requires pinned original PC executable')
    source=Source(image);library=ctypes.CDLL(str(args.native_library.resolve()))
    native=library.sadx_native_title_factory_probe
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];native.restype=None
    rows=cases(args.cases,args.seed,image);mismatches=[]
    for index,row in enumerate(rows):
        expected=source.run(row);out=(ctypes.c_uint32*128)();native((ctypes.c_uint32*512)(*row),out)
        differences=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
        if differences:
            mismatches.append({'case':index,'input':row,'differences':differences})
            if len(mismatches)==8:break
    root=Path(__file__).resolve().parents[2]
    paths=['src/game/title_menu.hpp','src/game/title_menu.cpp','tests/probes/native_title_factory_probe.cpp']
    report={'passed':not mismatches,'values_match':not mismatches,'cases':len(rows),'mismatches':len(mismatches),
            'first_mismatches':mismatches,'seed':args.seed,'source_sha256':image.sha256,
            'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},
            'native_library_sha256':hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
            'limits':['Task and allocation dependencies are traced seams; full source auxiliary bytes and callback stores are compared.',
                      'Allocation failure faults are not modeled; native composition reports null allocation explicitly.']}
    spans = [(5317792, 5317907), (5264896, 5265042), (5289104, 5289181), (5313536, 5313650), (5291824, 5291962), (5281888, 5281962), (5300848, 5300946), (5285568, 5285645), (5300256, 5300352), (5297264, 5297362), (5276832, 5276938), (5292352, 5292375), (5299296, 5299351), (5282624, 5282685), (5288384, 5288480), (5284848, 5284944), (5281424, 5281520), (5268016, 5268313), (5268320, 5268448)]
    report['source_ranges'] = {f'{start:#x}-{end:#x}':hashlib.sha256(image.read(start,end-start)).hexdigest() for start,end in spans}
    report['native_library_sha256'] = hashlib.sha256(args.native_library.read_bytes()).hexdigest()
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report));return int(bool(mismatches))
if __name__=='__main__':raise SystemExit(main())
