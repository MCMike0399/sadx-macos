"""Execute both original content entries; only actual lower callees become receipts."""
import argparse, ctypes, hashlib, json, random, struct, sys, os
from pathlib import Path
ROOT = Path(os.environ.get('SADX_SOURCE_ROOT', Path(__file__).resolve().parents[2]))
sys.path[:0] = [str(ROOT/'tools/lib'), str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPSW, UC_X86_REG_FPCW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP
ARGS = DATA + 0x1000
CALLS = {0x403070:1, 0x432f40:2, 0x506460:3, 0x433f80:4}
class Source:
    def __init__(self, image):
        self.m = Original(image,127).machine
        for section in image.sections:
            if section['name'] in ('.text','.rdata'): continue
            size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095
            self.m.mem_map(image.base+section['rva'],size)
            if section['raw_size']: self.m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
        for address in CALLS: self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=address,end=address)
        self.initial=self.m.context_save()
    def word(self,a): return int.from_bytes(self.m.mem_read(a,4),'little')
    def put(self,a,v): self.m.mem_write(a,struct.pack('<I',v))
    def hook(self,m,a,size,user):
        kind=CALLS[a]; sp=m.reg_read(UC_X86_REG_ESP)
        if kind==1:
            assert self.word(sp+4)==0x10d7c78
            row=[kind,19]
        elif kind==2: row=[kind,self.word(sp+4)]
        elif kind==3:
            row=[kind]; m.reg_write(UC_X86_REG_EAX,self.v[1])
        else: row=[kind]+[self.word(sp+4+n*4) for n in range(6)]
        self.trace.append(row+[0]*(8-len(row)))
        m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
    def run(self,v):
        self.v=v; self.trace=[]; self.m.context_restore(self.initial)
        self.m.reg_write(UC_X86_REG_FPCW,v[2])
        self.m.mem_write(ARGS,struct.pack('<5I',*v[3:8]))
        for a,n in ((0x8928c0,8),(0x8928c4,9),(0x10d7e14,10),(0x10d7e18,11),(0x10d7e1c,12),(0x10d7e20,13)):self.put(a,v[n])
        self.m.mem_write(STACK-256,struct.pack('<2I',STOP,ARGS));self.m.reg_write(UC_X86_REG_ESP,STACK-256)
        self.m.emu_start(0x50b590 if v[0] else 0x50b4e0,STOP,count=500)
        assert self.m.reg_read(UC_X86_REG_EIP)==STOP
        assert self.m.reg_read(UC_X86_REG_ESP)==STACK-252
        out=[0]*64;out[:2]=[len(self.trace),self.m.reg_read(UC_X86_REG_FPSW)&63]
        for n,row in enumerate(self.trace):out[8+n*8:16+n*8]=row
        return out

def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def vectors(image,seed,count):
    rng=random.Random(seed); layout=list(struct.unpack('<4I',image.read(0x10d7e14,16))); rows=[]
    def base(mode,optional,cw): return [mode,optional,cw]+list(map(bits,(320,240,20,1,1,1,1)))+layout
    for mode in (0,1):
        for optional in (0,1,0xffffffff):
            for cw in (127,639,895,1151,2175,3199):
                rows.append(base(mode,optional,cw))
                for index in range(3,14):
                    for value in (0.0,-0.0,-1.5,0.33333334,1e-30,1e30):
                        v=base(mode,optional,cw);v[index]=bits(value);rows.append(v)
    for _ in range(count):
        v=base(rng.randrange(2),rng.choice((0,1,0xffffffff)),rng.choice((127,639,895,1151,2175,3199)))
        for index in range(3,14):v[index]=bits(rng.uniform(-500,500) if index<6 else rng.uniform(-2,3))
        rows.append(v)
    return rows

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=506640);p.add_argument('--cases',type=int,default=1200);a=p.parse_args()
    image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256
    source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_main_menu_content_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2
    rows=vectors(image,a.seed,a.cases);bad=[];flags=0
    for n,v in enumerate(rows):
        expected=source.run(v);out=(ctypes.c_uint32*64)();probe((ctypes.c_uint32*14)(*v),out)
        diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if i!=1 and x!=y];flags+=expected[1]!=out[1]
        if diff:
            bad.append({'case':n,'input':v,'diff':diff})
            if len(bad)==8:break
    stage=Path(__file__).resolve().parents[2]
    paths=['src/game/main_menu_content.hpp','src/game/main_menu_content.cpp','tests/probes/native_main_menu_content_probe.cpp']
    report={'source_sha256':image.sha256,'source_cluster_sha256':hashlib.sha256(image.read(0x50b4e0,0x143)).hexdigest(),'passed':not bad and not flags,'values_match':not bad,'cases':len(rows),'executed':n+1,'mismatches':len(bad),'fp_flag_mismatches':flags,'first_mismatches':bad,'native_sources':{p:hashlib.sha256((stage/p).read_bytes()).hexdigest() for p in paths},'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'limits':['Entire original 50b4e0/50b590 instructions run. Only actual select-texture, material, optional-entry predicate and scaled-draw callees are receipts. Eligibility predicate is independently recovered by its owner. Canonical imported four-float trial layout plus live mutation cases are exercised. Unicorn 2.1.4 reports different x87 precision flags from the native exact context; values and flags remain separate. General unmasked exception delivery and final rendered pixels remain unqualified.']}
    Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or flags)
if __name__=='__main__':raise SystemExit(main())
