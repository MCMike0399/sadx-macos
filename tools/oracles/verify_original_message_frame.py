"""Original full MSG frame producer, with lower renderer/resource calls as receipts.

The actual401070 sprite copy/centering instructions execute. Uninitialized source
sprite z/angle words are not compared: attributes34 never consumes them. Queue
identities and reentry are compared; native type2 consumption is a separate proof.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ESI,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
CONTEXTS=[DATA+0x1000+n*0x1000 for n in range(4)];ANIM=DATA+0x8000
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for page in (0x3abe000,0x3b0e000,0x3b2c000,0x3ab9000,0x38f6000,0x892000):self.m.mem_map(page,4096)
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
 def put(self,address,value,n=4):self.m.mem_write(address,(value&((1<<(8*n))-1)).to_bytes(n,'little'))
 def word(self,address,n=4):return int.from_bytes(self.m.mem_read(address,n),'little')
 def event(self,tag,*args):self.events.append([tag,*args]+[0]*(7-len(args)))
 def mutate_material(self):
  if self.v[10]==2:
   self.put(self.current,3,1);self.put(self.current+2,777,2);self.put(self.current+4,-777,2);self.put(self.current+36,0x40000000);self.put(0x8928c0,0x3fc00000);self.put(0x8928c4,0x40200000);self.put(0x3b0e338,0)
 def hook(self,m,address,size,user):
  if address==0x40d527:self.current=m.reg_read(UC_X86_REG_ESI);return
  if address==0x40d799:
   self.event(13,*(self.word(0x38f6ce4+x) for x in (0,4,12,16)));return
  tags={0x402f00:1,0x402ed0:2,0x77ed00:3,0x77ed20:4,0x433170:5,0x4030d0:6,0x432f90:7,0x4343e0:8,0x4071c0:9,0x40d2a0:10,0x403d30:11,0x402f40:12,0x404660:14,0x40bcf0:16}
  if address not in tags:return
  sp=m.reg_read(UC_X86_REG_ESP);arg=lambda n:self.word(sp+n*4)
  if address==0x402f00:self.event(1);m.reg_write(UC_X86_REG_EAX,self.word(0x3ab98ec))
  elif address==0x402ed0:
   self.event(2,arg(1));self.put(0x3ab98ec,arg(1))
   if self.v[10]==4:self.put(0x3b2cbb4,self.v[7])
  elif address in (0x77ed00,0x77ed20):self.event(tags[address],arg(1))
  elif address==0x433170:self.event(5)
  elif address==0x4030d0:self.event(6,arg(1),arg(2))
  elif address==0x432f90:self.event(7,*(arg(n) for n in range(1,5)))
  elif address==0x4343e0:self.event(8,*(arg(n) for n in range(1,6)))
  elif address==0x4071c0:self.event(9,*(arg(n) for n in range(1,7)))
  elif address==0x40d2a0:
   c=arg(1);self.event(10,CONTEXTS.index(c));self.put(c+48,c+56 if self.v[8] else 0);self.put(c+68,self.word(c+68)&~2)
  elif address==0x403d30:
   self.event(11,arg(1));m.reg_write(UC_X86_REG_EAX,ANIM+self.allocated*32);self.allocated+=1
   if self.v[10]==1:self.put(self.current+10,257,2);self.put(self.current+12,129,2)
  elif address==0x402f40:self.event(12,*(self.word(arg(1)+x) for x in (0,4,8,12)));self.mutate_material()
  elif address==0x404660:
   sprite=arg(1);self.event(14,*(self.word(sprite+x) for x in (0,4,12,16)),arg(3),arg(4),arg(5));animation=self.word(sprite+28);self.event(15,*(self.word(animation+x) for x in (0,4,8,12,16)),arg(2))
  elif address==0x40bcf0:
   self.event(16,self.word(0x3b0e338))
   if self.v[10]==3:self.put(0x3b0e338,1)
  m.reg_write(UC_X86_REG_EIP,arg(0));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.events=[];self.allocated=0;self.current=0;m=self.m;m.context_restore(self.initial);m.reg_write(UC_X86_REG_FPCW,v[5]);m.reg_write(UC_X86_REG_FPSW,0)
  for a,x in [(0x3b0e338,v[0]),(0x3b2cbb4,v[1]),(0x3ab98ec,v[2]),(0x8928c0,v[3]),(0x8928c4,v[4])]:self.put(a,x)
  fields=[(0,1),(2,2),(4,2),(6,2),(8,2),(10,2),(12,2),(36,4),(28,4),(32,4),(68,4)]
  for n,c in enumerate(CONTEXTS):
   m.mem_write(c,bytes(72));row=v[16+n*16:32+n*16]
   for index,(offset,size) in enumerate(fields):self.put(c+offset,row[index],size)
   self.put(c+48,c+56 if row[11] else 0);self.put(0x3abe13c+n*4,0 if v[6]&(1<<n) else c)
  m.mem_write(STACK-256,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK-256);m.emu_start(0x40d4d0,STOP,timeout=0);assert m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*1024;out[:5]=[len(self.events),self.word(0x3b2cbb4),self.word(0x3ab98ec),self.word(0x3b0e338),m.reg_read(UC_X86_REG_FPSW)&63]
  for n,c in enumerate(CONTEXTS):out[16+n*3:19+n*3]=[self.word(c,1),self.word(c+68),int(bool(self.word(c+48)))]
  for n,event in enumerate(self.events):out[64+n*8:72+n*8]=event
  return out

def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
 def base():
  v=[0]*80;v[:11]=[1,0,0x31c7,bits(1),bits(1),127,0,0,1,0,0]
  for n in range(4):v[16+n*16:28+n*16]=[n,460,40,140,80,256,128,bits(1),0xffffffff,0,3,1]
  return v
 for kind in range(4):
  for attached in (0,1):
   for background in (0,0xffffff,0x80000000,0xffaabbcc):
    for mutation in range(5):
     v=base();v[16]=kind;v[27]=attached;v[25]=background;v[10]=mutation;rows.append(v)
 for count_ in range(5):
  for nulls in range(16):
   v=base();v[0]=count_;v[6]=nulls;rows.append(v)
 for saved in (0,1,12743,0xffffffff):
  v=base();v[2]=saved;rows.append(v)
 for _ in range(count):
  v=base();v[0]=rng.randrange(5);v[2]=rng.getrandbits(16);v[3]=bits(rng.choice([0,0.5,1,1.3,1.5,2]));v[4]=bits(rng.choice([0,0.75,1,1.3,2]));v[5]=rng.choice([127,639,895,3711]);v[6]=rng.randrange(16);v[7]=rng.getrandbits(16);v[8]=rng.randrange(2);v[10]=rng.randrange(5)
  for n in range(4):v[16+n*16:28+n*16]=[rng.randrange(4),rng.getrandbits(16),rng.getrandbits(16),rng.getrandbits(16),rng.getrandbits(16),rng.getrandbits(16),rng.getrandbits(16),bits(rng.choice([0,0.5,1,1.3,2])),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(2)]
  rows.append(v)
 return rows
def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=404004);p.add_argument('--cases',type=int,default=1000);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_message_frame_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;rows=vectors(a.seed,a.cases);bad=[];partials=0
 for index,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*1024)();probe((ctypes.c_uint32*80)(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y and i!=4];partials+=int(expected[4]!=out[4])
  if diff:bad.append({'case':index,'input':v,'diff':diff[:32]})
  if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];files=['src/game/message_frame.hpp','src/game/message_frame.cpp','tests/probes/native_message_frame_probe.cpp'];report={'passed':not bad and not partials,'values_match':not bad,'cases':len(rows),'mismatches':len(bad),'fp_partials':partials,'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'native_sources':{f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files},'limits':[__doc__]};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or partials)
if __name__=='__main__':raise SystemExit(main())
