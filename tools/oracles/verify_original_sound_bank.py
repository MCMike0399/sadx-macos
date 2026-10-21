"""Effective pinned LoadSoundList_r and original ReleaseBankSounds instructions with explicit native-owner fixtures.

40ff10 readiness executes unchanged. The active loader jump is applied. Formatting/ASCII comparison, replacement priorities and archive/PCM
resource boundaries are declared fixtures. Importer independently verifies source
catalog bytes; this judges selection/release ordering and partial failure stores.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_ECX,UC_X86_REG_EDX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,CODE,STACK,STOP
from sound_bank_original_reference import Source as OriginalSource
LOADER_SHA = '3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba'
class Source(OriginalSource):
 def __init__(self,image,loader):
  assert loader.sha256==LOADER_SHA
  super().__init__(image)
  for section in loader.sections:
   size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095
   self.m.mem_map(loader.base+section['rva'],size);self.m.mem_write(loader.base+section['rva'],loader.slice(section['file_offset'],section['raw_size']))
  self.m.mem_write(0x4238e0,b'\xe9'+struct.pack('<i',0x100979a0-0x4238e5))
  self.put(0x100cd0d8,CODE+528)
  assert loader.unpack('<II',0x100f19cc)==(0x90fcb8,123)
 def select(self,c):
  self.pack_priority=c['pack_priority'];self.file_priority=c['file_priority']
  return super().select(c)
 def hook(self,m,a,size,user):
  if self.release_mode:
   return super().hook(m,a,size,user)
  if a==CODE+528:return super().hook(m,a,size,user)
  if a not in [0x10049970,0x100822b0,0x423890,0x4b4f50,0x10096530,0x10097120]:return
  sp=m.reg_read(UC_X86_REG_ESP)
  if a==0x10049970:
   dest=self.get(sp+4);count=self.get(sp+8);fmt=self.string(self.get(sp+12));args=[self.string(self.get(sp+16))]
   if fmt.count('%s')==2:args.append(self.string(self.get(sp+20)))
   result=fmt % tuple(args);self.write_string(dest,result[:count-1]);self.ret(sp,len(result));return
  if a==0x100822b0:
   path=self.string(self.get(sp+4));result=self.pack_priority if path.endswith('\\') else self.file_priority;event='M|'+path+'|'+str(result)+'\n';pop=4
  elif a==0x423890:event='R|'+str(self.get(sp+4))+'\n';result=0;pop=0
  elif a==0x4b4f50:event='C|'+str(self.ids.get(self.get(sp+4),0))+'\n';result=0;pop=0
  else:
   path=self.string(m.reg_read(UC_X86_REG_ECX));slot=m.reg_read(UC_X86_REG_EDX);event=('P|' if a==0x10096530 else 'F|')+path+'|'+str(slot)+'\n';result=0;pop=0
  self.trace+=event;self.events+=1
  if self.events==self.fail_event:
   self.boundary=self.get(sp)-(2 if a in [0x423890,0x4b4f50] else 5);m.emu_stop();return
  if a in [0x10096530,0x10097120]:
   number=self.opened;self.opened+=1
   if self.mask&(1<<number):result=self.handle(101+number,path)
  self.ret(sp,result,pop)
def fixtures(seed):
 rng=random.Random(seed);cases=[]
 for n in range(700):
  entries=[(rng.randrange(16),rng.choice(['COMMON_BANK00','BANK_A','bank_b',''])) for _ in range(rng.randrange(9))];cd=rng.choice(['','D:\\','e:\\install\\']);initial=[rng.choice([None,'old.dat','SYSTEM\\SoundData\\SE\\OLD.dat']) for _ in range(16)]
  for slot,name in entries:
   if rng.randrange(3)==0:initial[slot]=(rng.choice(['SYSTEM\\SoundData\\SE\\'+name+'\\','SYSTEM\\SoundData\\SE\\'+name+'.dat',cd+'SoundData\\SE\\'+name+'.dat'])).swapcase()
  cases.append({'pack_priority':rng.choice([-2,-1,0,1,7]),'file_priority':rng.choice([-2,-1,0,1,7]),'index':rng.choice([42,rng.randrange(123),123,2147483647]),'ready':rng.choice([0,1,17]),'voice':rng.choice([0,1,7]),'entries':entries,'initial':initial,'cd':cd,'mask':rng.getrandbits(32),'fail':rng.choice([0,0,0,rng.randrange(1,25)])})
 cases += [{'pack_priority':-1,'file_priority':-1,'index':42,'ready':1,'voice':1,'entries':[(0,'X')]*256,'initial':[None]*16,'cd':'','mask':1,'fail':0}]
 cases += [{'pack_priority':-1,'file_priority':-1,'index':42,'ready':1,'voice':1,'entries':[(0,'A'*280)],'initial':[None]*16,'cd':'D:'+'x'*240,'mask':0,'fail':0}]
 cases += [{'pack_priority':-1,'file_priority':-1,'index':-1,'ready':r,'voice':v,'entries':[],'initial':[None]*16,'cd':'','mask':0,'fail':0} for r,v in [(0,0),(0,1),(1,0)]]
 return cases,rng
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('probe',type=pathlib.Path);p.add_argument('--output',required=True,type=pathlib.Path);p.add_argument('--seed',type=int,default=20261029);p.add_argument('--loader-dll',type=pathlib.Path,default=pathlib.Path('reference/updated-pc-reference/game/mods/.modloader/SADXModLoader.dll'));a=p.parse_args();image=PEImage(a.exe);loader=PEImage(a.loader_dll);source=Source(image,loader);lib=ctypes.CDLL(str(a.probe.resolve()));U=ctypes.c_uint32;C=ctypes.c_char_p
 select=lib.sound_bank_select_probe;select.argtypes=[ctypes.c_int32,U,U,ctypes.POINTER(U),ctypes.POINTER(C),U,C,ctypes.POINTER(C),U,U,ctypes.c_int32,ctypes.c_int32,ctypes.POINTER(U),ctypes.c_char_p,ctypes.c_size_t]
 release=lib.sound_bank_release_probe;release.argtypes=[U,ctypes.POINTER(U),U,ctypes.POINTER(U),ctypes.POINTER(U)]
 cases,rng=fixtures(a.seed);bad=[]
 for number,c in enumerate(cases):
  expected,trace=source.select(c);out=(U*18)();actual_trace=ctypes.create_string_buffer(1048576);slots=(U*len(c['entries']))(*[x[0] for x in c['entries']]);names=(C*len(c['entries']))(*[x[1].encode() for x in c['entries']]);initial=(C*16)(*[x.encode() if x is not None else None for x in c['initial']]);assert select(c['index'],c['ready'],c['voice'],slots,names,len(c['entries']),c['cd'].encode(),initial,c['mask'],c['fail'],c['pack_priority'],c['file_priority'],out,actual_trace,len(actual_trace))==1
  if list(out)!=expected or actual_trace.value.decode()!=trace:bad.append({'cohort':'select','case':number,'input':c,'expected':expected,'actual':list(out),'source_trace':trace,'native_trace':actual_trace.value.decode()})
 for number in range(700):
  rows=[[rng.getrandbits(32),rng.choice([0,rng.getrandbits(32)]),rng.getrandbits(32),rng.choice([0xffffffff,rng.getrandbits(32)]),rng.randrange(16),rng.choice([0,rng.getrandbits(32)])] for _ in range(36)];slot=rng.randrange(16);fail=rng.choice([0xffffffff,rng.randrange(36)]);expected,trace,completed=source.release(slot,rows,fail);flat=[v for row in rows for v in row];out=(U*216)();calls=(U*37)();done=release(slot,(U*216)(*flat),fail,out,calls)
  if list(out)!=expected or list(calls)[1:calls[0]+1]!=trace or done!=completed:bad.append({'cohort':'release','case':number,'expected':expected,'actual':list(out),'source_calls':trace,'native_calls':list(calls)[1:calls[0]+1],'completed':[completed,done]})
 report={'loader_sha256':loader.sha256,'source_sha256':image.sha256,'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'seed':a.seed,'selection_cases':len(cases),'release_cases':700,'mismatch_count':len(bad),'mismatches':bad[:10],'stores_calls_boundaries_match':not bad,'scope':__doc__};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['selection_cases','release_cases','mismatch_count']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
