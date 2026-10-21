"""Original caption tail plus reference fact-validation boundaries and resource identity reentry.

Unknown reference facts stop immediately before the original instruction that would
read them. The source caller's earlier stores and callee failure prefixes still execute.
Guest pointer identities are transported only by the reference adapter, never game state.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine;self.m.mem_map(0x3abd000,4096)
  for a in (0x40bcf0,0x40bcff,0x40bd17,0x40bd12,0x40bd1d,0x40e2c0,0x780fb0):self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=a,end=a)
  self.initial=self.m.context_save()
 def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
 def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
 def fail(self,kind,boundary,fact=0,pending=None):
  self.result=[kind,boundary,fact];self.pending=pending or [0,0,0];self.stopped=True;self.m.emu_stop()
 def hook(self,m,a,size,user):
  if a==0x40bcf0 and not self.timer_known:self.fail(2,a,0x3abdf88);return
  if a in (0x40bcff,0x40bd17) and not self.buffer_known:self.fail(2,a,0x3abdf84);return
  if a in (0x40bd12,0x40bd1d) and not self.v[8]:
   self.fail(4,a,pending=[a,0x40e2c0 if a==0x40bd12 else 0x780fb0,self.word(m.reg_read(UC_X86_REG_ESP))]);return
  if a not in (0x40e2c0,0x780fb0):return
  sp=m.reg_read(UC_X86_REG_ESP);site=0x40bd12 if a==0x40e2c0 else 0x40bd1d;argument=self.word(sp+4);self.trace.append([site,a,argument,self.word(0x3abdf88)])
  if len(self.trace)==self.v[4]:self.fail(3 if self.v[9] else 4,site,pending=[site,a,argument]);return
  if a==0x40e2c0:
   if self.v[5]:self.put(0x3abdf84,self.v[6])
   if self.v[7]:self.buffer_known=False
  elif self.v[10]:self.put(0x3abdf84,self.v[11])
  m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.timer_known=bool(v[2]);self.buffer_known=bool(v[3]);self.result=[0,0,0];self.pending=[0,0,0];self.trace=[];self.stopped=False;self.m.context_restore(self.initial);self.put(0x3abdf88,v[0]);self.put(0x3abdf84,v[1]);self.put(STACK-256,STOP);self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.emu_start(0x40bcf0,STOP,timeout=0);assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*64;out[:9]=self.result+[self.word(0x3abdf88) if self.timer_known else 0xdeadbeef,self.word(0x3abdf84) if self.buffer_known else 0xdeadbeef]+self.pending+[len(self.trace)]
  for n,row in enumerate(self.trace):out[24+n*4:28+n*4]=row
  return out

def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 for timer in (0,1,2,0x7fffffff,0x80000000,0xffffffff):
  for buffer in (0,0x1000):
   for known in range(4):
    for stop in (0,1,2):
     for invoke in (0,1):rows.append([timer,buffer,known&1,(known>>1)&1,stop,0,0,0,invoke,0,0,0])
 for replacement in (0,0x1000,0x2000,0xffffffff):
  for forget in (0,1):
   for stop in (0,1,2):rows.append([1,0x1000,1,1,stop,1,replacement,forget,1,0,1,0x3000])
 for _ in range(count):rows.append([rng.choice((0,1,2,rng.getrandbits(32))),rng.choice((0,0x1000,rng.getrandbits(32))),rng.randrange(2),rng.randrange(2),rng.randrange(3),rng.randrange(2),rng.choice((0,0x2000,rng.getrandbits(32))),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.getrandbits(32)])
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1500);p.add_argument('--seed',type=int,default=40_888);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_caption_frame_reference_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;rows=vectors(a.seed,a.cases);bad=[]
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*64)();probe((ctypes.c_uint32*12)(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];paths=['src/game/menu_caption.hpp','src/game/menu_caption.cpp','src/guest/caption_frame_reference.hpp','tests/probes/native_caption_frame_reference_probe.cpp'];report={'passed':not bad,'cases':len(rows),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'source_range_sha256':hashlib.sha256(image.read(0x40bcf0,64)).hexdigest(),'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'limits':[__doc__]};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
