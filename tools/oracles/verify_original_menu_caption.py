"""Original caption allocation/open/clear/empty-help/close/deferred release lifecycle.

The Windows SEH chain has its own page0 fixture; every accepted context and bitmap
pointer is valid, so no intended null-fault case can be concealed. TH_Wait executes
its real null-current-thread branch. Source lower allocation/free/texture calls
are receipts. Nonempty text runs all source caller logic through named message seams.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
CONTEXT=0x3abdc18;BUFFER=0x12000003;ARGS=DATA+0x1000;TEXT=DATA+0x2000
CALLS={0x780f60:1,0x780fb0:2,0x4014b0:3,0x40e570:4,0x40d290:5,0x40d490:6,0x403290:7}
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for section in image.sections:
   if section['name'] in ('.text','.rdata'):continue
   size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095;self.m.mem_map(image.base+section['rva'],size)
   if section['raw_size']:self.m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
  self.m.mem_map(0,4096);self.m.mem_map(BUFFER&~4095,0x50000)
  for address in CALLS:self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=address,end=address)
  self.initial=self.m.context_save()
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
 def hook(self,m,a,size,user):
  if a not in CALLS:return
  kind=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP);arg=0
  if kind==1:
   arg=self.word(sp+4);self.m.mem_write(BUFFER,b"\xa5"*262208)
   if not self.v[11]:self.allocated=True
  elif kind==3:
   arg=self.word(0x3b28114)
   if self.v[12]!=0xffffffff:self.put(0x3b0f0e8,self.v[12])
  elif kind==7:
   pointer=self.word(sp+4);assert self.word(pointer)==0x3abdfb0 and self.word(pointer+4)==0
  self.trace.extend([kind,arg])
  if kind==3:return
  m.reg_write(UC_X86_REG_EAX,BUFFER if kind==1 and not self.v[11] else 0);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def invoke(self,a,args=()):
  self.m.mem_write(STACK-256,struct.pack('<'+'I'*(1+len(args)),STOP,*(x&0xffffffff for x in args)));self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.emu_start(a,STOP,timeout=0);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
 def run(self,v):
  self.v=v;self.allocated=v[0]==1;self.trace=[];self.m.context_restore(self.initial);self.m.mem_write(CONTEXT,bytes(72));self.m.mem_write(0x3abdfb0,bytes(384));self.m.mem_write(0x3abe150,bytes(256));self.m.mem_write(BUFFER,b'\xa5'*262208)
  for address in (0x3abdf84,0x3abdf88,0x3abdc8c,0x3b0e334,0x38f6e08):self.put(address,0)
  self.put(0x3b28114,v[1]);self.put(0x3b0f0e8,1)
  if v[0]==2:self.put(0x3b0f0e8,v[9])
  if v[0]!=1:
   for cycle in range(v[10]):
    if v[0]==2:
     self.put(ARGS+20,v[5],1);self.invoke(0x5056a0,(ARGS,))
    else:
     self.invoke(0x40bc80);self.m.mem_write(TEXT,b'test\0' if v[2] else b'\0');self.invoke(0x40bd30,(TEXT,))
    if v[3]:self.invoke(0x40bcf0)
    if v[4]:self.invoke(0x40d3c0)
  else:self.invoke(0x40e1e0,(CONTEXT,v[5]&65535,v[6]&65535,v[7],v[8],v[9],BUFFER))
  out=[0]*256;out[:4]=[int(self.word(0x3abdf84)!=0),self.word(0x3abdf88),self.word(0x3b0e334),self.word(0x3b28114)]
  out[4:20]=[self.word(CONTEXT,1)]+[self.word(CONTEXT+n,2) for n in (2,4,6,8,10,12,14,16,18,20,22,24,26)]+[self.word(CONTEXT+28),self.word(CONTEXT+32)];out[20]=self.word(CONTEXT+36);out[21]=int(self.word(CONTEXT+40)!=0);out[22]=self.word(CONTEXT+44);out[23]=self.word(CONTEXT+68);out[24]=int(self.word(CONTEXT+48)!=0);out[25]=self.word(CONTEXT+52)
  pixels=self.word(CONTEXT+14,2)*self.word(CONTEXT+16,2);aligned=(BUFFER+31)&~31;data=self.m.mem_read(aligned,pixels*2+1);out[26]=sum(x!=0 for x in data[:-1]) if self.allocated else 0;out[27]=data[-1] if self.allocated else 0;out[28]=int(self.word(0x3abe150)!=0);out[64]=len(self.trace)//2;out[65:65+len(self.trace)]=self.trace;return out

def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 for text in (0,1):
  for complete in (0,1):
   for release in (0,1):rows.append([0,19,text,complete,release,0,0,0,0,0,3,0])
 for language in (0,1):
  for cursor in range(3):
   for fail in (0,1):
    for complete in (0,1):
     for release in (0,1):rows.append([2,27,0,complete,release,cursor,0,0,0,language,3,fail])
 for _ in range(count):
  op=rng.randrange(2);rows.append([op,rng.getrandbits(32),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.getrandbits(16),rng.getrandbits(16),rng.choice((-3,0,1,7,8,9,32,127,512,1024)),rng.choice((-1,0,1,8,9,40,64,128)),rng.getrandbits(32),rng.randrange(1,4),rng.randrange(2)])
 for row in rows:row.append(0xffffffff)
 for original,new in ((1,2),(1,3),(1,4),(2,1)):
  for cursor in range(3):
   for fail in (0,1):
    for complete in (0,1):rows.append([2,27,0,complete,1,cursor,0,0,0,original,3,fail,new])
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=40_850);p.add_argument('--cases',type=int,default=500);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_menu_caption_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2+[ctypes.POINTER(ctypes.c_char_p)];pool={};resources=[]
 for choice in range(3):
  directory=image.unpack("<I",0x10d7b18+choice*4)[0]
  for pointer in image.unpack("<5I",directory):
   if pointer not in pool:
    data=bytearray()
    while image.read(pointer+len(data),1)!=b"\0":data.extend(image.read(pointer+len(data),1))
    pool[pointer]=ctypes.create_string_buffer(bytes(data))
   resources.append(ctypes.cast(pool[pointer],ctypes.c_char_p))
 texts=(ctypes.c_char_p*15)(*resources);rows=vectors(a.seed,a.cases);bad=[]
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*256)();probe((ctypes.c_uint32*13)(*(x&0xffffffff for x in v)),out,texts);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if diff:
   bad.append({'case':n,'input':v,'diff':diff})
   if len(bad)==8:break
 root=Path(__file__).resolve().parents[2];paths=['src/game/menu_caption.hpp','src/game/menu_caption.cpp','tests/probes/native_menu_caption_probe.cpp'];report={'passed':not bad,'cases':len(rows),'mismatches':len(bad),'first_mismatches':bad,'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'limits':[__doc__,'Caption owner is PRIVATE proposed replacement; legacy GameFrameOwners owner/adapter migration still required before landing.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
