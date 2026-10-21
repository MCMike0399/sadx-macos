"""Run complete512ca0 and its actual unhooked flag callees; no drawing is part of this ABI."""
import argparse,ctypes,hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles')]
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA
DESCRIPTORS=0x7f0bdc;ROWS=DATA+0x1000
class Source:
 def __init__(self,image):
  self.m=Original(image,127).machine;self.descriptors=image.read(DESCRIPTORS,56)
  for s in image.sections:
   if s['name'] in ('.text','.rdata'):continue
   size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095;self.m.mem_map(image.base+s['rva'],size)
   if s['raw_size']:self.m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
  self.initial=self.m.context_save()
  self.flag_helper_proof=self.verify_flag_helpers()
 def verify_flag_helpers(self):
  samples=[]
  for byte in range(256):
   for entry,base in ((0x42fb10,0x3b18608),(0x412d20,0x3b18808)):
    m=self.m;m.context_restore(self.initial);m.mem_write(base+511,bytes([byte]));sp=STACK-128;m.mem_write(sp,struct.pack('<2I',STOP,511));m.reg_write(UC_X86_REG_ESP,sp);m.emu_start(entry,STOP,timeout=1000000)
    expected=(byte if byte<128 else byte-256)&0xffffffff if entry==0x42fb10 else int(byte!=0)
    actual=m.reg_read(UC_X86_REG_EAX);assert actual==expected
    assert m.reg_read(UC_X86_REG_EIP)==STOP and m.reg_read(UC_X86_REG_ESP)==sp+4
    if byte in (0,1,127,128,255):samples.append({'entry':hex(entry),'byte':byte,'eax':actual})
  return {'cutscene_signed_byte_cases':256,'story_nonzero_cases':256,'samples':samples}
 def run(self,v):
  m=self.m;m.context_restore(self.initial);m.mem_write(DESCRIPTORS,self.descriptors)
  # Mode0 keeps all actual source descriptors/rows; mode1 independently changes
  # authored rows/count, retaining the complete original control flow and callees.
  if v[2]:
   m.mem_write(DESCRIPTORS+v[0]*8,struct.pack('<II',ROWS,v[1]));m.mem_write(ROWS,struct.pack('<192I',*v[8:200]))
  m.mem_write(0x3b18808,bytes(v[200:712]));m.mem_write(0x3b18608,bytes(v[712:1224]));m.mem_write(0x3b18408,bytes([165])*512)
  extras=[0x11111111^v[0],v[1],v[2],v[200]*0x01010101,v[712]*0x01010101,0xfedcba98]
  sp=STACK-128;m.mem_write(sp,struct.pack('<8I',STOP,v[0],*extras));m.reg_write(UC_X86_REG_ESP,sp)
  m.emu_start(0x512ca0,STOP,timeout=1000000)
  assert m.reg_read(UC_X86_REG_EIP)==STOP and m.reg_read(UC_X86_REG_ESP)==sp+4
  assert bytes(m.mem_read(sp+8,24))==struct.pack('<6I',*extras)
  assert bytes(m.mem_read(0x3b18808,512))==bytes(v[200:712])
  assert bytes(m.mem_read(0x3b18608,512))==bytes(v[712:1224])
  return [m.reg_read(UC_X86_REG_EAX),0,0,0]

def u32(x):return x&0xffffffff
def source_catalog(image):
 rows=[]
 for i in range(7):
  pointer,count=image.unpack('<Ii',DESCRIPTORS+i*8);rows.append([list(image.unpack('<iii',pointer+j*12)) for j in range(count)])
 return rows

def vectors(image,seed,count):
 rng=random.Random(seed);catalog=source_catalog(image);out=[]
 def base(character,rows=None):
  actual=rows is None;rows=catalog[character] if actual else rows;v=[0]*1224;v[:3]=[character,len(rows),0 if actual else 1]
  for i,row in enumerate(rows):v[8+i*3:11+i*3]=list(map(u32,row))
  return v
 for character,rows in enumerate(catalog):
  out.append(base(character))
  for value,kind,index in rows:
   for byte in (1,2,127,128,255):
    v=base(character);v[(200 if kind==0 else 712)+index]=byte;out.append(v)
  for first,second in ((255,0),(0,255),(1,255),(128,2),(255,255)):
   v=base(character);v[200:712]=[first]*512;v[712:1224]=[second]*512;out.append(v)
 for character in range(7):
  for rows in [[],[[90,0,0],[5,1,0]],[[7,0,0],[0,1,0]],[[-12,1,0]],[[6,2,-1]],[[5,0,0],[99,2,-1],[3,1,0]]]:
   for first,second in ((0,0),(255,0),(0,128),(1,255)):
    v=base(character,rows);v[200]=first;v[712]=second;out.append(v)
  for negative in (-1,-2147483648):
   v=base(character,[]);v[1]=u32(negative);out.append(v)
 for _ in range(count):
  character=rng.randrange(7)
  if rng.randrange(2):
   rows=[[rng.randrange(-1000,1001),rng.choice((0,1,2,3,-1)),rng.randrange(512)] for _ in range(rng.randrange(65))];v=base(character,rows)
  else:v=base(character)
  for i in range(200,1224):v[i]=rng.choice((0,0,0,1,2,127,128,255))
  out.append(v)
 return out

def main():
 p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=1800);p.add_argument('--seed',type=int,default=512396);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);probe=ctypes.CDLL(a.library).sadx_native_character_completion_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;rows=vectors(image,a.seed,a.cases);bad=[]
 for n,v in enumerate(rows):
  expected=source.run(v);out=(ctypes.c_uint32*4)();probe((ctypes.c_uint32*1224)(*v),out)
  if list(out)!=expected:
   bad.append({'case':n,'character':v[0],'count':v[1],'synthetic':v[2],'expected':expected,'actual':list(out),'rows':v[8:200],'story':v[200:712],'cutscene':v[712:1224]})
   if len(bad)==8:break
 stage=Path(__file__).resolve().parents[2];paths=['src/game/character_completion.hpp','src/game/character_completion.cpp','tests/probes/native_character_completion_probe.cpp']
 report={'source_sha256':image.sha256,'flag_helper_proof':source.flag_helper_proof,'source_ranges':{f'{hex(x)}-{hex(y)}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in [(0x512ca0,0x512d2d),(0x42fb10,0x42fb1c),(0x412d20,0x412d32),(0x7f0850,0x7f0bdc),(0x7f0bdc,0x7f0c14)]},'passed':not bad,'values_match':not bad,'cases':len(rows),'executed':n+1,'mismatches':len(bad),'first_mismatches':bad,'native_library_sha256':hashlib.sha256(Path(a.library).read_bytes()).hexdigest(),'native_sources':{p:hashlib.sha256((stage/p).read_bytes()).hexdigest() for p in paths},'limits':['Actual512ca0 entry and both real unhooked flag callees execute; no child is replaced with a receipt or false return. Source descriptors/rows stay actual in canonical cases and change independently in authored-data stress cases.','Seven source selection indices map directly to catalogs; no SDK-character remapping, variant or unlock assumption is introduced. Negative signed cutscene returns still satisfy the source nonzero test.','Signed nonpositive source counts map to an empty native authored row sequence in the probe adapter; the source returns0 before dereferencing rows in each case.','Unknown source row kinds skip without reading their flag; known flag indices outside the imported512-byte owners and character indices outside0..6 fail explicitly in native boundary units, not fabricated data.','Six independently poisoned future-caller stack words remain unchanged; no drawing/507410 argument context exists in this helper ABI. Actual caller/runtime binding and pixels remain separate.']}
 Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
