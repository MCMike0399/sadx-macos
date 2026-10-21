"""Actual StopPCM/WMPClose and WinMain audio-conversion instructions.
External destructors, critical sections and reader helpers are ownership fixtures;
this does not judge the source Windows driver, scheduler or reader implementation.
"""
import argparse, ctypes, hashlib, json, pathlib, random, struct
import unicorn
from unicorn.x86_const import *
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP, DATA
class Source:
 def __init__(self,image):
  assert image.sha256==SOURCE_SHA256 and unicorn.__version__=='2.1.4'
  self.m=Original(image,127).machine
  self.m.mem_protect(DATA,4096,unicorn.UC_PROT_ALL)
  for s in image.sections:
   if s['name'] in ['.text','.rdata']:continue
   size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095
   self.m.mem_map(image.base+s['rva'],size)
   self.m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
 def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
 def get(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
 def hook(self,m,a,size,user):
  if a not in [DATA+0x800,DATA+0x810,DATA+0x820,0x411760,0x411720]:return
  sp=m.reg_read(UC_X86_REG_ESP);this=m.reg_read(UC_X86_REG_ECX)
  if a==DATA+0x800:self.lock+=1
  elif a==DATA+0x810:self.lock-=1
  elif a==DATA+0x820:
   n=(this-DATA-0x1000)//16
   assert self.get((0x3b0ec30 if n%2==0 else 0x3b0ed70)+n//2*4)==this
   assert self.lock==(0 if n%2 else 1)
   assert self.get(sp+4)==1
   self.trace.append(n)
  else:self.trace.append((10 if this==DATA+0x2000 else 20)+(a==0x411720))
  m.reg_write(UC_X86_REG_EIP,self.get(sp));m.reg_write(UC_X86_REG_ESP,sp+4+(4 if a in [DATA+0x800,DATA+0x810,DATA+0x820] else 0))
 def call(self,a,arg):
  self.put(STACK,STOP);self.put(STACK+4,arg);self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.emu_start(a,STOP,count=10000)
 def pcm(self,initialized,channel,masks):
  self.trace=[];self.lock=0;self.put(0x3b0ef2c,initialized)
  self.put(0x7db084,DATA+0x800);self.put(0x7db088,DATA+0x810);self.put(DATA+0x900,DATA+0x820)
  for n,mask in enumerate(masks):
   for pending in range(2):
    obj=DATA+0x1000+(n*2+pending)*16;self.put(obj,DATA+0x900)
    self.put((0x3b0ed70 if pending else 0x3b0ec30)+n*4,obj if mask&(1<<pending) else 0)
  self.call(0x410100,channel)
  return self.trace,[int(bool(self.get(0x3b0ec30+n*4)))|int(bool(self.get(0x3b0ed70+n*4)))<<1 for n in range(36)]
 def media(self,v):
  self.trace=[]
  for a,x in zip([0x3abdf9c,0x3abdf94,0x3abdfa0,0x3abdf98,0x3abdfa8,0x3abdfa4],[DATA+0x2000 if v[1] else 0,DATA+0x2100 if v[2] else 0,*v[3:]]):self.put(a,x)
  self.call(0x40cff0,v[0])
  return self.trace,[self.get(a) for a in [0x3abdfa0,0x3abdf98,0x3abdfa8,0x3abdfa4]]
 def segment(self,a,end,value):
  self.m.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.m.reg_write(UC_X86_REG_EBP,0);self.m.reg_write(UC_X86_REG_EBX,1);self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.emu_start(a,end,count=100)
 def config(self,v):
  self.segment(0x4027e3,0x4027fb,v[2]);self.segment(0x402821,0x40285f,v[3])
  self.call(0x40ccf0,0)
  self.segment(0x402884,0x40289c,v[0]);self.segment(0x4028c2,0x402900,v[1]);self.segment(0x40290d,0x40291f,0)
  return [self.get(a) for a in [0x3a7cf3c,0x909f28,0x38f6ec0,0x3abdfac,0x3abdf90]]
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('probe',type=pathlib.Path);p.add_argument('--output',required=True,type=pathlib.Path);p.add_argument('--seed',type=int,default=20261023);a=p.parse_args()
 image=PEImage(a.exe);s=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()));U=ctypes.c_uint32;I=ctypes.c_int32
 lib.audio_pcm_probe.argtypes=[U,I,ctypes.POINTER(U),ctypes.POINTER(U)];lib.audio_media_probe.argtypes=[ctypes.POINTER(U),ctypes.POINTER(U)];lib.audio_config_probe.argtypes=[ctypes.POINTER(I),ctypes.POINTER(I)]
 rng=random.Random(a.seed);bad=[];counts={}
 for n in range(1000):
  masks=[rng.randrange(4) for _ in range(36)];initialized=rng.choice([0,1,17,0xffffffff]);channel=rng.randrange(-1,36);expected=s.pcm(initialized,channel,masks);out=(U*109)();assert lib.audio_pcm_probe(initialized,channel,(U*36)(*masks),out)
  actual=(list(out)[1:out[0]+1],list(out)[73:109])
  if actual!=expected:bad.append(['pcm',n,expected,actual])
 counts['pcm']=1000
 n=0
 for mode in [0,1,2,0xffffffff]:
  for music in [0,1]:
   for voice in [0,1]:
    for opened in [0,1,17]:
     if mode==0 and opened and not music:continue
     v=[mode,music,voice,opened,37,51,93];expected=s.media(v);out=(U*7)();assert lib.audio_media_probe((U*7)(*v),out);actual=(list(out)[1:out[0]+1],list(out)[3:7]);n+=1
     if expected!=actual:bad.append(['media',v,expected,actual])
 counts['media']=n
 edges=[-2147483648,-1,0,1,30,70,99,100,101,2147483647];cases=[[v]*4 for v in edges]+[[rng.randint(-2147483648,2147483647) for _ in range(4)] for _ in range(2000)]
 for n,v in enumerate(cases):
  expected=s.config(v);out=(I*5)();lib.audio_config_probe((I*4)(*v),out);actual=[x&0xffffffff for x in out]
  if actual!=expected:bad.append(['config',v,expected,actual])
 counts['config']=len(cases)
 report={'passed':not bad,'counts':counts,'mismatch_count':len(bad),'mismatches':bad[:10],'seed':a.seed,'source_sha256':image.sha256,'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'scope':__doc__}
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','counts','mismatch_count']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
