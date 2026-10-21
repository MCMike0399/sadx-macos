"""Pinned compiled loader scope and MSG-forwarding qualification.
Actual wrapper100ae290 and actual scale_trampoline10038ef0 run. Original drawing
is an ordered receipt; active sprite_push is an explicit unsupported boundary.
Movie wrapper100aea50 is also executed with its temporary fill state. Scopes run against actual vector ownership with preallocated capacity; allocation
and C++runtime vector growth are not claimed. No sprite or FPpixel claim.
"""
import argparse,ctypes,hashlib,json,random,struct,sys
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
root=Path(__file__).resolve().parents[2];stage=root
source_dll=None;native_library=None;report_path=None;native_source_root=root
sys.path[:0]=[str(root/'tools/lib'),str(root/'tools/oracles')]
from pe_image import PEImage
from verify_original_math import Original,DATA,STACK,STOP
DLL_SHA='3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba'
BEGIN=DATA+0x2000;BODY=DATA+0x1000;OBJECT=DATA+0x100;SCRATCH=0x38f6ce4
class Source:
 def __init__(self):
  self.image=im=PEImage(source_dll);assert im.sha256==DLL_SHA
  self.m=m=Original(im,127).machine
  for s in im.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['raw_size'],s['virtual_size'])+4095)&~4095
   m.mem_map(im.base+s['rva'],size);m.mem_write(im.base+s['rva'],im.slice(s['file_offset'],s['raw_size']))
  m.mem_map(0x892000,0x2000);m.mem_map(0x38f6000,0x1000)
  m.mem_protect(DATA,0x10000,unicorn.UC_PROT_ALL)
  m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=BODY,end=BODY)
  m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=0x100ae1f0,end=0x100ae1f0)
  self.clean=m.context_save()
 def put(self,a,v):self.m.mem_write(a,(v&0xffffffff).to_bytes(4,'little'))
 def word(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def boolean(self,a,v):self.m.mem_write(a,bytes([int(v)]))
 def predicate(self):
  enabled=bool(self.m.mem_read(0x100fa2a1,1)[0]);end=self.word(0x100f8b6c);bg=end!=BEGIN and bool(self.m.mem_read(end-4,1)[0]);fill=self.m.mem_read(0x100f15a1,1)[0]
  return int(enabled and (not bg or fill!=0))
 def setup(self,enabled,fill,count,bg=True):
  m=self.m;m.context_restore(self.clean);self.boolean(0x100fa2a1,enabled);self.boolean(0x100f15a1,fill)
  self.put(0x100f8b68,BEGIN);self.put(0x100f8b6c,BEGIN+count*16);self.put(0x100f8b70,BEGIN+0x1000)
  for i in range(count):m.mem_write(BEGIN+i*16,struct.pack('<IffI',54,1,1,int(bg)))
  self.put(OBJECT+8,BODY);self.put(0x100f8bb8,OBJECT);self.put(0x100fa2fc,OBJECT);self.put(0x100fa2e0,OBJECT)
  self.calls=0;self.failed=False;self.out=[0]*64
 def hook(self,m,a,size,user):
  if a==0x100ae1f0:self.out[0]=0;m.emu_stop();return
  sp=m.reg_read(UC_X86_REG_ESP)
  if self.kind=='scope':
   row=16+self.calls*6;self.calls+=1;end=self.word(0x100f8b6c)
   self.out[row:row+6]=[m.mem_read(0x100fa2a1,1)[0],(end-BEGIN)//16,self.word(0x8928c0),self.word(0x8928c4),m.mem_read(end-16,1)[0],m.mem_read(end-4,1)[0]]
   if self.calls==self.v[6]:self.failed=True;m.emu_stop();return
   if self.calls<self.v[5]:m.reg_write(UC_X86_REG_EIP,0x100aea50 if self.v[8] else 0x10038eb0);return
   if self.v[7]:self.put(0x100f8b6c,BEGIN)
  else:self.out[:6]=[1,*struct.unpack('<5I',m.mem_read(sp+4,20))]
  m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def scope(self,v):
  self.kind='scope';self.v=v;self.setup(v[1],v[0],v[2]);m=self.m
  self.boolean(0x100f15a0,v[9]);self.put(0x8928c0,v[3]);self.put(0x8928c4,v[4]);m.mem_write(STACK,struct.pack('<3I',STOP,0x3f800000,0x40000000));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x100aea50 if v[8] else 0x10038eb0,STOP,timeout=0)
  self.out[:7]=[int(not self.failed),self.calls,m.mem_read(0x100fa2a1,1)[0],(self.word(0x100f8b6c)-BEGIN)//16,self.word(0x8928c0),self.word(0x8928c4),self.predicate()]
  self.out[7]=m.mem_read(0x100f15a1,1)[0]
  return self.out
 def forward(self,v):
  self.kind='forward';self.setup(v[0],v[1],v[2],v[3]);m=self.m
  words=[SCRATCH,*v[5:9]];assert v[4]==SCRATCH
  m.mem_write(STACK,struct.pack('<6I',STOP,*words));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x100ae290,STOP,timeout=0)
  if self.out[0]==0:self.out[1:6]=words # The native guard rejects before original body.
  return self.out[:6]
def run(seed=404660,count=800,output='dev.json'):
 source=Source();U=ctypes.c_uint32;lib=ctypes.CDLL(str(native_library));scope=lib.sadx_ui_scale_scope;forward=lib.sadx_ui_scale_forwarding
 scope.argtypes=forward.argtypes=[ctypes.POINTER(U)]*2;rng=random.Random(seed);bad=[];rows=[]
 for case in range(count+200):
  v=[rng.choice([1,2]),rng.randrange(2),rng.randrange(4),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(1,6),rng.choice([0,0,0,1,2,3]),rng.randrange(2),int(case>=count),1]
  expected=source.scope(v);out=(U*64)();assert scope((U*10)(*v),out)==1
  if list(out)!=expected:bad.append({'kind':'scope','case':case,'input':v,'expected':expected,'native':list(out)})
 for enabled in [0,1]:
  for fill in [0,1,2]:
   for depth in [0,1,3]:
    for bg in [0,1]:
     v=[enabled,fill,depth,bg,SCRATCH,rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32)];expected=source.forward(v);out=(U*6)();assert forward((U*9)(*v),out)==1
     if list(out)!=expected:bad.append({'kind':'forward','input':v,'expected':expected,'native':list(out)})
 report={'seed':seed,'scope_cases':count,'movie_scope_cases':200,'wrapper_cases':36,'mismatch_count':len(bad),'passed':not bad,'mismatches':bad[:6],'source_dll_sha256':DLL_SHA,'limits':__doc__};report['native_library_sha256']=hashlib.sha256(native_library.read_bytes()).hexdigest()
 report['native_sources']={name:hashlib.sha256((native_source_root/name).read_bytes()).hexdigest() for name in ['src/engine/ui_scale_profile.hpp','src/engine/ui_scale_profile.cpp','tests/probes/native_ui_scale_profile_probe.cpp']}
 report['source_module']='LOADER'
 report['source_ranges']={f'{a:#x}-{b:#x}':hashlib.sha256(source.image.read(a,b-a)).hexdigest() for a,b in [(0x10038eb0,0x10039089),(0x100ae290,0x100ae39f),(0x100aea50,0x100aea84),(0x100887d0,0x10088960)]}
 report_path.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:report[key] for key in ['scope_cases','movie_scope_cases','wrapper_cases','mismatch_count','passed']}));return bool(bad)
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('source_dll',type=Path);parser.add_argument('native_library',type=Path);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--seed',type=int,default=404660);parser.add_argument('--cases',type=int,default=800);parser.add_argument('--native-source-root',type=Path,default=root);args=parser.parse_args()
 source_dll=args.source_dll;native_library=args.native_library;report_path=args.output;native_source_root=args.native_source_root
 raise SystemExit(run(args.seed,args.cases))
