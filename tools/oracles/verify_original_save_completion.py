"""Original503600 stages12/13 completion suffix, after unchanged shared animation.

Actual wait/sign/store/transition instructions execute. Existing dialog/task/save
owners are ordered call receipts; their independent source judges remain required.
No successful-save flag is manufactured by this judge or the native helper.
"""
import argparse,ctypes,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EIP,UC_X86_REG_ESP
from verify_original_save_creation import Source as CaptureSource
from verify_original_math import STACK,STOP,DATA
from pe_image import PEImage
from extract import SOURCE_SHA256
FIELDS=[(0x3abdf78,1),(0x3b291b1,1),(0x3b2c58c,1),(0x3b291a4,4),(0x3b291ad,1),(0x3abdf76,1),(0x3b22e1e,1),(0x3b290d8,1),(0x3c5e8c4,2),(0x3b29198,4)]
class Source(CaptureSource):
 def __init__(self,image):
  super().__init__(image)
  for a in [0x5057b0,0x421ab0,0x432550,0x421bd0,0x505b40,0x5065d0,0x42d770]:self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=a,end=a)
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 def get(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def hook(self,m,a,size,user):
  sp=m.reg_read(UC_X86_REG_ESP);value=0
  if a==0x5057b0:self.calls.append(100+self.get(sp+4))
  elif a==0x421ab0:self.calls.append(2);assert self.get(sp+4)==7;value=0x2222
  elif a==0x432550:self.calls.append(3);value=self.selection
  elif a==0x421bd0:self.calls.append(4);assert self.get(sp+4) in [0x1111,0x2222]
  elif a==0x505b40:self.calls.append(5);assert self.get(sp+4)==self.next
  elif a==0x5065d0:self.calls.append(6)
  elif a==0x42d770:self.calls.append(7)
  m.reg_write(UC_X86_REG_EAX,value&0xffffffff);m.reg_write(UC_X86_REG_EIP,self.get(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  m=self.m;m.context_restore(self.clean);self.calls=[];self.selection=v[12];self.next=v[13];work=DATA+0x200
  m.mem_write(work,bytes(64));self.put(work+0x24,v[0]);self.put(work+0x30,v[1],2);self.put(work+8,v[13]);self.put(0x3c5e8a8,0x1111);self.put(0x3b0f0e8,1)
  for (a,n),value in zip(FIELDS,v[2:12]):self.put(a,value,n)
  sp=STACK-20;m.mem_write(sp,bytes(20)+struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,sp);m.reg_write(UC_X86_REG_ESI,work);m.reg_write(UC_X86_REG_EBX,0)
  m.emu_start(0x5038fa if v[0]==12 else 0x503976,STOP,timeout=1000000);assert m.reg_read(UC_X86_REG_EIP)==STOP
  return [self.get(work+0x24),self.get(work+0x30,2),*[self.get(a,n) for a,n in FIELDS],int(self.get(0x3c5e8a8)!=0x1111),len(self.calls),*self.calls]
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--cases',type=int,default=1000);p.add_argument('--seed',type=int,default=20261021);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);U=ctypes.c_uint32;fn=ctypes.CDLL(str(a.probe.resolve())).sadx_native_save_completion;fn.argtypes=[ctypes.POINTER(U),ctypes.POINTER(U)];rng=random.Random(a.seed);bad=[]
 for case in range(a.cases):
  v=[rng.choice([12,13]),rng.choice([0,1,4,5,6,32767,32768,65535]),rng.choice([0,1,2]),rng.choice([0,1,2,127,128,255]),rng.getrandbits(8),rng.getrandbits(32),rng.getrandbits(8),rng.getrandbits(8),rng.getrandbits(8),rng.getrandbits(8),rng.getrandbits(16),rng.getrandbits(32),rng.choice([0,1,2,127,128,255]),rng.randrange(14)]
  expected=source.run(v);out=(U*32)();assert fn((U*14)(*v),out)==1;observed=list(out[:len(expected)])
  if observed!=expected:bad.append({'case':case,'input':v,'expected':expected,'observed':observed})
 report={'cases':a.cases,'mismatch_count':len(bad),'values_match':not bad,'passed':not bad,'mismatches':bad[:8],'limits':__doc__,'source_sha256':image.sha256,'source_ranges':{'0x5038fa':'0x503a2a','0x503d45':'0x503d81'},'seed':a.seed};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':a.cases,'mismatches':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
