"""Run both complete original label/name entries; only actual rendering callees become receipts."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPSW,UC_X86_REG_FPCW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
CALLS={0x403070:1,0x432f40:2,0x4338d0:3,0x433f80:4}
TEXTURES={0x10d7be0:14,0x10d7be8:15,0x10d7bf0:28}
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for section in image.sections:
   if section['name'] in ('.text','.rdata'):continue
   size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095
   self.m.mem_map(image.base+section['rva'],size)
   if section['raw_size']:self.m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
  for a in CALLS:self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=a,end=a)
  self.initial=self.m.context_save()
 def word(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v))
 def hook(self,m,a,size,user):
  kind=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP)
  if kind==1:row=[kind,TEXTURES[self.word(sp+4)]]
  else:row=[kind]+[self.word(sp+4+i*4) for i in range({2:1,3:4,4:6}[kind])]
  self.trace.append(row+[0]*(8-len(row)));m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.trace=[];m=self.m;m.context_restore(self.initial);m.reg_write(UC_X86_REG_FPCW,v[12])
  self.put(0x8928c0,v[8]);self.put(0x8928c4,v[9]);self.put(0x3b0f0e8,v[11]);m.mem_write(0x3b2a2fa,bytes([v[10]&255]));m.mem_write(0x7ef6e8,bytes(v[13:20]));m.mem_write(0x7ef6f0,bytes(v[20:27]))
  sp=STACK-128;m.mem_write(sp,struct.pack('<8I',STOP,*v[1:8]));m.reg_write(UC_X86_REG_ESP,sp)
  m.emu_start(0x5071d0 if v[0] else 0x507050,STOP,timeout=1000000)
  assert m.reg_read(UC_X86_REG_EIP)==STOP and m.reg_read(UC_X86_REG_ESP)==sp+4
  out=[0]*64;out[0:3]=[len(self.trace),m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW)]
  for n,row in enumerate(self.trace):out[8+n*8:16+n*8]=row
  return out

def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def vectors(image,seed,count):
 rng=random.Random(seed);rows=[]
 def base(mode,index,variant,language,cw):return [mode,index,0x9b123456]+list(map(bits,(320,240,20,0.7,0.8,1,1)))+[variant,language,cw]+list(image.read(0x7ef6e8,7))+list(image.read(0x7ef6f0,7))
 for mode in (0,1):
  for index in range(7):
   for variant in (0,1,2,255):
    for language in (0,1,2,0xffffffff):
     for cw in (127,639,895,1151,2175,3199):rows.append(base(mode,index,variant,language,cw))
 for mode in (0,1):
  for index in (0,6):
   for variant in (0,1):
    for language in (0,1):
     for slot in range(3,10):
      for word in [0,0x80000000,bits(-1.5),bits(0.33333334),bits(1e-30),bits(1e30),0x7f800000,0xff800000,0x7fc12345,0x7fa12345]:
       v=base(mode,index,variant,language,127);v[slot]=word;rows.append(v)
 for _ in range(count):
  v=base(rng.randrange(2),rng.randrange(7),rng.randrange(4),rng.choice((0,1,0xffffffff)),rng.choice((127,639,895,1151,2175,3199)));v[2]=rng.getrandbits(32)
  for i in range(3,10):v[i]=bits(rng.uniform(-500,500) if i<6 else rng.uniform(-2,3))
  for i in range(13,27):v[i]=rng.randrange(256)
  rows.append(v)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=507050);p.add_argument('--cases',type=int,default=1800);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256
 source=Source(image);probe=ctypes.CDLL(a.library).sadx_native_character_select_labels_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;bad=[];flags=0;rows=vectors(image,a.seed,a.cases)
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*64)();probe((ctypes.c_uint32*27)(*v),out);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if i!=1 and x!=y];flags+=expected[1]!=out[1]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 stage=Path(__file__).resolve().parents[2];paths=['src/game/character_select_labels.hpp','src/game/character_select_labels.cpp','tests/probes/native_character_select_labels_probe.cpp']
 report={'source_sha256':image.sha256,'source_ranges':{f'{hex(x)}-{hex(y)}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in [(0x507050,0x5071c4),(0x5071d0,0x5072ca)]},'passed':not bad and not flags,'values_match':not bad,'cases':len(rows),'executed':n+1,'mismatches':len(bad),'fp_flag_mismatches':flags,'first_mismatches':bad,'native_sources':{f:hashlib.sha256((stage/f).read_bytes()).hexdigest() for f in paths},'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'limits':['Full original label/name entries execute; only real texture-select/material/unscaled/scaled PVR callees become receipts. Arrays include independent nonidentity bytes; canonical lookup addresses and texture identities are verified by actual original reads/calls.','Source required callers use indices0..6. Other indices fail at explicit source indexed-read boundaries instead of fabricated data.','All compared call words and FPCW are exact; FP flags reported separately. Unmasked exception delivery and actual pixels require composition beyond this helper judge.']}
 Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or flags)
if __name__=='__main__':raise SystemExit(main())
