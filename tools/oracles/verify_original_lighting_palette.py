"""Full412420/40a630/401440 with original stage/palette/matrix callees; device calls are ordered receipts."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path.cwd()));sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import *
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import Original,DATA,CODE,STACK,STOP
TABLES=[(0x9008b4,48),(0x9008e4,48),(0x900e88,12288),(0x903e88,24576)]
ACTIVE=[(0x3abd9f8,192),(0x3abdaf0,96),(0x3abdab8,48)]
MATRIX=DATA+0x4000;DEVICE=DATA+0x2000;VTABLE=DATA+0x2100
SETLIGHT=CODE+0x300;ENABLE=CODE+0x310;RENDER=CODE+0x320

def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
class Source:
 def __init__(self,image,unit):
  self.m=m=Original(image,127).machine
  for sec in image.sections:
   if sec['name']in('.text','.rdata'):continue
   size=(max(sec['virtual_size'],sec['raw_size'])+4095)&~4095;m.mem_map(image.base+sec['rva'],size);m.mem_write(image.base+sec['rva'],image.slice(sec['file_offset'],sec['raw_size']))
  m.mem_write(0x389d650,unit)
  self.put(DEVICE,VTABLE)
  for off,fn in [(0xb0,SETLIGHT),(0xb8,ENABLE),(0xc8,RENDER)]:self.put(VTABLE+off,fn)
  m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.ctx=m.context_save()
 def put(self,a,v,n=4):self.m.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 def word(self,a,n=4):return int.from_bytes(self.m.mem_read(a,n),'little')
 def ret(self,result,pop=0):
  m=self.m;sp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(self,m,a,n,user):
  if a in (0x412c10,0x62ea30):
   kind=4 if a==0x412c10 else 5;self.counts[kind]+=1;r=(self.v[7]+self.counts[4]-1)&0xffffffff if kind==4 else self.v[8];self.trace +=[kind,r];self.ret(r);return
  if a==0x76ff01:
   sp=m.reg_read(UC_X86_REG_ESP);src=self.word(sp+8);dst=self.word(sp+4)
   self.trace +=[6,*struct.unpack('<3I',m.mem_read(src,12))]
   m.mem_write(dst,struct.pack('<3I',*self.v[43:46]));self.ret(dst,8);return
  if a not in (SETLIGHT,ENABLE,RENDER):return
  sp=m.reg_read(UC_X86_REG_ESP);assert self.word(sp+4)==DEVICE
  arg=self.word(sp+8);value=self.word(sp+12)
  if a==SETLIGHT:self.trace +=[1,arg,*struct.unpack('<26I',m.mem_read(value,104))]
  elif a==ENABLE:self.trace +=[2,arg,value]
  else:self.trace +=[3,arg,value]
  self.ret(0,12)
 def run(self,v,data):
  self.v=v;self.trace=[];self.counts={4:0,5:0};m=self.m;m.context_restore(self.ctx);cursor=0
  for a,n in TABLES+ACTIVE:m.mem_write(a,data[cursor:cursor+n]);cursor+=n
  self.put(0x3b12208,v[2]);self.put(0x3abdbc0,v[3],1);self.put(0x3b22dcc,v[4],2);self.put(0x3b22dec,v[5],2);self.put(0x3b1117c,v[6]);self.put(0x3abd9f0,v[9]);self.put(0x3abdbbc,v[10]);m.mem_write(0x9008a8,struct.pack('<3I',*v[14:17]));m.mem_write(0x3abdb50,struct.pack('<26I',*v[17:43]));self.put(0x3d128b0,DEVICE)
  m.mem_write(MATRIX,struct.pack('<96I',*v[64:160]));self.put(0x3d0f818,MATRIX+(v[11]-1)*64);self.put(0x3d0fa28,6);self.put(0x3d0fa2c,v[11]);self.put(0x3d0f7ac,v[12]);m.reg_write(UC_X86_REG_FPCW,v[13]);m.reg_write(UC_X86_REG_FPSW,0)
  m.mem_write(STACK,struct.pack('<II',STOP,v[1]));m.reg_write(UC_X86_REG_ESP,STACK)
  try:m.emu_start([0x412420,0x40a630,0x401440][v[0]],STOP,timeout=0)
  except Exception:print('SOURCEFAULT',hex(m.reg_read(UC_X86_REG_EIP)),v[:17],flush=True);raise
  assert m.reg_read(UC_X86_REG_EIP)==STOP and m.reg_read(UC_X86_REG_ESP)==STACK+4
  out=[0]*16384;out[:8]=[self.word(0x3b12208),self.word(0x3abdbc0,1),self.word(0x3b22dcc,2),self.word(0x3b22dec,2),self.counts[4],self.counts[5],self.word(0x3d0fa2c),m.reg_read(UC_X86_REG_FPSW)&63]
  out[32:58]=struct.unpack('<26I',m.mem_read(0x3abdb50,104));out[58:61]=struct.unpack('<3I',m.mem_read(0x9008a8,12));cursor=64
  for a,n in ACTIVE:words=struct.unpack('<'+str(n//4)+'I',m.mem_read(a,n));out[cursor:cursor+len(words)]=words;cursor+=len(words)
  out[148:244]=struct.unpack('<96I',m.mem_read(MATRIX,384));cursor=256
  for a,n in TABLES:words=struct.unpack('<'+str(n//4)+'I',m.mem_read(a,n));out[cursor:cursor+len(words)]=words;cursor+=len(words)
  out[31]=len(self.trace);out[10000:10000+len(self.trace)]=self.trace;return out

def fixtures(image,count,seed,preview=False):
 rng=random.Random(seed);base=b''.join(image.read(a,n)for a,n in TABLES)+bytes([165])*336;rows=[];unit=image.read(0x389d650,64)
 def row(op):
  v=[0]*160;v[0]=op;v[1]=2;v[2]=19;v[3]=171;v[4]=0;v[5]=1;v[7]=1;v[8]=2;v[11]=1;v[13]=127;v[14:17]=[bits(x)for x in(0,-1,0)];v[17:43]=[0xa5a5a5a5]*26;v[43:46]=[bits(x)for x in(0.25,-0.5,0.75)];v[64:160]=list(struct.unpack('<16I',unit))*6;return v
 for selection in (-2147483648,-7,-3,-1,0,1,2,3,4,5,6,7,2147483647):
  for stage,act in [(0,1),(8,0),(9,0),(9,1),(19,0),(26,0),(26,4),(26,5),(29,0),(33,2),(-1,-1)]:
   for loop in (0,1,-1):
    v=row(0);v[1]=selection&0xffffffff;v[4]=stage&65535;v[5]=act&65535;v[6]=loop&0xffffffff;rows.append((v,base))
 for selection in (-2147483648,-7,-3,-1,0,1,2,3,4,5,6,7,2147483647):
  v=row(0);v[1]=selection&0xffffffff;v[2]=int(selection/2)&0xffffffff;rows.append((v,base))
 for flags in range(16):
  for stage in (0,8,9):
   for cw in (127,639,895):
    v=row(1);v[4]=stage;v[5]=0;v[13]=cw;d=bytearray(base);d[37155]=flags
    for off,value in [(4,0.25),(8,-0.5),(12,0.75),(16,0.8),(20,0.1),(24,0.2),(28,0.3),(52,0.4),(56,0.5),(60,0.6)]:struct.pack_into('<f',d,37152+off,value)
    rows.append((v,bytes(d)))
 for selection in (-2147483648,-257,-1,0,1,127,128,255,256,257,2147483647):
  v=row(2);v[1]=selection&0xffffffff;rows.append((v,base))
 for _ in range(count):
  v=row(0 if preview else rng.randrange(3));v[1]=2 if preview else rng.choice([-9,-3,-1,0,1,2,4,6,8,2147483647])&0xffffffff;v[2]=rng.getrandbits(32);v[3]=rng.randrange(256);v[4],v[5]=rng.choice([(0,0),(0,1),(8,0),(9,0),(19,1),(26,4),(33,1)]);v[6]=rng.choice([0,0,0,1,0xffffffff]);v[7]=rng.randrange(4);v[8]=rng.randrange(4);v[9:11]=[rng.getrandbits(32),rng.getrandbits(32)];v[11]=rng.choice([1,2,6]);v[12]=rng.choice([0,1]);v[13]=rng.choice([127,639,895,1151]);v[14:17]=[bits(rng.uniform(-2,2))for _ in range(3)];v[17:43]=[rng.getrandbits(32)for _ in range(26)];d=bytearray(base)
  # Direct Apply fixtures vary its live palette; Set fixtures vary imported file data.
  offset=37152 if v[0]==1 else 96+12288
  d[offset+3]=rng.randrange(16)
  for k in range(4,96,4):struct.pack_into('<I',d,offset+k,bits(rng.uniform(-2,2)))
  rows.append((v,bytes(d)))
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1000);p.add_argument('--seed',type=int,default=412420);p.add_argument('--preview-only',action='store_true');a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;assert unicorn.__version__=='2.1.4';unit=image.read(0x389d650,64);source=Source(image,unit);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_lighting_palette;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_uint32)];sine=(ctypes.c_float*1025).from_buffer_copy(image.read(TABLE_ADDRESS,4100));identity=(ctypes.c_float*16).from_buffer_copy(unit);rows=fixtures(image,a.cases,a.seed,a.preview_only);rows=[(v,d)for v,d in rows if not a.preview_only or v[0]==0 and v[1]==2];bad=[];flags=0;flag_examples=[];counts={}
 for index,(v,data)in enumerate(rows):
  want=source.run(v,data);out=(ctypes.c_uint32*16384)();assert probe((ctypes.c_uint32*160)(*v),(ctypes.c_uint8*len(data)).from_buffer_copy(data),sine,identity,out)==0;diff=[[i,x,y]for i,(x,y)in enumerate(zip(want,out))if i!=7 and x!=y];flags+=want[7]!=out[7];counts[v[0]]=counts.get(v[0],0)+1
  if want[7]!=out[7]and len(flag_examples)<6:flag_examples.append({'case':index,'operation':v[0],'selection':v[1],'cw':v[13],'source':want[7],'native':out[7]})
  if diff:
   bad.append({'case':index,'input':v[:17],'diff':diff[:25]})
   if len(bad)==8:break
 ranges=[(0x412420,0x412719),(0x40a630,0x40a893),(0x401440,0x40144a)];stage=Path(__file__).resolve().parents[2]
 report={'scope':'actual preview412420(2)' if a.preview_only else 'all recovered source lighting entries','passed':not bad and not flags,'values_match':not bad,'cases':len(rows),'tested':index+1,'operations':counts,'seed':a.seed,'mismatches':bad,'fp_flag_mismatches':flags,'flag_examples':flag_examples,'source_sha256':image.sha256,'source_ranges':{f'0x{x:x}-0x{y:x}':hashlib.sha256(image.read(x,y-x)).hexdigest()for x,y in ranges},'native_sources':{str(p.relative_to(stage)):hashlib.sha256(p.read_bytes()).hexdigest()for p in[stage/'src/game/lighting_palette.hpp',stage/'src/game/lighting_palette.cpp',stage/'src/engine/device_lights.hpp',stage/'src/engine/device_lights.cpp',stage/'tests/probes/native_lighting_palette_probe.cpp']},'limits':['Full original lighting entries plus stage/palette selection and real matrix primitives execute. Sequence/hotel queries, COM device methods, and mode0 D3DX direction normalization are ordered receipts; all104 light bytes, canonical palette number/copy, active environments, imported tables, shared direction and matrix storage are compared.','Preview mode2 never calls the unrecovered mode0 normalization seam; native mode0 stops there until recovered. Native device adapter also receives commands; its renderer snapshots/GPU output have separate tests. No full CharacterSelect actor/image or gameplay claim.','FP status differences remain visible as partial; value equality does not imply hardware flag fidelity.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or flags)
if __name__=='__main__':raise SystemExit(main())
