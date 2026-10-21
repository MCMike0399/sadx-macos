"""Execute full5125a0 callback; actual matrix/device/object boundaries become ordered receipts."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
TASK=DATA+0x1000;PRIMARY=DATA+0x1100;DEVICE=DATA+0x4000;VTABLE=DATA+0x5000;SET_STATE=CODE+0x200
CALLS={0x7850f0:1,0x785070:2,0x784fb0:3,0x784ee0:4,0x784e20:5,SET_STATE:6,0x403070:7,0x408530:8,0x785140:9}
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for s in image.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095;self.m.mem_map(image.base+s['rva'],size)
   if s['raw_size']:self.m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
  self.m.mem_write(SET_STATE,b'\xc3');self.put(0x3d128b0,DEVICE);self.put(DEVICE,VTABLE);self.put(VTABLE+0xc8,SET_STATE);self.put(TASK+0x20,PRIMARY)
  for a in CALLS:self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=a,end=a)
  self.initial=self.m.context_save()
 def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
 def word(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def hook(self,m,a,size,user):
  kind=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP);pop=4
  if kind in (1,2,3,4,5):assert m.reg_read(UC_X86_REG_ECX)==0
  if kind==1:row=[kind];m.reg_write(UC_X86_REG_EAX,self.v[7])
  elif kind==2:
   assert m.reg_read(UC_X86_REG_EDX)==PRIMARY+0x20
   row=[kind]+[self.word(PRIMARY+0x20+4*i) for i in range(3)]
  elif kind in (3,4,5):row=[kind,m.reg_read(UC_X86_REG_EDX)]
  elif kind==6:
   assert self.word(sp+4)==DEVICE
   row=[kind,self.word(sp+8),self.word(sp+12)];pop=16;m.reg_write(UC_X86_REG_EAX,self.v[8])
  elif kind==7:
   assert self.word(sp+4)==0x10d7c00;row=[kind,2]
  elif kind==8:
   assert self.word(sp+4)==0x10d7774;row=[kind,2942]
  else:row=[kind,m.reg_read(UC_X86_REG_ECX)]
  self.trace.append(row+[0]*(8-len(row)));m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+pop)
 def run(self,v):
  self.v=v;self.trace=[];m=self.m;m.context_restore(self.initial);self.put(0x3b1118c,v[0])
  # Actual factory-zero mode/scale are not callback inputs. Poisoned untouched
  # fields discriminate invented initialization; only yaw may be written here.
  primary=bytearray([165]*64)
  struct.pack_into('<3I',primary,0x14,*v[1:4]);struct.pack_into('<3I',primary,0x20,*v[4:7]);m.mem_write(PRIMARY,bytes(primary))
  sp=STACK-128;m.mem_write(sp,struct.pack('<2I',STOP,TASK));m.reg_write(UC_X86_REG_ESP,sp);m.emu_start(0x5125a0,STOP,timeout=1000000)
  assert m.reg_read(UC_X86_REG_EIP)==STOP and m.reg_read(UC_X86_REG_ESP)==sp+4
  after=bytes(m.mem_read(PRIMARY,64));assert after[:0x18]==primary[:0x18] and after[0x1c:]==primary[0x1c:]
  out=[0]*96;out[0]=len(self.trace);out[1:4]=struct.unpack('<3I',after[0x14:0x20]);out[4:7]=v[4:7]
  for n,row in enumerate(self.trace):out[8+n*8:16+n*8]=row
  return out

def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 def base(ticks,pitch,yaw,roll):return [ticks,pitch,yaw,roll,bits(400),bits(0),bits(-20),1,0]
 words=[0,1,65535,65536,0xffff0000,0x7fffffff,0x80000000,0xffffffff]
 for ticks in (0,1,2,0xffffffff,0x80000000):
  for pitch in words:
   for roll in words:
    for yaw in (0,16384,65536,0x80000000,0xffffffff):rows.append(base(ticks,pitch,yaw,roll))
 for ticks in (0,1,2,0xffffffff,0x80000000):
  v=base(ticks,0,(-ticks*728)&0xffffffff,0);rows.append(v)
 for push in (0,1,0xdeadbeef):
  for hresult in (0,1,0x80004005,0xffffffff):
   v=base(1,0,16384,0);v[7:9]=[push,hresult];rows.append(v)
 for slot in (4,5,6):
  for word in (0,0x80000000,0x7f800000,0xff800000,0x7fa12345,0x7fc12345):
   v=base(1,0,16384,0);v[slot]=word;rows.append(v)
 for _ in range(count):
  v=[rng.getrandbits(32) for _ in range(9)];rows.append(v)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1600);p.add_argument('--seed',type=int,default=512540);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);probe=ctypes.CDLL(a.library).sadx_native_selection_question_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;rows=vectors(a.seed,a.cases);bad=[]
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*96)();probe((ctypes.c_uint32*9)(*v),out)
  if list(out)!=expected:
   bad.append({'case':n,'input':v,'diff':[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]})
   if len(bad)==8:break
 stage=Path(__file__).resolve().parents[2];paths=['src/game/selection_question.hpp','src/game/selection_question.cpp','tests/probes/native_selection_question_probe.cpp'];report={'source_sha256':image.sha256,'source_ranges':{'0x5125a0-0x51263b':hashlib.sha256(image.read(0x5125a0,0x9b)).hexdigest()},'passed':not bad,'values_match':not bad,'cases':len(rows),'executed':n+1,'mismatches':len(bad),'first_mismatches':bad,'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'native_sources':{f:hashlib.sha256((stage/f).read_bytes()).hexdigest() for f in paths},'limits':['Entire5125a0 executes. Only actual existing matrix primitives, COM render-state call, texture selection, dsDrawObject and tail pop become ordered receipts; final matrices, culling, object rendering and pixels require their real native owners.','All32-bit angular guards/low16 arguments, modulo yaw arithmetic, unchanged pose fields, ignored push/HRESULT results and all draw calls are compared exactly. No mode/offscreen/scale predicate exists in this source callback.','Source callback is exec/update/render combined; no initializer, separate display callback or destructor is invented. Assets retain existing fixed menu2/object2942 bindings from the combined pack.','Missing native render service must fail explicitly; no visibility shortcut or automatic cleanup rollback is supplied. Product callback/runtime composition remains root-owned.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
