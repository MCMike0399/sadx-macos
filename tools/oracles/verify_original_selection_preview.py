"""Original preview task/factory/resource setup; actual player/camera callbacks are explicit lower services."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDI,UC_X86_REG_EIP,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK,WORK,OTHER=DATA+0x1000,DATA+0x1100,DATA+0x1200
TASKS=[DATA+0x2000+256*n for n in range(16)]
POSES=[DATA+0x4000+256*n for n in range(16)]
MOTIONS=[DATA+0x6000+256*n for n in range(16)]
GROUPS=[DATA+0x8000+64*n for n in range(9)]
LISTS=[DATA+0x9000+64*n for n in range(18)]
NAMES=[DATA+0xa000+16*n for n in range(18)]
IDS={TASK:30,OTHER:31,**{t:n+1 if n<7 else n+4 if n<14 else 21 if n==14 else 22 for n,t in enumerate(TASKS)}}
ACTORS=[0x49a9b0,0x461700,0x47a770,0x48abf0,0x490a00,0x483430,0x5125a0]
class Source:
 def __init__(self,image):
  self.image=image;self.m=Original(image,127).machine
  for a in [0x3c5ff80,0x3b2a2fd,0x8928c0,0x3b2cbb4,0x10db88c,0x3b290c4]:
   page=a&~4095
   if not any(x<=page<=y for x,y,_ in self.m.mem_regions()):self.m.mem_map(page,4096)
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.m.hook_add(unicorn.UC_HOOK_MEM_INVALID,self.invalid_memory);self.initial=self.m.context_save()
 def invalid_memory(self,m,access,address,size,value,user):
  self.fault={'pc':m.reg_read(UC_X86_REG_EIP),'address':address,'access':access,'size':size};return False
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def put(self,a,x,n=4):self.m.mem_write(a,(x&((1<<(8*n))-1)).to_bytes(n,'little'))
 def event(self,k,a=0,b=0,c=0):self.trace.extend([k,a,b,c])
 def hook(self,m,a,size,user):
  if a not in [0x40b860,0x40b2d0,0x412420,0x421180,0x4228e0,0x403290,0x40b570,0x402ed0,0x512910]:return
  sp=m.reg_read(UC_X86_REG_ESP);result=0
  if a==0x40b860:
   flags,level,callback=[self.word(sp+n) for n in (4,8,12)]
   if callback==0x512b40:
    self.event(1,flags,level)
    if not self.v[4]&1:
     result=TASK;self.put(TASK+16,callback);self.put(TASK+20,0);self.put(TASK+24,0);self.put(TASK+44,0)
   elif callback==0x433410:self.event(4,flags,level);result=0 if self.v[4]&4 else TASKS[15]
   else:
    slot=ACTORS.index(callback);self.event(5,slot,flags,level);result=0 if self.v[4]&(1<<(8+slot)) else TASKS[slot+7]
  elif a==0x40b2d0:
   self.event(2,self.word(sp+4),self.word(sp+8),IDS.get(self.word(0x3c5ff80),0));result=0 if self.v[4]&2 else WORK
   if result:self.m.mem_write(WORK,bytes(56))
  elif a==0x412420:
   self.event(3,self.word(sp+4))
   if self.v[15]!=0xffffffff:self.put(0x8928c0,self.v[15])
  elif a in (0x421180,0x4228e0):
   pointer=self.word(sp+8 if a==0x421180 else sp+4);index=LISTS.index(pointer)
   name=int.from_bytes(self.m.mem_read(self.word(sp+4),3),'little') if a==0x421180 else 0
   self.event(6 if a==0x421180 else 7,index,name)
   if self.v[10]!=0xffffffff:self.put(0x3b290c4+index//2,self.v[10],1)
  elif a==0x403290:self.event(8,LISTS.index(self.word(sp+4)))
  elif a==0x40b570:self.event(9,IDS[self.word(sp+4)])
  elif a==0x402ed0:self.event(10,self.word(sp+4))
  elif a==0x512910:self.event(11,self.word(WORK+44,1));self.put(WORK+52,self.v[42],1)
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.trace=[];self.fault=None;self.m.context_restore(self.initial);self.m.reg_write(UC_X86_REG_FPCW,v[6])
  self.m.mem_write(WORK,bytes(56));self.m.mem_write(TASK,bytes(52));self.m.mem_write(OTHER,bytes(52));self.put(TASK+44,0 if v[30] else WORK)
  for n in range(7):
   self.put(WORK+4*n,0 if v[5]&(1<<n) else TASKS[n]);self.put(WORK+45+n,v[32+n],1)
   for k in range(3):self.put(0x3c5ff10+12*n+4*k,v[64+3*n+k])
  self.put(WORK+28,TASKS[14] if v[40] else 0);self.put(WORK+32,v[27]);self.put(WORK+36,v[28]);self.put(WORK+40,v[29]);self.put(WORK+44,v[1],1);self.put(WORK+52,v[39],1)
  self.put(0x3c5ff80,TASK if v[41] else 0);self.put(0x3c5ff64,v[11]);self.put(0x3b2cbb4,v[12]);self.put(0x8928c0,v[3]);self.put(0x3b2a2fd,v[2],1);self.put(0x10db828,v[13])
  for n,t in enumerate(TASKS):
   self.m.mem_write(t,bytes(52));self.m.mem_write(POSES[n],bytes(64));self.m.mem_write(MOTIONS[n],bytes(64));self.put(t+32,POSES[n]);self.put(t+36,MOTIONS[n])
   self.put(POSES[n]+2,v[58],1);self.put(POSES[n],v[55],1);self.put(POSES[n]+8,v[56],1);self.put(POSES[n]+9,v[57],1)
   for k in range(3):self.put(POSES[n]+20+4*k,v[51+k]);self.put(POSES[n]+32+4*k,v[48+k])
   self.put(MOTIONS[n]+32,v[54])
  for c in range(9):
   self.put(0x10db88c+4*c,0 if v[7]&(1<<c) else GROUPS[c]);self.put(0x3b290c4+c,v[16+c],1);self.m.mem_write(GROUPS[c],bytes(32))
   if v[8]&(1<<c):continue
   for k in range(2):
    i=2*c+k;self.put(GROUPS[c]+8*k,0 if k==0 and v[9]&(1<<c) else NAMES[i]);self.put(GROUPS[c]+8*k+4,LISTS[i]);self.m.mem_write(NAMES[i],('A'+str(c)+str(k)).encode()+b'\0')
  op=v[0];entry=[0x512bc0,0x512af0,0x512840,0x512640,0x512b40,0x5127f0,0x512b60,0x512a70,0x512ba0,0x512c20,0x512550][op]
  self.m.mem_write(STACK-256,struct.pack('<II',STOP,TASK));self.m.reg_write(UC_X86_REG_ESP,STACK-256);self.m.reg_write(UC_X86_REG_ESI,WORK);self.m.reg_write(UC_X86_REG_EAX,TASK if op==1 else WORK);self.m.reg_write(UC_X86_REG_EDI,v[2]);error=actor_error=0
  try:self.m.emu_start(entry,STOP,timeout=0);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
  except unicorn.UcError:
   expected_offset=28 if op in (0,1,8) else 44
   if self.fault and self.word(TASK+44)==0 and op in (0,1,4,6,8) and self.fault['address']==expected_offset:
    error=2
   elif self.fault and self.fault['address']==32 and 0x512640<=self.fault['pc']<0x5127f0:
    missing=[n for n in range(7) if self.word(WORK+4*n)==0]
    if not missing:raise
    error=3;actor_error=missing[0]
   else:raise RuntimeError('unexpected original source fault: '+str(self.fault))
  out=[0]*1024;out[:3]=[IDS.get(self.word(0x3c5ff80),0),self.word(0x3c5ff64),self.word(0x3b2cbb4)]
  out[3:10]=[IDS.get(self.word(WORK+4*n),0) for n in range(7)];out[10]=IDS.get(self.word(WORK+28),0)
  out[11:15]=[self.word(WORK+32),self.word(WORK+36),self.word(WORK+40),self.word(WORK+44,1)]
  out[15:22]=self.m.mem_read(WORK+45,7);out[22]=self.word(WORK+52,1);out[23:26]=[int(self.word(TASK+16)==0x512b40),2 if self.word(TASK+20)==0x512b60 else 0,3 if self.word(TASK+24)==0x512ba0 else 0];out[26]=int(self.word(TASK+44)!=0);out[27:29]=[error,actor_error];out[31]=self.m.reg_read(UC_X86_REG_FPSW)&63
  out[32:53]=struct.unpack('<21I',self.m.mem_read(0x3c5ff10,84));out[56:65]=self.m.mem_read(0x3b290c4,9)
  for n in range(16):
   p=POSES[n];out[80+8*n:88+8*n]=[self.word(p,1)|(self.word(p+8,1)<<8)|(self.word(p+9,1)<<16)|(self.word(p+2,1)<<24),self.word(p+20),self.word(p+24),self.word(p+28),self.word(p+32),self.word(p+36),self.word(p+40),self.word(MOTIONS[n]+32)]
  out[220]=len(self.trace);out[224:224+len(self.trace)]=self.trace;return out

def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def cases(seed,count):
 rng=random.Random(seed);rows=[]
 def base(op):
  v=[0]*128;v[0]=op;v[1]=2;v[2]=0;v[3]=bits(1);v[6]=127;v[10]=0xffffffff;v[11:14]=[99,123,16384];v[15]=0xffffffff;v[27:30]=[bits(2),bits(3),bits(4)];v[32:39]=[11]*7;v[39:43]=[1,1,1,1];v[48:58]=[bits(1),bits(2),bits(3),123,456,789,98765,13,14,15]
  for n in range(21):v[64+n]=bits(float(n+1))
  return v
 for op in range(11):
  for scale in (0,.5,1,1.25,-1):
   for warmup in (-128,-1,0,1,2,127):
    v=base(op);v[3]=bits(scale);v[1]=warmup&255;rows.append(v)
 for op in (0,1,2,3,4,5,6,7,8):
  for slot in range(7):
   v=base(op)
   if op in (0,1,2):v[4]=1<<(8+slot)
   else:v[5]=1<<slot
   rows.append(v)
 for op in (0,1,4,6,8):
  v=base(op);v[30]=1;v[4]=2 if op==0 else 0;rows.append(v)
 for flags in (0,1,2,4):
  for present in (0,1):
   v=base(0);v[4]=flags;v[41]=present;rows.append(v)
 for op in (2,3,4,5):
  for aim in (0,1,0x1234,0xffffffff):
   v=base(op);v[13]=aim;v[52]=0x54321;rows.append(v)
 for count_byte in (0,1,2,127,128,255):
  for c in range(9):
   for kind in (0,1,2):
    v=base(10);v[2]=c;v[16:25]=[count_byte]*9
    if kind==1:v[7]=1<<c
    if kind==2:v[8]=1<<c
    rows.append(v)
 for op in (7,8):
  for counter in (0,1,2,127,128,255):
   for declarations in (0,511):
    v=base(op);v[16:25]=[counter]*9;v[7]=declarations;rows.append(v)
 for _ in range(count):
  v=base(rng.randrange(11));v[1]=rng.randrange(256);v[2]=rng.randrange(9 if v[0]==10 else 7);v[3]=rng.choice([bits(0),bits(1),bits(1.5),0x7fc01234]);v[4]=rng.choice([0,0,0,1,2,4,1<<rng.randrange(8,15)]);v[5]=rng.choice([0,0,0,rng.randrange(128)]);v[6]=rng.choice([127,0x23f,0x27f,0x37f]);v[7:10]=[rng.randrange(512) for _ in range(3)];v[10]=rng.choice([0xffffffff,0,1,127,255]);v[15]=rng.choice([0xffffffff,bits(.75)]);v[16:25]=[rng.choice([0,0,1,2,127,128,255]) for _ in range(9)];v[30]=rng.randrange(8)==0;v[32:39]=[rng.randrange(256) for _ in range(7)];v[39:43]=[rng.randrange(256),rng.randrange(2),rng.randrange(2),rng.randrange(256)];rows.append(v)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=512840);p.add_argument('--cases',type=int,default=1000);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256
 table=list(struct.unpack('<21I',image.read(0x7f07f8,84)))+list(struct.unpack('<7H',image.read(0x7ef6b4,14)));native_table=(ctypes.c_uint32*len(table))(*table);source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_selection_preview_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*3;rows=cases(a.seed,a.cases);bad=[];flags=0;tested=0
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*1024)();probe((ctypes.c_uint32*128)(*v),out,native_table);diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if i!=31 and x!=y];flags+=expected[31]!=out[31];tested+=1
  if diff:
   bad.append({'case':n,'input':v,'diff':diff[:24]})
   if len(bad)==8:break
 stage=Path(__file__).resolve().parents[2];paths=['src/game/selection_preview.hpp','src/game/selection_preview.cpp','src/game/selection_preview_textures.cpp','tests/probes/native_selection_preview_probe.cpp'];report={'passed':not bad and not flags,'values_match':not bad,'cases':len(rows),'tested':tested,'mismatches':len(bad),'fp_flag_mismatches':flags,'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'source_ranges':{f'0x{x:x}-0x{y:x}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in [(0x512550,0x51259c),(0x512640,0x512910),(0x512a70,0x512c3d)]},'native_sources':{p:hashlib.sha256((stage/p).read_bytes()).hexdigest() for p in paths},'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'limits':['Factory/task allocation, palette2, actual camera/player callbacks, PVM loaders, FOV calculation and event animation are ordered lower service receipts; no player rendering or native CharacterSelect completion claim.','Full original upper entries execute, including inline512550 texture-count behavior and512a70 release-count behavior. Every native actor/camera must bind its actual callback or fail explicitly; fixture tasks are only oracle-owned objects.','All seven actor pose stores, old/new task fields, canonical task publication, signed warmup byte, source texture counter wrap, actor/camera absence and allocation-failure prefixes are compared.','FP status-only differences stay partial; no flags masked from report. Producer-selected indices0..6 are compared; a native out-of-range selected index is a tagged boundary.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or flags)
if __name__=='__main__':raise SystemExit(main())
