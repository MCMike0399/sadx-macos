"""PlayVoiceFile original instructions with existing native reader resource receipts.

Actual40ce30 control flow, conversion call, live attenuation load, state getter and
startup polling execute. Stop/reset/open/start/Sleep are explicit resource receipts
for independently qualified native reader ownership. State0/2/other waits until1.
Null reader after reentrant resource callback faults in original helper and is a
tagged native boundary; page0 is never mapped. No codec-bit/Windows latency claim.
"""
import argparse,ctypes,hashlib,json,random
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,CODE,STACK,STOP
BASE=0x22000000;PLAYER=BASE+0x1000;MUSIC=BASE+0x1100;BUFFER=BASE+0x1200;PATH=BASE+0x2000
WIDE=CODE+0x300;SLEEP=CODE+0x310
class Source:
 def __init__(self,image):
  assert image.sha256==SOURCE_SHA256 and unicorn.__version__=='2.1.4'
  self.m=m=Original(image,127).machine
  for section in image.sections:
   if section['name'] in ('.text','.rdata'):continue
   size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095;m.mem_map(image.base+section['rva'],size);m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
  m.mem_map(BASE,0x10000);self.put(0x7db080,WIDE);self.put(0x7db07c,SLEEP);m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.context=m.context_save()
 def get(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 def string(self,a,wide=False):
  step=2 if wide else 1;out=[]
  while self.get(a,step):out.append(chr(self.get(a,step)));a+=step
  return ''.join(out)
 def ret(self,value=0,pop=0):
  sp=self.m.reg_read(UC_X86_REG_ESP);self.m.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.m.reg_write(UC_X86_REG_ESP,sp+4+pop);self.m.reg_write(UC_X86_REG_EIP,self.get(sp))
 def event(self,kind,path='',value=0):
  fields=[self.get(0x3abdfac),int(self.get(0x3abdf94)!=0),int(self.get(PLAYER+4)!=0),self.get(PLAYER+0x44),self.get(0x3abdfa0),self.get(0x3abdf98),self.get(0x3abdfa8),self.get(0x3abdfa4)]
  self.events.append('|'.join(map(str,[kind,path,value]+fields))+'\n');calls=len(self.events)
  if calls==self.v[5]:self.failed=1;self.m.emu_stop();return
  if calls==self.v[6]:
   self.put(0x3abdfac,self.v[7])
   if self.v[8]:self.put(0x3abdf94,0)
   for at,value in zip([0x3abdfa0,0x3abdf98,0x3abdfa8,0x3abdfa4],self.v[15:19]):self.put(at,value)
 def hook(self,m,a,size,user):
  sp=m.reg_read(UC_X86_REG_ESP)
  if a==WIDE:
   text=self.string(self.get(sp+12));m.mem_write(self.get(sp+20),text.encode('utf-16-le')+b'\0\0');self.ret(len(text)+1,24)
  elif a in (0x411760,0x411720,0x411830,0x410ff0):
   if m.reg_read(UC_X86_REG_ECX)==0:return
   assert m.reg_read(UC_X86_REG_ECX)==PLAYER
   kind={0x411760:1,0x411720:2,0x411830:3,0x410ff0:4}[a]
   if kind==3:self.event(3,self.string(self.get(sp+4),True))
   elif kind==4:
    assert self.get(sp+4)==0 and self.get(sp+8)==0;self.event(4,'',self.get(sp+12))
   else:self.event(kind)
   if self.failed:return
   if kind in (1,2):self.put(PLAYER+4,0);self.ret()
   elif kind==3:self.ret(self.v[1],4)
   else:self.put(PLAYER+4,BUFFER);self.put(PLAYER+0x44,self.v[3]);self.ret(pop=12)
  elif a==SLEEP:
   assert self.get(sp+4)==0;self.event(5)
   if self.failed:return
   if self.pending:self.pending-=1
   if not self.pending:self.put(PLAYER+0x44,1)
   self.ret(pop=4)
 def run(self,v):
  self.v=v;self.events=[];self.failed=0;self.pending=v[4];m=self.m;m.context_restore(self.context)
  for at,value in [(0x3abdf94,PLAYER if v[0] else 0),(0x3abdf9c,MUSIC),(0x3abdfac,v[2]),(PLAYER+0x44,v[13]),(PLAYER+4,BUFFER if v[14] else 0)]+list(zip([0x3abdfa0,0x3abdf98,0x3abdfa8,0x3abdfa4],v[9:13])):self.put(at,value)
  m.mem_write(PATH,b'SYSTEM\\SoundData\\VOICE_US\\WMA\\0001.wma\0');self.put(STACK,STOP);self.put(STACK+4,PATH);m.reg_write(UC_X86_REG_ESP,STACK)
  try:m.emu_start(0x40ce30,STOP)
  except unicorn.UcError:
   pc=m.reg_read(UC_X86_REG_EIP)
   if not self.get(0x3abdf94) and 0x410ff0<=pc<0x411970:self.failed=2
   else:raise
  if not self.failed and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('source did not return')
  out=[0]*32;out[0]=self.failed
  if not self.failed:out[1]=m.reg_read(UC_X86_REG_EAX)
  out[2:11]=[self.get(0x3abdfac),int(self.get(0x3abdf94)!=0),int(self.get(PLAYER+4)!=0),self.get(PLAYER+0x44),self.get(0x3abdfa0),self.get(0x3abdf98),self.get(0x3abdfa8),self.get(0x3abdfa4),len(self.events)]
  return out,''.join(self.events)
def fixtures(seed,count):
 rng=random.Random(seed);cases=[]
 for _ in range(count):
  cases.append([rng.randrange(2),rng.choice([0,1,0x80070002,0xffffffff]),rng.choice([0,0xfffff31c,0xffffe318,0xffffd8f0]),rng.choice([0,1,2,3,0xffffffff]),rng.randrange(1,5),rng.choice([0,0,rng.randrange(1,10)]),rng.choice([0,0,rng.randrange(1,10)]),rng.choice([0,0xfffff31c,0xffffe318]),rng.randrange(2),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.choice([0,1,2]),rng.randrange(2),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32)])
 for state in [0,1,2,3,0xffffffff]:
  for fail in range(10):
   for drop in range(2):cases.append([1,0,0xfffff31c,state,3,fail,4,0xffffe318,drop,7,8,9,10,2,1,3,4,5,6])
 return cases
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--seed',type=int,default=20261115);p.add_argument('--cases',type=int,default=900);p.add_argument('--native-source-root',type=Path,default=Path(__file__).resolve().parents[2]);a=p.parse_args();image=PEImage(a.exe);source=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()));U=ctypes.c_uint32;probe=lib.sadx_native_voice_playback;probe.argtypes=[ctypes.POINTER(U),ctypes.POINTER(U),ctypes.c_char_p,ctypes.c_size_t];bad=[];cases=fixtures(a.seed,a.cases)
 for n,v in enumerate(cases):
  expected,et=source.run(v);out=(U*32)();trace=ctypes.create_string_buffer(8192);assert probe((U*len(v))(*v),out,trace,len(trace))==1
  differences=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if differences or et!=trace.value.decode():bad.append({'case':n,'input':v,'differences':differences,'source_trace':et,'native_trace':trace.value.decode()})
 paths=['src/game/voice_playback.hpp','src/game/voice_playback.cpp','tests/probes/native_voice_playback_probe.cpp'];ranges=[(0x40ce30,0x40cf13),(0x4113e0,0x4113e4)]
 r={'source_sha256':image.sha256,'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'seed':a.seed,'cases':len(cases),'mismatch_count':len(bad),'mismatches':bad[:8],'stores_calls_waits_failure_prefixes_match':not bad,'limits':__doc__,'native_sources':{path:hashlib.sha256((a.native_source_root/path).read_bytes()).hexdigest() for path in paths},'source_ranges':{f'{hex(x)}-{hex(y)}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in ranges}}
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({'cases':len(cases),'mismatch_count':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
