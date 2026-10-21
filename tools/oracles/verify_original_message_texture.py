"""Execute pinned loader MipGuard wrapper and original MSG texture upload.

The source texture-info/name builders execute; real lower resource load/reload
boundaries are receipts with controlled publication/reentry. No upload success
or native GPU resource delivery is claimed by this caller oracle.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
CONTEXT=DATA+0x1000;BITMAP=DATA+0x3000;RESIDENT=DATA+0x4000
DLL_SHA='3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba'
class Source:
 def __init__(self,exe,dll):
  self.m=Original(exe,127).machine
  for section in dll.sections:
   if not section['raw_size'] and not section['virtual_size']:continue
   size=(max(section['raw_size'],section['virtual_size'])+4095)&~4095
   self.m.mem_map(dll.base+section['rva'],size)
   if section['raw_size']:self.m.mem_write(dll.base+section['rva'],dll.slice(section['file_offset'],section['raw_size']))
  #32-bit FS:[0] SEH list fixture only. Every source Context is nonnull;
  #this oracle makes no null-pointer-fault claim.
  self.m.mem_map(0,4096);self.put(0x100f7ef8,0x40d2a0)
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
  self.initial=self.m.context_save()
 def put(self,address,value,n=4):self.m.mem_write(address,(value&((1<<(8*n))-1)).to_bytes(n,'little'))
 def word(self,address,n=4):return int.from_bytes(self.m.mem_read(address,n),'little')
 def hook(self,m,address,size,user):
  if address not in (0x4228e0,0x77fa10):return
  sp=m.reg_read(UC_X86_REG_ESP);self.out[9]=self.word(0x100f8ba4,1)
  if address==0x4228e0:
   self.out[8]=1;listptr=self.word(sp+4);names=self.word(listptr);info=self.word(names)
   self.out[10]=bool(names);self.out[11]=self.word(listptr+4)
   self.out[12]=bool(self.word(info));self.out[13]=self.word(info+4)
   self.out[14]=self.word(info+16);self.out[15]=self.word(info+20)
   self.out[16]=self.word(names+4);self.out[17]=self.word(names+8)
   if self.v[9]:self.put(names+8,RESIDENT)
   m.reg_write(UC_X86_REG_EAX,self.v[6]);pop=0
  else:
   self.out[8]=2;self.out[10:14]=[self.word(sp+x) for x in (4,8,12,16)];self.out[11]=bool(self.out[11]);pop=0
  self.put(CONTEXT+68,self.v[7]);self.put(0x100f8ba4,self.v[8],1)
  m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def run(self,v):
  self.v=v;self.out=[0]*32;m=self.m;m.context_restore(self.initial);m.mem_write(CONTEXT,bytes(72))
  for offset,value,n in [(40,BITMAP if v[0] else 0,4),(48,CONTEXT+56 if v[1] else 0,4),(52,v[10],4),(68,v[2],4),(14,v[3],2),(16,v[4],2),(44,v[5],4)]:self.put(CONTEXT+offset,value,n)
  self.put(0x100f8ba4,v[11],1);m.mem_write(STACK-256,struct.pack('<II',STOP,CONTEXT));m.reg_write(UC_X86_REG_ESP,STACK-256);m.emu_start(0x10037050,STOP,timeout=0);assert m.reg_read(UC_X86_REG_EIP)==STOP
  self.out[:5]=[self.word(CONTEXT+68),bool(self.word(CONTEXT+48)),self.word(0x100f8ba4,1),self.word(CONTEXT+52),int(self.word(CONTEXT+64)==RESIDENT)]
  return list(map(int,self.out))
def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 for bitmap in (0,1):
  for names in (0,1):
   for result in (0,1,0xffffffff,0x80000000,0x7fffffff):
    for policy in (0,1,2,255):rows.append([bitmap,names,3,256,128,0xd0000028,result,7,33,1,2,policy])
 for _ in range(count):rows.append([rng.randrange(2),rng.randrange(2),rng.getrandbits(32),rng.getrandbits(16),rng.getrandbits(16),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(8),rng.randrange(2),rng.randrange(5),rng.getrandbits(8)])
 return rows
def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--loader-dll',default='reference/updated-pc-reference/game/mods/.modloader/SADXModLoader.dll');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=402001);p.add_argument('--cases',type=int,default=1000);a=p.parse_args();exe=PEImage(a.exe);dll=PEImage(a.loader_dll);assert exe.sha256==SOURCE_SHA256;assert dll.sha256==DLL_SHA;source=Source(exe,dll);lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_message_texture_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;bad=[];rows=vectors(a.seed,a.cases)
 for index,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*32)();probe((ctypes.c_uint32*12)(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if diff:bad.append({'case':index,'input':v,'diff':diff})
  if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];files=['src/game/message_texture.hpp','src/game/message_texture.cpp','tests/probes/native_message_texture_probe.cpp'];report={'passed':not bad,'cases':len(rows),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':exe.sha256,'loader_sha256':dll.sha256,'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'native_sources':{f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files},'limits':[__doc__]};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
