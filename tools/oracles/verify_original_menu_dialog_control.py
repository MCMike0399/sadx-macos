"""Actual close/idle/quick-open/advanced-wrapper entries, with real cursor/open callees."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK=DATA+0x1000;WORK=DATA+0x2000;OTHER=DATA+0x3000;OTHERWORK=DATA+0x4000;SOUNDS=DATA+0x5000;DISABLED=DATA+0x6000;TABLE=0x7ee328;OWNER=0x3b2c588
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine;self.m.mem_map(OWNER&~4095,4096);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=0x423d70,end=0x423d70);self.initial=self.m.context_save()
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
 def hook(self,m,a,size,user):
  sp=m.reg_read(UC_X86_REG_ESP);assert all(self.word(sp+4*n)==0 for n in (2,3,4));self.sound=[1,self.word(sp+4)]
  if self.v[13]:self.put(OWNER,OTHER)
  m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.sound=[0,0];self.m.context_restore(self.initial)
  for n in range(3):
   self.m.mem_write(TABLE+60*n,bytes(60));self.put(TABLE+60*n+56,v[9],1);self.put(TABLE+60*n+16,SOUNDS+n*20 if v[10] else 0);self.put(SOUNDS+n*20,v[11]+n)
  self.m.mem_write(WORK,bytes(68));self.m.mem_write(OTHERWORK,bytes(68));self.put(TASK+44,WORK);self.put(OTHER+44,OTHERWORK);self.put(OWNER,TASK)
  self.put(WORK+20,v[15]);self.put(WORK,TABLE);self.put(WORK+4,v[1]);self.put(WORK+36,v[2],2);self.put(WORK+38,v[3],2);self.put(WORK+32,v[4]);self.put(WORK+40,v[5],1);self.put(WORK+60,v[6]);self.m.mem_write(WORK+42,bytes(x&255 for x in v[16:32]));self.m.mem_write(DISABLED,bytes(x&255 for x in v[32:48]))
  disabled={0:0,1:DISABLED,2:WORK+42,3:WORK+43}[v[12]];op=v[0]
  args=() if op<2 else (v[14],) if op==2 else (v[7] if op<5 else TABLE+60*v[7],v[8],disabled)
  addr=[0x432580,0x4325b0,0x4325c0,0x5057d0,0x5057f0,0x432cb0][op]
  self.m.mem_write(STACK-256,struct.pack('<'+'I'*(1+len(args)),STOP,*args));self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.emu_start(addr,STOP,count=500000)
  assert self.m.reg_read(UC_X86_REG_EIP)==STOP and self.m.reg_read(UC_X86_REG_ESP)==STACK-252
  out=[0]*64;out[0]=self.m.reg_read(UC_X86_REG_EAX) if op==0 else self.m.reg_read(UC_X86_REG_EAX)&65535 if op==1 else 0
  out[1:8]=[self.word(WORK+4),self.word(WORK+36,2),self.word(WORK+38,2),self.word(WORK+32),self.word(WORK+40,1),self.word(WORK+60),(self.word(WORK)-TABLE)//60]
  out[8:24]=list(self.m.mem_read(WORK+42,16));out[32:34]=self.sound;out[34]=int(self.word(OWNER)==OTHER);out[35]=self.word(WORK+20);out[36]=1;return out

def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 def base(op):
  v=[0]*48;v[:15]=[op,2,163,17,0x3f800000,1,0xffffffff,1,0,3,1,25,0,0,44];v[16:32]=[2,0,1,255]+[255]*12;v[32:48]=[0,1,255]+[255]*13;return v
 for op in range(6):
  for phase in (0,1,2,3,4,5,0x7fffffff,0x80000000,0xffffffff):
   for frames in (0,1,32767,32768,65535):
    v=base(op);v[1]=phase;v[2]=frames;rows.append(v)
 for op in (3,4,5):
  for cursor in (0,1,2,3,127,128,255):
   for choices in (0,1,2,3,127,128,255):
    for mode in range(4):
     v=base(op);v[8]=cursor;v[9]=choices;v[12]=mode;rows.append(v)
 for _ in range(count):
  v=base(rng.randrange(6));v[1]=rng.getrandbits(32);v[2]=rng.getrandbits(16);v[3]=rng.getrandbits(16);v[4]=rng.getrandbits(32);v[5]=rng.getrandbits(8);v[6]=rng.getrandbits(32);v[7]=rng.randrange(3);v[8]=rng.getrandbits(8);v[9]=rng.choice((0,1,3,6,127,255));v[10]=rng.randrange(2);v[11]=rng.getrandbits(32);v[12]=rng.randrange(4);v[13]=rng.randrange(2);v[14]=rng.getrandbits(32)
  for start in (16,32):
   length=rng.randrange(15);v[start:start+16]=[rng.randrange(8) for _ in range(length)]+[255]+[rng.getrandbits(8) for _ in range(15-length)]
  if v[12]==3:v[31]=255
  rows.append(v)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=432580);p.add_argument('--cases',type=int,default=1200);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_menu_dialog_control_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;original_rows=vectors(a.seed,a.cases);rows=[v[:] for v in original_rows];rows += [v[:15]+[0x3fc00000]+v[16:] for v in rows];bad=[]
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*64)();probe((ctypes.c_uint32*48)(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 stage=Path(__file__).resolve().parents[2];paths=['src/game/menu_dialog_control.hpp','src/game/menu_dialog_control.cpp','tests/probes/native_menu_dialog_control_probe.cpp'];ranges=[(0x432580,40),(0x4325b0,13),(0x4325c0,18),(0x432cb0,110),(0x5057d0,22),(0x5057f0,22)]
 report={'passed':not bad,'values_match':not bad,'cases':len(rows),'executed':n+1,'mismatches':len(bad),'first_mismatches':bad,'source_sha256':image.sha256,'source_ranges':{hex(x):hashlib.sha256(image.read(x,n)).hexdigest() for x,n in ranges},'native_sources':{p:hashlib.sha256((stage/p).read_bytes()).hexdigest() for p in paths},'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'limits':['All six actual source entries plus full SetDialogCursor and normal open execute; only actual sound callee423d70 is a receipt. AX-only source idle result compares semantic16-bit return. Real resource choices include aliases/overlap and audio callback canonical-owner mutation. Missing task/work and invalid resource definitions remain explicit native errors; successful original executions require valid owners and terminated bounded choices. No native visible MainMenu/gameplay claim.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
