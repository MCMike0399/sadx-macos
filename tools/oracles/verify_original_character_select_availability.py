"""Original count/clear/trial eligibility bodies with unhooked callees and live story mutations."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path.cwd()));sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
from pe_image import PEImage
from verify_original_math import Original,STACK,STOP
from extract import SOURCE_SHA256
from unicorn.x86_const import *
p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('probe');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=512650);p.add_argument('--cases',type=int,default=1000);a=p.parse_args()
image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;m=Original(image,127).machine
for sec in image.sections:
 if sec['name']in('.text','.rdata'):continue
 size=(max(sec['virtual_size'],sec['raw_size'])+4095)&~4095;m.mem_map(image.base+sec['rva'],size);m.mem_write(image.base+sec['rva'],image.slice(sec['file_offset'],sec['raw_size']))
ctx=m.context_save();B=ctypes.c_uint8;H=ctypes.c_uint16;lib=ctypes.CDLL(a.probe);probe=lib.sadx_native_character_select_availability;probe.argtypes=[ctypes.POINTER(B),ctypes.c_uint32,ctypes.c_int32,ctypes.POINTER(B),ctypes.POINTER(H),ctypes.POINTER(B)];probe.restype=ctypes.c_int32
chars=image.unpack('<7H',0x7ef6b4);catalog=bytearray()
for base in [0x7ef5ec,0x7ef624]:
 for row in range(6):
  pointer,count=image.unpack('<II',base+row*8);catalog.append(count&255);catalog.extend(image.read(pointer,(count&255)*2))
rng=random.Random(a.seed);cases=[]
for character in range(7):
 for variant in (0,1,2,3,255):
  for override in (0,1,255):
   for subgame in (0,1):
    for set_flags in (0,1,255):
     v=bytearray(937);v[18:530]=bytes([set_flags])*512;v[530]=override;v[531]=variant;v[532:876]=bytes([set_flags])*344;v[876]=subgame
     for kind in range(3):cases.append((kind,character,v))
for emblems in (0,129,130,131,136):
 for variant in (0,1,2):
  v=bytearray(937);v[531]=variant
  for bit in range(emblems):v[1+bit//8]|=1<<(bit%8)
  cases.append((0,6,v));cases.append((2,6,v))
for _ in range(a.cases):
 v=bytearray(rng.randbytes(937));v[530]=rng.choice([0,0,0,1,255]);v[531]=rng.choice([0,1,2,3,255]);cases.append((rng.randrange(3),rng.randrange(7),v))
bad=[];fp_bad=0
for index,(kind,character,v)in enumerate(cases):
 m.context_restore(ctx);m.mem_write(0x3b2b3a8+576,bytes(v[1:18]));m.mem_write(0x3b18808,bytes(v[18:530]));m.mem_write(0x3b2a2f0,bytes(v[530:531]));m.mem_write(0x3b2a2fa,bytes(v[531:532]));m.mem_write(0x3b18250,bytes(v[532:876]));m.mem_write(0x3b2a2f1,bytes(v[876:877]));m.mem_write(STACK,struct.pack('<II',STOP,character));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start([0x512c50,0x5063e0,0x506700][kind],STOP,timeout=0)
 want=m.reg_read(UC_X86_REG_EAX);flags=bytes(m.mem_read(0x3b18808,512));out=(B*512)();got=probe((B*937).from_buffer_copy(v),kind,character,(B*len(catalog)).from_buffer_copy(catalog),(H*7)(*chars),out);fp_bad+=m.reg_read(UC_X86_REG_FPSW)!=0
 if want!=got or flags!=bytes(out):bad.append({'case':index,'kind':kind,'character':character,'expected':want,'actual':got})
r={'cases':len(cases),'seed':a.seed,'values_match':not bad,'passed':not bad and not fp_bad,'flag_mismatches':fp_bad,'mismatches':bad[:8],'source_sha256':image.sha256,'source_ranges':{f'0x{x:x}-0x{y:x}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in[(0x512c50,0x512c96),(0x5063e0,0x50640a),(0x506700,0x506771)]},'limits':__doc__};Path(a.output).write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r));raise SystemExit(bool(bad or fp_bad))
