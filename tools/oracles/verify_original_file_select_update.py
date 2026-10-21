"""Full original FileSelect task phase dispatcher; connected callee seams are receipts."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
WORK=DATA+0x1000;TASK=DATA+0x2000;SEQ=DATA+0x3000;SEQWORK=DATA+0x4000;OTHER=DATA+0x5000;OTHERWORK=DATA+0x6000
CALLS={0x505320:1,0x506e10:2,0x506e40:3,0x503600:4,0x506040:5,0x420a30:6,0x504ed0:7}
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for p in [0x3c5e000,0x3b11000,0x3b2c000]:self.m.mem_map(p,4096)
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
 def word(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
 def snapshot(self):
  sequence=self.word(self.word(0x3c5e8d0)+0x2c)
  return [self.word(WORK),self.word(WORK+16),self.word(WORK+20),self.word(sequence+4),self.word(sequence+8),self.word(sequence+12),self.m.mem_read(sequence+31,1)[0],self.word(0x3b1118c),self.m.mem_read(0x3b2c58c,1)[0]]
 def hook(self,m,a,size,user):
  if a not in CALLS:return
  kind=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP);arg=self.word(sp+4) if kind in (2,3,5) else 0
  if kind==6:assert self.word(sp+4)==0x90e494
  if kind==4:assert m.reg_read(UC_X86_REG_EAX)==WORK
  self.trace.append([kind,arg]+self.snapshot()+[0])
  if len(self.trace)==self.v[9]:self.stopped=True;m.emu_stop();return
  if kind==self.v[10]:
   current=self.word(self.word(0x3c5e8d0)+44)
   if self.v[15]:m.mem_write(OTHERWORK,bytes(m.mem_read(current,36)));self.write(0x3c5e8d0,OTHER);current=OTHERWORK
   self.write(current+4,self.v[11]);self.write(current+8,self.v[12]);self.write(0x3b1118c,self.v[13]);self.write(WORK+16,self.v[14])
  m.reg_write(UC_X86_REG_EAX,0x76543210 if kind==2 else 0x87654321 if kind==3 else 0);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.trace=[];self.stopped=False;self.m.context_restore(self.initial);self.m.mem_write(WORK,bytes(64));self.write(TASK+44,WORK);self.write(SEQ+44,SEQWORK);self.write(OTHER+44,OTHERWORK);self.write(0x3c5e8d0,SEQ);self.m.mem_write(SEQWORK,bytes(36));self.write(WORK,v[0]);self.write(WORK+16,v[3]);self.write(WORK+20,v[4]);self.write(SEQWORK+4,v[1]);self.write(SEQWORK+8,v[2]);self.write(SEQWORK+12,v[5]);self.m.mem_write(SEQWORK+31,bytes([v[6]&255]));self.write(0x3b1118c,v[7]);self.m.mem_write(0x3b2c58c,bytes([v[8]&255]));self.m.mem_write(STACK-0x100,struct.pack('<II',STOP,TASK));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.emu_start(0x5053f0,STOP,timeout=0);assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*256;out[:9]=self.snapshot();out[9]=len(self.trace);out[10]=int(self.stopped);out[11]=self.m.reg_read(UC_X86_REG_FPSW)&63
  for n,row in enumerate(self.trace):out[32+n*12:44+n*12]=row
  return out

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1500);p.add_argument('--seed',type=int,default=5053);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);rng=random.Random(a.seed);vectors=[]
 for phase in range(5):
  for mode in (0,2,3):
   for old in (0,2,3):
    for progress in (0,0x3f733333,0x3f800000,0x3f866666,0x7fc00001,0x7f800001):
     for ticks in (0,1,2,0xffffffff):vectors.append([phase,mode,old,progress,123,9,7,ticks,6,0,0,0,0,0,0,0])
 for kind in range(1,8):
  for replace in (0,1):
   for phase in (0,1,2,3):vectors.append([phase,2,0,0x3f733333,123,9,7,2,6,0,kind,3,2,3,0x3f000000,replace])
 for _ in range(a.cases):
  v=[rng.getrandbits(32) for _ in range(16)];v[0]=rng.randrange(5);v[1]=rng.choice((0,2,3));v[2]=rng.choice((0,2,3));v[9]=rng.randrange(5);v[10]=rng.randrange(8);v[15]=rng.randrange(2);vectors.append(v)
 lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_file_select_update_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;bad=[];flags=[]
 for n,v in enumerate(vectors):
  expected=source.run(v);out=(ctypes.c_uint32*256)();probe((ctypes.c_uint32*16)(*v),out);actual=list(out)
  diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,actual)) if x!=y]
  if diff:
   item={'case':n,'input':v,'diff':diff}
   if all(i==11 for i,x,y in diff):flags.append(item)
   else:bad.append(item)
   if len(bad)>=8:break
 paths=['src/game/file_select.cpp','src/game/file_select.hpp','tests/probes/native_file_select_update_probe.cpp'];root=Path(__file__).resolve().parents[2]
 report={'passed':not bad and not flags,'values_match':not bad,'cases':len(vectors),'mismatches':len(bad),'fp_flag_partials':len(flags),'first_mismatches':bad,'first_fp_partials':flags[:3],'seed':a.seed,'source_sha256':image.sha256,'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'source_ranges':{'0x5053f0-0x50554c':hashlib.sha256(image.read(0x5053f0,0x15c)).hexdigest()},'limits':['Full phase caller executes, including indirect jump suffixes. Initialization, fade-color, active controls and resource-release calls are ordered receipts; their internals are separate modules/boundaries. Callback mutations include replacing canonical sequence work.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return not report['passed']
if __name__=='__main__':raise SystemExit(main())
