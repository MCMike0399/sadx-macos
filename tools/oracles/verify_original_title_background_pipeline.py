"""Composed original background→actual scaled-quad instructions and native same-call chain.

Texture metadata is a declared synthetic fixture, with varying odd dimensions. Selection and
final Ninja submission are receipts; color and scaled-quad bodies execute on both sides.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import DATA,STACK,STOP
from verify_original_title_background import Source as BackgroundSource,WORK,LISTS
class Source(BackgroundSource):
    def __init__(self,image):
        super().__init__(image)
        regions=list(self.m.mem_regions())
        for page in (0x3b2c000,0x3d0f000):
            if not any(a<=page<=b for a,b,_ in regions):self.m.mem_map(page,4096)
        self.m.mem_write(0x43434a,b'\x00')
        self.clean=self.m.context_save()
    def receipt(self,sp):
        self.m.reg_write(UC_X86_REG_EIP,self.word(sp));self.m.reg_write(UC_X86_REG_ESP,sp+4)
    def hook(self,m,address,size,user):
        if address in (0x433010,0x432f40,0x433f80):return
        sp=m.reg_read(UC_X86_REG_ESP)
        if address==0x403070:
            self.current=LISTS[self.word(sp+4)];self.put(0x3d0fa24,DATA+0x1800+self.current*16);self.receipt(sp)
        elif address==0x77ed00:
            self.depth=self.word(sp+4);self.depth_calls+=1;self.receipt(sp)
        elif address==0x403130:
            assert self.word(sp+8)==4
            words=list(struct.unpack('<24I',m.mem_read(self.word(sp+4),96)))
            self.quads += [self.current,self.word(sp+12),self.word(sp+16),self.depth]+words+[
                self.word(WORK+0x30),self.word(WORK+0x34),m.mem_read(WORK+0x38,1)[0],0]
            self.receipt(sp)
    def run_pipeline(self,v):
        self.quads=[];self.current=0;self.depth=3;self.depth_calls=0
        # Initialize the same authoritative PVR state before the background's first color call.
        self.m.context_restore(self.clean)
        self.put(STACK,STOP);self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.emu_start(0x432ea0,STOP,count=10000)
        if len(v)>20 and v[20]:
            self.put(0x3b2c658,v[21])
            for index,color in enumerate(v[22:26]):self.put(0x3b2c604+index*24,color)
        for group in range(4):
            table=DATA+0x1800+group*16;names=DATA+0x2000+group*0x400
            self.put(table,names);self.put(table+4,67)
            for index in range(67):
                surface=DATA+0x4000+group*0x1200+index*64
                self.put(names+index*12+8,surface)
                for offset,value in ((0,group*100+index+1),(32,0x20000000),(36,256+index%3),(40,128+index%5)):self.put(surface+offset,value)
        # BackgroundSource restores only CPU context; the initialized PVR/data fixture remains.
        result=super().run(v)
        out=[0]*4096;out[:7]=result[:3]+[len(self.quads)//32,result[5],self.depth,self.depth_calls]
        out[8:8+len(self.quads)]=self.quads;return out

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('exe',type=Path);p.add_argument('native_library',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--seed',type=int,default=423723);p.add_argument('--cases',type=int,default=300);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise ValueError('requires pinned source')
    source=Source(image);lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_title_background_pipeline_probe
    fn.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;fn.restype=None;rng=random.Random(a.seed)
    layout=list(struct.unpack('<9I',image.read(0x7ef928,36)));cases=[]
    for width in (0x3f800000,0x3fa00000,0x3f400000):
        for frame in (0,49,50,54,55,60,69,70,77,78,79,80,150):
            for language in (0,1,4):cases.append([frame,0x437f0000,0,150,rng.randrange(512),width,0x3f800000,1,1,language,0]+layout)
    for _ in range(a.cases):
        width=rng.choice([0x3f800000,0x3fa00000,0x3f400000]);compact=width==0x3f800000
        cases.append([rng.randrange(200),rng.choice([0,0x437fffff]),rng.randrange(5 if compact else 10),rng.randrange(200),rng.getrandbits(32),width,rng.choice([0x3f800000,0x3f400000,0x3fa00000]),rng.randrange(7),rng.randrange(4),rng.randrange(5),0]+layout)
    for width in (0x3f800000,0x3fa00000):
        for frame in (0,1,50,80,150):
            for language in (0,1):
                cases.append([frame,0,0,150,200,width,0x3f800000,0,1,language,0]+layout+
                             [1,0x81234567,0,0x12345678,0xffffffff,0x80123456])
    bad=[];flags=0
    for index,row in enumerate(cases):
        expected=source.run_pipeline(row);out=(ctypes.c_uint32*4096)();fn((ctypes.c_uint32*26)(*(row+[0]*(26-len(row)))),out)
        if out[4]!=expected[4]:flags+=1
        diff=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if i!=4 and x!=y]
        if diff:
            bad.append({'case':index,'input':row,'diff':diff[:20]})
            if len(bad)==8:break
    root=Path(__file__).resolve().parents[2];paths=['src/game/title_menu.hpp','src/game/title_menu.cpp','src/engine/pvr_draw.hpp','src/engine/pvr_draw.cpp','tests/probes/native_title_background_pipeline_probe.cpp']
    report={'passed':not bad and not flags,'values_match':not bad,'cases':len(cases),'mismatches':len(bad),'first_mismatches':bad,'fp_flag_mismatches':flags,'seed':a.seed,'source_sha256':image.sha256,
            'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'native_library_sha256':hashlib.sha256(a.native_library.read_bytes()).hexdigest(),
            'source_ranges':{f'{s:#x}-{e:#x}':hashlib.sha256(image.read(s,e-s)).hexdigest() for s,e in ((0x50e470,0x51011c),(0x433f80,0x434370),(0x432ea0,0x432f40),(0x432df0,0x432e9c),(0x432f40,0x432f83),(0x433010,0x43305d))},
            'limits':['Runs actual background/color/scaled-quad bodies together; synthetic texture metadata and selector/submission receipts do not prove imported archive pixels. Effective43434a patch applied. Control0x007f, low-six flags separately reported.']}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
