"""Original FileSelect stage2, including source saved-character mapping and filename publication."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
WORK,LIST,NAMES,COPY,LENGTH=DATA+0x1000,DATA+0x2000,DATA+0x3000,DATA+0x4000,0x421f80
CALLS={0x432550:1,0x505b40:2,0x5057d0:3,0x644e51:4,0x645002:5,0x644df9:6,LENGTH:7}
FIELDS=[(0x10d77ad,1,1),(0x3b290d8,2,1),(0x3b2a2f9,3,1),(0x3b290e0,4,1),(0x3c5e8c4,5,2),(0x3b2c58c,6,1),(0x3b2b601,7,1),(0x3b2a2f2,8,1),(0x3b1118c,13,4)]
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for a,_,_ in FIELDS+[(0x2bcbddc,0,4),(0x3b290dc,0,4),(0x3c5e8b8,0,4)]:
   page=a&~4095
   if not any(x<=page<=y for x,y,_ in self.m.mem_regions()):self.m.mem_map(page,4096)
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
 def string(self,p):
  out=bytearray()
  while self.word(p+len(out),1):out.append(self.word(p+len(out),1))
  return bytes(out)
 def hook(self,m,a,size,user):
  if a not in CALLS:return
  kind=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP);arg=0;result=0;pop=0
  if kind==1:result=self.v[0]
  elif kind==2:
   arg=self.word(sp+4)
   if self.v[16]!=0xffffffff:self.put(0x3b290d8,self.v[16],1)
  elif kind==3:assert [self.word(sp+n) for n in (4,8,12)]==[1,1,0]
  elif kind==4:assert self.word(sp+4)==NAMES+128
  elif kind==5:arg=self.word(sp+4);result=COPY
  elif kind==6:
   assert self.word(sp+4)==COPY and self.word(0x3b290dc)==COPY
   name=self.string(self.word(sp+12));arg=name[-1];self.m.mem_write(COPY,name+b'\0')
  elif kind==7:result=len(self.string(self.word(sp+4)));pop=4
  if kind!=7:self.trace.extend([kind,arg])
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def run(self,v):
  self.v=v;self.trace=[];self.m.context_restore(self.initial);self.m.mem_write(WORK,bytes(64));self.m.mem_write(COPY,bytes(32));self.put(WORK+36,2);self.put(WORK+8,v[10]);self.put(WORK+48,v[11],2);self.put(WORK+32,v[12]);self.put(WORK+24,v[14]);self.put(WORK+44,v[15]);self.put(0x3b290dc,NAMES+128 if v[9] else 0);self.put(0x3c5e8b8,LIST);self.put(0x7db0c8,LENGTH)
  for a,index,n in FIELDS:self.put(a,v[index],n)
  for i in range(4):self.put(LIST+i*16,NAMES+i*16);self.put(LIST+i*16+12,LIST+(i+1)*16 if i<3 else 0);self.m.mem_write(NAMES+i*16,('Slot'+str(i)).encode()+b'\0')
  self.m.mem_write(NAMES+128,b'old\0');self.m.mem_write(STACK-256,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.reg_write(UC_X86_REG_EAX,WORK);self.m.emu_start(0x503600,STOP,timeout=0);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
  p=self.word(0x3b290dc);out=[0]*128;out[:13]=[self.word(WORK+36),self.word(0x3b290d8,1),self.word(0x3b2a2f9,1),self.word(0x3c5e8c4,2),self.word(0x3b2c58c,1),self.word(0x3b2a2f2,1),self.word(WORK+48,2),self.word(WORK+32),self.word(WORK+24),self.word(WORK+28),self.word(WORK+50,1),self.word(WORK+44),0 if p==0 else 2 if p==COPY else 1]
  if p==COPY:out[16:24]=struct.unpack('<8I',self.m.mem_read(COPY,32))
  out[62]=self.m.reg_read(UC_X86_REG_FPSW)&63;out[63]=len(self.trace)//2;out[64:64+len(self.trace)]=self.trace;return out

def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 for choice in (0,1,2,3,127,128,255,0xffffffff):
  for saved in range(12):
   for old in (0,1):rows.append([choice,1,2,3,3,99,1,saved,9,old,2,17,0,1,0x3f800000,0xffffffff,0xffffffff])
 for _ in range(count):rows.append([rng.choice((0,0,1,2,255)),rng.randrange(1,4),rng.randrange(1,4),rng.randrange(256),rng.randrange(256),rng.getrandbits(16),rng.randrange(256),rng.randrange(256),rng.randrange(256),rng.randrange(2),rng.randrange(13),rng.getrandbits(16),rng.getrandbits(32),rng.choice((0,1,2,0xffffffff)),rng.choice((0,0x3f800000,0x3fc00000)),rng.getrandbits(32),rng.choice((1,2,3,0xffffffff))])
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=503602);p.add_argument('--cases',type=int,default=1000);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_menu_dialog_confirmation_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;rows=vectors(a.seed,a.cases);bad=[];flags=0
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*128)();probe((ctypes.c_uint32*17)(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if i!=62 and x!=y];flags+=expected[62]!=out[62]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];paths=['src/game/menu_dialog_file_select.hpp','src/game/menu_dialog_file_select.cpp','tests/probes/native_menu_dialog_confirmation_probe.cpp'];report={'source_sha256':image.sha256,'source_ranges':{hex(a):hashlib.sha256(image.read(a,n)).hexdigest() for a,n in ((0x503600,0x848),(0x505e90,0x40),(0x5068e0,0x6b))},'passed':not bad and not flags,'values_match':not bad,'cases':len(rows),'mismatches':len(bad),'fp_flag_mismatches':flags,'first_mismatches':bad,'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'limits':['Stage2 complete branches including delete-request seam are compared. Other established stages delegate their unchanged helper; later delete/new-save stages remain explicit. Source saved-character mapping and loading_status execute, not receipts. Heap allocation/free/string copy and common transition are ordered resource receipts.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or flags)
if __name__=='__main__':raise SystemExit(main())
