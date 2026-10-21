"""Original complete dialog phases and normal menu navigation; native resource seams are receipts."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK,WORK,PARAMS,SOUNDS=DATA+0x1000,DATA+0x2000,DATA+0x3000,DATA+0x4000
CALLS={0x423d70:1,0x42c4e0:2,0x402f10:3,0x432f40:4,0x433010:5,0x4128f0:6,0x4030d0:7}
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for a in (0x3b2c588,0x3b1118c,0x3b0e354):
   page=a&~4095
   if not any(x<=page<=y for x,y,_ in self.m.mem_regions()):self.m.mem_map(page,4096)
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,x,n=4):self.m.mem_write(a,(x&((1<<(8*n))-1)).to_bytes(n,'little'))
 def hook(self,m,a,size,user):
  if a not in CALLS:return
  sp=m.reg_read(UC_X86_REG_ESP);kind=CALLS[a];arg=0
  if kind==1:
   arg=self.word(sp+4);assert all(self.word(sp+n)==0 for n in (8,12,16))
  elif kind==2:assert self.word(self.word(sp+4))==0
  elif kind==3:
   arg=self.word(sp+4);assert arg==self.word(sp+8)==self.word(sp+12)
  elif kind in (4,5):arg=self.word(sp+4)
  elif kind==7:kind=7 if self.word(sp+4)==0 else 8;arg=self.word(sp+8)
  self.trace.extend([kind,arg]);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.trace=[];self.m.context_restore(self.initial);self.m.mem_write(WORK,bytes(68));self.m.mem_write(PARAMS,bytes(60));self.put(TASK+44,WORK);self.put(0x3b2c588,TASK);self.put(0x3b1118c,v[2]);self.put(0x3b0e354,v[13]);self.put(WORK,PARAMS);self.put(PARAMS,v[1]);self.put(PARAMS+16,SOUNDS if v[11] else 0);self.put(PARAMS+56,v[9],1);self.put(PARAMS+57,v[10],1);self.m.mem_write(SOUNDS,struct.pack('<5I',21,22,1,2,3))
  for off,index,n in [(4,0,4),(32,3,4),(20,4,4),(40,5,1),(60,6,4),(38,7,2),(36,8,2),(64,12,4),(8,14,4),(24,15,4),(28,16,4),(12,17,4),(16,18,4)]:self.put(WORK+off,v[index],n)
  self.m.mem_write(WORK+42,bytes(x&255 for x in v[20:36]));self.m.mem_write(STACK-256,struct.pack('<2I',STOP,TASK));self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.emu_start(0x432720,STOP,timeout=0);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
  o=[0]*128;o[:13]=[self.word(WORK+4),self.word(WORK+32),self.word(WORK+20),self.word(WORK+41,1),self.word(WORK+24),self.word(WORK+28),self.word(WORK+8),self.word(WORK+12),self.word(WORK+16),self.word(WORK+36,2),self.word(WORK+38,2),self.word(WORK+40,1),self.word(WORK+60)];o[13:29]=self.m.mem_read(WORK+42,16);o[46]=self.m.reg_read(UC_X86_REG_FPSW)&63;o[47]=len(self.trace)//2;o[48:48+len(self.trace)]=self.trace;return o

def cases(seed,count):
 rng=random.Random(seed);rows=[]
 def base(phase,style):
  v=[0]*40;v[:19]=[phase,style,1,0,0x3f800000,0,0xffffffff,0,0,3,1,1,1,0,0,0,0,0,0];v[20:36]=[255]*16;return v
 for phase in range(6):
  for style in (0,1,3,4,5):
   for ticks in (0,1,2,3,0x7fffffff,0xffffffff,0x80000000):
    for frame in (0,3,14,15):
     v=base(phase,style);v[2]=ticks;v[7]=frame;rows.append(v)
 for _ in range(count):
  v=base(rng.randrange(6),rng.choice((0,1,3,4,5)));v[2]=rng.choice((0,1,2,3,0xffffffff));v[3]=rng.getrandbits(32);v[4]=rng.choice((0,0x3f800000,0x3fc00000,0x40000000));v[5]=rng.randrange(3);v[6]=rng.choice((0xffffffff,0,1,2));v[7]=rng.randrange(16);v[8]=rng.getrandbits(16);v[11]=rng.randrange(2);v[12]=rng.getrandbits(32);v[13]=rng.choice((0,4,8,2,1024,16,32,64,128,12,1026));v[14]=rng.getrandbits(32);v[17]=rng.getrandbits(32);v[18]=rng.getrandbits(32);v[20:23]=[rng.randrange(3),rng.randrange(3),255];rows.append(v)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=432720);p.add_argument('--cases',type=int,default=1000);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.library);native=lib.sadx_native_menu_dialog_update_probe;native.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*3;table=(ctypes.c_uint32*76)(*struct.unpack('<76I',image.read(0x7dfe20,304)));rows=cases(a.seed,a.cases);bad=[];flags=0
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*128)();native((ctypes.c_uint32*40)(*v),out,table);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if i!=46 and x!=y];flags+=expected[46]!=out[46]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];paths=['src/game/menu_dialog.hpp','src/game/menu_dialog.cpp','src/game/title_menu.hpp','tests/probes/native_menu_dialog_update_probe.cpp'];report={'source_sha256':image.sha256,'source_cluster_sha256':hashlib.sha256(image.read(0x431960,0x1470)).hexdigest(),'passed':not bad and not flags,'values_match':not bad,'cases':len(rows),'mismatches':len(bad),'fp_flag_mismatches':flags,'first_mismatches':bad,'seed':a.seed,'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'limits':['Style2 nearest-point navigation remains an explicit native boundary; reached FileSelect style5 and other linear/threeway styles execute fully. Renderer/caption resources are separate pending clusters.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or flags)
if __name__=='__main__':raise SystemExit(main())
