"""Original MSG24 lookup/advance/rasterizer/plain-text/font initialization and dirty flag.

The actual user's FONTDATA0/1 files provide bitmap bytes, not the unrelated menu font.
Every bitmap word is compared, including allocation padding reached by source clipping.
Aligned text prefixes7/9 and invalid asset/resource indices remain explicit boundaries.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
CONTEXT=DATA+0x1000;MAP=DATA+0x3000;TEXT=DATA+0x6000
SINGLE=0x12000000;WIDE=0x13000000;BITMAP=0x14000000
ENTRIES=[0x40d8c0,0x40d970,0x40d9f0,0x40e570,0x40e2e0,0x40d290]
class Source:
 def __init__(self,image,single,wide):
  self.m=Original(image,127).machine;self.image=image;self.single=single;self.wide=wide
  for page,size in ((0x909000,4096),(0x3b0f000,4096),(0x3abe000,4096),(SINGLE,0x10000),(WIDE,0x100000),(BITMAP,0x200000)):
   self.m.mem_map(page,size)
  self.m.mem_write(SINGLE,single);self.m.mem_write(WIDE,wide);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=0x422310,end=0x422310);self.initial=self.m.context_save()
 def word(self,address,n=4):return int.from_bytes(self.m.mem_read(address,n),'little')
 def put(self,address,value,n=4):self.m.mem_write(address,(value&((1<<(n*8))-1)).to_bytes(n,'little'))
 def hook(self,m,address,size,user):
  sp=m.reg_read(UC_X86_REG_ESP);target=self.word(sp+8);family=0 if target==0x3abe130 else 1
  self.trace.extend([family,int(bool(self.word(0x3abe130))),int(bool(self.word(0x3abe138))),int(bool(self.word(0x909f3c))),int(bool(self.word(0x909f38)))])
  self.put(target,(WIDE if self.v[13] else 0) if family==0 else SINGLE)
  m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v,pixels,text):
  self.v=v;self.trace=[];self.m.context_restore(self.initial);self.m.mem_write(0x909f30,self.image.read(0x909f30,100));self.put(0x3b0f0e8,v[2]);self.put(0x3abe130,0);self.put(0x3abe138,0)
  for language in range(5):
   row=0x909f30+language*20;self.put(row,MAP if v[14] else 0)
   if v[0]!=4:self.put(row+8,SINGLE);self.put(row+12,WIDE if v[13] else 0)
  self.m.mem_write(MAP,bytes(x&255 for x in v[256:480]));self.m.mem_write(0x3abe250,bytes(x&255 for x in v[32:256]));self.m.mem_write(CONTEXT,bytes(72));self.m.mem_write(BITMAP,pixels);self.m.mem_write(TEXT,text+b'\0')
  for offset,index,n in [(40,3,4),(10,4,2),(12,5,2),(18,6,2),(20,7,2),(22,8,2),(24,9,2),(26,10,2),(68,11,4)]:self.put(CONTEXT+offset,BITMAP if offset==40 and v[3] else 0 if offset==40 else v[index],n)
  args={0:[v[1]],1:[v[1]],2:[CONTEXT,v[1]],3:[CONTEXT,TEXT],4:[],5:[CONTEXT]}[v[0]]
  self.m.mem_write(STACK-256,struct.pack('<'+'I'*(len(args)+1),STOP,*(x&0xffffffff for x in args)));self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.emu_start(ENTRIES[v[0]],STOP,timeout=0);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
  returned=self.m.reg_read(UC_X86_REG_EAX);out=[0]*512
  if v[0]==0:
   if returned==SINGLE:out[0]=3
   elif SINGLE+16<=returned<SINGLE+len(self.single):out[0]=1;out[1]=(returned-SINGLE-16)//88
   elif WIDE+16<=returned<WIDE+len(self.wide):out[0]=2;out[1]=(returned-WIDE-16)//88
   else:raise ValueError('source glyph outside imported resource: '+hex(returned))
  elif v[0]==1:out[0]=returned
  out[2]=self.word(CONTEXT+18,2);out[3]=self.word(CONTEXT+20,2);out[4]=self.word(CONTEXT+68);out[32:256]=self.m.mem_read(0x3abe250,224);out[299]=len(self.trace)//5;out[300:300+len(self.trace)]=self.trace
  return out,bytes(self.m.mem_read(BITMAP,len(pixels)))

def vectors(seed,count,widths):
 rng=random.Random(seed);rows=[]
 def base(kind,code=65):
  v=[0]*512;v[:15]=[kind,code,1,1,64,48,0,0,0,0,65535,1,64*80+1,1,0];v[32:256]=widths;v[256:480]=range(32,256);return v
 for kind in (0,1):
  for language in range(5):
   for code in list(range(32,256))+[0x8140,0x817e,0x8180,0x81fc,0x8240]:
    v=base(kind,code);v[2]=language;v[13]=rng.randrange(2);rows.append((v,b'Now Saving...'))
 for x in (-24,-2,-1,0,1,39,40,63,64):
  for y in (-24,-23,-2,-1,0,24,47,48):
   for color in (0,0x8000,0x1234,0xffff):
    v=base(2);v[6]=x&65535;v[7]=y&65535;v[10]=color;rows.append((v,b''))
 for code in (32,65,87,255):
  for width in (16,64):
   for offset in (-1,0,1):
    v=base(2,code);v[4]=width;v[6]=(width-(widths[code-32]+1)+offset)&65535;rows.append((v,b""))
 for height in (0,1,8):
  for y in (-23,-2,-1,0,7):
   v=base(2);v[5]=height;v[7]=y&65535;rows.append((v,b""))
 v=base(2);v[6]=63;v[32+65-32]=254;rows.append((v,b""))
 for language in range(5):
  for wide in (0,1):
   v=base(4);v[2]=language;v[13]=wide;rows.append((v,b''))
 for message in (b'',b'Now Saving...',b'A\nB',b' \tABC',b'\x81\x40Hi',b'AAAAAAA',b'...'):
  v=base(3);rows.append((v,message))
 for _ in range(count):
  kind=rng.randrange(6);code=rng.choice(list(range(32,256))+[0,1,9,10,31,0x8140,0x8240])
  if kind in (0,1) and code<32:code=32
  v=base(kind,code);v[2]=rng.randrange(5);v[3]=rng.randrange(2);v[6]=rng.randrange(-24,72)&65535;v[7]=rng.randrange(-24,52)&65535;v[8]=rng.getrandbits(16);v[9]=rng.getrandbits(16);v[10]=rng.getrandbits(16);v[11]=rng.getrandbits(32);v[13]=rng.randrange(2);v[14]=rng.randrange(2)
  if kind!=4:v[32:256]=[rng.getrandbits(8) for _ in range(224)]
  if v[14]:v[256:480]=[rng.randrange(32,256) for _ in range(224)]
  rows.append((v,rng.choice((b'Now Saving...',b'A\nB',b'Hello',b' \tABC'))))
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--font-directory',default='reference/updated-pc-reference/game/system');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1000);p.add_argument('--seed',type=int,default=409900);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;folder=Path(a.font_directory);single=(folder/'FONTDATA1.BIN').read_bytes();wide=(folder/'FONTDATA0.BIN').read_bytes();source=Source(image,single,wide)
 spec=__import__('importlib.util',fromlist=['util']);module_spec=spec.spec_from_file_location('font_import',Path(__file__).resolve().parents[1]/'import/message_fonts.py');font_import=spec.module_from_spec(module_spec);module_spec.loader.exec_module(font_import);records,_=font_import.decode_records(single);widths=[font_import.generated_width(records[x]) for x in range(32,256)]
 lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_message_text_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint16),ctypes.c_char_p]
 single_array=(ctypes.c_uint8*len(single)).from_buffer_copy(single);wide_array=(ctypes.c_uint8*len(wide)).from_buffer_copy(wide);rows=vectors(a.seed,a.cases,widths);rng=random.Random(a.seed+1);bad=[]
 for n,(v,message) in enumerate(rows):
  values=[rng.getrandbits(16) for _ in range(v[12])];raw=struct.pack('<'+'H'*len(values),*values);expected,bitmap=source.run(v,raw,message);out=(ctypes.c_uint32*512)();pixels=(ctypes.c_uint16*len(values))(*values);probe((ctypes.c_uint32*512)(*v),single_array,len(single),wide_array,len(wide),out,pixels,message);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y];got=bytes(pixels)
  if diff or bitmap!=got:
   pixel_diff=[[i//2,int.from_bytes(bitmap[i:i+2],'little'),int.from_bytes(got[i:i+2],'little')] for i in range(0,len(bitmap),2) if bitmap[i:i+2]!=got[i:i+2]];bad.append({'case':n,'input':v[:15],'message':message.hex(),'state_diff':diff[:16],'pixel_mismatches':len(pixel_diff),'pixels':pixel_diff[:16]})
   if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];files=['src/game/message_text.hpp','src/game/message_text.cpp','tests/probes/native_message_text_probe.cpp'];report={'passed':not bad,'cases':len(rows),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'font_sha256':{'single':hashlib.sha256(single).hexdigest(),'wide':hashlib.sha256(wide).hexdigest()},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'native_sources':{f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files},'limits':[__doc__]};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
