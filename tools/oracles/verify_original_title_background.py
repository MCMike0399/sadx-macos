"""Full original title-background entry, including cloud/water/logo/text/prompt passes.

Renderer calls are receipts with exact binary32 arguments and work snapshots; their native
scaled-quad callee has its own original-instruction oracle. This caller claims no GPU pixels.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_ESI,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
WORK=DATA+0x1000
LISTS={0x10d7c58:0,0x10d7c50:1,0x10d7c48:2,0x10d9fb4:3}
class Source:
    def __init__(self,image):
        self.m=Original(image,127).machine
        mapped=list(self.m.mem_regions())
        for page in (0x892000,0x3b11000,0x3b0f000):
            if not any(a<=page<=b for a,b,_ in mapped):self.m.mem_map(page,4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.clean=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def hook(self,m,a,size,user):
        calls={0x433010:(1,1),0x432f40:(2,1),0x403070:(3,1),0x433f80:(4,6),0x77ed00:(5,1)}
        if a not in calls:return
        kind,count=calls[a];sp=m.reg_read(UC_X86_REG_ESP)
        values=[self.word(sp+4+i*4) for i in range(count)]
        if kind==3:values=[LISTS[values[0]]]
        self.trace += [kind]+values+[0]*(6-count)+[self.word(WORK+0x30),self.word(WORK+0x34),self.m.mem_read(WORK+0x38,1)[0]]
        if len(self.trace)//10==self.v[10]:self.stopped=True;m.emu_stop();return
        m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
    def run(self,v):
        self.v=v;self.trace=[];self.stopped=False;self.m.context_restore(self.clean)
        self.m.mem_write(WORK,bytes(60))
        for offset,value in ((0x30,v[0]),(0x34,v[1]),(0x24,v[3]),(0x28,v[4])):self.put(WORK+offset,value)
        self.m.mem_write(WORK+0x38,bytes([v[2]&255]))
        for a,value in zip((0x8928c0,0x8928c4,0x3b11180,0x3b1118c,0x3b0f0e8),v[5:10]):self.put(a,value)
        self.m.mem_write(0x7ef928,struct.pack('<9I',*v[11:20]))
        self.put(STACK,STOP);self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.reg_write(UC_X86_REG_ESI,WORK)
        self.m.emu_start(0x50e470,STOP,count=30000)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=[0]*2048;out[:6]=[self.word(WORK+0x30),self.word(WORK+0x34),self.m.mem_read(WORK+0x38,1)[0],int(self.stopped),len(self.trace)//10,self.m.reg_read(UC_X86_REG_FPSW)&63]
        out[8:8+len(self.trace)]=self.trace;return out

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('exe',type=Path);p.add_argument('native_library',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--seed',type=int,default=423723);p.add_argument('--cases',type=int,default=1500);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise ValueError('requires pinned source')
    source=Source(image);lib=ctypes.CDLL(str(a.native_library.resolve()));native=lib.sadx_native_title_background_probe
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;native.restype=None;rng=random.Random(a.seed)
    layout=list(struct.unpack('<9I',image.read(0x7ef928,36)));cases=[]
    for width in (0x3f800000,0x3fa00000,0x3f400000):
        for frame in (0,1,49,50,54,55,59,60,69,70,77,78,79,80,149,150,0xffffffff):
            for language in (0,1,4):
                for stop in (0,1,4,17):cases.append([frame,0x437f0000,rng.randrange(10),150,rng.randrange(512),width,0x3f800000,2,1,language,stop]+layout)
    for width in (0x3f800000,0x3fa00000):
        for offset in (0,0x437f0000,0x437fffff,0x43800000,0x7fc12345,0x7f800000):
            cases.append([0,offset,0,0,0,width,0x3f800000,0xffffffff,0,1,0]+layout)
    cases.append([0,0,0,0,0,0x3f800000,0x40000000,0,3,1,0]+layout)
    scales=[0,0x80000000,0x3f800000,0x3fa00000,0x3f400000,0x40000000,0xbf800000,0x7fc12345,0x7f800000,1]
    for _ in range(a.cases):
        row=[rng.choice([0,49,50,54,55,70,77,78,79,80,150,rng.getrandbits(32)]),rng.choice([0,0x437fffff,0x43800000,0x7fc12345,rng.getrandbits(32)]),rng.randrange(256),rng.choice([0,149,150,0xffffffff]),rng.getrandbits(32),rng.choice(scales),rng.choice(scales),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(5),rng.choice([0,0,1,2,3,17,50])]+layout
        cases.append(row)
    bad=[];flags=0
    for index,row in enumerate(cases):
        expected=source.run(row);out=(ctypes.c_uint32*2048)();native((ctypes.c_uint32*20)(*row),out)
        if out[5]!=expected[5]:flags+=1
        diff=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if i!=5 and x!=y]
        if diff:
            bad.append({'case':index,'input':row,'diff':diff[:20]})
            if len(bad)==8:break
    root=Path(__file__).resolve().parents[2];paths=['src/game/title_menu.hpp','src/game/title_menu.cpp','tests/probes/native_title_background_probe.cpp']
    paths += ['src/cpu/fidelity_float.hpp', 'src/cpu/x87_math.hpp', 'src/cpu/x87_math.cpp', 'src/cpu/crt_integer.hpp', 'src/cpu/crt_integer.cpp', 'src/engine/task.hpp', 'src/engine/task_work.hpp']
    report={'passed':not bad and not flags,'values_match':not bad,'cases':len(cases),'mismatches':len(bad),'first_mismatches':bad,'fp_flag_mismatches':flags,'seed':a.seed,'source_sha256':image.sha256,
            'source_ranges':{'0x50e470-0x51011c':hashlib.sha256(image.read(0x50e470,0x1cac)).hexdigest()},'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},
            'native_library_sha256':hashlib.sha256(a.native_library.read_bytes()).hexdigest(),'limits':['Full original entry includes alternate tail after the firstRET. Renderer callees are traced receipts; scaled quad has a separate oracle. Caller uses post-device control0x007f; low-six flags remain separately reported.']}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
