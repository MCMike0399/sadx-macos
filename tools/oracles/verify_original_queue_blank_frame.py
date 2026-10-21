"""Actual4086f0 early-mode path including real4071c0 and401420 instructions.

403d30 temporary allocations are actual-owned-memory service fixtures; allocation
cursor/growth is not certified.4073b0 queued-box and4010d0 polygon bodies are ordered renderer receipts; the
existing PVR primitive port is reused, not fabricated. Nonzero-mode branch and
mode0 with displaydisabled are compared; normal mode0 record traversal remains
outside this bounded path. Finite scale inputs, all-masked x87 controls; FP status
is reported separately. No GPU image, active UI scaling or queued-box consumer claim.
"""
import argparse,ctypes,hashlib,json,random,struct,sys
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_EAX
root=Path(__file__).resolve().parents[2];stage=root;source_exe=None;native_library=None;report_path=None;native_source_root=root;sys.path[:0]=[str(root/'tools/lib'),str(root/'tools/oracles')]
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
class Source:
 def __init__(self):
  self.image=im=PEImage(source_exe);assert im.sha256==SOURCE_SHA256
  self.m=m=Original(im,127).machine
  for s in im.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['raw_size'],s['virtual_size'])+4095)&~4095;m.mem_map(im.base+s['rva'],size);m.mem_write(im.base+s['rva'],im.slice(s['file_offset'],s['raw_size']))
  for address in [0x403d30,0x77ff90,0x77ed00,0x4073b0,0x4010d0]:m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=address,end=address)
  self.clean=m.context_save()
 def word(self,a):return int.from_bytes(self.m.mem_read(a,4),'little')
 def put(self,a,v):self.m.mem_write(a,(v&0xffffffff).to_bytes(4,'little'))
 def byte(self,a,v):self.m.mem_write(a,bytes([v&255]))
 def hook(self,m,address,size,user):
  sp=m.reg_read(UC_X86_REG_ESP);row=[0]*16
  if address==0x403d30:
   self.heap+=128;m.reg_write(UC_X86_REG_EAX,self.heap);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4);return
  if address==0x77ff90:row[:5]=[4,*struct.unpack('<4I',m.mem_read(self.word(sp+4),16))]
  elif address==0x77ed00:row[:2]=[1,self.word(sp+4)]
  elif address==0x4073b0:
   assert self.word(sp+28)==1
   row[:7]=[2,*struct.unpack('<6I',m.mem_read(sp+4,24))]
  else:
   packet=self.word(sp+4);points=self.word(packet);colors=struct.unpack('<4I',m.mem_read(self.word(packet+4),16));assert len(set(colors))==1 and self.word(packet+8)==0 and self.word(packet+12)==4 and self.word(sp+8)==4; color=colors[0];row[:12]=[3,*struct.unpack('<8I',m.mem_read(points,32)),color,self.word(sp+12),self.word(sp+16)]
  row[12]=m.mem_read(0x909eb4,1)[0];row[13]=self.word(0x3abd9c0);self.trace.append(row)
  if len(self.trace)==self.v[8]:self.failed=True;m.emu_stop();return
  if row[0]==1 and row[1]==7 and (self.v[9]&1):self.put(0x8928c0,self.v[10]);self.put(0x8928c4,self.v[11])
  if row[0] in [2,3] and (self.v[9]&2):self.put(0x8928c8,self.v[12])
  if row[0]==2 and (self.v[9]&4):self.put(0x3b1117c,self.v[13]);self.byte(0x909eb4,self.v[14])
  if address!=0x77ff90:m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
 def run(self,v):
  self.v=v;self.trace=[];self.failed=False;self.heap=0x11004000;m=self.m;m.context_restore(self.clean)
  for a,x in [(0x3b1117c,v[1]),(0x8928c0,v[4]),(0x8928c4,v[5]),(0x3abd9c0,v[6]),(0x8928c8,v[7]),(0x3ab98ac,v[16])]:self.put(a,x)
  self.byte(0x909eb4,v[2]);self.byte(0x3abdcfe,v[3]);m.mem_write(0x3b18230,struct.pack('<4f',1,1,1,1))
  m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,v[0]);m.reg_write(UC_X86_REG_FPSW,0);m.emu_start(0x4086f0,STOP,timeout=0)
  assert self.failed or m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*128;out[:10]=[int(not self.failed),self.word(0x3ab98ac),m.mem_read(0x909eb4,1)[0],self.word(0x3abd9c0),self.word(0x3b1117c),self.word(0x8928c8),len(self.trace),m.reg_read(UC_X86_REG_FPSW)&63,self.word(0x8928c0),self.word(0x8928c4)]
  for n,row in enumerate(self.trace):out[16+n*16:32+n*16]=row
  return out

def run(count=1200,seed=409024,output='dev.json'):
 source=Source();U=ctypes.c_uint32;fn=ctypes.CDLL(str(native_library)).sadx_native_queue_blank_frame;fn.argtypes=[ctypes.POINTER(U)]*2;rng=random.Random(seed);bad=[];flags=0;outcomes={}
 bits=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
 for case in range(count):
  mode=rng.choice([0,1,2,2,2,2,3,0xffffffff]);disabled=1 if mode==0 else rng.choice([0,0,0,1]);v=[rng.choice([127,639,895,3199]),disabled,rng.choice([0,1,2,3,7,255]),rng.choice([0,0,1]),bits(rng.choice([0,1,1.3,2,-1,rng.uniform(-2,3)])),bits(rng.choice([0,1,1.3,2,-1,rng.uniform(-2,3)])),rng.getrandbits(32),rng.randrange(8),rng.choice([0,0,0,1,2,3,4,5]),rng.randrange(8),bits(rng.uniform(-2,3)),bits(rng.uniform(-2,3)),rng.randrange(8),rng.choice([0,1,0xffffffff]),rng.randrange(256),0,mode]
  expected=source.run(v);out=(U*128)();assert fn((U*17)(*v),out)==1
  diff=[(i,a,b) for i,(a,b) in enumerate(zip(expected,out)) if a!=b and i!=7];flags+=expected[7]!=out[7];key=f'mode{mode}:complete{expected[0]}:calls{expected[6]}';outcomes[key]=outcomes.get(key,0)+1
  if diff:bad.append({'case':case,'input':v,'differences':diff[:20]})
 report={'cases':count,'mismatch_count':len(bad),'flag_mismatches':flags,'values_match':not bad,'passed':not bad and not flags,'mismatches':bad[:8],'outcomes':outcomes,'seed':seed,'source_sha256':source.image.sha256,'limits':__doc__};report['native_library_sha256']=hashlib.sha256(native_library.read_bytes()).hexdigest()
 report['native_sources']={name:hashlib.sha256((native_source_root/name).read_bytes()).hexdigest() for name in ['src/engine/queue_blank_frame.hpp','src/engine/queue_blank_frame.cpp','tests/probes/native_queue_blank_frame_probe.cpp']}
 report['source_ranges']={f'{a:#x}-{b:#x}':hashlib.sha256(source.image.read(a,b-a)).hexdigest() for a,b in [(0x4086f0,0x40903b),(0x4071c0,0x4072e5),(0x401420,0x40142d)]}
 report_path.write_text(json.dumps(report,indent=2)+'\n');print({k:report[k] for k in ['cases','mismatch_count','flag_mismatches','passed']});return bool(bad)
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--seed',type=int,default=409024);parser.add_argument('--cases',type=int,default=1200);parser.add_argument('--native-source-root',type=Path,default=root);args=parser.parse_args()
 source_exe=args.exe;native_library=args.native_library;report_path=args.output;native_source_root=args.native_source_root
 raise SystemExit(run(args.cases,args.seed))
