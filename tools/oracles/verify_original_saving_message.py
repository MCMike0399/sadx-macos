"""Original saving-message caller composed with real MSG24 rasterizer/open/clear/queue.

The real arena-end pointer calculation, null cooperative wait and MSG_Disp execute.
Fog/box/device-reset are receipts; arena reset still zeroes the SAME bitmap bytes.
No native upload/render delivery is claimed. Source queue entries remain identities.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
CONTEXT=0x3abdca8;BITMAP=0x14000000;SINGLE=0x12000000;WIDE=0x13000000
class Source:
 def __init__(self,image,single,wide,widths):
  self.m=Original(image,127).machine
  for page,size in [(0x3abd000,8192),(0x3ab9000,4096),(0x3b0e000,8192),(0x3b28000,4096),(0x3b11000,4096),(0x38f6000,4096),(0x909000,4096),(0x892000,4096),(SINGLE,65536),(WIDE,0x100000),(BITMAP,0x21000)]:self.m.mem_map(page,size)
  self.m.mem_write(SINGLE,single);self.m.mem_write(WIDE,wide);self.m.mem_write(0x3abe250,bytes(widths));self.m.mem_write(0x909f30,image.read(0x909f30,100))
  for index in range(5):self.put(0x909f38+index*20,SINGLE);self.put(0x909f3c+index*20,WIDE)
  for entry in (0x403db0,0x4014b0,0x411b40,0x4071c0,0x407ab0,0x403e70,0x40d490):
   self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=entry,end=entry)
  self.initial=self.m.context_save()
 def put(self,address,value,n=4):self.m.mem_write(address,(value&((1<<(8*n))-1)).to_bytes(n,'little'))
 def word(self,address,n=4):return int.from_bytes(self.m.mem_read(address,n),'little')
 def hook(self,m,address,size,user):
  tags={0x403db0:1,0x4014b0:2,0x411b40:3,0x4071c0:4,0x407ab0:5,0x403e70:6,0x40d490:7}
  if address not in tags:return
  self.events.append(tags[address]);sp=m.reg_read(UC_X86_REG_ESP)
  if address==0x403db0:self.out[64]=self.word(sp+4)
  if address==0x4071c0:self.out[65:69]=[self.word(sp+x) for x in (12,16,20,24)]
  if address==0x407ab0:self.m.mem_write(BITMAP+self.v[12],bytes(131104));self.put(0x3b1117c,0)
  if address in (0x411b40,0x4071c0,0x407ab0):m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.out=[0]*128;self.events=[];m=self.m;m.context_restore(self.initial);m.reg_write(UC_X86_REG_FPCW,v[10]);m.reg_write(UC_X86_REG_FPSW,0)
  m.mem_write(CONTEXT,bytes(72));self.put(CONTEXT,v[13],1);self.put(CONTEXT+68,v[14]);m.mem_write(BITMAP,bytes([165])*0x21000)
  for address,value,n in [(0x3abdf79,v[0],1),(0x3b28114,v[1],4),(0x3b280a8,v[2],1),(0x8928c0,v[3],4),(0x8928c4,v[4],4),(0x3b0f0e8,v[5],4),(0x3ab98ac,v[6],4),(0x3b1117c,v[7],4),(0x3abe134,v[8],1),(0x3b0e338,v[9],4),(0x3ab9874,BITMAP+v[12]+131104,4),(0x38f6e08,0,4)]:self.put(address,value,n)
  for index in range(4):self.put(0x3abe13c+index*4,DATA+0x5000+index*16)
  m.mem_write(STACK-256,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK-256);m.emu_start(0x40be40,STOP,timeout=0);assert m.reg_read(UC_X86_REG_EIP)==STOP
  self.out[:6]=[self.word(a,n) for a,n in [(0x3abdf79,1),(0x3b28114,4),(0x3b280a8,1),(0x3ab98ac,4),(0x3b1117c,4),(0x3b0e338,4)]]
  for index in range(4):self.out[6+index]=1 if self.word(0x3abe13c+index*4)==CONTEXT else index+2
  fields=[(0,1),(2,2),(4,2),(6,2),(8,2),(10,2),(12,2),(14,2),(16,2),(18,2),(20,2),(22,2),(24,2),(26,2),(28,4),(32,4),(36,4),(40,4),(44,4),(68,4)]
  self.out[16:36]=[self.word(CONTEXT+offset,n) for offset,n in fields]
  if self.out[33]:self.out[33]=self.out[33]-(BITMAP+v[12])+1
  self.out[36]=m.reg_read(UC_X86_REG_FPSW)&0x3f;self.out[79]=len(self.events);self.out[80:80+len(self.events)]=self.events
  return self.out,bytes(m.mem_read(BITMAP+v[12],131104))
def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
 for pending in (0,1,255):
  for pause in (0,1,2,3):
   for reset in (0,1,255):
    for alignment in (0,1,15,31):rows.append([pending,pause,reset,bits(1),bits(1),1,0,0,1,0,127,0,alignment,0,0])
 for _ in range(count):rows.append([rng.randrange(2),rng.randrange(4),rng.randrange(2),bits(rng.choice([1,0,2,-1,1.25,0.75,1.3])),bits(rng.choice([1,0,2,-1,1.5,0.5,1.3])),rng.randrange(5),rng.randrange(3),rng.getrandbits(1),rng.randrange(2),rng.randrange(6),rng.choice([127,639,895,3711]),0,rng.randrange(32),rng.randrange(4),rng.getrandbits(32)])
 return rows
def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--font-directory',default='reference/updated-pc-reference/game/system');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=400040);p.add_argument('--cases',type=int,default=1000);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;folder=Path(a.font_directory);single=(folder/'FONTDATA1.BIN').read_bytes();wide=(folder/'FONTDATA0.BIN').read_bytes()
 import importlib.util
 spec=importlib.util.spec_from_file_location('fonts',Path(__file__).resolve().parents[1]/'import/message_fonts.py');fonts=importlib.util.module_from_spec(spec);spec.loader.exec_module(fonts);records,_=fonts.decode_records(single);widths=[fonts.generated_width(records[x]) for x in range(32,256)];source=Source(image,single,wide,widths);lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_saving_message_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8)];sa=(ctypes.c_uint8*len(single)).from_buffer_copy(single);wa=(ctypes.c_uint8*len(wide)).from_buffer_copy(wide);width_array=(ctypes.c_uint8*224)(*widths);rows=vectors(a.seed,a.cases);bad=[];partials=0
 for index,v in enumerate(rows):
  expected,bitmap=source.run(v);out=(ctypes.c_uint32*128)();pixels=(ctypes.c_uint8*131104)(*([165]*131104));probe((ctypes.c_uint32*15)(*v),sa,len(single),wa,len(wide),width_array,out,pixels);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y and i!=36];pixel_diff=sum(x!=y for x,y in zip(bitmap,pixels));partials+=int(expected[36]!=out[36])
  if diff or pixel_diff:bad.append({'case':index,'input':v,'diff':diff,'pixel_mismatches':pixel_diff})
  if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];files=['src/game/saving_message.hpp','src/game/saving_message.cpp','src/game/menu_caption.hpp','src/game/menu_caption.cpp','tests/probes/native_saving_message_probe.cpp'];report={'passed':not bad and not partials,'values_match':not bad,'cases':len(rows),'mismatches':len(bad),'fp_partials':partials,'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'native_sources':{f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files},'limits':[__doc__]};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or partials)
if __name__=='__main__':raise SystemExit(main())
