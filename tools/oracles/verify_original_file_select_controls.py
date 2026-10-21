"""Original active FileSelect startup/selection/navigation caller with ordered resource seams.

Unsupported dialog stages stop explicitly after their common animation prefix; they are not
successful full-control cases. Original animation, cosine, color interpolation and button-mask
helpers execute. Save/dialog/audio resource calls are receipts with all live shared snapshots.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
WORK=DATA+0x1000
FIELDS=[(0x3b290e0,6,1),(0x3b2a2f9,4,1),(0x10d77ad,7,1),(0x3b290d8,5,1),(0x3c5e8a4,8,2),(0x3c5e8b4,9,1),(0x3b291b8,10,1),(0x3abdf78,11,1),(0x3c5e8e0,12,4),(0x3b1118c,13,4),(0x3b22dc0,14,2),(0x3b291b3,15,1)]
CALLS={0x5057b0:1,0x5065d0:2,0x42d770:3,0x5065e0:4,0x421bd0:5,0x421de0:6,0x505b40:7,0x6fc8a0:8,0x505830:9}
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for a,_,_ in FIELDS+[(0x3b0e354,0,4)]:
   p=a&~4095
   if not any(x<=p<=y for x,y,_ in self.m.mem_regions()):self.m.mem_map(p,4096)
  self.stage_targets=image.unpack('<18I',0x503e00);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def write(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
 def snapshot(self):
  return [self.word(WORK+36),self.word(WORK+24),self.word(WORK+32),self.word(WORK+50,1),self.word(WORK+28)]+[self.word(a,n) for a,_,n in FIELDS]+[self.word(WORK+4),self.word(WORK+8),self.word(WORK+48,2)]
 def hook(self,m,a,size,user):
  if self.v[0] in range(18) and self.v[0] not in (0,1,9,10) and a==self.stage_targets[self.v[0]]:
   self.boundary=True;m.emu_stop();return
  if a not in CALLS:return
  kind=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP);arg=self.word(sp+4) if kind in (1,7) else 0
  if kind==5:assert self.word(sp+4)==DATA+0x9000
  self.trace.append([kind,arg]+self.snapshot()+[0,0])
  if len(self.trace)==self.v[21]:self.stopped=True;m.emu_stop();return
  if kind==self.v[22]:self.write(0x10d77ad,self.v[23],1);self.write(0x3b2a2f9,self.v[24],1)
  m.reg_write(UC_X86_REG_EAX,self.v[20] if kind==5 else 0);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.trace=[];self.stopped=False;self.boundary=False;self.m.context_restore(self.initial);self.m.mem_write(WORK,bytes(64))
  for a,i,n in FIELDS:self.write(a,v[i],n)
  for offset,index,n in [(36,0,4),(24,1,4),(32,2,4),(50,3,1),(28,18,4),(48,19,2),(4,16,4),(8,25,4)]:self.write(WORK+offset,v[index],n)
  self.write(0x3b0e354,v[17]);self.write(0x3c5e8a8,DATA+0x9000);self.m.mem_write(STACK-0x100,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.reg_write(UC_X86_REG_EAX,WORK);self.m.emu_start(0x503600,STOP,timeout=0);assert self.stopped or self.boundary or self.m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*256;out[:20]=self.snapshot();out[23]=len(self.trace);out[24]=int(self.stopped);out[25]=self.m.reg_read(UC_X86_REG_FPSW)&63;out[26]=int(self.boundary);out[27]=v[0] if self.boundary else 0
  for n,row in enumerate(self.trace):out[32+n*24:56+n*24]=row
  return out

def vectors(seed,count):
 rng=random.Random(seed);result=[]
 for stage in (0,1,9,10,18,0xffffffff):
  for buttons in (0,4,8,2,0x400,16,32,48,12,0x42e):
   for files,selected in ((0,0),(1,1),(2,2),(99,0),(99,99)):
    for ready in (0,1,2):
     v=[stage,0x3f800000,0,0,selected,selected,files,123,0,0,1,0,ready,1,7,0,0,buttons,0,0,1,0,0,0,0,3];result.append(v)
 for stage in range(18):
  if stage not in (0,1,9,10):result.append([stage,0x3fc00000,0xffffffff,123,1,1,2,1,0xffff,255,2,1,1,2,7,1,7,0,0,5,1,0,0,0,0,3])
 for kind in range(1,10):
  for buttons in (4,2,16,32):
   for stop in (0,1,2):
    for cursor in (0,3,255):
     v=[1,0x3fc00000,0x7fffffff,5,1,1,2,1,0x7fff,cursor,1,1,1,2,8,0,0,buttons,0,0,1,stop,kind,2,0,3];result.append(v)
 for _ in range(count):
  v=[rng.getrandbits(32) for _ in range(26)];v[0]=rng.choice((0,1,9,10,18,0xffffffff));v[6]=rng.randrange(256);v[4]=rng.randrange(v[6]+1);v[5]=v[4];v[12]=rng.choice((0,1,2));v[17]=rng.choice((0,4,8,2,0x400,16,32,48,12,0x42e));v[21]=rng.randrange(5);v[22]=rng.randrange(10);v[16]=rng.choice((0,7));result.append(v)
 return result

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1200);p.add_argument('--seed',type=int,default=503600);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_file_select_controls_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;bad=[];flags=[];cases=vectors(a.seed,a.cases)
 for n,v in enumerate(cases):
  expected=source.run(v);out=(ctypes.c_uint32*256)();probe((ctypes.c_uint32*26)(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if diff:
   item={'case':n,'input':v,'diff':diff}
   if all(i==25 for i,x,y in diff):flags.append(item)
   else:bad.append(item)
   if len(bad)>=8:break
 root=Path(__file__).resolve().parents[2];paths=['src/game/file_select_controls.cpp','src/game/file_select.hpp','tests/probes/native_file_select_controls_probe.cpp'];ranges=[(0x5035d0,0x878),(0x5067e0,9),(0x5067f0,11),(0x431960,0x65),(0x4319d0,0xf7),(0x7883c0,0x130)]
 report={'passed':not bad and not flags,'values_match':not bad,'cases':len(cases),'mismatches':len(bad),'fp_flag_partials':len(flags),'first_mismatches':bad,'first_fp_partials':flags[:3],'seed':a.seed,'source_sha256':image.sha256,'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'source_ranges':{f'0x{x:x}-0x{x+n:x}':hashlib.sha256(image.read(x,n)).hexdigest() for x,n in ranges},'limits':['Full startup0/select1/cancel9/wait10 bodies and invalid-stage guard; remaining dialog stages explicitly stop at their original switch entry after common animation. Original animation/cosine/interpolation/mask helpers execute; save/dialog/audio calls are ordered resource receipts. All float words are compared; software FPflag differences are reported separately.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return not report['passed']
if __name__=='__main__':raise SystemExit(main())
