"""Original dialog animation and inactive callback prefixes, with explicit active-phase stops."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK,WORK,PARAMS=DATA+0x1000,DATA+0x2000,DATA+0x3000
class Source:
    def __init__(self,image):
        self.m=Original(image,127).machine
        regions=list(self.m.mem_regions())
        for page in (0x3b11000,0x3b2c000):
            if not any(a<=page<=b for a,b,_ in regions):self.m.mem_map(page,4096)
        self.phases=struct.unpack('<6I',image.read(0x432c28,24))
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def hook(self,m,a,size,user):
        if self.v[0]==1 and a in self.phases[1:]:self.result=self.phases.index(a);m.emu_stop()
        if self.v[0]==2 and a==0x433010:self.result=1;m.emu_stop()
    def run(self,v):
        self.v=v;self.result=0;self.m.context_restore(self.initial)
        self.m.mem_write(TASK,bytes(0x2100));self.write(TASK+0x2c,WORK)
        self.write(0x3b2c588,TASK);self.write(0x3b1118c,v[1])
        for offset,value in [(0,PARAMS if v[6] else 0),(4,v[4]),(8,v[7]),(12,v[8]),(16,v[9]),
                             (20,v[3]),(24,v[7]),(28,v[2]),(32,v[2])]:self.write(WORK+offset,value)
        self.write(PARAMS,v[5])
        args=[WORK+41,WORK+32,WORK+20] if v[0]==0 else [TASK]
        self.m.mem_write(STACK-0x100,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100)
        self.m.emu_start([0x431960,0x432720,0x432480][v[0]],STOP,count=10000)
        assert self.result or self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=[0]*32;out[:10]=[self.result,self.word(WORK+32),self.word(WORK+20),self.m.mem_read(WORK+41,1)[0],
            self.word(WORK+24),self.word(WORK+28),self.word(WORK+8),self.word(WORK+12),self.word(WORK+16),self.word(WORK+36)]
        out[10:14]=struct.unpack('<4I',self.m.mem_read(WORK+42,16));out[14]=self.word(WORK+60);out[15]=self.m.reg_read(UC_X86_REG_FPSW)&63
        return out

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('native_library',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--seed',type=int,default=423723);p.add_argument('--cases',type=int,default=1500);args=p.parse_args();image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise ValueError('requires pinned original source')
    source=Source(image);lib=ctypes.CDLL(str(args.native_library.resolve()));native=lib.sadx_native_title_dialog_probe;native.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;native.restype=None
    rng=random.Random(args.seed);cases=[]
    for kind in range(3):
        for phase in (0,1,2,3,4,5,6,0xffffffff):
            for fade in (0,0x3f800000,0x3fc00000,0x7fc00000,0x7f800000,0x80000000):
                for style in range(8):cases.append([kind,rng.choice([0,1,2,0xffffffff]),rng.getrandbits(32),fade,phase,style,style%2,rng.getrandbits(32),fade,0])
    cases += [[rng.randrange(3),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(8),rng.randrange(8),rng.randrange(2),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32)] for _ in range(args.cases)]
    bad=[];flags=0
    for index,row in enumerate(cases):
        expected=source.run(row);out=(ctypes.c_uint32*32)();native((ctypes.c_uint32*10)(*row),out)
        if out[15]!=expected[15]:flags+=1
        diff=[(i,a,b) for i,(a,b) in enumerate(zip(expected,out)) if i!=15 and a!=b]
        if diff:
            bad.append({'case':index,'input':row,'diff':diff})
            if len(bad)==8:break
    root=Path(__file__).resolve().parents[2];paths=['src/game/title_menu.hpp','src/game/title_menu.cpp','tests/probes/native_title_dialog_probe.cpp']
    report={'passed':not bad and not flags,'values_match':not bad,'mismatches':len(bad),'first_mismatches':bad,'fp_flag_mismatches':flags,'cases':len(cases),'seed':args.seed,
            'source_sha256':image.sha256,'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},
            'limits':['Active dialog phase bodies stop before their first instruction and remain explicitly unimplemented. Known poison animation counters are compared; genuinely indeterminate MAlloc counter is a native optional, whose all-u32 set is closed under wrapping increment.']}
    spans = [(4397408, 4397509), (4400928, 4402189), (4400256, 4400429)]
    report['source_ranges'] = {f'{start:#x}-{end:#x}':hashlib.sha256(image.read(start,end-start)).hexdigest() for start,end in spans}
    report['native_library_sha256'] = hashlib.sha256(args.native_library.read_bytes()).hexdigest()
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(bool(bad or flags))
if __name__=='__main__':raise SystemExit(main())
