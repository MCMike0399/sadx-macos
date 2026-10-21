"""Full source512540 setter and505e60 routing, through actual music-shutdown callee receipt."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]));sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
CHAR=DATA+0x1000;SEQ=DATA+0x2000;OTHERCHAR=DATA+0x3000;OTHERSEQ=DATA+0x4000;CW=DATA+0x5000;SW=DATA+0x6000;OCW=DATA+0x7000;OSW=DATA+0x8000
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine;self.m.mem_map(0x3c5e000,8192);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=0x505870,end=0x505870);self.initial=self.m.context_save()
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 def hook(self,m,a,size,user):
  self.receipt=[1,self.word(CW+8),self.word(SW+30,1),self.word(SW+24)]
  if self.v[6]:self.put(0x3c5e8d0,OTHERSEQ)
  if self.v[7]:self.put(0x3c5ff08,OTHERCHAR)
  sp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.receipt=[0]*4;self.m.context_restore(self.initial)
  for a,w in [(CHAR,CW),(SEQ,SW),(OTHERCHAR,OCW),(OTHERSEQ,OSW)]:self.m.mem_write(w,bytes(64));self.put(a+44,w)
  self.put(0x3c5ff08,CHAR);self.put(0x3c5e8d0,SEQ)
  for a,i,n in [(CW+8,2,4),(SW+30,3,1),(SW+24,4,4),(OCW+8,8,4),(OSW+30,9,1),(OSW+24,10,4)]:self.put(a,v[i],n)
  self.m.mem_write(STACK-256,struct.pack('<2I',STOP,v[1]));self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.emu_start(0x505e60 if v[0] else 0x512540,STOP,count=1000)
  assert self.m.reg_read(UC_X86_REG_EIP)==STOP and self.m.reg_read(UC_X86_REG_ESP)==STACK-252
  out=[0]*32;out[:8]=[self.word(CW+8),self.word(SW+30,1),self.word(SW+24),self.word(OCW+8),self.word(OSW+30,1),self.word(OSW+24),int(self.word(0x3c5ff08)==OTHERCHAR),int(self.word(0x3c5e8d0)==OTHERSEQ)];out[16:20]=self.receipt;return out

def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 for op in (0,1):
  for next_mode in (0,1,3,8,9,255,0x7fffffff,0x80000000,0xffffffff):
   for replace in range(4):
    for waiting in (0,1,127,255):rows.append([op,next_mode,7,waiting,12,0,replace&1,replace>>1,25,77,13])
 for _ in range(count):
  v=[rng.getrandbits(32) for _ in range(11)];v[0]=rng.randrange(2);v[3]=rng.getrandbits(8);v[6]=rng.randrange(2);v[7]=rng.randrange(2);v[9]=rng.getrandbits(8);rows.append(v)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=505660);p.add_argument('--cases',type=int,default=1200);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_menu_navigation_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;rows=vectors(a.seed,a.cases);bad=[]
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*32)();probe((ctypes.c_uint32*11)(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 stage=Path(__file__).resolve().parents[2];paths=['src/game/menu_navigation.hpp','src/game/menu_navigation.cpp','tests/probes/native_menu_navigation_probe.cpp']
 report={'passed':not bad,'values_match':not bad,'cases':len(rows),'executed':n+1,'mismatches':len(bad),'first_mismatches':bad,'source_sha256':image.sha256,'source_ranges':{hex(x):hashlib.sha256(image.read(x,n)).hexdigest() for x,n in [(0x512540,16),(0x505e60,41)]},'native_sources':{p:hashlib.sha256((stage/p).read_bytes()).hexdigest() for p in paths},'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'limits':['Both actual source entries execute512540 within505e60. Only actual505870 menu music shutdown is receipt, recording visible pre-call fields and mutating canonical task owners. Captured sequence work remains destination after callback owner replacement. Missing task/work fail explicitly in native boundary tests; successful source runs use valid owners. No CharacterSelect initialization/render/gameplay claim.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
