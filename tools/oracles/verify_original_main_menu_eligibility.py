"""Original MainMenu predicates/getters, unhooked helpers. Language return ABI compares consumed signed/unsigned AL; source upper EAX is caller scratch."""
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
context=m.context_save();B=ctypes.c_uint8;H=ctypes.c_uint16;lib=ctypes.CDLL(str(a.probe.resolve()));probe=lib.sadx_native_main_menu_eligibility;probe.argtypes=[ctypes.POINTER(B),ctypes.c_uint32,ctypes.c_int32,ctypes.POINTER(B),ctypes.POINTER(H)];probe.restype=ctypes.c_int32
chars=struct.unpack('<7H',image.read(0x7ef6b4,14));ptrs=struct.unpack('<12I',image.read(0x7ef5ec,48))[::2];stages=[image.read(ptr+2,1)[0] for ptr in ptrs];catalog=bytearray()
for base in [0x7ef5ec,0x7ef624]:
 for row in range(6):
  ptr,count=struct.unpack('<II',image.read(base+row*8,8));catalog.append(count&255);catalog.extend(image.read(ptr,(count&255)*2))
rng=random.Random(a.seed);cases=[]
for options in range(256):
 for language in [-2147483648,-1,0,1,4,5,127,128,255,256,2147483647]:
  for kind in [2,3]:
   data=bytearray(937);data[0]=options;cases.append((kind,language,data))
for char in range(6):
 for flag in [0,1,2,255]:
  for completed in [0,1,2,255]:
   for override in [0,2,255]:
    data=bytearray(937);data[18+[65,66,67,68,70,69][char]]=flag;data[18+[72,73,74,75,77,76][char]]=flag;data[532+chars[char]*43+stages[char]]=completed;data[530]=override
    for kind in [0,1]:cases.append((kind,0,data))
for _ in range(a.cases):
 data=bytearray(rng.randbytes(937));data[530]=rng.choice([0,0,0,1,2,255]);data[876]=rng.randrange(7);cases.append((rng.randrange(8),rng.randrange(-2147483648,2147483648),data))
for count in [0,1,19,20,21,59,60]:
 for emblems in [0,19,20,21]:
  for override in [0,2,255]:
   data=bytearray(937);data[530]=override
   for bit in range(emblems):data[1+bit//8]|=1<<(bit%8)
   for mission in range(count):data[877+mission]=128
   for kind in [4,7]:cases.append((kind,0,data))
for current in range(7):
 for row in range(8):
  data=bytearray(937);data[876]=current
  for stage in range(43):data[532+row*43+stage]=255
  cases.append((5,0,data));cases.append((6,0,data))
bad=[];fp_bad=0
for index,(kind,language,data) in enumerate(cases):
 m.context_restore(context);m.mem_write(0x3b2b5f9,bytes(data[:1]));m.mem_write(0x3b2b3a8+576,bytes(data[1:18]));m.mem_write(0x3b18808,bytes(data[18:530]));m.mem_write(0x3b2a2f0,bytes(data[530:531]));m.mem_write(0x3b2a2fa,bytes(data[531:532]));m.mem_write(0x3b18250,bytes(data[532:876]));m.mem_write(0x3b2a2fd,bytes(data[876:877]));m.mem_write(0x3b2b368,bytes(data[877:937]));m.mem_write(0x3b0f0e8,struct.pack('<i',language));cw=rng.choice([127,895,3967]);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_EAX,0x8badf00d);m.reg_write(UC_X86_REG_ECX,0xdeadbeef);m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start([0x506410,0x506780,0x506840,0x5068a0,0x506460,0x5061f0,0x506210,0x4b51a0][kind],STOP,count=20000);assert m.reg_read(UC_X86_REG_ESP)==STACK+4
 expected=m.reg_read(UC_X86_REG_EAX)
 if kind in [2,5,6,7]:expected&=255
 if kind==3:expected=ctypes.c_int8(expected&255).value
 actual=probe((B*937).from_buffer_copy(data),kind,language,(B*len(catalog)).from_buffer_copy(catalog),(H*7)(*chars));fp_bad+=m.reg_read(UC_X86_REG_FPCW)!=cw or m.reg_read(UC_X86_REG_FPSW)!=0
 if expected!=actual:bad.append({'case':index,'kind':kind,'options':data[0],'language':language,'expected':expected,'actual':actual})
report={'cases':len(cases),'random_cases':a.cases,'seed':a.seed,'mismatch_count':len(bad),'fp_flag_partials':fp_bad,'passed':not bad and not fp_bad,'values_match':not bad,'mismatches':bad[:10],'source_sha256':image.sha256,'native_library_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'limits':__doc__};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','mismatch_count','fp_flag_partials']}));raise SystemExit(bool(bad) or bool(fp_bad))
