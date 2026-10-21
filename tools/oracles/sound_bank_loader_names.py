"""Execute loader filename filtering/ID/format instructions with explicit stdlib fixtures.

The LoadSoundPackFromFile body range10097266..10097739 executes unchanged.
GetExtension, string assign/erase/equality, atoi/snprintf/free are declared library
fixtures; this does not judge C++ container allocation or the complete bank loader.
"""
import ctypes,json,pathlib,random,struct,sys
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX
from pe_image import PEImage
DLLSHA='3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba'
DATA=0x22000000;STACK=DATA+0xf000;FRAME=DATA+0xc000
class Source:
 def __init__(self,dll):
  assert dll.sha256==DLLSHA
  self.m=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32)
  for section in dll.sections:
   size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095
   self.m.mem_map(dll.base+section['rva'],size);self.m.mem_write(dll.base+section['rva'],dll.slice(section['file_offset'],section['raw_size']))
  self.m.mem_map(DATA,0x10000);self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.put(0x100cd3a4,DATA+0x200)
  self.m.mem_protect(DATA,4096,unicorn.UC_PROT_ALL)
 def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
 def get(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
 def string(self,a):
  b=bytearray()
  while self.m.mem_read(a,1)[0]:b+=self.m.mem_read(a,1);a+=1
  return b.decode('ascii')
 def obj(self,a):return self.string(self.get(a) if self.get(a+20)>=16 else a)
 def store(self,a,text):
  data=text.encode()+b'\0';capacity=15 if len(text)<16 else 63
  if capacity==15:self.m.mem_write(a,data)
  else:self.put(a,DATA+0x5000);self.m.mem_write(DATA+0x5000,data)
  self.put(a+16,len(text));self.put(a+20,capacity)
 def ret(self,sp,result=0,pop=0):
  self.m.reg_write(UC_X86_REG_EAX,result&0xffffffff);self.m.reg_write(UC_X86_REG_EIP,self.get(sp));self.m.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(self,m,a,size,user):
  if a in [0x1009734d,0x1009798d]:self.kind=0 if a==0x1009734d else -1;m.emu_stop();return
  targets=[0x10038620,0x100480d0,0x100b7c50,0x10062c90,0x100bfccd,0x10049970,DATA+0x200]
  if a not in targets:return
  sp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX)
  if a==0x10038620:
   text=bytes(m.mem_read(self.get(sp+4),self.get(sp+8))).decode('ascii');self.store(ecx,text);self.ret(sp,ecx,8)
  elif a==0x100480d0:
   expected=bytes(m.mem_read(self.get(sp+4),self.get(sp+8)));actual=bytes(m.mem_read(ecx,edx));self.ret(sp,int(actual==expected))
  elif a==0x100b7c50:
   name=self.obj(edx);dot=name.rfind('.');self.store(ecx,name[dot:] if dot>=0 else '');self.ret(sp,ecx)
  elif a==0x10062c90:
   text=self.obj(ecx);start=self.get(sp+4);count=self.get(sp+8);self.store(ecx,text[:start]+text[min(len(text),start+count):]);self.ret(sp,ecx,8)
  elif a==0x100bfccd:self.ret(sp)
  elif a==DATA+0x200:
   text=self.string(self.get(sp+4)).lstrip();digits='';sign=1
   if text[:1] in ['+','-']:sign=-1 if text[0]=='-' else 1;text=text[1:]
   for c in text:
    if not c.isdigit():break
    digits+=c
   self.ret(sp,sign*int(digits or '0'))
  else:
   target,count,fmt=self.get(sp+4),self.get(sp+8),self.string(self.get(sp+12));slot,identifier=self.get(sp+16),self.get(sp+20)
   args=(slot,identifier,self.string(self.get(sp+24))) if '%s' in fmt else (slot,identifier)
   text=fmt%args;m.mem_write(target,text[:count-1].encode()+b'\0');self.ret(sp,len(text))
 def run(self,name,slot,counter,adx):
  self.kind=1;self.m.mem_write(FRAME-0x100,bytes(256));self.store(FRAME-0x28,name)
  self.put(FRAME-0x60,counter);self.put(FRAME-0xac,slot);self.put(FRAME-0x84,DATA+0x4000);self.m.mem_write(DATA+0x400e,b'Z' if adx else b'\0');self.put(FRAME-0x5c,DATA+0x4100);self.put(DATA+0x4100,DATA+0x4200)
  self.m.reg_write(UC_X86_REG_EBP,FRAME);self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.reg_write(UC_X86_REG_EBX,0)
  self.m.emu_start(0x10097266,0x10097739,count=10000)
  if self.kind==1:assert self.m.reg_read(UC_X86_REG_EIP)==0x10097739
  return self.kind,self.get(FRAME-0x60),self.string(DATA+0x4200) if self.kind==1 else ''
def main():
 dll=PEImage(sys.argv[1]);source=Source(dll);lib=ctypes.CDLL(str(pathlib.Path(sys.argv[2]).resolve()));fn=lib.sound_bank_name_probe;U=ctypes.c_uint32;fn.argtypes=[ctypes.c_char_p,U,ctypes.c_int32,U,ctypes.c_char_p,ctypes.POINTER(U)]
 rng=random.Random(20261030);names=['B00_00_00.WAV','_B02_00_15.WAV','B04_00_09.WAV','name.wav','b00_00_10.wav','B00_00_-1.WAV','B00_00_1x.WAV','B0.WAV','B00_0.WAV','empty','']
 names += [f'B0{rng.randrange(10)}_0{rng.randrange(10)}_{rng.randrange(100):02d}.WAV' for _ in range(500)]
 bad=[];cases=[]
 for name in names:
  slot=rng.randrange(16);counter=rng.choice([0,1,17,99,2147483647,-1]);adx=rng.randrange(2);expected=source.run(name,slot,counter,adx);out=ctypes.create_string_buffer(64);next_counter=U();kind=fn(name.encode(),slot,counter,adx,out,ctypes.byref(next_counter));actual=(kind,next_counter.value,out.value.decode() if kind==1 else '');cases.append([name,slot,counter,adx])
  if actual!=expected:bad.append({'input':cases[-1],'expected':expected,'actual':actual})
 if len(sys.argv)>4:
  manifest=json.loads(pathlib.Path(sys.argv[4]).read_text())
  for bank in manifest['banks']:
   counter=0;slot=next(entry['slot'] for entry in manifest['lists'][42] if entry['name']==bank['name'])
   for sample in bank['samples']:
    name=sample['name'];expected=source.run(name,slot,counter,bank['archive_tag']==90);out=ctypes.create_string_buffer(64);next_counter=U();kind=fn(name.encode(),slot,counter,bank['archive_tag']==90,out,ctypes.byref(next_counter));actual=(kind,next_counter.value,out.value.decode() if kind==1 else '');cases.append([name,slot,counter,bank['archive_tag']==90]);counter=next_counter.value
    if actual!=expected:bad.append({'input':cases[-1],'expected':expected,'actual':actual})
 report={'loader_sha256':dll.sha256,'cases':len(cases),'mismatch_count':len(bad),'mismatches':bad[:20],'scope':__doc__};pathlib.Path(sys.argv[3]).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':len(cases),'mismatch_count':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
