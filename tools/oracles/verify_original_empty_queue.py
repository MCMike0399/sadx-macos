"""Native4086f0 empty/default-mode dispatcher vs actual body, preserving nonempty stop.

Material clamping, mode/counter stores, matrix copies and2048bucket walking execute
in the original.77ed00/77ed20 andfog owners are ordered service receipts (their
separate native implementations remain required). Mode2stops at4071c0, nonempty
at408800before unported native record headers; neither is silently skipped.
"""
import ctypes,hashlib,json,random,struct,sys
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
root=Path(__file__).resolve().parents[2];sys.path[:0]=[str(root/'tools/lib'),str(root/'tools/oracles')]
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
BUCKETS=0x21000000;NODE=0x21005000;MATRIX=0x21006000
class Source:
 def __init__(self,image):
  self.m=m=Original(image,127).machine
  for s in image.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095;m.mem_map(image.base+s['rva'],size);m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
  m.mem_map(BUCKETS,0x10000)
  for address in [0x77ff90,0x784bc0,0x784d00,0x77ed00,0x77ed20,0x411af0,0x411b40,0x4071c0,0x408800]:m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=address,end=address)
  self.clean=m.context_save()
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
 def get(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def hook(self,m,address,size,user):
  sp=m.reg_read(UC_X86_REG_ESP)
  if address==0x4071c0:self.status=3;m.emu_stop();return
  if address==0x408800:self.status=2;m.emu_stop();return
  ident={0x77ff90:1,0x784bc0:2,0x784d00:3,0x77ed00:4,0x77ed20:5,0x411af0:6,0x411b40:6}[address]
  arg=self.get(sp+4) if ident in [4,5] else int(address==0x411af0) if ident==6 else 0
  color=list(struct.unpack('<4I',m.mem_read(self.get(sp+4),16))) if ident==1 else [0]*4
  self.trace.append([ident,arg,*color])
  if len(self.trace)==self.v[15]:self.status=100;m.emu_stop();return
  if len(self.trace)==1 and self.v[41]:
   for a,value in zip([0x3d0f9c8,0x3d0f840,0x3d0f9c4],self.v[42:45]):self.put(a,value)
   m.mem_write(0x3ab9864,struct.pack('<4I',*self.v[45:49]))
  if ident in [4,5,6]:m.reg_write(UC_X86_REG_EIP,self.get(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.trace=[];self.status=0;m=self.m;m.context_restore(self.clean)
  for a,value,n in [(0x3b1117c,v[1],4),(0x3ab98ac,v[2],4),(0x3abd9d4,v[3],4),(0x3abd9d8,v[4],4),(0x3abd9dc,v[5],4),(0x3abd9e0,v[6],4),(0x3b12208,v[7],4),(0x3abdbc0,v[8],1),(0x909eb4,v[9],1),(0x3abdcfe,v[10],1),(0x3d0f9c8,v[11],4),(0x3d0f840,v[12],4),(0x3d0f9c4,v[13],4),(0x3abd950,0x11001000 if v[40] else 0,4),(0x90083c,BUCKETS,4),(0x3d0f818,MATRIX,4),(0x8928c0,0x3f800000,4),(0x8928c4,0x3f800000,4)]:self.put(a,value,n)
  m.mem_write(0x3b18230,struct.pack('<4I',*v[16:20]));m.mem_write(0x3ab9864,struct.pack('<4I',*v[20:24]));m.mem_write(0x3d0f7f0,bytes(16));m.mem_write(MATRIX,struct.pack('<16I',*v[24:40]));m.mem_write(BUCKETS,b''.join(struct.pack('<II',BUCKETS+8*i,BUCKETS+8*i) for i in range(2048)))
  if v[14]:self.put(BUCKETS+2047*8,NODE);self.put(NODE,BUCKETS+2047*8)
  m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,v[0]);m.reg_write(UC_X86_REG_FPSW,0);m.emu_start(0x4086f0,STOP,timeout=0);assert self.status or m.reg_read(UC_X86_REG_EIP)==STOP
  output=[0]*256;output[:16]=[self.status,int(self.status==0),self.get(0x3ab98ac),self.get(0x3abd9d8),self.get(0x3abd9e0),self.get(0x3b12208),self.get(0x3abdbc0,1),self.get(0x3d0f9c8),self.get(0x3d0f840),self.get(0x3d0f9c4),int(self.get(0x3abd950)!=0),self.get(0x3abdcfe,1),self.get(0x909eb4,1),len(self.trace),m.reg_read(UC_X86_REG_FPSW)&63,0]
  output[16:20]=struct.unpack('<4I',m.mem_read(0x3ab9864,16));output[20:24]=struct.unpack('<4I',m.mem_read(0x3d0f7f0,16));output[24:40]=struct.unpack('<16I',m.mem_read(MATRIX,64));output[40]=int(self.status==2);output[41]=2047 if self.status==2 else 0
  for i,event in enumerate(self.trace):output[48+i*6:54+i*6]=event
  return output

def run(count=600,seed=408600,output='consumer-dev.json'):
 image=PEImage(oracle_exe);assert image.sha256==SOURCE_SHA256;source=Source(image);U=ctypes.c_uint32;fn=ctypes.CDLL(str(oracle_library)).sadx_native_empty_queue;fn.argtypes=[ctypes.POINTER(U)]*2;rng=random.Random(seed);bad=[];flags=0;outcomes={}
 for case in range(count):
  v=[0]*64;v[:16]=[rng.choice([127,639,895,3199]),rng.choice([0,0,0,1]),rng.choice([0,0,0,1,2]),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(8),rng.randrange(4),rng.getrandbits(8),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),int(case%7==0),rng.choice([0,0,0,1,2,3,4,5,6])]
  v[16:24]=[struct.unpack('<I',struct.pack('<f',rng.uniform(-2,2)))[0] for _ in range(8)];v[24:40]=[rng.getrandbits(32) for _ in range(16)];v[40]=rng.randrange(2);v[41]=rng.randrange(2);v[42:45]=[rng.getrandbits(32) for _ in range(3)];v[45:49]=[struct.unpack('<I',struct.pack('<f',rng.uniform(-2,2)))[0] for _ in range(4)]
  expected=source.run(v);got=(U*256)();assert fn((U*64)(*v),got)==1;different=[(i,x,y) for i,(x,y) in enumerate(zip(expected,got)) if x!=y and i!=14];flags+=expected[14]!=got[14];outcomes[expected[0]]=outcomes.get(expected[0],0)+1
  if different:bad.append({'case':case,'input':v,'differences':different[:16]})
 report={'cases':count,'mismatch_count':len(bad),'flag_mismatches':flags,'values_match':not bad,'passed':not bad and not flags,'mismatches':bad[:8],'outcomes':outcomes,'seed':seed,'limits':__doc__,'source_sha256':image.sha256};Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':count,'mismatches':len(bad),'flags':flags,'outcomes':outcomes}));return len(bad)>0
if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path)
 parser.add_argument('--output',type=Path,required=True)
 parser.add_argument('--cases',type=int,default=600);parser.add_argument('--seed',type=int,default=408600)
 args=parser.parse_args();oracle_exe=args.exe;oracle_library=args.native_library.resolve()
 raise SystemExit(run(args.cases,args.seed,args.output))
