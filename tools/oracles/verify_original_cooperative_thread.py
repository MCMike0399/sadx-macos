"""TH_Wait real null-current-thread return, or explicit first continuation-store boundary.

The active branch stops BEFORE writing its original stack continuation. That branch
is not reported as a completed native switch. Repeated calls observe the same live
resource owner, including a caller reset between calls; no guest addresses enter it.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine;self.m.mem_map(0x38f6000,4096);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=0x4014bc,end=0x4014bc);self.initial=self.m.context_save()
 def hook(self,m,a,size,user):self.active=True;m.emu_stop()
 def invoke(self):
  self.active=False;self.m.mem_write(STACK-256,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.emu_start(0x4014b0,STOP,timeout=0);assert self.active or self.m.reg_read(UC_X86_REG_EIP)==STOP;return int(self.active)
 def run(self,row):
  self.m.context_restore(self.initial);self.m.mem_write(0x38f6e08,struct.pack('<I',row[0]));first=self.invoke();before=int(bool(row[0]))
  if row[1]:self.m.mem_write(0x38f6e08,bytes(4))
  return [first,before,self.invoke(),before if not row[1] else 0,0,0,0,0]
def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1000);p.add_argument('--seed',type=int,default=401450);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_cooperative_thread_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;rng=random.Random(a.seed);rows=[[v,r] for v in (0,1,0x38f6e0c,0xffffffff) for r in (0,1)]+[[rng.choice((0,rng.getrandbits(32))),rng.randrange(2)] for _ in range(a.cases)];bad=[]
 for n,row in enumerate(rows):
  expected=source.run(row);out=(ctypes.c_uint32*8)();probe((ctypes.c_uint32*2)(*row),out)
  if list(out)!=expected:bad.append({'case':n,'input':row,'expected':expected,'native':list(out)})
 root=Path(__file__).resolve().parents[2];files=['src/engine/cooperative_thread_native.hpp','src/engine/cooperative_thread_native.cpp','tests/probes/native_cooperative_thread_probe.cpp'];result={'passed':not bad,'cases':len(rows),'mismatches':len(bad),'first_mismatches':bad[:8],'seed':a.seed,'source_sha256':image.sha256,'source_range_sha256':hashlib.sha256(image.read(0x4014b0,32)).hexdigest(),'native_sources':{f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files},'limits':[__doc__]};Path(a.output).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
