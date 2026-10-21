"""Actual404660 guards/priority and genuine403f60NULL→404290/77e050 fallback.

77e050 is an ordered callee receipt (unchanged producer/quad oracle separately
qualifies its shared body). Type2eligible403f60 stopsBEFORE403d30/header allocation,
not a fabricatedNULL or immediate shortcut. All other local source instructions,
including texture gate/float comparisons/resetmaterial run unchanged. Native
resources correspond to valid typed sprites; arbitrary invalid low guest pointers
are outside this native object API, never tested by casting guest addresses.
"""
import ctypes,itertools,json,random,struct,sys
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
root=Path(__file__).resolve().parents[2];sys.path[:0]=[str(root/'tools/lib'),str(root/'tools/oracles')]
from verify_original_math import Original,STACK,STOP,DATA
from pe_image import PEImage
from extract import SOURCE_SHA256
SPRITE=DATA+0x100;LIST=DATA+0x200;ENTRY=DATA+0x300
class Source:
 def __init__(self,image):
  self.m=m=Original(image,127).machine
  for s in image.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095;m.mem_map(image.base+s['rva'],size);m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
  for address in [0x403f7b,0x77ed00,0x77ed20,0x77ff90,0x77e050]:m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=address,end=address)
  self.clean=m.context_save()
 def word(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def put(self,a,v):self.m.mem_write(a,(v&0xffffffff).to_bytes(4,'little'))
 def hook(self,m,address,size,user):
  sp=m.reg_read(UC_X86_REG_ESP)
  if address==0x403f7b:self.route=2;m.emu_stop();return
  if address==0x77e050:self.route=1;event=[4,self.word(sp+8),self.word(sp+12),self.word(sp+16),0,0,0,0];assert self.word(sp+4)==SPRITE
  elif address==0x77ff90:event=[3,0,0,0,*struct.unpack('<4I',m.mem_read(self.word(sp+4),16))]
  else:event=[1 if address==0x77ed00 else 2,self.word(sp+4),0,0,0,0,0,0]
  self.trace.append(event)
  if len(self.trace)==self.v[9]:self.failed=True;m.emu_stop();return
  if len(self.trace)==1 and self.v[24]:m.mem_write(0x3ab9864,struct.pack('<4I',*self.v[25:29]))
  if address!=0x77ff90:m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.trace=[];self.route=0;self.failed=False;m=self.m;m.context_restore(self.clean)
  for a,value in [(0x3b1117c,v[1]),(0x3ab98ac,v[2]),(0x8928c8,v[10])]:self.put(a,value)
  m.mem_write(0x3b18230,struct.pack('<4I',*v[12:16]));m.mem_write(0x3ab9864,struct.pack('<4I',*v[16:20]));m.mem_write(0x3abd9a8,struct.pack('<4I',*v[20:24]));m.mem_write(0x3d0f7f0,bytes(16));m.mem_write(SPRITE,bytes(32));self.put(SPRITE+24,0 if v[7]==0 else LIST);m.mem_write(LIST,struct.pack('<2I',ENTRY,0 if v[7]==1 else 1));m.mem_write(ENTRY,struct.pack('<3I',0xffffffff if v[7]==3 else 0,0,0 if v[7]==2 else DATA+0x500))
  m.mem_write(STACK,struct.pack('<6I',STOP,SPRITE if v[8] else 0,v[5],v[6],v[4],v[3]));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,v[0]);m.reg_write(UC_X86_REG_FPSW,0);m.emu_start(0x404660,STOP,timeout=0);assert self.failed or self.route==2 or m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*128;out[:4]=[99 if self.failed else self.route,int(not self.failed and self.route!=2),len(self.trace),m.reg_read(UC_X86_REG_FPSW)&63];out[4:8]=struct.unpack('<4I',m.mem_read(0x3ab9864,16));out[8:12]=struct.unpack('<4I',m.mem_read(0x3d0f7f0,16))
  for i,event in enumerate(self.trace):out[16+i*8:24+i*8]=event
  return out

def run(count=1000,seed=404660,output='late-dev.json'):
 image=PEImage(oracle_exe);assert image.sha256==SOURCE_SHA256;source=Source(image);U=ctypes.c_uint32;fn=ctypes.CDLL(str(oracle_library)).sadx_native_late_sprite;fn.argtypes=[ctypes.POINTER(U)]*2;rng=random.Random(seed);bad=[];flags=0;outcomes={}
 priorities=[-3.,-2.,-1.,0.,1.,9999.,10000.,22046.900390625]
 for case in range(count):
  v=[0]*48;v[:12]=[rng.choice([127,639,895,3199]),rng.choice([0,0,0,1]),rng.choice([0,1,1,2,0xffffffff]),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),struct.unpack('<I',struct.pack('<f',rng.choice(priorities+[rng.uniform(-100000,100000)])))[0],rng.randrange(5),rng.choice([0,1,1,1]),rng.choice([0,0,0,1,2,3,4,5,6]),rng.randrange(8),0];v[12:24]=[struct.unpack('<I',struct.pack('<f',rng.uniform(-2,2)))[0] for _ in range(12)];v[24]=rng.randrange(2);v[25:29]=[struct.unpack('<I',struct.pack('<f',rng.uniform(-2,2)))[0] for _ in range(4)]
  expected=source.run(v);out=(U*128)();assert fn((U*48)(*v),out)==1;different=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y and i!=3];flags+=expected[3]!=out[3];outcomes[expected[0]]=outcomes.get(expected[0],0)+1
  if different:bad.append({'case':case,'input':v,'differences':different[:16]})
 report={'cases':count,'mismatch_count':len(bad),'flag_mismatches':flags,'values_match':not bad,'passed':not bad and not flags,'mismatches':bad[:8],'outcomes':outcomes,'seed':seed,'limits':__doc__,'source_sha256':image.sha256};Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':count,'mismatches':len(bad),'flags':flags,'outcomes':outcomes}));return len(bad)>0
if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path)
 parser.add_argument('--output',type=Path,required=True)
 parser.add_argument('--cases',type=int,default=1000);parser.add_argument('--seed',type=int,default=404660)
 args=parser.parse_args();oracle_exe=args.exe;oracle_library=args.native_library.resolve()
 raise SystemExit(run(args.cases,args.seed,args.output))
