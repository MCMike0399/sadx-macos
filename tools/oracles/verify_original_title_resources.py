"""Original mode-resource caller, main-menu setter and title color helpers."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK,WORK,DEVICE,VTABLE=DATA+0x1000,DATA+0x2000,DATA+0x3000,DATA+0x4000
class Source:
    def __init__(self,image):
        self.m=Original(image,127).machine
        regions=list(self.m.mem_regions())
        for page in (0x3c5e000,0x3c5f000,0x3d12000,0x3b0f000,0x10d7000):
            if not any(a<=page<=b for a,b,_ in regions):self.m.mem_map(page,4096)
        self.m.mem_write(0x10d7cb0,image.read(0x10d7cb0,20))
        self.m.mem_write(0x10d7cc4,image.read(0x10d7cc4,56))
        self.textures={struct.unpack('<2I',image.read(0x7eede0+i*8,8)):i for i in range(30)}
        self.m.mem_write(STOP+256,b'\xc2\x0c\x00')
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def hook(self,m,a,size,user):
        if a not in (0x421180,STOP+256):return
        sp=m.reg_read(UC_X86_REG_ESP)
        if a==0x421180:
            pair=(self.word(sp+4),self.word(sp+8));assert pair in self.textures
            kind,value=2,self.textures[pair]
        else:
            assert [self.word(sp+4+i*4) for i in range(3)]==[DEVICE,29,0]
            kind,value=1,0
        self.trace += [kind,value,self.word(0x3c5e8d8),self.word(0x3c5e8dc),self.word(0x3c5e8e0)]
        if len(self.trace)//5==self.v[9]:self.stopped=True;m.emu_stop();return
        m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4+(12 if kind==1 else 0))
    def run(self,v):
        self.v=v;self.trace=[];self.stopped=False;self.m.context_restore(self.initial)
        self.m.mem_write(TASK,bytes(0x4000));self.write(TASK+0x2c,WORK)
        self.write(0x3c5e8d0,TASK);self.write(0x3c5fee4,TASK);self.write(WORK+8,v[4]);self.write(WORK+4,v[7])
        for a,value in zip((0x3c5e8d8,0x3c5e8dc,0x3c5e8e0),(v[1],v[2],v[3])):self.write(a,value)
        self.write(0x3b0f0e8,1);self.write(0x3d128b0,DEVICE);self.write(DEVICE,VTABLE);self.write(VTABLE+200,STOP+256)
        arguments=[[v[5]],[],[v[7],v[8],v[6]],[v[6]],[v[6]]][v[0]]
        self.m.mem_write(STACK-0x100,struct.pack('<'+'I'*(len(arguments)+1),STOP,*arguments));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
        self.m.emu_start([0x506010,0x50bd10,0x4319d0,0x506e10,0x506e40][v[0]],STOP,count=20000)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=[0]*256
        out[:8]=[self.m.reg_read(UC_X86_REG_EAX) if v[0]>=2 and not self.stopped else 0,
                 self.word(0x3c5e8d8),self.word(0x3c5e8dc),self.word(0x3c5e8e0),self.word(WORK+4),int(self.stopped),self.m.reg_read(UC_X86_REG_FPSW)&63,len(self.trace)//5]
        out[8:8+len(self.trace)]=self.trace;return out

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('native_library',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--seed',type=int,default=423723);p.add_argument('--cases',type=int,default=1500);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise ValueError('requires pinned source')
    source=Source(image);library=ctypes.CDLL(str(a.native_library.resolve()));native=library.sadx_native_title_resources_probe
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;native.restype=None;rng=random.Random(a.seed)
    content=[]
    for pointer in struct.unpack('<14I',image.read(0x10d7cc4,56)):
        ids=[]
        for i in range(31):
            value,=struct.unpack('<I',image.read(pointer+i*4,4))
            if value==30:break
            ids.append(value)
        content += [len(ids)]+ids+[0]*(30-len(ids))
    rows=[]
    for kind in range(5):
        for mode in range(14):
            for progress in (0,0x3f000000,0x3f400000,0x3f800000,0x40000000,0xbf800000,0x41200000,0x4b000001,0x7fc00000,0xff800000,0x7f800000):
                for stop in (0,1,2):rows.append([kind,0,0,1,rng.choice([0,1,0xffffffff]),mode,progress,rng.getrandbits(32),rng.getrandbits(32),stop])
    rows += [[rng.randrange(5),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(14),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(4)] for _ in range(a.cases)]
    bad=[];flags=0
    for index,row in enumerate(rows):
        v=row+[0]*(512-len(row));v[16:16+len(content)]=content
        expected=source.run(v);out=(ctypes.c_uint32*256)();native((ctypes.c_uint32*512)(*v),out)
        if out[6]!=expected[6]:flags+=1
        diff=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if i!=6 and x!=y]
        if diff:
            bad.append({'case':index,'input':row,'diff':diff})
            if len(bad)==8:break
    root=Path(__file__).resolve().parents[2];paths=['src/game/title_menu.hpp','src/game/title_menu.cpp','tests/probes/native_title_resources_probe.cpp']
    report={'passed':not bad and not flags,'values_match':not bad,'mismatches':len(bad),'first_mismatches':bad,'fp_flag_mismatches':flags,'cases':len(rows),'seed':a.seed,'source_sha256':image.sha256,
            'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'limits':['Post-device x87 control0x007f; rendering/texture callees are traced owner seams. Flag mismatches remain partial.']}
    spans = [(5267472, 5267519), (5291280, 5291296), (4397520, 4397825), (5271056, 5271095), (5271104, 5271184)]
    report['source_ranges'] = {f'{start:#x}-{end:#x}':hashlib.sha256(image.read(start,end-start)).hexdigest() for start,end in spans}
    report['native_library_sha256'] = hashlib.sha256(a.native_library.read_bytes()).hexdigest()
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(bool(bad or flags))
if __name__=='__main__':raise SystemExit(main())
