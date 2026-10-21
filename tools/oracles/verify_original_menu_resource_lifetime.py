"""Original menu texture release and sequence/task cleanup with native resource receipts.

Actual505f40/505fe0/506040/505ae0 and all13 cleanup leaves execute. Existing
njReleaseTexture, DestroyTask and FreeTask are receipts for independently ported
engine owners; this judge does not replace their lower resource/lifetime judges.
Japanese extra is the caller's direct403290 operation on list10d9fb4. No Puti
resource destructor/active Chao cleanup claim; FreeTask is deferred as in source.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
BASE=0x22000000
LISTS=BASE+0x1000;MODES=BASE+0x3000;IDS=BASE+0x4000;TASK=BASE+0x5000;WORK=BASE+0x5100;SEQUENCE=BASE+0x5200;ALTERNATE=BASE+0x5400;LANGUAGES=BASE+0x6000;HANDLES=BASE+0x7000
TASK_GLOBALS=[0,0x3c5ff08,0x3c5e8c0,0x3c5fee0,0x3c5ff04,0x3c5ff00,0x3c5fee4,0x3c5fed8,0x3c5fef8,0x3c5fedc,0x3c5fef4,0x3c5feec,0x3c5fed4,0x3c5fee8]
LEAVES=[0,0x512520,0x5049c0,0x50b340,0x511480,0x50e440,0x50bcf0,0x5098b0,0x50e2e0,0x50a570,0x50e080,0x50d4e0,0x508510,0x50c160]
class Source:
 def __init__(self,image):
  assert image.sha256==SOURCE_SHA256 and unicorn.__version__=='2.1.4'
  self.m=m=Original(image,127).machine
  for section in image.sections:
   if section['name'] in ('.text','.rdata'):continue
   size=(max(section['raw_size'],section['virtual_size'])+4095)&~4095;m.mem_map(image.base+section['rva'],size);m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
  m.mem_map(BASE,0x10000);m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.context=m.context_save()
  assert list(image.unpack('<14I',0x10d7b84))==LEAVES
 def get(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 def ret(self):
  sp=self.m.reg_read(UC_X86_REG_ESP);self.m.reg_write(UC_X86_REG_EAX,0);self.m.reg_write(UC_X86_REG_ESP,sp+4);self.m.reg_write(UC_X86_REG_EIP,self.get(sp))
 def tid(self,p):return (p-HANDLES)//64 if p else 0
 def hook(self,m,a,size,user):
  if a not in (0x403290,0x40b570,0x40b6c0):return
  p=self.get(m.reg_read(UC_X86_REG_ESP)+4);texture=a==0x403290
  identity=151 if p==0x10d9fb4 else (p-LISTS)//8+1 if texture and p else self.tid(p)
  if self.task_group:
   words=[self.get(x) for x in [0x3abdf70,0x3c5e8d8,0x3c5e8dc,0x3c5e8e0]]+[self.tid(self.get(0x3b2c588))]+[self.tid(self.get(x)) for x in TASK_GLOBALS[1:]]
   kind=1 if texture else 2 if a==0x40b570 else 3
  else:
   words=[self.get(x) for x in [0x3b0f0e8,0x3c5e8d8,0x3c5e8dc,0x3c5e8e0,0x3abdf70,WORK+4,WORK+8]];kind=1
  self.events.append('|'.join(map(str,[kind,identity]+words))+'\n');calls=len(self.events)
  if calls==self.v[10]:self.failed=1;m.emu_stop();return
  if calls==self.v[11]:
   if self.task_group:
    if 0<self.v[19]<14:self.put(TASK_GLOBALS[self.v[19]],HANDLES+self.v[20]*64 if self.v[20] else 0)
    self.put(0x3b2c588,HANDLES+self.v[24]*64 if self.v[24] else 0);self.put(0x3abdf70,self.v[26]);self.put(WORK,ALTERNATE)
   else:
    for at,value in zip([0x3b0f0e8,0x3c5e8d8,0x3c5e8dc,0x3c5e8e0,WORK+4,WORK+8],self.v[12:18]):self.put(at,value)
  self.ret()
 def run(self,v,data):
  self.v=v;self.task_group=v[0]>=4;self.events=[];self.failed=0;m=self.m;m.context_restore(self.context)
  for at,value in [(0x3b0f0e8,v[3]),(0x3c5e8d8,v[4]),(0x3c5e8dc,v[5]),(0x3c5e8e0,v[6]),(0x3abdf70,v[7]),(0x3c5e8d0,TASK),(TASK+0x2c,WORK),(WORK+4,v[1]),(WORK+8,v[2])]:self.put(at,value)
  for language in range(5):
   row=LANGUAGES+language*240;self.put(0x10d7cb0+language*4,row)
   for i in range(30):self.put(row+i*8+4,LISTS+(i%5 if v[21] else language*30+i)*8)
  for mode in range(14):
   pointer=MODES+mode*128;self.put(0x10d7cc4+mode*4,pointer)
   for i in range(data[mode*32]):self.put(pointer+i*4,data[mode*32+1+i])
   self.put(pointer+data[mode*32]*4,30)
  for i in range(v[8]):self.put(IDS+i*4,data[448+i])
  self.put(IDS+v[8]*4,30)
  for mode in range(1,14):self.put(TASK_GLOBALS[mode],HANDLES+v[31+mode]*64 if v[31+mode] else 0)
  self.put(0x3b2c588,HANDLES+v[46]*64 if v[46] else 0)
  for pointer,offset in [(SEQUENCE,480),(ALTERNATE,496)]:
   for i in range(v[23]):self.put(pointer+i*16,data[offset+i])
   self.put(pointer+v[23]*16,0)
  self.put(WORK,0 if v[25] else SEQUENCE);self.put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
  entry=[0x505f40,0x505fe0,0x506040,0x403290,LEAVES[v[18]] if v[0]==4 else 0x505ae0,0x505ae0,0x432530][v[0]]
  if v[0]==0:m.reg_write(UC_X86_REG_EAX,0 if v[9] else IDS)
  elif v[0]==3:self.put(STACK+4,0x10d9fb4)
  elif v[0]==5:self.put(STACK+4,TASK)
  try:m.emu_start(entry,STOP)
  except unicorn.UcError:
   if self.task_group and m.reg_read(UC_X86_REG_EIP)==0x505af9 and self.get(WORK)==0:self.failed=6
   else:raise
  if not self.failed and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('source did not return')
  out=[0]*(64 if self.task_group else 32);out[0]=self.failed
  out[1:4]=[self.get(x) for x in [0x3c5e8d8,0x3c5e8dc,0x3c5e8e0]]
  if self.task_group:
   out[4:7]=[self.get(0x3abdf70),self.tid(self.get(0x3b2c588)),len(self.events)]
   for mode in range(1,14):out[7+mode]=self.tid(self.get(TASK_GLOBALS[mode]))
  else:out[4:8]=[self.get(0x3b0f0e8),self.get(WORK+4),self.get(WORK+8),len(self.events)]
  return out,''.join(self.events)
def fixtures(seed,count):
 rng=random.Random(seed);cases=[]
 for number in range(count):
  v=[0]*64;v[0]=number%7;v[1]=rng.choice([0,1,2,5,13,14,16,0xffffffff]);v[2]=rng.randrange(14);v[3]=rng.randrange(5);v[4:8]=[rng.getrandbits(32) for _ in range(4)];v[8]=rng.randrange(31);v[9]=rng.randrange(2);v[10]=rng.choice([0,0,0,rng.randrange(1,20)]);v[11]=rng.choice([0,0,rng.randrange(1,20)]);v[12:18]=[rng.randrange(5),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(17),rng.randrange(14)];v[18]=rng.randrange(1,14);v[19]=rng.randrange(14);v[20]=rng.randrange(17);v[21]=rng.randrange(2);v[23]=rng.randrange(1,15);v[24]=rng.randrange(17);v[25]=rng.randrange(10)==0;v[26]=rng.getrandbits(32)
  for i in range(32,47):v[i]=rng.randrange(17)
  data=[0]*512
  for mode in range(14):
   n=rng.randrange(16);data[mode*32]=n
   for i in range(n):data[mode*32+1+i]=rng.randrange(30)
  for i in range(v[8]):data[448+i]=rng.randrange(30)
  for offset in (480,496):
   for i in range(v[23]):data[offset+i]=rng.randrange(14)
  cases.append((v,data))
 # Actual Title->FileSelect difference, retain shared window1.
 v=[0]*64;v[0]=2;v[1]=2;v[2]=5;v[3]=1;v[4]=0xffffffff;v[5]=0xdeadbeef
 data=[0]*512;data[5*32:5*32+5]=[4,18,17,1,29];data[2*32:2*32+3]=[2,1,4];cases.append((v,data))
 return cases
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--seed',type=int,default=20261112);p.add_argument('--cases',type=int,default=1400);p.add_argument('--native-source-root',type=Path,default=Path(__file__).resolve().parents[2]);a=p.parse_args();image=PEImage(a.exe);source=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()));U=ctypes.c_uint32;bad=[];cases=fixtures(a.seed,a.cases)
 for number,(v,data) in enumerate(cases):
  expected,et=source.run(v,data);probe=getattr(lib,'sadx_native_menu_task_release' if v[0]>=4 else 'sadx_native_menu_texture_release');probe.argtypes=[ctypes.POINTER(U),ctypes.POINTER(U),ctypes.POINTER(U),ctypes.c_char_p,ctypes.c_size_t];out=(U*len(expected))();trace=ctypes.create_string_buffer(32768);assert probe((U*64)(*v),(U*512)(*data),out,trace,len(trace))==1
  diff=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
  if diff or et!=trace.value.decode():bad.append({'case':number,'input':v,'data':data,'differences':diff,'source_trace':et,'native_trace':trace.value.decode()})
 paths=['src/game/menu_resource_lifetime.hpp','src/game/menu_resource_lifetime.cpp','tests/probes/native_menu_resource_lifetime_probe.cpp'];ranges=[(0x505f40,0x505fb0),(0x505fe0,0x506004),(0x506040,0x5060d2),(0x505ae0,0x505b17),(0x432530,0x43254d),(0x10d7b84,0x10d7bbc)]+[(address,address+(13 if address==0x50c160 else 17)) for address in LEAVES[1:]]
 report={'source_sha256':image.sha256,'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'seed':a.seed,'cases':len(cases),'mismatch_count':len(bad),'mismatches':bad[:8],'stores_calls_failure_prefixes_match':not bad,'limits':__doc__,'native_sources':{path:hashlib.sha256((a.native_source_root/path).read_bytes()).hexdigest() for path in paths},'source_ranges':{f'{hex(start)}-{hex(end)}':hashlib.sha256(image.read(start,end-start)).hexdigest() for start,end in ranges}}
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':len(cases),'mismatch_count':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
