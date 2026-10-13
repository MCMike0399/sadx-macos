"""43f2a0 real interpolation/traversal/angle/ftol instructions.

Malloc is an explicit fixture supplier returning distinct32-byte buffers. Its
policy is independently owned by TaskWorkArena and is not proved by this oracle.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn.x86_const import *
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,DATA,STACK,STOP
STATE=DATA+0x100;TABLE=DATA+0x200;ACTION=DATA+0x300;MOTION=DATA+0x400
BUFFER=DATA+0x500;BMOTION=DATA+0x520;MDATA=DATA+0x600;OBJECT=DATA+0x700
OLDNODES=DATA+0x800;NEWNODES=DATA+0x900;KEYS=DATA+0xa00;ALLOC=DATA+0x2000
class BlendOriginal(Original):
 def __init__(self,image):
  super().__init__(image,0x37f);self.allocated=0
  def malloc(m,address,size,user):
   if address==0x40b220:
    esp=m.reg_read(UC_X86_REG_ESP);ret,n=struct.unpack('<II',m.mem_read(esp,8))
    if n!=32:raise RuntimeError('Unexpected source allocation size')
    target=ALLOC+self.allocated*32;self.allocated+=1;m.mem_write(target,bytes([0xa5])*32);m.reg_write(UC_X86_REG_EAX,target);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
  self.machine.hook_add(unicorn.UC_HOOK_CODE,malloc)
 def run(self,f):
  m=self.machine;self.allocated=0
  def put(a,fmt,*v):m.mem_write(a,struct.pack('<'+fmt,*v))
  m.mem_write(STATE,bytes(48));put(STATE+4,'HH',0,1);put(STATE+0x10,'I',f[1]);put(STATE+0x20,'I',TABLE);put(STATE+0x28,'I',BUFFER)
  for n in range(2):put(TABLE+n*16,'IBbHII',ACTION+n*8,1,3,0,0,0);put(ACTION+n*8,'II',OBJECT,MOTION+n*12);put(MOTION+n*12,'IIHH',NEWNODES if n==0 else OLDNODES,f[4],f[3]if n==0 else f[2],2)
  put(BUFFER,'II',OBJECT,BMOTION);put(BMOTION,'IIHH',MDATA,2,3,2);m.mem_write(MDATA,bytes(24));put(OBJECT,'13I',0,0,*f[53:62],0,0)
  for n in range(2):
   channels=3 if f[2 if n else 3]&4 else 2;node=OLDNODES if n else NEWNODES;mask=f[5 if n else 6]
   pointers=[]
   for ch in range(channels):
    addr=KEYS+n*0x100+ch*48;pointers.append(addr if mask&(1<<ch)else 0)
    if n:put(addr,'12I',*f[8+ch*12:20+ch*12])
    else:put(addr,'4I',0,*f[44+ch*3:47+ch*3])
   put(node,f'{channels*2}I',*pointers,*([f[7]if n else 1]*channels))
  m.reg_write(UC_X86_REG_FPCW,f[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);put(STACK,'II',STOP,STATE);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x43f2a0,STOP,count=100000)
  if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Original did not return')
  channels=struct.unpack('<H',m.mem_read(BMOTION+10,2))[0];out=[0]*35
  for ch in range(channels):
   ptr=struct.unpack('<I',m.mem_read(MDATA+ch*4,4))[0];out[ch]=bool(ptr);out[3+ch]=struct.unpack('<I',m.mem_read(MDATA+(channels+ch)*4,4))[0]
   if ptr:out[6+ch*8:14+ch*8]=struct.unpack('<8I',m.mem_read(ptr,32))
  out[30]=m.reg_read(UC_X86_REG_FPSW)&63;out[31]=m.reg_read(UC_X86_REG_FPCW);out[34]=1;return out

def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def fixtures():
 base=[0x37f,bits(0.5),3,3,10,7,7,3]
 for ch in range(3):
  for k in range(3):base += [k*4,*([bits(k+0.25),bits(-k-0.5),bits(k*2)]if ch!=1 else [0xfffff000+k*0x1000,0xffff+k*0x8000,k*0x3000])]
 base += [bits(5),bits(6),bits(7),0x1000,0xffffc000,0xffff,bits(2),bits(3),bits(4)]
 base += [bits(1),bits(2),bits(3),0x7000,0xffff9000,0x8000,bits(1),bits(1),bits(1)]
 assert len(base)==62
 for typ in (3,7):
  for nxt in (3,7):
   for oldmask in range(8):
    for newmask in (0,7):
     for frame in (-1,0,0.5,4,5,8,9,12):
      f=base.copy();f[1]=bits(frame);f[2]=typ;f[3]=nxt;f[5]=oldmask;f[6]=newmask;yield f
 rng=random.Random(0x43f2a0)
 for _ in range(150):
  f=base.copy();f[1]=bits(rng.uniform(-2,14));f[2]=rng.choice((3,7));f[3]=rng.choice((3,7));f[7]=rng.randint(1,3)
  for ch in range(3):
   for k in range(3):
    for axis in range(3):f[9+ch*12+k*4+axis]=rng.getrandbits(32)if ch==1 else bits(rng.uniform(-50,50))
  yield f

def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
 if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
 cb=b''.join(image.read(x,4)for x in (0x7dcb2c,0x7dcb10,0x7dcb7c));constants=(ctypes.c_float*3).from_buffer_copy(cb);lib=ctypes.CDLL(str(a.native_library.resolve()));probe=lib.sadx_native_player_animation_blend;probe.argtypes=[ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int;oracle=BlendOriginal(image)
 sha=lambda b:hashlib.sha256(b).hexdigest();report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),cases=0,stored_mismatches=0,status_mismatches=0,examples=[],status_examples=[],source_range=dict(address='0x43f2a0',bytes=0x750,sha256=sha(image.read(0x43f2a0,0x750))),limits=['Synthetic single-object graph and two MDATA2/3 motion descriptors; not actual model DLL resource execution','Fixture allocator supplies32-byte buffers at40b220; original allocator policy is not compared here','Integer/float stored keys and metadata compared; full x87 status/stack/tags are outside native context','Unicorn2.1.4 is an instruction oracle, not physical Windows execution'])
 root=pathlib.Path(__file__).resolve().parents[2];report['native_sources']={n:sha((root/n).read_bytes())for n in ('src/player/player_animation.hpp','src/player/player_animation_blend.cpp','tests/probes/native_player_animation_blend_probe.cpp','tools/oracles/verify_original_player_animation_blend.py')};start=time.monotonic();digest=hashlib.sha256()
 for cw in (pc|rc|0x7f for pc in (0,0x200,0x300)for rc in (0,0x400,0x800,0xc00)):
  for f in fixtures():
   f=[x&0xffffffff for x in f];f[0]=cw;digest.update(struct.pack('<62I',*f));expected=oracle.run(f);output=(ctypes.c_uint32*35)()
   if probe(constants,(ctypes.c_uint32*62)(*f),output):raise RuntimeError('Native probe rejected fixture')
   for field,value in enumerate(expected):
    if output[field]!=value:
     status=field==30;name='status_mismatches'if status else'stored_mismatches';report[name]+=1;bucket=report['status_examples']if status else report['examples']
     if len(bucket)<30:bucket.append(dict(case=report['cases'],field=field,source=value,native=output[field],input=f))
   report['cases']+=1
 report.update(case_sha256=digest.hexdigest(),seconds=time.monotonic()-start,stored_state_matches=not report['stored_mismatches'],passed=not(report['stored_mismatches']or report['status_mismatches']));a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('cases','stored_mismatches','status_mismatches','seconds','passed')}));return int(not report['stored_state_matches'])
if __name__=='__main__':raise SystemExit(main())
