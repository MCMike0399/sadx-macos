import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path.cwd()))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import *
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
W,T,S,ST= [DATA+i*4096 for i in range(1,5)]
GLOBALS=[(0x3b2a2fa,15,1),(0x3b2a2fd,16,1),(0x3b2a2fb,17,1),(0x3c5e8e0,18,4),(0x3b1118c,19,4),(0x3b1117c,20,4),(0x3b11184,21,4),(0x3b22dc0,22,2),(0x3b2a2f1,23,1),(0x3b0e354,29,4)]
CALLS={0x506010:1,0x512bc0:2,0x505990:3,0x505a40:4,0x506630:5,0x506700:6,0x5063e0:7,0x503480:8,0x413920:9,0x6fc8a0:10,0x5067e0:11,0x5067f0:12,0x5114a0:13,0x505b40:14,0x505830:15,0x5057b0:16,0x432550:17,0x505a60:18,0x512c40:19,0x512c50:20,0x414570:21,0x512c20:22,0x506e40:23,0x506e10:24,0x506040:25,0x403070:40,0x432f40:41,0x432f90:42,0x4338d0:43,0x433f80:44,0x4334f0:45,0x77ed00:46,0x401420:47,0x431c30:48,0x4319d0:49,0x5071d0:50,0x507050:51,0x512ca0:52,0x507410:53,0x433170:54,0x433010:55,0x507bb0:56}
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for a,_,_ in GLOBALS+[(0x3c5e8d0,0,0),(0x8928c0,0,0),(0x3b0e000,0,0)]:
   p=a&~4095
   if not any(x<=p<=y for x,y,_ in self.m.mem_regions()):self.m.mem_map(p,4096)
  self.m.mem_write(0x3b0e000,bytes(4096))
  from extract import TABLE_ADDRESS
  self.m.mem_write(TABLE_ADDRESS,image.read(TABLE_ADDRESS,4100))
  # Original njSin/njCos read the pinned table from rdata directly.
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.ctx=self.m.context_save()
 def get(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 def hook(self,m,a,size,user):
  if a not in CALLS:return
  if a==0x5114a0 and self.v[0]==8:return
  k=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP);arg=self.get(sp+4);r=0;x=0
  if k in (1,3,4,5,6,7,14,16,18,19,23,24,25):x=arg
  if k in (5,6,7,11,12,13,17,20):r=self.v[{5:30,6:31,7:32,11:33,12:34,13:35,17:36,20:37}[k]]
  if k==21:x=(arg&65535)|(self.get(sp+8)<<16)
  if k in (23,24):r=arg^(12345 if k==23 else 67890)
  if k>=40:
   if k==40:
    assert arg in (0x10d7c00,0x10d7c08)
    receipt=[40,0 if arg==0x10d7c00 else 1]
   elif k in (41,46,53,55,56):receipt=[k,arg]
   elif k in (47,54):receipt=[k,0]
   else:
    n={42:4,43:4,44:6,45:5,48:6,49:3,50:7,51:5,52:7}[k]
    receipt=[k]+[self.get(sp+4+4*i)for i in range(n)]
    if k==49:r=receipt[1]^receipt[2]^receipt[3]
    if k==52:r=37
   self.trace+=receipt
  else:self.trace += [k,x]
  m.reg_write(UC_X86_REG_EAX,r);m.reg_write(UC_X86_REG_EIP,self.get(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.trace=[];m=self.m;m.context_restore(self.ctx);m.mem_write(W,bytes(64));m.mem_write(S,bytes(64))
  # exact source menu work offsets, including the otherwise unused selected-file byte.
  for off,i in [(0,1),(4,2),(8,3),(12,4),(16,5),(20,6),(24,7),(28,8),(32,9),(40,11),(48,13)]:self.put(W+off,v[i])
  self.put(W+36,v[10],1);self.put(W+44,v[12],2);self.put(W+52,v[14],1)
  self.put(0x8928c0,v[39]);self.put(0x8928c4,v[40])
  self.m.mem_write(0x7efb2c,struct.pack("<7I",*v[41:48]))
  self.put(T+44,W);self.put(ST+44,S);self.put(0x3c5e8d0,ST)
  for a,i,n in GLOBALS:self.put(a,v[i],n)
  for off,i in [(4,24),(8,25),(12,26),(16,27)]:self.put(S+off,v[i])
  self.put(S+33,v[28],1);self.put(S+31,v[28]>>8,1)
  m.reg_write(UC_X86_REG_FPCW,v[38]);m.reg_write(UC_X86_REG_ESI,W);m.reg_write(UC_X86_REG_EDI,W);m.reg_write(UC_X86_REG_ECX,v[9])
  m.mem_write(STACK-256,struct.pack('<2I',STOP,T));m.reg_write(UC_X86_REG_ESP,STACK-256)
  
  try:m.emu_start([0x5114e0,0x511570,0x5122d0,0x5119f0,0x511e10,0x511e90,0x512110,0x512450,0x5114a0][v[0]],STOP,timeout=0)
  except Exception:
   print('source crash',hex(m.reg_read(UC_X86_REG_EIP)),v,flush=True);raise
  assert m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*1024;offs=[0,4,8,12,16,20,24,28,32,36,40,44,48,52]
  out[:14]=[self.get(W+off,1 if off in (36,52) else 2 if off==44 else 4) for off in offs]
  out[14:23]=[self.get(0x3b2a2fd,1),self.get(0x3b2a2fb,1),self.get(0x3b1117c),self.get(0x3b11184),self.get(0x3b22dc0,2),self.get(S+12),self.get(S+16),self.get(S+33,1),self.get(S+31,1)]
  if v[0]==8:out[29]=int(bool(m.reg_read(UC_X86_REG_EAX)))
  out[30]=m.reg_read(UC_X86_REG_FPSW)&63;out[31]=len(self.trace);out[32:32+len(self.trace)]=self.trace;return out

def cases(seed,count):
 rng=random.Random(seed);rows=[]
 def base(op):
  v=[0]*64;v[0]=op;v[2:4]=[6,8];v[4]=0x45a5a000;v[6]=0xffffffff;v[9]=2;v[14]=7;v[16]=2;v[17]=4;v[18]=1;v[19]=1;v[20:23]=[7,8,9];v[24]=1;v[26:28]=[7,103];v[28]=0x0101;v[30:36]=[1]*6;v[36]=255;v[37]=7;v[38]=127;v[39:41]=[1065353216]*2;v[41:48]=[struct.unpack("<I",struct.pack("<f",x))[0]for x in (143,122,227,102,89,209,230)];return v
 for variant in (0,1,2,3,255):
  for selected in range(7):
   for enabled in (0,1):
    v=base(0);v[15]=variant;v[9]=selected;v[30:33]=[enabled]*3;rows.append(v)
 for stage in range(12):
  for variant in (0,1,2):
   for choice in (-128,-1,0,1,2,3,4,127):
    for tick in (0,1,10,300,32767,-1):
     v=base(1);v[8]=stage;v[15]=variant;v[36]=choice&0xffffffff;v[19]=tick&0xffffffff;v[12]=299;rows.append(v)
 for op in (3,4,5,6,7,8):
  for variant in (0,1,2,3):
   for selected in range(7):
    for stage in (0,3):
     v=base(op);v[15]=variant;v[9]=selected;v[8]=stage;rows.append(v)
 for op in (1,2,5):
  for stage in (1,2):
   for progress in (0,0x3f733333,0xbf733333,0x3f800000,0xbf800000,0x7fc01234,0x7f800000,0xff800000):
    for cw in (127,575,639,895):
     v=base(op);v[8]=stage;v[11]=progress;v[38]=cw;v[5]=progress;rows.append(v)
 for _ in range(count):
  v=base(rng.randrange(9));v[1]=rng.randrange(5);v[5]=rng.choice([0,0x3f666666,0x3f800000]);v[7]=rng.getrandbits(32);v[8]=rng.randrange(12);v[9]=rng.randrange(7);v[11]=rng.choice([0,0x3f733333,0xbf733333,0x3f800000,0xbf800000]);v[12]=rng.randrange(65536);v[13]=rng.randrange(5);v[15]=rng.randrange(4);v[18]=rng.randrange(3);v[19]=rng.choice([0,1,2,10,300,0xffffffff]);v[24:26]=[rng.randrange(8),rng.randrange(8)];v[29]=rng.choice([0,64,128,192]);v[30:36]=[rng.randrange(2) for _ in range(6)];v[36]=rng.randrange(256);v[38]=rng.choice([127,575,639,895]);v[39:41]=rng.choices([1056964608,1065353216,1068149419],k=2);rows.append(v)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=511570);p.add_argument('--cases',type=int,default=1000);a=p.parse_args()
 image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;assert unicorn.__version__=="2.1.4";source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_character_select_menu_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2+[ctypes.POINTER(ctypes.c_float)]
 from extract import TABLE_ADDRESS
 sine=(ctypes.c_float*1025).from_buffer_copy(image.read(TABLE_ADDRESS,4100));bad=[];flags=0;rows=cases(a.seed,a.cases)
 for n,v in enumerate(rows):
  want=source.run(v);got=(ctypes.c_uint32*1024)();probe((ctypes.c_uint32*64)(*v),got,sine);diff=[[i,x,y]for i,(x,y)in enumerate(zip(want,got))if i!=30 and x!=y];flags+=want[30]!=got[30]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 ranges=[(0x5114e0,0x511570),(0x511570,0x5119f0),(0x5119f0,0x511e10),(0x511e10,0x511e90),(0x511e90,0x512110),(0x512110,0x5122d0),(0x5122d0,0x512438),(0x512450,0x51249b),(0x5114a0,0x5114d3)]
 stage=Path(__file__).resolve().parents[2]
 r={'source_sha256':image.sha256,'seed':a.seed,'source_ranges':{f'0x{x:x}-0x{y:x}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in ranges},'native_sources':{str(p.relative_to(stage)):hashlib.sha256(p.read_bytes()).hexdigest()for p in [stage/'src/game/character_select_menu.hpp',stage/'src/game/character_select_menu.cpp',stage/'tests/probes/native_character_select_menu_probe.cpp']},'limits':['Nine full original menu entries execute; ordered service/draw receipts replace independent callees. No GPU or real actor claim.', 'FPstatusdifferences are retained as partial; values_match is not strictpass.', 'Character name5071d0, label507050, emblems512ca0, selected voice505a60, sky507bb0 and round panel431c30 remain composition boundaries. These receipts do not prove those callees. Availability helpers512c50/5063e0/506700 have a separate unhooked original judge.', 'Existing title_menu callback annotations5122d0/512450 must migrate on integration. No shared Main writes.'], 'passed':not bad and not flags,'values_match':not bad,'tested':n+1,'cases':len(rows),'mismatches':bad,'flag_mismatches':flags};Path(a.output).write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r));return bool(bad or flags)
if __name__=='__main__':raise SystemExit(main())
