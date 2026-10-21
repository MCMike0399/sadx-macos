"""Original full style4 dialog renderer, through proven PVR/shadow-window resource seams."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK,WORK,PARAMS,PANELS=DATA+0x1000,DATA+0x2000,DATA+0x3000,DATA+0x4000
CALLS={0x403070:1,0x432f40:2,0x433010:3,0x432f90:4,0x4030d0:5,0x433f80:6,0x4343e0:7,0x5056a0:8,0x50b4e0:8,0x50b590:8}
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine;self.m.mem_map(0x3b2c000,4096);self.m.mem_map(0x892000,4096);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
 def hook(self,m,a,size,user):
  if a not in CALLS:return
  kind=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP)
  if kind==1:
   pointer=self.word(sp+4);assert pointer in (0x915060,0x10d7c78,0x915068);row=[kind,{0x915060:4,0x10d7c78:5,0x915068:6}[pointer]]
  elif kind in (2,3):row=[kind,self.word(sp+4)]
  elif kind==4:row=[kind]+[self.word(sp+4+n*4) for n in range(4)]
  elif kind==5:row=[kind,self.word(sp+4),self.word(sp+8)]
  elif kind==6:row=[kind]+[self.word(sp+4+n*4) for n in range(6)]
  elif kind==7:row=[kind]+[self.word(sp+4+n*4) for n in range(5)]
  else:
   pointer=self.word(sp+4);row=[kind]+[self.word(pointer+n*4) for n in range(5)]+[self.word(pointer+20,1)]
   if self.v[8]!=0xffffffff:self.put(0x3b2c5e4,self.v[8])
  self.trace.append(row+[0]*(8-len(row)));m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.trace=[];self.m.context_restore(self.initial);self.m.mem_write(WORK,bytes(68));self.m.mem_write(PARAMS,bytes(60));self.put(TASK+44,WORK);self.put(0x3b2c588,TASK);self.put(0x3b2c5e4,v[7]);self.put(WORK,PARAMS)
  for off,index,n in [(8,0,4),(12,1,4),(16,2,4),(20,3,4),(40,4,1),(60,5,4),(41,6,1),(24,19,4),(28,20,4)]:self.put(WORK+off,v[index],n)
  self.m.mem_write(WORK+42,bytes(x&255 for x in v[24:40]));self.put(PARAMS,4);self.put(PARAMS+4,v[59] if v[9] else 0);self.put(PARAMS+8,0x10d7c78);self.put(PARAMS+12,PANELS);self.put(PARAMS+56,v[40],1)
  for off,index in [(20,10),(24,11),(28,12),(32,13),(36,14),(40,15),(44,16),(48,17),(52,18)]:self.put(PARAMS+off,v[index])
  for n in range(v[40]):self.m.mem_write(PANELS+n*12,struct.pack('<3I',*v[41+n*3:44+n*3]))
  self.put(0x8928c0,v[21]);self.put(0x8928c4,v[22]);self.m.mem_write(STACK-256,struct.pack('<2I',STOP,TASK));self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.emu_start(0x432480,STOP,timeout=0);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*1024;out[:3]=[self.word(0x3b2c5e4),len(self.trace),self.m.reg_read(UC_X86_REG_FPSW)&63]
  for n,row in enumerate(self.trace):out[32+n*8:40+n*8]=row
  return out

def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 def base():
  v=[0]*40;v[:23]=[0xffffffff,bits(1),bits(1),bits(1),0,0xffffffff,255,0,0xffffffff,1,0x97008740,0x97008740,bits(320),bits(357),bits(20),bits(568),bits(80),bits(1.625),bits(.8),0xffff0000,0xff0000ff,bits(1),bits(1)];v[24:40]=[255]*16;return v
 for flag in (0,1,0xffffffff):
  for mutation in (0,1,0xffffffff):
   for cursor in (0,1,2,3,127):
    for visible in (0,1):
     v=base();v[7]=flag;v[8]=mutation;v[4]=cursor;v[0]=0xffffffff if visible else 0;rows.append(v)
 for _ in range(count):
  v=base();v[0]=rng.getrandbits(32);v[1]=bits(rng.choice((0,.0625,.5,1,1.2,-1)));v[2]=bits(rng.choice((0,.25,.8,1,1.6)));v[3]=bits(rng.choice((0,1,1.5,2)));v[4]=rng.randrange(4);v[5]=rng.choice((0xffffffff,0,1,2));v[6]=rng.randrange(256);v[7]=rng.randrange(2);v[8]=rng.choice((0,1,0xffffffff));v[9]=rng.randrange(2);v[10]=rng.getrandbits(32);v[11]=rng.getrandbits(32);v[14]=bits(rng.choice((1,2,20,50)));v[19]=rng.getrandbits(32);v[20]=rng.getrandbits(32);v[21]=bits(rng.choice((.5,1,1.6)));v[22]=bits(rng.choice((.5,1,1.2)));v[24:27]=[rng.randrange(3),rng.randrange(3),255];rows.append(v)
 return rows

def style4_vectors(image,seed,count):
 definitions=[]
 for identity in (2,3,4):
  raw=image.read(0x7ee328+identity*60,60);words=struct.unpack('<15I',raw)
  assert words[0]==4
  choices=raw[56]
  panels=struct.unpack('<'+str(choices*3)+'I',image.read(words[3],choices*12))
  definitions.append((choices,words,panels))
 rows=[]
 for index,base in enumerate(vectors(seed,count)):
  choices,words,panels=definitions[index%3]
  v=base+[0]*20
  v[10:19]=list(words[5:14])
  v[4]=v[4]%(choices+1)
  v[59]=words[1]
  v[40]=choices
  v[41:41+len(panels)]=panels
  rows.append(v)
 # Each original choice/previous-choice/disabled path at full scale, exact declaration geometry.
 for choices,words,panels in definitions:
  for cursor in range(choices+1):
   for previous in range(-1,choices):
    for disabled in range(-1,choices):
     v=vectors(seed,0)[0]+[0]*20
     v[0]=0xffffffff
     v[10:19]=list(words[5:14]);v[4]=cursor;v[5]=previous&0xffffffff
     v[24:40]=[255]*16
     if disabled>=0:v[24]=disabled
     v[59]=words[1]
     v[40]=choices;v[41:41+len(panels)]=panels;rows.append(v)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=432480);p.add_argument('--cases',type=int,default=1000);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_menu_dialog_square_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;rows=style4_vectors(image,a.seed,a.cases);bad=[];flags=0
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*1024)();probe((ctypes.c_uint32*len(v))(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if i!=2 and x!=y];flags+=expected[2]!=out[2]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];paths=['src/game/menu_dialog.hpp','src/game/menu_dialog_draw.cpp','tests/probes/native_menu_dialog_square_probe.cpp','src/engine/menu_square.cpp','src/engine/menu_square.hpp'];report={'source_sha256':image.sha256,'source_cluster_sha256':hashlib.sha256(image.read(0x431960,0x1470)).hexdigest(),'passed':not bad and not flags,'values_match':not bad,'cases':len(rows),'mismatches':len(bad),'fp_flag_mismatches':flags,'first_mismatches':bad,'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'limits':['Reached style4 MainMenu definitions2/3/4 panels/cursor execute fully. Existing lower PVR scaled and shadow window services are ordered receipts; caption callback is an explicit separate resource seam with live window-order mutation. Other styles/special panel resources remain explicit boundaries. MainMenu backdrop callbacks are receipts, separately owned proof.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or flags)
if __name__=='__main__':raise SystemExit(main())
