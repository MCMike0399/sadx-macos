"""Original bitmap-font load/copy instructions; texture loading is an ordered resource receipt."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
INFO=DATA+0x1000;TEXTURE=DATA+0x2000;PROP=DATA+0x3000;OLD=DATA+0x4000
NATIVE_PATHS=['src/engine/font_native.cpp', 'src/engine/font_native.hpp', 'tests/probes/native_font_native_probe.cpp']
def native_sources():
    root=Path(__file__).resolve().parents[2]
    return {name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in NATIVE_PATHS}

class Source:
 def __init__(self,image):
  self.m=Original(image,0x7f).machine;self.m.mem_map(0x3b28000,4096);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
 def word(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v))
 def hook(self,m,a,size,user):
  if a not in (0x4228e0,0x403290):return
  sp=m.reg_read(UC_X86_REG_ESP);assert self.word(sp+4)==TEXTURE;self.receipt=[2 if a==0x403290 else 1,self.word(0x3b280ec),self.word(0x3b280bc),int(self.word(0x3b280e8)!=0)];m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.m.context_restore(self.initial);self.receipt=[0]*4
  self.m.mem_write(INFO,struct.pack('<10I',*v[4:8],TEXTURE if v[2] else 0,PROP if v[3] else 0,*v[8:12]));self.m.mem_write(0x3b280c0,struct.pack('<10I',*v[15:19],TEXTURE if v[19] else 0,PROP if v[20] else 0,*v[21:25]));self.write(0x3b280ec,v[12]);self.write(0x3b280bc,v[13]);self.write(0x3b280e8,OLD if v[14] else 0)
  self.m.mem_write(STACK-0x100,struct.pack('<II',STOP,INFO if v[0] or v[1] else 0));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.emu_start(0x420a30 if v[0]==2 else 0x420a50 if v[0] else 0x420a00,STOP,timeout=0);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*32;source=self.word(0x3b280e8);out[:3]=[self.word(0x3b280ec),self.word(0x3b280bc),0 if source==0 else 1 if source==INFO else 2];current=list(struct.unpack('<10I',self.m.mem_read(0x3b280c0,40)));current[4]=int(current[4]!=0);current[5]=int(current[5]!=0);out[3:13]=current;out[16:20]=self.receipt;return out

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1200);p.add_argument('--seed',type=int,default=420050);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);rng=random.Random(a.seed)
 vectors=[]
 for mode in (0,1,2):
  for present in (0,1):
   for textures in (0,1):
    for prop in (0,1):
     for bits in (0,0x80000000,0x3f800000,0x7f800000,0x7fc00001,0x7f800001,0xffffffff):
      v=[rng.getrandbits(32) for _ in range(25)];v[:4]=[mode,present,textures,prop];v[4:8]=[bits]*4;vectors.append(v)
 for _ in range(a.cases):
  v=[rng.getrandbits(32) for _ in range(25)];v[:4]=[rng.randrange(2) for _ in range(4)];v[0]=rng.randrange(3);vectors.append(v)
 lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_font_native_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;bad=[]
 for n,v in enumerate(vectors):
  expected=source.run(v);out=(ctypes.c_uint32*32)();probe((ctypes.c_uint32*25)(*v),out)
  if list(out)!=expected:
   bad.append({'case':n,'input':v,'expected':expected,'native':list(out)})
   if len(bad)==8:break
 result={'passed':not bad,'cases':len(vectors),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'native_sources':native_sources(),'source_functions':{f'0x{x:x}-0x{x+n:x}':hashlib.sha256(image.read(x,n)).hexdigest() for x,n in [(4327936, 47), (4328016, 21), (4327984,21)]},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'source_ranges':{hex(x):hashlib.sha256(image.read(x,n)).hexdigest() for x,n in [(0x420a00,0x2f),(0x420a50,0x15)]},'limits':['Original load, copy and release execute. Texture backend is a receipt comparing published state. Pointer identities are native resources; float payloads compare as unchanged raw words.']};Path(a.output).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
