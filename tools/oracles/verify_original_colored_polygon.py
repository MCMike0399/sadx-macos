"""Original4010d0 ->77e9f0 ->78eab0 colored strips, through actual D3D8 API calls.

The real alpha/filter/cache/strip instructions execute; device methods are receipts
recording their state and converted bytes. Projected-depth78dbe0 is a declared
seeded FLD/RET service, independently covered by the existing projection oracle.
Depth receipts can rebind the live texture cache to test the required second clear.
No GPU-pixel, projection-math or Windows driver-failure parity is asserted here.

Profile qualification: the active loader njDrawTriangle2D_r hook forwards to this
original body unchanged only when its UIScale stack is inactive. Current FileSelect
calls follow background scale_pop, with scaling disabled at 640x480. This proves
the original body plus that bounded forwarding condition, not the hook globally.
The 4010d0 positive-flags normalization clears bit0, so the patched circle-call
branch is unreachable for these cases. Scaling-enabled wrapper behavior is outside
this proof; no source inputs or expectations are filtered to conceal mismatches.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FP0
from pe_image import PEImage
from verify_original_math import Original,DATA,CODE,STACK,STOP,bits
from extract import SOURCE_SHA256
DEVICE=DATA+0x100;VTABLE=DATA+0x400;BLOCK=DATA+0x800;POINTS=DATA+0x900;COLORS=DATA+0xa00;BUFFER=DATA+0x1000;SEED=DATA+0x5000
METHODS={0xc8:(CODE+0x300,12),0xfc:(CODE+0x310,16),0xf4:(CODE+0x320,12),0x130:(CODE+0x330,8),0x120:(CODE+0x340,20)}
RENDER=[27,22,137,145,14,23,9];STAGES=[4,16,17,18]
class Source:
 def __init__(self,image):
  self.m=m=Original(image,127).machine
  for s in image.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095;m.mem_map(image.base+s['rva'],size);m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
  for offset,(stub,pop) in METHODS.items():m.mem_write(stub,b'\x31\xc0\xc2'+struct.pack('<H',pop));self.put(VTABLE+offset,stub)
  self.put(DEVICE,VTABLE);m.mem_write(0x78dbe0,b'\xd9\x05'+struct.pack('<I',SEED)+b'\xc2\x04\0');m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.context=m.context_save()
 def word(self,address):return int.from_bytes(self.m.mem_read(address,4),'little')
 def put(self,address,value):self.m.mem_write(address,(value&0xffffffff).to_bytes(4,'little'))
 def state_words(self):return [self.render[i] for i in RENDER]+[self.stages[i] for i in STAGES]
 def hook(self,m,a,size,user):
  sp=m.reg_read(UC_X86_REG_ESP)
  if a==0x77ea46 and self.observe_spill:
   status=m.reg_read(UC_X86_REG_FPSW);top=(status>>11)&7;mantissa,exponent=m.reg_read(UC_X86_REG_FP0+top)
   self.spill={'pc':hex(a),'opcode':bytes(m.mem_read(a,4)).hex(),'control_word':m.reg_read(UC_X86_REG_FPCW),'st0_significand':mantissa,'st0_sign_exponent':exponent,'device_state':self.state_words(),'depth_calls':len(self.depths),'unbind_calls':len(self.markers)}
   m.emu_stop();return
  if a==METHODS[0xc8][0]:self.render[self.word(sp+8)]=self.word(sp+12)
  elif a==METHODS[0xfc][0]:assert self.word(sp+8)==0;self.stages[self.word(sp+12)]=self.word(sp+16)
  elif a==METHODS[0xf4][0]:assert self.word(sp+8)==0;self.texture=self.word(sp+12)!=0
  elif a==METHODS[0x130][0]:self.fvf=self.word(sp+8)
  elif a==METHODS[0x120][0]:
   kind,count,pointer,stride=[self.word(sp+8+i*4) for i in range(4)];assert pointer==BUFFER
   self.draw=[kind,count,int(self.texture),*struct.unpack('<20I',m.mem_read(pointer,80))];assert stride==20;self.draw_state=self.state_words();self.draw_fvf=self.fvf
  elif a==0x78bba0:
   self.markers.append(self.state_words()+[self.word(0x3d0fdf4),int(self.texture)])
  elif a==0x78dbe0:
   self.depths.append(self.word(sp+4));i=len(self.depths)
   if i==self.v[18]:self.texture=True;self.put(0x3d0fdf4,99);self.put(0x3d129a0,0x1234)
   self.put(SEED,self.v[20+i-1])
 def run(self,v,observe_spill=False):
  self.v=v;self.observe_spill=observe_spill;self.spill=None;m=self.m;m.context_restore(self.context);self.render=dict(zip(RENDER,v[32:39]));self.stages=dict(zip(STAGES,v[39:43]));self.texture=v[17]!=0;self.fvf=0;self.depths=[];self.markers=[];self.draw=[];self.draw_state=[]
  self.put(0x3d128b0,DEVICE);self.put(0x3d128ac,BUFFER);self.put(0x3d0fdf4,v[16]);self.put(0x3d129a0,0x1234 if v[17] else 0)
  m.mem_write(BLOCK,struct.pack('<4I',POINTS,COLORS,0,4));m.mem_write(POINTS,struct.pack('<8I',*v[4:12]));m.mem_write(COLORS,struct.pack('<4I',*v[12:16]));m.mem_write(BUFFER,bytes(80));m.reg_write(UC_X86_REG_FPCW,v[0]);m.reg_write(UC_X86_REG_FPSW,0);m.mem_write(STACK,struct.pack('<5I',STOP,BLOCK,4,v[2],v[3]));m.reg_write(UC_X86_REG_ESP,STACK)
  failed=False
  try:m.emu_start(0x4010d0,STOP,timeout=1000000)
  except unicorn.UcError as error:
   if error.errno!=unicorn.UC_ERR_EXCEPTION:raise
   failed=True
  if observe_spill:
   assert self.spill is not None and not failed
   return self.spill
  assert failed or m.reg_read(UC_X86_REG_EIP)==STOP
  out=[0]*128;out[0]=int(failed);out[1]=int(bool(self.draw))
  if self.draw:out[2:25]=self.draw;out[31]=self.draw_fvf;out[43:54]=self.draw_state
  out[25:31]=[self.word(0x3d0fdf4),int(self.texture),int(self.word(0x3d129a0)!=0),m.reg_read(UC_X86_REG_FPSW)&63,len(self.depths),len(self.markers)]
  out[32:43]=self.state_words();out[56:56+len(self.depths)]=self.depths
  for i,marker in enumerate(self.markers):out[64+i*13:77+i*13]=marker
  return out

def fixtures(seed,count):
 rng=random.Random(seed);cases=[]
 for cw in [127,639,895,1151,1663,1919,2175,2687,2943,3199,3711,3967]:
  for priority in [bits(-1),bits(1),bits(-0.5),bits(0),bits(-0.0),1,0x80000001,0x7fc00000]:
   for rebind in [0,1,2,4]:
    v=[0]*64;v[0]=cw;v[2]=priority;v[3]=rng.choice([0,32,64,96]);v[4:12]=[bits(x) for x in [0,0,0,480,640,0,640,480]];v[12:16]=[0xff112233,0x80445566,0x40778899,0x00aabbcc];v[16]=rng.choice([0xffffffff,7]);v[17]=rng.randrange(2);v[18]=rebind;v[20:24]=[bits(x) for x in [0.1,0.3,0.5,0.7]];v[32:43]=[0,2,1,0,1,4,2,3,2,2,2];v[43:46]=[rng.randrange(2) for _ in range(3)];cases.append(v)
 for _ in range(count):
  v=[0]*64;v[0]=rng.choice([127,639,895,1151,1663,1919,2175,2687,2943,3199,3711,3967]);v[2]=bits(rng.uniform(-100,100));v[3]=rng.getrandbits(31);v[4:12]=[bits(rng.uniform(-10000,10000)) for _ in range(8)];v[12:16]=[rng.getrandbits(32) for _ in range(4)];v[16]=rng.choice([0xffffffff,7,99]);v[17]=rng.randrange(2);v[18]=rng.randrange(5);v[20:24]=[bits(rng.uniform(-10,10)) for _ in range(4)];v[32:43]=[rng.randrange(2),rng.randrange(1,4),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(1,9),rng.randrange(1,3),rng.choice([3,4]),rng.randrange(1,4),rng.randrange(1,4),rng.randrange(1,4)];v[43:46]=[rng.randrange(2) for _ in range(3)];cases.append(v)
 # OE unmasked, DE masked: force the binary32 FST to fail BEFORE clamp/device calls.
 for priority in [1,0x80000001]:
  v=cases[0].copy();v[0]=119;v[2]=priority;v[18]=1;cases.append(v)
 return cases

def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--cases',type=int,default=500);p.add_argument('--seed',type=int,default=20261119);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256 and unicorn.__version__=='2.1.4';source=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()));U=ctypes.c_uint32;fn=lib.sadx_native_colored_polygon;fn.argtypes=[ctypes.POINTER(U),ctypes.POINTER(U)];bad=[];flags=0;architectural=[];cases=fixtures(a.seed,a.cases)
 for index,v in enumerate(cases):
  expected=source.run(v);out=(U*128)();fn((U*len(v))(*v),out);different=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y and i!=28];flags+=expected[28]!=out[28]
  if different:bad.append({'case':index,'input':v,'differences':different[:32]})
  if v[0]==119:
   observed=source.run(v,observe_spill=True);exponent=observed['st0_sign_exponent']&32767
   overflow=16383+128<=exponent<32767
   instruction=observed['opcode']=='d95c2420' and observed['control_word']==119
   prefix=observed['depth_calls']==0 and observed['unbind_calls']==0 and observed['device_state']==v[32:43]
   native=out[0]==1 and out[1]==0 and out[29]==0 and out[30]==0 and list(out[32:43])==observed['device_state']
   architectural.append({'case':index,'source_spill_observation':observed,'float32_overflow_from_finite_st0':overflow,'overflow_mask_clear':not bool(observed['control_word']&8),'source_prefix_before_device':prefix,'native_exception_prefix_matches_architecture':bool(instruction and overflow and prefix and native),'raw_unicorn_failed':expected[0],'raw_unicorn_draws':expected[1],'raw_unicorn_flags':expected[28]})
 root=Path.cwd();paths=['src/engine/pvr_device.hpp','src/engine/pvr_device.cpp'];report={'cases':len(cases),'normal_cases':sum(v[0]!=119 for v in cases),'normal_case_mismatch_count':sum(b['input'][0]!=119 for b in bad),'architectural_prefix_mismatches':sum(not p['native_exception_prefix_matches_architecture'] for p in architectural),'architectural_spill_checks':architectural,'pinned_profile_control_word':127,'architectural_reference':'https://cdrdv2-public.intel.com/671110/325383-sdm-vol-2abcd.pdf FST/FSTP: unmasked overflow to memory suppresses store; instruction bytes and finite ST0 exponent independently establish the boundary.','unicorn_limit':'Raw two CW119 traces remain value mismatches: Unicorn2.1.4 silently stores overflowing float32 instead of trapping. Architectural checks are separate; expected execution is never replaced. Low-six FP flags also remain partial; unmasked status-flag exactness is unqualified.','profile_qualification':{'kind':'original body with bounded active-hook forwarding','hook':'77e9f0 njDrawTriangle2D_r; original forwarded when uiscale::is_scale_enabled() is false','actual_caller':'FileSelect after background scale_pop, 640x480; pinned normal CW127','unreachable_hook':'77ea10 circle call requires bit0, cleared by positive flags normalization in4010d0','not_proved':'compiled loader wrapper execution; scaling-enabled behavior; full wrapper implementation','source_evidence':{f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in ['reference/sadx-mod-loader/SADXModLoader/uiscale.cpp','reference/sadx-mod-loader/SADXModLoader/bgscale.cpp','tools/gate/profile_patches.json']}},'source_ranges':{'0x4010d0':'0x40140f','0x77e9f0':'0x77eaca','0x78eab0':'0x78eb4f','0x791940':'0x79198b','0x78bba0':'0x78bbd0','0x78b780':'0x78b7f9','0x78b800':'0x78b83d'},'seed':a.seed,'mismatch_count':len(bad),'flag_mismatches':flags,'values_match':not bad,'passed':not bad and not flags,'mismatches':bad[:8],'limits':__doc__,'source_sha256':image.sha256,'native_library_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'native_sources':{f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in paths}}
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','mismatch_count','flag_mismatches']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
