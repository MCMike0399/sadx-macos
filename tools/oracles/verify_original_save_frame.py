"""Actual GameLoop saving slices before/after411d00; resource callees are receipts.

40cba4..40cbf1 and40cc1d..40cc61 execute unmodified. The composed case executes
the actual CALL411d00 at40cc18 between them, trapping its separate timing owner.
Intervening40cbf2..40cc17 render/message owners are outside this bounded proof.
Caller EBX=0/ESI=1 are source40cad0 prologue facts, preserved by callee ABI.
Callbacks can replace all live fields or stop, testing exact stores/reloads/failure
prefixes. Actual MSGC identity is preserved; neither message rendering nor disk I/O
is faked as a production implementation by these oracle receipts.
"""
import argparse,ctypes,hashlib,itertools,json,random,struct,sys
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EBX,UC_X86_REG_ESI
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK
FIELDS=[(0x3b291ad,1),(0x3b291b2,1),(0x3b291a4,4),(0x3abdf76,1),(0x3abdf79,1),(0x3b28114,4)]
class Source:
 def __init__(self,image):
  assert image.read(0x40cad7,2)==bytes.fromhex('33db') and image.read(0x40cae4,5)==bytes.fromhex('be01000000')
  self.m=m=Original(image,127).machine
  for s in image.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095;m.mem_map(image.base+s['rva'],size);m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
  for address in [0x40be40,0x40e2c0,0x421fd0,0x411d00]:m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=address,end=address)
  self.clean=m.context_save()
 def state(self):return [int.from_bytes(self.m.mem_read(a,n),'little') for a,n in FIELDS]
 def store(self,values):
  for (address,n),value in zip(FIELDS,values):self.m.mem_write(address,(value&((1<<(8*n))-1)).to_bytes(n,'little'))
 def hook(self,m,address,size,user):
  ident={0x40be40:1,0x40e2c0:2,0x421fd0:3,0x411d00:4}[address];sp=m.reg_read(UC_X86_REG_ESP)
  if ident==2:assert int.from_bytes(m.mem_read(sp+4,4),'little')==0x3abdca8
  self.trace.append([ident,*self.state()]);ordinal=len(self.trace)
  if ordinal==self.v[7]:self.failed=True;m.emu_stop();return
  if self.v[8]&(1<<(ordinal-1)):self.store(self.v[16+(ordinal-1)*6:22+(ordinal-1)*6])
  m.reg_write(UC_X86_REG_EIP,int.from_bytes(m.mem_read(sp,4),'little'));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.trace=[];self.failed=False;m=self.m;m.context_restore(self.clean);self.store(v[1:7]);m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_EBX,0);m.reg_write(UC_X86_REG_ESI,1)
  if v[0]!=1:m.emu_start(0x40cba4,0x40cbf2,timeout=0)
  if not self.failed and v[0]!=0:m.emu_start(0x40cc18 if v[0]==2 else 0x40cc1d,0x40cc62,timeout=0)
  output=[0]*80;output[0:8]=[int(self.failed),len(self.trace),*self.state()]
  for i,event in enumerate(self.trace):output[16+i*7:23+i*7]=event
  return output
def fixtures(seed,count):
 rng=random.Random(seed);cases=[]
 for op,ready,counter,phase,execute,pending,pause,fail in itertools.product(range(3),[0,1],[0,255],[0,1,0xffffffff,2],[0,1],[0,1],[0,2],range(5)):
  cases.append([op,ready,counter,phase,execute,pending,pause,fail,0]+[0]*31)
 for _ in range(count):
  v=[rng.randrange(3),rng.getrandbits(8),rng.getrandbits(8),rng.choice([0,1,0xffffffff,rng.getrandbits(32)]),rng.getrandbits(8),rng.getrandbits(8),rng.choice([0,1,2,3,rng.getrandbits(32)]),rng.randrange(5),rng.getrandbits(4)]+[0]*7
  for _ in range(4):v.extend([rng.getrandbits(8),rng.getrandbits(8),rng.choice([0,1,0xffffffff,2]),rng.randrange(2),rng.randrange(2),rng.choice([0,1,2,3])])
  cases.append(v)
 return cases
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--cases',type=int,default=500);p.add_argument('--seed',type=int,default=20261022);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);U=ctypes.c_uint32;fn=ctypes.CDLL(str(a.probe.resolve())).sadx_native_save_frame;fn.argtypes=[ctypes.POINTER(U)]*2;bad=[];cases=fixtures(a.seed,a.cases)
 for index,v in enumerate(cases):
  expected=source.run(v);out=(U*80)();assert fn((U*40)(*v),out)==1
  if list(out)!=expected:bad.append({'case':index,'input':v,'differences':[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y]})
 report={'cases':len(cases),'mismatch_count':len(bad),'values_match':not bad,'passed':not bad,'mismatches':bad[:8],'source_sha256':image.sha256,'source_ranges':{'0x40cba4':'0x40cbf2','0x40cc18':'0x40cc62'},'seed':a.seed,'limits':__doc__,'native_library_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest()};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':len(cases),'mismatches':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
