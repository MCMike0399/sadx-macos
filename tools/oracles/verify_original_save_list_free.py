"""Original reverse-tail free loop, sentinel preservation and global clear."""
import argparse,ctypes,hashlib,json,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
HEAD=0x3c5e8b8
NATIVE_PATHS=['src/game/save_file_list.cpp', 'src/game/save_file_list.hpp', 'tests/probes/native_save_list_free_probe.cpp']
def native_sources():
    root=Path(__file__).resolve().parents[2]
    return {name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in NATIVE_PATHS}

class Source:
 def __init__(self,image):
  self.m=Original(image,0x7f).machine;self.m.mem_map(HEAD&~4095,4096);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
 def word(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def hook(self,m,a,size,user):
  if a!=0x644e51:return
  sp=m.reg_read(UC_X86_REG_ESP);pointer=self.word(sp+4)
  if DATA<=pointer<DATA+0x1000:self.trace.append((pointer-DATA)//16)
  else:assert pointer==DATA+0x2000+(self.n-len(self.trace)-1)*32
  m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,n):
  self.n=n;self.trace=[];self.m.context_restore(self.initial);self.m.mem_write(HEAD,struct.pack('<I',DATA if n else 0))
  for k in range(n):self.m.mem_write(DATA+k*16,struct.pack('<4I',0xcdcdcdcd if k==0 else DATA+0x2000+k*32,0xcdcdcdcd,0xcdcdcdcd,DATA+(k+1)*16 if k+1<n else 0))
  self.m.mem_write(STACK-0x100,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.emu_start(0x504ed0,STOP,timeout=0);assert self.m.reg_read(UC_X86_REG_EIP)==STOP;return [int(self.word(HEAD)!=0),len(self.trace)]+self.trace+[0]*(254-len(self.trace))
def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_save_list_free_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;bad=[]
 for n in range(101):
  expected=source.run(n);out=(ctypes.c_uint32*256)();probe((ctypes.c_uint32*1)(n),out)
  if list(out)!=expected:bad.append({'nodes':n,'expected':expected,'native':list(out)})
 report={'passed':not bad,'cases':101,'mismatches':len(bad),'first_mismatches':bad[:3],'source_sha256':image.sha256,'native_sources':native_sources(),'source_functions':{f'0x{x:x}-0x{x+n:x}':hashlib.sha256(image.read(x,n)).hexdigest() for x,n in [(5263056, 103)]},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'source_range_sha256':hashlib.sha256(image.read(0x504ed0,0x67)).hexdigest(),'limits':['Original CRT free is a resource receipt. Every filename is freed before its node; sentinel indeterminate filename is never freed. Native node destructor order and final canonical list null are compared; CRT allocator reentry is not a game callback. Ghidra analyze timed out; actual original instructions execute.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
