"""Execute actual object wrapper/traversal/simple bodies and actual403250 gate; retain only lower operation receipts."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
NODES=DATA+0x1000;LIST=DATA+0x5000;NAMES=DATA+0x5100
MODELS=[0,1,1048575,1048576,285237248,4294967295,1048574,17661820]
CALLS={0x407b70:1,0x781c80:3,0x781c10:4,0x781770:5,0x7819c0:6,0x781700:7,0x7816e0:8,0x77ef70:9}
def node_address(id):return 0 if id==0 else NODES+(id-1)*64
def node_id(address):return 0 if address==0 else (address-NODES)//64+1
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine
  for s in image.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095;self.m.mem_map(image.base+s['rva'],size)
   if s['raw_size']:self.m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
  for a in CALLS:self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=a,end=a)
  self.m.hook_add(unicorn.UC_HOOK_CODE,self.gate_hook,begin=0x403250,end=0x403250)
  self.initial=self.m.context_save()
 def word(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
 def setup_gate(self,gate):
  self.gate=gate;self.put(0x3abd950,0 if gate==1 else LIST);count=0 if gate==2 else 2 if gate>=5 else 1;self.put(LIST,NAMES);self.put(LIST+4,count)
  for n in range(2):
   self.put(NAMES+n*12,0xffffffff if gate==4 and n==0 else DATA+0x5200)
   self.put(NAMES+n*12+8,0 if gate==3 or (gate==5 and n==1) else DATA+0x5300)
 def receipt(self,row):self.trace.append(row+[0]*(8-len(row)))
 def gate_hook(self,m,a,size,user):
  assert m.reg_read(UC_X86_REG_EAX)==(0 if self.gate==1 else LIST);self.receipt([2,self.gate])
 def hook(self,m,a,size,user):
  kind=CALLS[a];sp=m.reg_read(UC_X86_REG_ESP);pop=4;current=node_id(m.reg_read(UC_X86_REG_ESI))
  if kind==1:
   assert self.word(sp+8)==1;row=[kind,node_id(self.word(sp+4)),1,self.word(0x909eb4)&255,self.word(0x3abd9c0)]
   if self.v[9]&1:m.mem_write(0x909eb4,bytes([self.v[12]&255]))
   if self.v[9]&2:self.put(0x3b1117c,self.v[13])
   if self.v[9]&4:self.setup_gate(self.v[14])
   if self.v[9]&8:self.put(0x3abd9c0,self.v[15])
  elif kind==3:
   row=[kind]
   if not self.push_changed:
    if self.v[10]&1:self.put(node_address(current),self.word(node_address(current))^16)
    if self.v[10]&2:self.put(node_address(current)+4,MODELS[4])
    self.push_changed=True
  elif kind in (4,7):
   p=m.reg_read(UC_X86_REG_ECX);row=[kind]+[self.word(p+4*n) for n in range(3)]
  elif kind in (5,6):
   assert m.reg_read(UC_X86_REG_ECX)==0
   row=[kind,m.reg_read(UC_X86_REG_EDX),self.word(sp+4),self.word(sp+8)];pop=12
  elif kind==8:row=[kind]
  else:
   raw=self.word(sp+4);code=MODELS.index(raw);row=[kind,code if raw>=0x100000 else 0]
   if not self.model_changed and 1<=current<=5:
    if self.v[10]&4:self.put(node_address(current)+0x30,node_address(5 if current<5 else 0))
    if self.v[10]&8:self.put(node_address(current)+0x2c,node_address(5 if current<5 else 0))
    self.model_changed=True
  self.receipt(row);m.reg_write(UC_X86_REG_EAX,self.v[11]);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+pop)
 def run(self,v):
  self.v=v;self.trace=[];self.push_changed=False;self.model_changed=False;m=self.m;m.context_restore(self.initial)
  for a,x in [(0x3b1117c,v[4]),(0x3abd9c0,v[5]),(0x3b12208,v[7])]:self.put(a,x)
  m.mem_write(0x909eb4,bytes([v[2]&255]));m.mem_write(0x3abdcfe,bytes([v[3]&255]));m.mem_write(0x90a0a0,bytes([v[6]&255]));self.setup_gate(v[8])
  for n in range(5):
   row=v[16+n*13:29+n*13];raw=[row[0],MODELS[row[1]],*row[2:11],node_address(row[11]),node_address(row[12])];m.mem_write(node_address(n+1),struct.pack('<13I',*raw))
  entry=[0x408530,0x4034b0,0x401430][v[0]];arg=node_address(v[1]) if v[0]!=2 else MODELS[v[1]];sp=STACK-128;m.mem_write(sp,struct.pack('<2I',STOP,arg));m.reg_write(UC_X86_REG_ESP,sp);m.reg_write(UC_X86_REG_ESI,0)
  m.emu_start(entry,STOP,timeout=1000000);assert m.reg_read(UC_X86_REG_EIP)==STOP and m.reg_read(UC_X86_REG_ESP)==sp+4
  out=[0]*2048;out[0:5]=[len(self.trace),self.word(0x909eb4)&255,self.word(0x3abd9c0),self.word(0x3b1117c),self.gate]
  if v[0]==2:out[5]=m.reg_read(UC_X86_REG_EAX)
  for n,row in enumerate(self.trace):out[8+n*8:16+n*8]=row
  for n in range(5):
   a=node_address(n+1);rawmodel=self.word(a+4);out[1984+n*4:1988+n*4]=[self.word(a),MODELS.index(rawmodel) if rawmodel>=0x100000 else 0,node_id(self.word(a+0x2c)),node_id(self.word(a+0x30))]
  return out

def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def vectors(seed,count):
 rng=random.Random(seed);rows=[]
 def base(mode=0,flags=0):
  v=[0]*81;v[:16]=[mode,1,0,0,0,bits(0.125),1,2,0,0,0,0xdeadbeef,164,0,0,bits(-7)]
  for n in range(5):v[16+n*13:29+n*13]=[flags,4,bits(n+0.25),bits(-2),bits(3),100,200,300,bits(1),bits(2),bits(3),0,0]
  return v
 for mode in (0,1):
  for flags in range(64):
   for model in range(8):
    v=base(mode,flags);v[17]=model;rows.append(v)
 for flags in range(64):
  v=base(1,flags);v[27]=2;v[28]=3;v[40]=4;v[41]=5
  for node,model in ((1,3),(2,4),(3,5),(4,7)):v[17+node*13]=model
  rows.append(v)
 v=base(1);v[17]=0;v[10]=2;rows.append(v)
 for fog in (0,1,2,3,7,255):
  for active in (0,1,255):
   for pending in (0,1,0xffffffff):
    for gate in range(7):
     v=base();v[2:5]=[fog,active,pending];v[8]=gate;rows.append(v)
 for mutation in range(16):
  v=base();v[2]=3;v[9]=mutation;v[12]=164;v[13]=0xffffffff if mutation&2 else 0;v[14]=5 if mutation&4 else 0;rows.append(v)
  v=base();v[10]=mutation;v[27]=2;v[28]=3;rows.append(v)
 for simple in (0,1,255):
  for palette in (0,1,2,0xffffffff,0x80000000):
   v=base();v[6]=simple;v[7]=palette;rows.append(v)
 for model in (3,4,5,7):v=base(2);v[1]=model;rows.append(v)
 for _ in range(count):
  mode=rng.randrange(3);v=base(mode);v[1]=rng.choice((3,4,5,7)) if mode==2 else rng.randrange(6);v[2:8]=[rng.randrange(256),rng.choice((0,1,255)),rng.choice((0,0,1,0xffffffff)),rng.getrandbits(32),rng.randrange(2),rng.choice((0,1,2,0xffffffff))];v[8]=rng.randrange(7);v[9]=rng.randrange(16);v[10]=rng.randrange(16);v[11]=rng.getrandbits(32)
  for n in range(5):
   a=16+n*13;v[a]=rng.randrange(256);v[a+1]=rng.randrange(8)
   for j in range(2,11):v[a+j]=rng.getrandbits(32)
   choices=[0]+list(range(n+2,6));v[a+11]=rng.choice(choices);v[a+12]=rng.choice(choices)
  rows.append(v)
 return rows

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1400);p.add_argument('--seed',type=int,default=403450);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);probe=ctypes.CDLL(a.library).sadx_native_object_draw_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;rows=vectors(a.seed,a.cases);bad=[]
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*2048)();probe((ctypes.c_uint32*81)(*v),out)
  if list(out)!=expected:
   bad.append({'case':n,'input':v,'diff':[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y][:24]})
   if len(bad)==8:break
 stage=Path(__file__).resolve().parents[2];paths=['src/engine/object_draw.hpp','src/engine/object_draw.cpp','tests/probes/native_object_draw_probe.cpp'];ranges=[(0x408530,0x40859b),(0x4034b0,0x4037c5),(0x401430,0x401440),(0x403250,0x40328d),(0x4037c8,0x4037e4)]
 report={'source_sha256':image.sha256,'source_ranges':{f'{hex(x)}-{hex(y)}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in ranges},'passed':not bad,'values_match':not bad,'cases':len(rows),'executed':n+1,'mismatches':len(bad),'first_mismatches':bad,'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'native_sources':{f:hashlib.sha256((stage/f).read_bytes()).hexdigest() for f in paths},'limits':['Actual408530/4034b0/401430 execute with recursive child/sibling traversal. Actual403250 texture gate executes unhooked, with entry observation only; native probe delegates the existing gate and never duplicates it.','Only original lower matrix primitives, queuedlate407b70 and njDrawModel77ef70 become receipts. Model geometry/culling/pixels and late queue bodies remain required native owners, never false draw success.','Raw source model pointers below1MiB normalize to typed nondrawable0 only in this reference probe. Valid imported model IDs remain address-independent. Object/model metadata is borrowed from one stable mutable graph.','Source flags are cached per node; model/child/sibling links remain live across actual child-call boundaries. Adversarial receipt mutations discriminate stale snapshots and state restoration. Source numeric transform arguments are exact words; transformed matrix outputs remain existing primitive scope.','Simple401430 executes actual77ef70 thenreturns0; traversal return is void per recovered body. Rendering branch choice retained in code; both branches ultimately call the same actual model renderer.','Unmasked exception delivery, malformed cyclic graphs, real GPU output and application composition are unqualified here.']};Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
