"""Actual source menu PVR producers under the pinned GVR format-mask patch.

Resource lookup/submission are receipts; actual vertex stores, color multiplication,
anchor arithmetic and reciprocal execute. This does not certify GPU pixels.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from verify_original_pvr_draw import Source as PvrSource
from verify_original_math import DATA,STACK,STOP,bits
from extract import SOURCE_SHA256
class Source(PvrSource):
 def __init__(self,image):
  super().__init__(image);self.m.mem_write(0x433f4d,b'\0');self.m.mem_map(0x3b28000,4096)
 def sized(self,v):
  self.out=[0]*32;m=self.m;m.mem_write(0x3b2c5f0,bytes(96))
  self.put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,v[9]);m.reg_write(UC_X86_REG_FPSW,0)
  m.emu_start(0x432ea0,STOP,timeout=1000000)
  self.put(0x3b1117c,v[8]);self.put(0x3d0fa24,DATA+0x1000);self.put(DATA+0x1000,DATA+0x2000);self.put(DATA+0x2008+(v[0]&0x30ffffff)*12,DATA+0x3000)
  for off,value in [(0,v[6]),(32,v[7]),(36,257),(40,255)]:self.put(DATA+0x3000+off,value)
  for i in range(4):self.put(0x3b2c604+i*24,v[10+i])
  self.put(STACK,STOP)
  for i,value in enumerate(v[:6]):self.put(STACK+4+i*4,value)
  m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPSW,0)
  if not v[8]:self.out[28]=v[0]&0x30ffffff
  m.emu_start(0x433c90,STOP,timeout=1000000)
  assert m.reg_read(UC_X86_REG_EIP)==STOP
  self.out[:24]=struct.unpack('<24I',m.mem_read(0x3b2c5f0,96));self.out[30]=m.reg_read(UC_X86_REG_FPCW);self.out[31]=m.reg_read(UC_X86_REG_FPSW)&63
  return self.out
 def empty_text_width(self,text):
  self.m.mem_write(DATA+0x7000,text);self.put(0x3b280d4,0);self.put(STACK,STOP);self.put(STACK+4,DATA+0x7000)
  self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.emu_start(0x420cf0,STOP,timeout=1000000)
  from unicorn.x86_const import UC_X86_REG_EAX
  return self.m.reg_read(UC_X86_REG_EAX)
 def vertex_colors(self,v):
  self.put(0x3b2c658,v[4]);self.put(STACK,STOP)
  for i,value in enumerate(v[:4]):self.put(STACK+4+i*4,value)
  self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.emu_start(0x432f90,STOP,timeout=1000000)
  return [self.word(0x3b2c604+i*24) for i in range(4)]+[self.word(0x3b2c650),self.word(0x3b2c658)]
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--seed',type=int,default=20261117);p.add_argument('--cases',type=int,default=1000);p.add_argument('--native-source-root',type=Path,default=Path(__file__).resolve().parents[2]);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256
 source=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()));U=ctypes.c_uint32;rng=random.Random(a.seed);cases=[]
 for cw in [127,639,895,1151,1663,1919,2175,2687,2943,3199,3711,3967]:
  for anchor in range(9):
   for flip in range(4):cases.append([(anchor<<24)|(flip<<30),bits(-64.25),bits(12.5),bits(1.1),bits(257.25),bits(255.5),123,rng.getrandbits(32),0,cw]+[rng.getrandbits(32) for _ in range(4)])
 for _ in range(a.cases):cases.append([(rng.randrange(9)<<24)|(rng.randrange(4)<<30),bits(rng.uniform(-10000,10000)),bits(rng.uniform(-10000,10000)),bits(rng.uniform(-100,100)),bits(rng.uniform(-2048,2048)),bits(rng.uniform(-2048,2048)),rng.getrandbits(32),rng.getrandbits(32),int(rng.randrange(8)==0),rng.choice([127,895,3967])]+[rng.getrandbits(32) for _ in range(4)])
 bad=[];flags=0
 for i,v in enumerate(cases):
  expected=source.sized(v);out=(U*32)();lib.sadx_native_menu_sized((U*len(v))(*v),out);diff=[(n,x,y) for n,(x,y) in enumerate(zip(expected,out)) if x!=y and n!=31]
  flags+=expected[31]!=out[31]
  if diff:bad.append({'kind':'sized','case':i,'input':v,'differences':diff})
 for i in range(a.cases):
  v=[rng.getrandbits(32) for _ in range(5)];expected=source.vertex_colors(v);out=(U*6)();lib.sadx_native_menu_vertex_colors((U*5)(*v),out)
  if expected!=list(out):bad.append({'kind':'colors','case':i,'input':v,'expected':expected,'actual':list(out)})
 empty=lib.sadx_native_menu_empty_text_width;empty.argtypes=[ctypes.c_char_p];empty.restype=ctypes.c_int32
 for text in [b'\0',b'\0A\0']:
  expected=source.empty_text_width(text);actual=empty(text)
  if expected!=actual:bad.append({'kind':'empty-font-width','source':expected,'native':actual})
 paths=['src/engine/menu_draw_primitives.hpp','src/engine/menu_draw_primitives.cpp','tests/probes/native_menu_draw_primitives_probe.cpp'];ranges=[(0x432f90,0x433001),(0x433c90,0x433f73),(0x420cf0,0x420d26)]
 report={'cases':len(cases)+a.cases+2,'sized_cases':len(cases),'vertex_color_cases':a.cases,'mismatch_count':len(bad),'mismatches':bad[:8],'fp_flag_partials':flags,'flag_mismatches':flags,'values_match':not bad,'passed':not bad and not flags,'seed':a.seed,'source_sha256':image.sha256,'native_sources':{f:hashlib.sha256((a.native_source_root/f).read_bytes()).hexdigest() for f in paths},'source_ranges':{f'{hex(x)}-{hex(y)}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in ranges},'profile_patch':{'va':'0x433f4d','original_bytes':image.read(0x433f4d,1).hex(),'effective_bytes':'00','source':'GVR_Init gvm.cpp851'},'limits':__doc__}
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','mismatch_count','fp_flag_partials']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
