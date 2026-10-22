"""Full433410 + actual matrix primitives; only final njSetCamera is an ordered lower receipt."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path.cwd()));sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import Original,DATA,STACK,STOP
MATRIX=DATA+0x4000;TASK=DATA+0x100;POSE=DATA+0x200;CAMERA=0x3aad0a0
sha=lambda b:hashlib.sha256(b).hexdigest()
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
class Source(Original):
 def __init__(self,image,unit):
  super().__init__(image,127);m=self.machine
  for p in [0x389d000,0x3d0f000,0x3aad000]:m.mem_map(p,4096)
  m.mem_write(0x389d650,unit);self.trace=[];m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=m.context_save()
 def put(self,a,x):self.machine.mem_write(a,struct.pack('<I',x))
 def word(self,a):return struct.unpack('<I',self.machine.mem_read(a,4))[0]
 def hook(self,m,a,n,user):
  if a==0x781250:
   sp=m.reg_read(UC_X86_REG_ESP);assert self.word(sp+4)==CAMERA
   self.trace.append(tuple(struct.unpack('<8I',m.mem_read(CAMERA,32))))
   m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v,cw):
  m=self.machine;m.context_restore(self.initial);self.trace=[]
  m.mem_write(TASK,bytes(52));self.put(TASK+32,POSE);m.mem_write(POSE,bytes(64));m.mem_write(POSE+20,struct.pack('<6I',*v[:6]));m.mem_write(CAMERA,struct.pack('<8I',*v[8:16]));m.mem_write(MATRIX,struct.pack('<96I',*v[16:]));
  for a,x in [(0x3d0f818,MATRIX+(v[6]-1)*64),(0x3d0fa28,6),(0x3d0fa2c,v[6]),(0x3d0f7ac,v[7])]:self.put(a,x)
  m.mem_write(STACK,struct.pack('<II',STOP,TASK));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0)
  m.emu_start(0x433410,STOP,timeout=0);assert m.reg_read(UC_X86_REG_EIP)==STOP and m.reg_read(UC_X86_REG_ESP)==STACK+4
  assert len(self.trace)==1
  return [*struct.unpack('<8I',m.mem_read(CAMERA,32)),self.word(0x3d0fa2c),m.reg_read(UC_X86_REG_FPSW)&63,len(self.trace),*self.trace[0],*struct.unpack('<96I',m.mem_read(MATRIX,384))]
def fixtures(unit,n,seed):
 r=random.Random(seed);frames=list(struct.unpack('<16I',unit))*6;rows=[]
 for a in [0,1,15,16,16384,32768,49152,65535,65536,-1,-65536,-2147483648,2147483647]:
  for axis in range(3):
   for depth in [1,2,6]:
    for norm in [0,1]:
     angles=[0,0,0];angles[axis]=a&0xffffffff;rows.append([*angles,*map(bits,[1,-2,3]),depth,norm,*[r.getrandbits(32) for _ in range(8)],*frames])
 for _ in range(n):
  rows.append([*[r.getrandbits(32) for _ in range(3)],*[bits(r.uniform(-100,100)) for _ in range(3)],r.choice([1,2,5,6]),r.choice([0,1,0x80000000]),*[r.getrandbits(32) for _ in range(8)],*[bits(r.uniform(-4,4)) for _ in range(96)]])
 return rows
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('native_library',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--native-source-root',type=Path,default=ROOT);p.add_argument('--cases',type=int,default=2000);p.add_argument('--seed',type=int,default=433410);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256
 unit=image.read(0x389d650,64);table=image.read(TABLE_ADDRESS,4100);rows=fixtures(unit,a.cases,a.seed);original=Source(image,unit);library=ctypes.CDLL(str(a.native_library.resolve()));fn=library.sadx_native_selection_camera;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.POINTER(ctypes.c_ubyte),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
 tb=(ctypes.c_ubyte*4100).from_buffer_copy(table);ub=(ctypes.c_ubyte*64).from_buffer_copy(unit);mismatch=flags=tested=0;examples=[]
 for cw in [127,639,895,1151,1663,1919,2175,2687,2943,3199,3711,3967]:
  for i,row in enumerate(rows):
   expected=original.run(row,cw);out=(ctypes.c_uint32*115)();status=fn(tb,ub,(ctypes.c_uint32*112)(*row),cw,out);assert status==0,(i,status)
   actual=list(out);bad=[k for k in range(115) if k!=9 and actual[k]!=expected[k]];mismatch+=bool(bad);flags+=actual[9]!=expected[9];tested+=1
   if bad and len(examples)<8:examples.append({'case':i,'cw':cw,'indices':bad[:10],'expected':[hex(expected[k]) for k in bad[:10]],'actual':[hex(actual[k]) for k in bad[:10]]})
 report={'passed':mismatch==0 and flags==0,'values_match':mismatch==0,'cases':tested,'mismatches':mismatch,'fp_flag_mismatches':flags,'first_mismatches':examples,'seed':a.seed,'source_sha256':image.sha256,'source_ranges':{hex(x)+'-'+hex(y):sha(image.read(x,y-x)) for x,y in [(0x433410,0x4334e8),(0x7850f0,0x785137),(0x784e20,0x78506f),(0x784d60,0x784e1e),(0x785140,0x785162),(0x51f680,0x51f681)]},'native_library_sha256':sha(a.native_library.read_bytes()),'native_sources':{str(f.relative_to(a.native_source_root)):sha(f.read_bytes()) for f in [a.native_source_root/'src/game/selection_camera.hpp',a.native_source_root/'src/game/selection_camera.cpp',a.native_source_root/'tests/probes/native_selection_camera_probe.cpp']},'limits':['Actual433410 and actual Ninja matrix primitives execute; njSetCamera781250 lower service records all eight camera words, not GPU projection implementation.','All six retained matrix slots and depth/capacity failure, low16 rotation guards and full32 roll negation compared. Twelve x87 control words; flag-only differences are partial.','Existing shared view::Camera and NinjaMatrixPrimitives borrowed; no duplicate globals or native task/factory claim.','FP status differences remain undiagnosed in this caller; strict reports remain partial, not hardware flag equivalence.']};a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','cases','mismatches','fp_flag_mismatches','first_mismatches']}));return 0 if report['passed'] else 1
if __name__=='__main__':sys.exit(main())
