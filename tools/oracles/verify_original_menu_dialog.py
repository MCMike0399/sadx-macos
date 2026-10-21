"""Original menu dialog request/open/cursor/result with real task work and sound receipts."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK=DATA+0x1000;WORK=DATA+0x2000;OTHER=DATA+0x3000;OTHERWORK=DATA+0x4000;SOUNDS=DATA+0x5000;DISABLED=DATA+0x6000;TABLE=0x7ee328;OWNER=0x3b2c588
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine;self.m.mem_map(OWNER&~4095,4096);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
 def hook(self,m,a,size,user):
  if a!=0x423d70:return
  sp=m.reg_read(UC_X86_REG_ESP);assert all(self.word(sp+4*i)==0 for i in (2,3,4));self.sound=[1,self.word(sp+4)]
  if self.v[45]:self.stopped=True;m.emu_stop();return
  if self.v[44]:self.put(OWNER,OTHER)
  m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def invoke(self,address,args=(),cursor=0):
  self.m.mem_write(STACK-0x100,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.reg_write(UC_X86_REG_EDX,cursor&255);self.m.emu_start(address,STOP,timeout=0);assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
  return self.m.reg_read(UC_X86_REG_EAX)
 def run(self,v):
  self.v=v;self.m.context_restore(self.initial);self.sound=[0,0];self.stopped=False
  for n in range(3):
   self.m.mem_write(TABLE+60*n,bytes(60));self.put(TABLE+60*n+56,v[5],1);self.put(TABLE+60*n+16,SOUNDS+n*20 if v[6] else 0);self.put(SOUNDS+n*20,v[7]+n)
  self.m.mem_write(WORK,bytes(68));self.m.mem_write(OTHERWORK,bytes(68));self.put(TASK+44,WORK if v[58] else 0);self.put(OTHER+44,OTHERWORK);self.put(OTHERWORK+4,v[46]);self.put(OTHERWORK+40,v[47],1);self.put(OWNER,TASK if v[57] else 0)
  self.put(WORK,TABLE+60*(v[8]%3));self.put(WORK+4,v[1]);self.put(WORK+40,v[2],1);self.put(WORK+60,v[3]);self.put(WORK+36,v[4],2);self.put(WORK+38,v[49],2);self.m.mem_write(WORK+42,bytes(x&255 for x in v[28:44]));self.m.mem_write(DISABLED,bytes(x&255 for x in v[12:28]));disabled={0:0,1:DISABLED,2:WORK+42,3:WORK+43}[v[11]]
  returned=0
  if v[0]==0:self.invoke(0x432d20,(TABLE,v[10]&255,disabled))
  elif v[0]==1:self.invoke(0x432db0,(TABLE,))
  elif v[0]==2:self.invoke(0x5057b0,(v[9],))
  elif v[0]==3:self.invoke(0x4325e0,cursor=v[10])
  else:returned=self.invoke(0x432550)
  out=[0]*256;out[:8]=[(self.word(WORK)-TABLE)//60+1,returned,int(self.stopped),self.word(WORK+4),self.word(WORK+40,1),self.word(WORK+60),self.word(WORK+36,2),self.word(WORK+38,2)];out[8:24]=list(self.m.mem_read(WORK+42,16));out[80:82]=self.sound
  self.stopped=False;out[24]=self.invoke(0x432550);out[25]=0 if self.word(OWNER)==0 else 1 if self.word(OWNER)==TASK else 2
  return out

def vectors(seed,count):
 rng=random.Random(seed);result=[]
 def base(op,cursor=0,choices=3):
  v=[0]*64;v[:12]=[op,0,53,0xffffffff,99,choices,1,21,2,1,cursor,0];v[12:28]=[0xffffffff]*16;v[28:44]=[0xffffffff]*16;v[46]=6;v[47]=53;v[49]=15;v[57]=v[58]=1;return v
 for op in range(5):
  for cursor in (0,1,2,3,127,128,255):
   for choices in (0,1,2,3,127,128,255):
    for mode in (0,1,2,3):
     v=base(op,cursor,choices);v[11]=mode;v[12:15]=[0,1,255];v[28:32]=[2,0,1,255];result.append(v)
 for phase in (0,1,4,5,6,0x7fffffff,0xffffffff,0x80000000):
  for task in (0,1):
   for work in (0,1):
    v=base(4);v[1]=phase;v[2]=rng.getrandbits(8);v[57]=task;v[58]=work;result.append(v)
 for op in (0,1,2):
  for stop in (0,1):
   for replace in (0,1):
    v=base(op);v[44]=replace;v[45]=stop;result.append(v)
 for _ in range(count):
  op=rng.randrange(5);v=base(op,rng.randrange(256),rng.choice([0,1,2,3,4,6,127,255]));v[1]=rng.getrandbits(32);v[2]=rng.getrandbits(8);v[3]=rng.getrandbits(32);v[4]=rng.getrandbits(16);v[6]=rng.randrange(2);v[7]=rng.getrandbits(32);v[8]=rng.randrange(3);v[9]=rng.randrange(3);v[11]=rng.randrange(4);v[44]=rng.randrange(2);v[45]=rng.randrange(2);v[49]=rng.getrandbits(16)
  for start in (12,28):
   length=rng.randrange(15);v[start:start+16]=[rng.randrange(8) for _ in range(length)]+[255]+[rng.getrandbits(8) for _ in range(15-length)]
  if v[11]==3:v[43]=255
  result.append(v)
 return result

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1000);p.add_argument('--seed',type=int,default=432520);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_menu_dialog_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;cases=vectors(a.seed,a.cases);bad=[]
 for n,v in enumerate(cases):
  expected=source.run(v);out=(ctypes.c_uint32*256)();probe((ctypes.c_uint32*64)(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];paths=['src/game/menu_dialog.hpp','src/game/menu_dialog.cpp','src/game/title_menu.hpp','tests/probes/native_menu_dialog_probe.cpp'];ranges=[(0x4325e0,0x66),(0x432d20,0x86),(0x432db0,0x11),(0x5057b0,0x15),(0x432550,0x2d)]
 report={'passed':not bad,'cases':len(cases),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'source_ranges':{f'0x{x:x}-0x{x+n:x}':hashlib.sha256(image.read(x,n)).hexdigest() for x,n in ranges},'limits':['Full request/open/cursor/result bodies execute. Parameters/disabled bytes are resource inputs; audio is an ordered receipt. Disabled-list aliasing, missing result-owner, and sound-callback replacement of canonical task are compared. Invalid missing request owners or unterminated/overflowing choice resources are explicit native boundaries, not successful original executions.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
