"""Actual StoreSaveData42d630 and its packed story helpers, through complete1392-byte publication.

The original game instructions execute except the compiler stack-cookie receipt.
Source runtime sequence byte1 is skipped, preserving persisted padding. Flags pack
only byte value1. All source save snapshots and live lives are compared.
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
  self.m=m=Original(image,127).machine
  for s in image.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095
   m.mem_map(image.base+s['rva'],size);m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
  m.hook_add(unicorn.UC_HOOK_CODE,self.cookie,begin=0x643ea7,end=0x643ea7)
  self.clean=m.context_save()
 def cookie(self,m,a,size,user):
  sp=m.reg_read(UC_X86_REG_ESP);target=int.from_bytes(m.mem_read(sp,4),'little');m.reg_write(UC_X86_REG_ESP,sp+4);m.reg_write(UC_X86_REG_EIP,target)
 def run(self,data,words,story):
  m=self.m;m.context_restore(self.clean);m.mem_write(0x3b2b3a8,data)
  for a,v,n in zip([0x3abdc7c,0x3b0f13c,0x3b22dc0,0x3b0ef34,0x3b18db5,0x3b18db4,0x3b2c3f8,0x913b10,0x3b2c3fc],words,[4,4,2,1,1,1,4,1,4]):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
  for a,start,n in [(0x3b18808,0,512),(0x3b18608,512,512),(0x3b18a18,1024,8),(0x3b183a8,1032,96),(0x3b18250,1128,344),(0x3b2b368,1472,60)]:m.mem_write(a,story[start:start+n])
  m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x42d630,STOP,timeout=1000000);assert m.reg_read(UC_X86_REG_EIP)==STOP
  return bytes(m.mem_read(0x3b2b3a8,1392)),m.mem_read(0x3b0ef34,1)[0]
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--cases',type=int,default=1000);p.add_argument('--seed',type=int,default=20261021);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()));B=ctypes.c_uint8;U=ctypes.c_uint32;fn=lib.sadx_native_save_capture;fn.argtypes=[ctypes.POINTER(B),ctypes.POINTER(U),ctypes.POINTER(B),ctypes.POINTER(B),ctypes.POINTER(U)];rng=random.Random(a.seed);bad=[]
 for case in range(a.cases):
  data=rng.randbytes(1392);story=bytes(rng.choice([0,1,2,128,255]) for _ in range(1024))+rng.randbytes(508);words=[rng.choice([0,4,9,21]),rng.getrandbits(32),rng.randrange(8),rng.getrandbits(8),rng.randrange(2),rng.randrange(2),rng.getrandbits(32),rng.getrandbits(8),rng.getrandbits(32)]
  expected,lives=source.run(data,words,story);out=(B*1392)();result=(U*1)();assert fn((B*1392).from_buffer_copy(data),(U*9)(*words),(B*1532).from_buffer_copy(story),out,result)==1
  differences=[i for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if differences or lives!=result[0]:bad.append({'case':case,'words':words,'byte_offsets':differences[:24],'source_lives':lives,'native_lives':result[0]})
 report={'cases':a.cases,'mismatch_count':len(bad),'values_match':not bad,'passed':not bad,'mismatches':bad[:8],'source_sha256':image.sha256,'limits':__doc__,'source_ranges':{'0x42d630':'0x42d76c','0x42d0d0':'0x42d125','0x42d130':'0x42d185','0x42d190':'0x42d1f1','0x42d200':'0x42d24a','0x425b00':'0x425b26'},'seed':a.seed}
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':a.cases,'mismatches':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
