"""Actual505550 file-select display with pinned loader patches and typed resources.

Original entry/helper/primitive/font instructions execute to quad/polygon boundaries.
Native resource selection, decoder-free texture metadata, local calendar conversion,
sprintf and optional message task dispatch are explicit receipts. No GPU-pixel or
Windows/native timezone-order parity is claimed. All source save/list/font data is
fixture input; sentinel filename/time remains indeterminate and is never supplied.
"""
import argparse,ctypes,hashlib,importlib.util,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from verify_original_math import Original,CODE,STACK,STOP,bits
from extract import SOURCE_SHA256

# Complete inclusive Ghidra bodies converted to exclusive ends; no64-byte defaults.
SOURCE_RANGES=[(5264720, 5264886), (5274544, 5274599), (6564576, 6564600), (6564608, 6564697), (5263168, 5263428), (5261824, 5262926), (5259744, 5260744), (5261328, 5261745), (5260752, 5261244), (5259520, 5259744), (5261248, 5261322), (5258832, 5258989), (5258992, 5259238), (5259248, 5259517), (5272592, 5272621), (5272624, 5272858), (5272864, 5272936), (5273296, 5273393), (5274416, 5274431), (5269840, 5269895), (5269920, 5269972), (5269984, 5269990), (5270000, 5270028), (5269040, 5269212), (5268448, 5268490), (5268624, 5268682), (4802256, 4802265), (4271360, 4271372), (4271392, 4271410), (5269504, 5269510), (4931472, 4931514), (5272016, 5272266), (4398256, 4398486), (4398000, 4398119), (5273520, 5273714), (4328528, 4328675), (4328096, 4328523), (4328688, 4328739), (4328752, 4328775), (4328784, 4328805), (4403600, 4403877), (4404464, 4404639), (4408288, 4408763), (4403088, 4403208), (4406416, 4407155)]
EVIDENCE={'0x505550': 'ev_1f8c7da6f23fbf81ec9a20add0d42834face6a0b4327a76ec9684157e2c07b60', '0x507bb0': 'ev_429d35819700cd76b63273d2e88bacd0f273ca4e51eb24fef5f39cb42c4df778', '0x642ae0': 'ev_7c18ef55c914f84be964142318eba530f895062ef06bb51a8f39f299f3167fbc', '0x504f40': 'ev_2562a773bacd86aefbb0d00fd94d2836f523a10dd5f47522c08efe3893ab75eb', '0x504a00': 'ev_ac34fec455690c9fd185c55eabdc036bb3ecfea4579454c650250b2f0961ca67', '0x5041e0': 'ev_54ad67725d39b99f2a6d39f320606261c6a211834e1ffbac61904c571a6ba8ce', '0x504810': 'ev_d784996fa3c26889deeae1df27fc75dae53cd67d67cf574a22e7d27fa18ba4ac', '0x5045d0': 'ev_96d28b51a914655c184bd49bf06b11f044d32ecd78686b6ed63d2e00cfc392bc', '0x504100': 'ev_7429b9a70e14513f272bd4c5efa4df8af113834c89f612bc125ed6b732cf34cf', '0x5047c0': 'ev_9e1b9bd0a5eb550e340ea4996cb4b9fd4cb85b4d53e15cfb0ee172bb60a9eb24', '0x503e50': 'ev_e4ddab7f60e2aefd80f5b35ade6f43b23c2071358332794fb83ffadb84aae845', '0x503ef0': 'ev_92e3d8f0052778776bf2f751b47d1042b22ba071d2532442e17f7b185c8f8ffe', '0x503ff0': 'ev_87d75c56f233cdcc6b2a4a3d739adc0acd56cbf395e046864ec2382a33addb0f', '0x507410': 'ev_728ced2d6644c881fcd7e18880eddbb320b4e240e2d80115bd12610f38e78f0b', '0x507520': 'ev_b6e84657764b3bcd8819b929484f06d14e27b64b4e3df36a8bb307a52e0ecd96', '0x5076d0': 'ev_7b15633cd5080812b34079a89a5c3a5ce908e229bc704046347623e506793990', '0x507b30': 'ev_a8531daedf5d3c0e2b7c2e2f65857a82a4870fba3a9ad4ae0d2454aa6d5a683e', '0x506950': 'ev_cd6f40cbecc465949dfdec5f651a985ff37d11e04f324d29b20d21f9ac8d6fcf', '0x5069a0': 'ev_eaf94f088624deab964489f87aff4beabfc1377b7ca42022cd6c41f8142b59cb', '0x5069e0': 'ev_e89b6c81c9efbeb9c2b54e1af796e4cd3428ecbbdf20eab4cee15d2ee2a52435', '0x5069f0': 'ev_65c9884ce2d45bc03127aad67e8159f7be8c5b837161ce3bd24d060609584bdb', '0x506630': 'ev_c0b65e58125fa62299e2477cc7801087522cef43a614b5b55dcb6b7c1614e74e', '0x5063e0': 'ev_0293e022be17c8d80aeae22bb7361c1da326351f586915badbb26a34c07e5ca4', '0x506490': 'ev_c046295028dbbf25e7042ff28409cd4ed1998e256874e5653e80e512ce4f9a60', '0x4946d0': 'ev_1e187751eb71b88a111239bcde598c964c2304647a01f8c71ed09353b5273768', '0x412d00': 'ev_181853778a01df4bba7d3093b46b44af2feb1b3f626fdbb6e46f251e44a416ad', '0x412d20': 'ev_33b718827c4731d4e39cc8d8b344fe7c5fc09c6b96d346dce85d5fd31f206de7', '0x506800': 'ev_452df0412843464c966dd3767de11ead24137707d0fd139534ac15535d79ef86', '0x4b3f90': 'ev_2914c16ff86e58eeb025301b2665152c42144fd225896f9f024d97b79d0315ba', '0x5071d0': 'ev_9c7bbad1e76f70db916a6e4a9363170abf3175d42605f6b2a1ebb052a1105fe1', '0x431cb0': 'ev_73192f7325b0594b293abda73e308f5d430cc6800ca42a2406ec069176b0360e', '0x431bb0': 'ev_794b9e0e8c25d0ec4364460904a05976e669234b3af93cafefafb0059ff03b74', '0x5077b0': 'ev_09fcb28a7a0268f5ff26ee4c5ff44b0a72d64b1589b073d8058628613c4795fc', '0x420c50': 'ev_f58957074cd7d828424305cf0964f2a9b0c26bba24ca68b217c34525acbb02a8', '0x420aa0': 'ev_5b4b4b7f638ced87bb364ad88897b04ed67f2e4b1a924e62267dc2ac6a90350c', '0x420cf0': 'ev_fde958489ff973c85fa4e022e92d5f9a6669c78a601d5f96b5f6bc5ff4946ebb', '0x420d30': 'ev_085959c55620240207d6ff21d4e7c3bea1bce7582f646651a5088480671095ca', '0x420d50': 'ev_d9e9d6c8adbaabad97716b4852d93ca92fe21a8c490e01ad210193e8b7081cdd', '0x433190': 'ev_b43d18d79285859aa871f1f6e7c228939c4a3831ef887b4425827083007bccf9', '0x4334f0': 'ev_36f075c0c0b5f8cf69402ca615dfe6b2b3c9f0bf345e003a04de53d8c91dbd2b', '0x4343e0': 'ev_3e8d1046b4d6e3e8bc895f94bc16157b861cd6ee83f3ed488d1eb679408df836', '0x432f90': 'ev_a11e6ecd07ee5b549b228cc15ebc1c06c527bf2658e1b3012ebad2fcda4b19b4', '0x433c90': 'ev_1c058bb27282907699190c5b133cfa3c5477930f31924e151e9632b282e657b2'}
HEAP=0x22000000;TASK=0x23000000;WORK=TASK+0x100;NODES=TASK+0x1000;NAMES=TASK+0x3000;PROPS=TASK+0x5000
LISTS={0:0x10d7c90,3:0x10d7c08,7:0x10d7c28,8:0x10d7c30,11:0x10d7c18,13:0x10d7c38,15:0x10d7be8,22:0x10d7c98,24:0x10d7ca8,28:0x10d7bf0,100:0x915088,101:0x915068,102:TASK+0x6000}
REVERSE={v:k for k,v in LISTS.items()}
FTLOCAL=CODE+0x300;FTSYSTEM=CODE+0x310;LENGTH=CODE+0x320
class Source:
 def __init__(self,image):
  self.m=m=Original(image,127).machine
  for section in image.sections:
   if section['name'] in ('.text','.rdata'):continue
   size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095
   m.mem_map(image.base+section['rva'],size);m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
  m.mem_map(HEAP,0x800000);m.mem_map(TASK,0x10000)
  for identity,address in LISTS.items():
   names=HEAP+identity*0x10000;self.put(address,names);self.put(address+4,128)
   for index in range(128):
    memory=names+0x2000+index*64;self.put(names+index*12+8,memory)
    for off,value in [(0,identity*1000+index),(32,0),(36,32+(index*7+identity*3)%100),(40,17+(index*3+identity)%85)]:self.put(memory+off,value)
  self.put(0x7db104,FTLOCAL);self.put(0x7db100,FTSYSTEM);self.put(0x7db0c8,LENGTH)
  for address,data in [(0x50473e,b'\xeb\0'),(0x433c56,b'\0'),(0x433f4d,b'\0'),(0x43434a,b'\0')]:m.mem_write(address,data)
  m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.context=m.context_save()
 def word(self,address,size=4):return int.from_bytes(self.m.mem_read(address,size),'little')
 def put(self,address,value,size=4):self.m.mem_write(address,(value&((1<<(size*8))-1)).to_bytes(size,'little'))
 def ret(self,value=0,pop=0):
  sp=self.m.reg_read(UC_X86_REG_ESP);self.m.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.m.reg_write(UC_X86_REG_EIP,self.word(sp));self.m.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def event(self,kind,*values):
  self.events.extend([kind,len(values),*values]);self.boundaries+=1
  if self.boundaries==self.fail:self.failed=True;self.m.emu_stop()
 def text(self,address):
  data=bytearray()
  while self.word(address,1):data.append(self.word(address,1));address+=1
  return data.decode('ascii')
 def hook(self,m,a,size,user):
  sp=m.reg_read(UC_X86_REG_ESP)
  if a==0x403070:
   address=self.word(sp+4);self.list=REVERSE[address];self.put(0x3d0fa24,address);self.put(0x3abd950,address);self.put(0x3ab9934,0);self.event(1,self.list);self.ret(1)
  elif a==0x4030d0:self.event(2,self.word(sp+4),self.word(sp+8));self.ret()
  elif a==0x433060:self.event(3,self.list,m.reg_read(UC_X86_REG_ECX)&0x30ffffff)
  elif a==0x4332b0:
   identity=REVERSE[self.word(sp+4)];index=self.word(sp+8);self.event(3,identity,index)
   self.put(self.word(sp+12),32+(index*7+identity*3)%100);self.put(self.word(sp+16),17+(index*3+identity)%85);self.ret()
  elif a==0x4332f0:
   identity=REVERSE[self.word(sp+4)];index=self.word(sp+8);self.event(10,identity,index);self.ret(identity*1000+index)
  elif a==0x403130:
   vertices=self.word(sp+4);assert self.word(sp+8)==4
   self.event(4,self.word(sp+12),self.word(sp+16),*struct.unpack('<24I',m.mem_read(vertices,96)));self.ret()
  elif a==0x4128f0:self.event(5);self.ret()
  elif a==0x77ed00:self.event(6,self.word(sp+4));self.ret()
  elif a==0x77ed20:self.event(7,self.word(sp+4));self.ret()
  elif a==0x401420:self.event(9);self.ret()
  elif a==0x4010d0:
   block=self.word(sp+4);assert self.word(sp+8)==4
   points=struct.unpack('<8I',m.mem_read(self.word(block),32));colors=struct.unpack('<4I',m.mem_read(self.word(block+4),16))
   self.event(8,self.word(sp+16),self.word(sp+12),*points,*colors);self.ret()
  elif a==FTLOCAL:
   time=self.word(self.word(sp+4),8);self.event(11,time&0xffffffff,time>>32);m.mem_write(self.word(sp+8),time.to_bytes(8,'little'));self.ret(1,8)
  elif a==FTSYSTEM:
   time=self.word(self.word(sp+4),8);i=(time-132000000000000000)//10000000
   fields=[2020+i,1+i%12,0,2+i%27,3+i%20,4+i%50,5+i%50,0];m.mem_write(self.word(sp+8),struct.pack('<8H',*fields));self.ret(1,8)
  elif a==LENGTH:self.ret(len(self.text(self.word(sp+4))),4)
  elif a==0x644df9:
   assert self.text(self.word(sp+8))=='%02d/%02d/%02d  %02d:%02d:%02d'
   values=[self.word(sp+12+i*4) for i in range(6)];formatted='%02d/%02d/%02d  %02d:%02d:%02d'%tuple(values);m.mem_write(self.word(sp+4),formatted.encode()+b'\0');self.ret(len(formatted))
  elif a==0x421920:self.event(12,int(self.word(sp+4)!=0));self.ret()
 def run(self,v,save):
  m=self.m;m.context_restore(self.context);self.events=[];self.list=0;self.failed=False;self.fail=v[40];self.boundaries=0
  m.mem_write(0x3b2c5f0,bytes(96));self.put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,v[18]);m.reg_write(UC_X86_REG_FPSW,0);m.emu_start(0x432ea0,STOP,timeout=1000000)
  self.put(0x3b2c658,v[13]);self.put(0x3b1117c,v[12]);self.put(0x8928c0,bits(1));self.put(0x8928c4,bits(1))
  for i in range(4):self.put(0x3b2c604+24*i,v[14+i])
  self.put(TASK+0x2c,WORK)
  for offset,index in [(0xc,0),(0x14,1),(0x34,2),(0x18,3),(0x32,4)]:self.put(WORK+offset,v[index],1 if offset==0x32 else 4)
  for address,index,size in [(0x3b290e0,5,1),(0x3c5e8a4,6,2),(0x3c5e8b4,7,1),(0x3b290d8,8,1),(0x10d77ad,9,1),(0x3b2a2f0,10,1),(0x3b2a2fa,11,1),(0x3c5e8bc,22,1),(0x3abdf78,23,1)]:self.put(address,v[index],size)
  self.put(0x3c5e8b0,TASK+0x800);self.put(0x3c5e8a8,TASK+0x900)
  m.mem_write(0x3b2b3a8,bytes(save));m.mem_write(0x3b18808,bytes(v[384:896]));self.put(0x3c5e8b8,NODES)
  for i in range(v[19]+1):
   self.put(NODES+i*16,0 if i==0 else NAMES+i*32);self.put(NODES+i*16+4,0 if i==0 else 132000000000000000+i*10000000,8);self.put(NODES+i*16+12,NODES+(i+1)*16 if i<v[19] else 0)
   if i:m.mem_write(NAMES+i*32,f'SonicDX{i:02d}.snc'.encode()+b'\0')
  m.mem_write(PROPS,bytes(v[256:384]));m.mem_write(0x3b280c0,struct.pack('<4I',*v[24:28])+struct.pack('<2I',LISTS[102],PROPS)+struct.pack('<4I',*v[30:34]));self.put(0x3b280ec,v[28]);self.put(0x3b280bc,v[29])
  self.put(STACK,STOP);self.put(STACK+4,TASK);m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPSW,0);m.emu_start(0x505550,STOP,timeout=1000000);assert self.failed or m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*64;out[:24]=struct.unpack('<24I',m.mem_read(0x3b2c5f0,96));out[24:30]=[self.word(0x3b2c658),self.word(0x3b2c650),self.word(WORK+0x34),self.word(0x3c5e8b4,1),self.word(0x3b290d8,1),self.word(0x3b18808+89,1)];out[30:32]=[self.word(0x3b280c0),self.word(0x3b280c4)];out[32:36]=struct.unpack('<4I',m.mem_read(0x3b280d8,16));out[36]=m.reg_read(UC_X86_REG_FPCW);out[37]=m.reg_read(UC_X86_REG_FPSW)&63
  out[63]=int(self.failed)
  return out,self.events

def table_input(image):
 a=[0]*896;p=64
 for i in range(3):
  values=struct.unpack('<3fII4B',image.read(0x7eca38+24*i,24));words=[bits(x) for x in values[:3]]+list(values[3:]);a[p:p+len(words)]=words;p+=len(words)
 words=struct.unpack('<24h',image.read(0x10d7cfe,48));a[p:p+24]=[x&0xffffffff for x in words];p+=24
 for address,fmt in [(0x7eca20,'<4B'),(0x7eca24,'<7B'),(0x7eca2c,'<7B'),(0x7ef6f0,'<7B'),(0x7ef65c,'<44B'),(0x7ef6b4,'<8H'),(0x7ef6c4,'<7I'),(0x7ef704,'<10B')]:
  words=struct.unpack(fmt,image.read(address,struct.calcsize(fmt)));a[p:p+len(words)]=words;p+=len(words)
 a[256:384]=image.read(0x90e400,128)
 return a

def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--seed',type=int,default=20261118);p.add_argument('--cases',type=int,default=600);p.add_argument('--native-source-root',type=Path,default=Path(__file__).resolve().parents[2]);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256
 source=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()));U=ctypes.c_uint32;B=ctypes.c_uint8;probe=lib.sadx_native_file_select_display;probe.argtypes=[ctypes.POINTER(U),ctypes.POINTER(B),ctypes.POINTER(U),ctypes.POINTER(U),ctypes.c_size_t];rng=random.Random(a.seed);bad=[];flags=0
 for case in range(a.cases):
  v=table_input(image);count=[0,1,2,3,4,5,8,98,99][case%9];scroll=0 if count<4 or case%4==0 else rng.randrange(count-2);card=rng.randrange(min(3,count-scroll)+1) if count else 0;displayed=scroll+card
  v[:34]=[bits(rng.choice([-1.1,-2.25,-10.375,-0.5])),rng.choice([0xffffffff,0x7fabcdef,0]) if case else 0xffffffff,bits(rng.uniform(-100,100)),bits(rng.uniform(0,2)),rng.randrange(256),count,scroll,card,displayed,displayed,rng.randrange(2),1,0,rng.getrandbits(32),*[rng.getrandbits(32) for _ in range(4)],rng.choice([127,895,3967]),count,0,0,rng.randrange(2),rng.randrange(2),bits(rng.choice([13,16,17.25])),bits(rng.choice([13,16,19.25])),bits(256),bits(128),bits(16),bits(16),*[rng.getrandbits(32) for _ in range(4)]]
  v[12]=rng.choice([0,0,0,1,3,0xffffffff])
  if count==0:v[7]=rng.randrange(1,4);v[8]=rng.randrange(1,100)
  v[40]=rng.choice([0,0,0,1,5,20,50,100,200,500,750])
  v[384:896]=[rng.randrange(2) for _ in range(512)]
  save=bytearray(1392);struct.pack_into('<I',save,4,rng.randrange(100000000));save[601]=rng.randrange(9);struct.pack_into('<H',save,604,rng.choice([1,5,26,35,38,43,100]));save[576:593]=bytes([255]*17 if case%5==0 else [rng.randrange(256) for _ in range(17)])
  for i in range(8):struct.pack_into('<H',save,744+i*12+8,rng.choice([1,26,27,33])<<8)
  expected,et=source.run(v,save);out=(U*64)();trace=(U*65536)();length=probe((U*len(v))(*v),(B*1392).from_buffer_copy(save),out,trace,len(trace));assert length>=0
  differences=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y and i!=37];actual=list(trace[:length]);flags+=expected[37]!=out[37]
  if differences or et!=actual:
   first=next((i for i,(x,y) in enumerate(zip(et,actual)) if x!=y),min(len(et),len(actual)))
   bad.append({'case':case,'input_prefix':v[:34],'differences':differences,'trace_lengths':[len(et),len(actual)],'first_trace_difference':first,'source_near':et[max(0,first-8):first+30],'native_near':actual[max(0,first-8):first+30]})
 paths=['src/engine/menu_draw_primitives.hpp','src/engine/menu_draw_primitives.cpp','src/game/file_select_display.hpp','src/game/file_select_display.cpp','tests/probes/native_file_select_display_probe.cpp']
 report={'cases':a.cases,'seed':a.seed,'mismatch_count':len(bad),'fp_flag_partials':flags,'flag_mismatches':flags,'mismatches':bad[:8],'values_match':not bad,'passed':not bad and not flags,'limits':__doc__,'source_sha256':image.sha256,'native_library_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'native_sources':{f:hashlib.sha256((a.native_source_root/f).read_bytes()).hexdigest() for f in paths},'source_ranges':{f'{hex(start)}-{hex(end)}':hashlib.sha256(image.read(start,end-start)).hexdigest() for start,end in SOURCE_RANGES},'evidence':EVIDENCE,'profile_patches':[{'va':'0x50473e','original':'75f0','effective':'eb00','source':'loaderdllmain944'},{'va':'0x433f4d','original':'38','effective':'00','source':'GVR_Initgvm851'},{'va':'0x507bb0','effective':'background-scale wrapper identity qualified only640x480','source':'bgscale28/uiscale.cpp'}]}
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','mismatch_count','fp_flag_partials']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
