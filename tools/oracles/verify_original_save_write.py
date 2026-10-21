"""Original421fd0 write ordering, filename allocation and first-free slot selection.

CRT/filesystem allocation,formatting,CompareStringA and I/O are receipts, never
production implementations. Source canonical ASCII filenames bound comparison;
write return0/1 is retained (the original ignores it). Native real-I/O failure
must instead report a platform boundary, not fabricate source success.
"""
import argparse,ctypes,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from verify_original_save_creation import Source as CaptureSource
from verify_original_math import STACK,STOP,CODE
from pe_image import PEImage
from extract import SOURCE_SHA256
FIELDS=[(0x3b29198,4),(0x3abdf7a,1),(0x3b291b1,1),(0x3b291b2,1),(0x3b291a4,4),(0x3b291ad,1),(0x3abdf76,1),(0x3b22e1e,1),(0x3b291b0,1),(0x3b291b3,1),(0x3b290e0,1),(0x3b290d8,1),(0x3b290e8,1)]
SCRATCH=0x21000000
class Source(CaptureSource):
 def __init__(self,image):
  super().__init__(image);self.m.mem_map(SCRATCH,0x10000)
  for a in [0x42d630,0x40f350,0x645002,0x644e51,0x644df9,0x644de6,0x644e89,0x644c7d]:self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=a,end=a)
  for i,(pointer,pop) in enumerate([(0x7db0d0,8),(0x7db0cc,24),(0x7db0c8,4)]):
   a=CODE+0x600+i*16;self.put(pointer,a);self.m.mem_write(a,b'\xc2'+struct.pack('<H',pop));self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=a,end=a)
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 def get(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def string(self,a):
  if not a:return None
  result=bytearray()
  while True:
   v=self.m.mem_read(a,1)[0]
   if not v:return result.decode('ascii')
   result.append(v);a+=1
 def hook(self,m,a,size,user):
  sp=m.reg_read(UC_X86_REG_ESP);arg=lambda i:self.get(sp+4*i);result=0;pop=0
  if a==CODE+0x600:self.calls.append(1);pop=8
  elif a==CODE+0x610:result=2 if self.string(arg(3)).lower()==self.string(arg(5)).lower() else 1;pop=24
  elif a==CODE+0x620:result=len(self.string(arg(1)));pop=4
  elif a==0x42d630:self.calls.append(2)
  elif a==0x40f350:self.calls.append(5)
  elif a==0x645002:result=self.allocate;self.allocate+=256
  elif a==0x644e51:pass
  elif a==0x644df9:
   fmt=self.string(arg(2));value=self.string(arg(3)) if '%s' in fmt else arg(3);text=(fmt%value).encode();m.mem_write(arg(1),text+b'\0');result=len(text)
  elif a==0x644de6:self.calls.append(3);self.path=self.string(arg(1));result=0x1234
  elif a==0x644e89:
   self.calls.append(4);assert arg(2)==1392 and arg(3)==1 and arg(4)==0x1234;self.written=bytes(m.mem_read(arg(1),1392));result=self.items
  elif a==0x644c7d:self.calls.append(6);assert arg(1)==0x1234
  else:raise AssertionError(hex(a))
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_EIP,self.get(sp));m.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def run(self,v,name,names,data):
  m=self.m;m.context_restore(self.clean);self.calls=[];self.path='';self.written=bytes(1392);self.allocate=SCRATCH+0x2000;self.items=v[15]
  for (a,n),value in zip(FIELDS,v):self.put(a,value,n)
  self.put(0x3b290dc,SCRATCH if v[13] else 0);m.mem_write(SCRATCH,name.encode()+b'\0');m.mem_write(0x3b2b3a8,data)
  self.put(0x3c5e8b8,SCRATCH+0x100);self.put(SCRATCH+0x10c,SCRATCH+0x200 if names else 0)
  for i,text in enumerate(names):
   node=SCRATCH+0x200+i*16;self.put(node,SCRATCH+0x1000+i*16);self.put(node+12,node+16 if i+1<len(names) else 0);m.mem_write(SCRATCH+0x1000+i*16,text.encode()+b'\0')
  m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x421fd0,STOP,timeout=1000000);assert m.reg_read(UC_X86_REG_EIP)==STOP
  out=[m.reg_read(UC_X86_REG_EAX)]
  for index in [1,2,3,4,5,6,7,9,10,11,12]:
   a,n=FIELDS[index];out.append(int.from_bytes(m.mem_read(a,n),'little'))
  pointer=self.get(0x3b290dc);out+=[int(pointer!=0),len(self.calls),*self.calls]
  return out,self.string(pointer) or '',self.path,self.written

def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--cases',type=int,default=1000);p.add_argument('--seed',type=int,default=20261021);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);B=ctypes.c_uint8;U=ctypes.c_uint32;C=ctypes.c_char;fn=ctypes.CDLL(str(a.probe.resolve())).sadx_native_save_write;fn.argtypes=[ctypes.POINTER(U),ctypes.c_char_p,ctypes.c_char_p,ctypes.POINTER(B),ctypes.POINTER(U),ctypes.POINTER(C),ctypes.POINTER(C),ctypes.POINTER(B)];rng=random.Random(a.seed);bad=[]
 for case in range(a.cases):
  names=[f'SonicDX{i:02}.snc' for i in rng.sample(range(1,100),rng.randrange(100))];rng.shuffle(names);names=[n.lower() if rng.randrange(2) else n for n in names];name='SonicDX42.snc';v=[rng.choice([0,0,0,1,0xffffffff]),rng.getrandbits(8),rng.getrandbits(8),rng.choice([0,4,5,255]),rng.getrandbits(32),rng.getrandbits(8),rng.getrandbits(8),rng.getrandbits(8),rng.randrange(2),rng.randrange(2),rng.choice([0,1,98,99,255]),rng.getrandbits(8),rng.getrandbits(8),1,len(names),rng.randrange(2)];data=rng.randbytes(1392)
  expected=source.run(v,name,names,data);out=(U*32)();out_name=(C*64)();path=(C*256)();written=(B*1392)();assert fn((U*16)(*v),name.encode(),b''.join(n.encode().ljust(16,b'\0') for n in names),(B*1392).from_buffer_copy(data),out,out_name,path,written)==1
  observed=(list(out[:len(expected[0])]),out_name.value.decode(),path.value.decode(),bytes(written))
  if observed!=expected:bad.append({'case':case,'input':v,'expected_fields':expected[0],'observed_fields':observed[0],'expected_name':expected[1],'observed_name':observed[1],'expected_path':expected[2],'observed_path':observed[2],'bytes_equal':expected[3]==observed[3]})
 report={'cases':a.cases,'mismatch_count':len(bad),'values_match':not bad,'passed':not bad,'mismatches':bad[:8],'limits':__doc__,'source_sha256':image.sha256,'source_ranges':{'0x421fd0':'0x4221cc'},'seed':a.seed};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':a.cases,'mismatches':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
