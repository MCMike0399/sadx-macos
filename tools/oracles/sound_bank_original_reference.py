"""Actual SetBankDir and ReleaseBankSounds instructions with explicit native-owner fixtures.

40ff10 readiness executes unchanged. Formatting/ASCII comparison and archive/PCM
resource boundaries are declared fixtures. Importer independently verifies source
catalog bytes; this judges selection/release ordering and partial failure stores.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,CODE,STACK,STOP
class Source:
 def __init__(self,image):
  assert image.sha256==SOURCE_SHA256 and unicorn.__version__=='2.1.4'
  self.m=Original(image,127).machine
  self.m.mem_map(0x21000000,0x20000)
  for section in image.sections:
   if section['name'] in ['.text','.rdata']:continue
   size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095
   self.m.mem_map(image.base+section['rva'],size);self.m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
  self.put(0x7db228,CODE+512);self.put(0x7db0e4,CODE+528)
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
 def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
 def get(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
 def string(self,a):
  out=bytearray()
  while True:
   b=self.m.mem_read(a,1)[0];a+=1
   if b==0:return out.decode('ascii')
   out.append(b)
 def write_string(self,a,s):self.m.mem_write(a,s.encode('ascii')+b'\0')
 def handle(self,identity,path):
  address=DATA+0x1000+identity*64
  self.put(address+16,self.cursor);self.write_string(self.cursor,path);self.cursor+=len(path)+1;self.ids[address]=identity
  return address
 def ret(self,sp,result,pop=0):
  self.m.reg_write(UC_X86_REG_EAX,result&0xffffffff);self.m.reg_write(UC_X86_REG_EIP,self.get(sp));self.m.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(self,m,a,size,user):
  if self.release_mode:
   if a!=0x410100:return
   sp=m.reg_read(UC_X86_REG_ESP);channel=self.get(sp+4);self.channels.append(channel)
   if channel==self.fail_channel:self.stopped=True;m.emu_stop();return
   self.ret(sp,0);return
  if a not in [CODE+512,CODE+528,0x423890,0x4b4f50,0x4b4d10]:return
  sp=m.reg_read(UC_X86_REG_ESP)
  if a==CODE+512:
   assert self.string(self.get(sp+8))=='%sSoundData\\SE\\%s.dat'
   path=self.string(self.get(sp+12))+'SoundData\\SE\\'+self.string(self.get(sp+16))+'.dat';self.write_string(self.get(sp+4),path);self.ret(sp,len(path));return
  if a==CODE+528:
   equal=self.string(self.get(sp+4)).lower()==self.string(self.get(sp+8)).lower();self.ret(sp,0 if equal else 1,8);return
  if a==0x423890:event='R|'+str(self.get(sp+4))+'\n'
  elif a==0x4b4f50:event='C|'+str(self.ids.get(self.get(sp+4),0))+'\n'
  else:event='O|'+self.string(self.get(sp+4))+'|'+str(self.get(sp+8))+'\n'
  self.trace+=event;self.events+=1
  if self.events==self.fail_event:self.boundary=self.get(sp)-5;m.emu_stop();return
  result=0
  if a==0x4b4d10:
   number=self.opened;self.opened+=1
   if self.mask&(1<<number):result=self.handle(101+number,self.string(self.get(sp+4)))
  self.ret(sp,result)
 def select(self,c):
  self.release_mode=False;self.trace='';self.events=0;self.opened=0;self.mask=c['mask'];self.fail_event=c['fail'];self.boundary=0;self.cursor=DATA+0x6000;self.ids={}
  for n,path in enumerate(c['initial']):self.put(0x3b291c8+n*4,self.handle(n+1,path) if path is not None else 0)
  self.put(0x3b0ef2c,c['ready']);self.put(0x38f6ec0,c['voice']);self.write_string(0x3a8af78,c['cd'])
  if 0<=c['index']<123:
   table=0x90fcb8+c['index']*8;self.put(table,len(c['entries']));self.put(table+4,0x21000000)
   for n,(slot,name) in enumerate(c['entries']):self.put(0x21000000+n*8,slot);self.put(0x21000004+n*8,0x21010000+n*64);self.write_string(0x21010000+n*64,name)
  self.put(STACK,STOP);self.put(STACK+4,c['index']);self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.emu_start(0x4238e0,STOP,count=100000)
  if not self.boundary:assert self.m.reg_read(UC_X86_REG_EIP)==STOP
  return [0,self.boundary,*[self.ids.get(self.get(0x3b291c8+n*4),0) for n in range(16)]],self.trace
 def release(self,slot,rows,fail):
  self.release_mode=True;self.channels=[];self.fail_channel=fail;self.stopped=False
  for n,row in enumerate(rows):
   base=0x3b292fc+n*56
   for offset,value in zip([0,4,8,12,48],row[:5]):self.put(base+offset,value)
   self.put(0x3b29b90+n*4,row[5])
  self.put(STACK,STOP);self.put(STACK+4,slot);self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.emu_start(0x423890,STOP,count=10000)
  result=[]
  for n in range(36):
   base=0x3b292fc+n*56;result.extend([self.get(base+offset) for offset in [0,4,8,12,48]]+[self.get(0x3b29b90+n*4)])
  return result,self.channels,int(not self.stopped)
def fixtures(seed):
 rng=random.Random(seed);cases=[]
 for n in range(700):
  entries=[(rng.randrange(16),rng.choice(['COMMON_BANK00','BANK_A','bank_b',''])) for _ in range(rng.randrange(9))];cd=rng.choice(['','D:\\','e:\\install\\']);initial=[rng.choice([None,'old.dat','SYSTEM\\SoundData\\SE\\OLD.dat']) for _ in range(16)]
  for slot,name in entries:
   if rng.randrange(3)==0:initial[slot]=(rng.choice(['SYSTEM\\',cd])+'SoundData\\SE\\'+name+'.dat').swapcase()
  cases.append({'index':rng.choice([42,rng.randrange(123),123,2147483647]),'ready':rng.choice([0,1,17]),'voice':rng.choice([0,1,7]),'entries':entries,'initial':initial,'cd':cd,'mask':rng.getrandbits(32),'fail':rng.choice([0,0,0,rng.randrange(1,25)])})
 cases += [{'index':-1,'ready':r,'voice':v,'entries':[],'initial':[None]*16,'cd':'','mask':0,'fail':0} for r,v in [(0,0),(0,1),(1,0)]]
 return cases,rng
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('probe',type=pathlib.Path);p.add_argument('--output',required=True,type=pathlib.Path);p.add_argument('--seed',type=int,default=20261029);a=p.parse_args();image=PEImage(a.exe);source=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()));U=ctypes.c_uint32;C=ctypes.c_char_p
 select=lib.sound_bank_select_probe;select.argtypes=[ctypes.c_int32,U,U,ctypes.POINTER(U),ctypes.POINTER(C),U,C,ctypes.POINTER(C),U,U,ctypes.POINTER(U),ctypes.c_char_p,ctypes.c_size_t]
 release=lib.sound_bank_release_probe;release.argtypes=[U,ctypes.POINTER(U),U,ctypes.POINTER(U),ctypes.POINTER(U)]
 cases,rng=fixtures(a.seed);bad=[]
 for number,c in enumerate(cases):
  expected,trace=source.select(c);out=(U*18)();actual_trace=ctypes.create_string_buffer(16000);slots=(U*len(c['entries']))(*[x[0] for x in c['entries']]);names=(C*len(c['entries']))(*[x[1].encode() for x in c['entries']]);initial=(C*16)(*[x.encode() if x is not None else None for x in c['initial']]);assert select(c['index'],c['ready'],c['voice'],slots,names,len(c['entries']),c['cd'].encode(),initial,c['mask'],c['fail'],out,actual_trace,len(actual_trace))==1
  if list(out)!=expected or actual_trace.value.decode()!=trace:bad.append({'cohort':'select','case':number,'input':c,'expected':expected,'actual':list(out),'source_trace':trace,'native_trace':actual_trace.value.decode()})
 for number in range(700):
  rows=[[rng.getrandbits(32),rng.choice([0,rng.getrandbits(32)]),rng.getrandbits(32),rng.choice([0xffffffff,rng.getrandbits(32)]),rng.randrange(16),rng.choice([0,rng.getrandbits(32)])] for _ in range(36)];slot=rng.randrange(16);fail=rng.choice([0xffffffff,rng.randrange(36)]);expected,trace,completed=source.release(slot,rows,fail);flat=[v for row in rows for v in row];out=(U*216)();calls=(U*37)();done=release(slot,(U*216)(*flat),fail,out,calls)
  if list(out)!=expected or list(calls)[1:calls[0]+1]!=trace or done!=completed:bad.append({'cohort':'release','case':number,'expected':expected,'actual':list(out),'source_calls':trace,'native_calls':list(calls)[1:calls[0]+1],'completed':[completed,done]})
 report={'source_sha256':image.sha256,'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'seed':a.seed,'selection_cases':len(cases),'release_cases':700,'mismatch_count':len(bad),'mismatches':bad[:10],'stores_calls_boundaries_match':not bad,'scope':__doc__};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['selection_cases','release_cases','mismatch_count']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
