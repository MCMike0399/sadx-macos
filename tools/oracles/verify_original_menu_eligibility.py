"""Full original eligibility bodies and all wrappers; no call hooks or guessed invalid return."""
import argparse,ctypes,hashlib,json,random,struct,sys
from pathlib import Path
sys.path[:0]=['tools/lib','tools/oracles']
from pe_image import PEImage
from verify_original_math import Original,STACK,STOP
from extract import SOURCE_SHA256
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--seed',type=int,default=20261021);p.add_argument('--cases',type=int,default=5000);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256
m=Original(image,127).machine
for sec in image.sections:
 if sec['name'] in ('.text','.rdata'):continue
 size=(max(sec['virtual_size'],sec['raw_size'])+4095)&~4095;m.mem_map(image.base+sec['rva'],size);m.mem_write(image.base+sec['rva'],image.slice(sec['file_offset'],sec['raw_size']))
context=m.context_save();B=ctypes.c_uint8;lib=ctypes.CDLL(str(a.probe.resolve()));probe=lib.sadx_native_menu_eligibility;probe.argtypes=[ctypes.POINTER(B),ctypes.c_uint32,ctypes.c_uint32,ctypes.POINTER(B)];probe.restype=ctypes.c_int32
rng=random.Random(a.seed);cases=[]
for count in [0,1,127,128,129,130,131,135,136]:
 for kind in range(3):
  for char in range(7):
   for tutorial in [0,1,2,255]:
    for override in [0,2,255]:
     data=bytearray(531)
     for bit in range(count):data[bit//8]|=1<<(bit%8)
     data[529:]=bytes([override,tutorial]);cases.append((kind,char,data))
for char in range(7):
 for flag in [0,1,2,127,255]:
  for tutorial in [0,1,255]:
   data=bytearray(531);data[17+[65,66,67,68,70,69,71][char]]=flag;data[17+89]=flag;data[530]=tutorial;cases.append((2,char,data))
for char in [7,8,255,0x80000000,0xffffffff]:
 for override in [0,1,255]:
  data=bytearray(531);data[529]=override;cases.append((2,char,data))
for _ in range(a.cases):
 data=bytearray(rng.randbytes(531));data[529]=rng.choice([0,0,0,1,2,255]);data[530]=rng.choice([0,1,2,255]);cases.append((rng.randrange(3),rng.randrange(7),data))
bad=[];unknown=0;fp_bad=0
for index,(kind,char,data) in enumerate(cases):
 m.context_restore(context);m.mem_write(0x3b2b3a8+576,bytes(data[:17]));m.mem_write(0x3b18808,bytes(data[17:529]));m.mem_write(0x3b2a2f0,bytes(data[529:530]));m.mem_write(0x3b2a2fa,bytes(data[530:]));cw=rng.choice([127,895,3967]);sw=0
 m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,sw);m.reg_write(UC_X86_REG_ECX,0xdeadbeef);m.mem_write(STACK,struct.pack('<II',STOP,0x3b2b3a8 if kind==0 else char));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start([0x4b3f90,0x506490,0x506630][kind],STOP,count=20000);assert m.reg_read(UC_X86_REG_ESP)==STACK+4
 expected=m.reg_read(UC_X86_REG_EAX);flags=bytes(m.mem_read(0x3b18808,512));out=(B*512)();actual=probe((B*531).from_buffer_copy(data),kind,char,out)
 is_unknown=kind==2 and char>6 and data[529]==0
 if is_unknown:unknown+=1;match=actual==-999 and expected==0xdeadbeef
 else:match=actual==expected
 fp_bad+=m.reg_read(UC_X86_REG_FPCW)!=cw or m.reg_read(UC_X86_REG_FPSW)!=sw
 if not match or bytes(out)!=flags:bad.append({'case':index,'kind':kind,'character':char,'expected':expected,'actual':actual})
report={'cases':len(cases),'seed':a.seed,'random_cases':a.cases,'mismatch_count':len(bad),'fp_flag_partials':fp_bad,'unknown_character_cases':unknown,'values_match':not bad,'passed':not bad and not fp_bad,'mismatches':bad[:10],'source_sha256':image.sha256,'native_library_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'limits':__doc__,'source_ranges':[[hex(x),hex(y),hashlib.sha256(image.read(x,y-x)).hexdigest()] for x,y in [(0x4b3f90,0x4b3fba),(0x506490,0x5064ca),(0x506630,0x5066f8),(0x507b30,0x507b3f),(0x506800,0x506806),(0x4946d0,0x4946d9),(0x412d00,0x412d0c),(0x412d20,0x412d32)]]}
a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','mismatch_count','fp_flag_partials','unknown_character_cases']}));raise SystemExit(bool(bad) or bool(fp_bad))
