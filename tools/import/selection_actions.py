"""One title/selection graph from pinned CHRMODELS assets and source-constructed EXE motions."""
import hashlib,json,os,struct,sys,tempfile
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(Path(__file__).resolve().parent),str(ROOT/'tools/lib'),str(ROOT/'tools/import')]
from character_actions import Importer,export,TITLE_ACTIONS,SOURCE_SHA
from pe_image import PEImage
from extract import SOURCE_SHA256
from file_select_display import atomic_json
SELECTION=(
 (0,'SONIC',(5,100),((0,0x7d23e0,0x38f6e80,0x3c5e884,0x10c9018),)),
 (2,'MILES',(17,39),((0,0x7d2400,0x3aad124,0x3c5e89c,0x10c97c8),)),
 (3,'KNUCKLES',(81,40),((0,0x7d2420,0x3a7cf1c,0x3c5e874,0x10ca008),)),
 (5,'AMY',(70,29),((0,0x7d2440,0x38f6e70,0x3c5e87c,0x10ca898),)),
 (7,'BIG',(16,29,30),((0,0x7d2460,0x38f6e98,0x3c5e88c,0x10cb058),)),
 (6,'E102',(2,),((0,0x7d24a0,0x3a8af6c,0x3c5e894,0x10d43f8),(1,0x7d2480,0x3a8af6c,0x3c5e86c,0x10cb898))))

SEQUENCE_LAYOUT={0:(0x7d24c0,0x38f6e7c,0x3c5ff94),2:(0x7d24f0,0x3a7cf20,0x3c5ff84),3:(0x7d2520,0x3a7cf28,0x3c5ffa4),5:(0x7d2560,0x3aad0e0,0x3c5ffd8),7:(0x7d2590,0x3aad104,0x3c5ffc4),6:(0x7d25d0,0x3ab916c,0x3c5ffb4)}
def source_sequence_bytes(table_slot,destination,indices,custom):
 result=bytearray(b'\xa1'+struct.pack('<I',table_slot))
 for item,index in enumerate(indices):
  register=(1,2,0)[item];offset=index*4
  result+=bytes([139,(64 if offset<128 else 128)+register*8])+struct.pack('<B' if offset<128 else '<I',offset)
  address=destination+4*item
  result+=(b'\xa3'+struct.pack('<I',address)) if register==0 else (b'\x89'+bytes([5+8*register])+struct.pack('<I',address))
 for item,value in enumerate([row[3] for row in custom]+[0],len(indices)):
  result+=b'\xc7\x05'+struct.pack('<II',destination+4*item,value)
 return bytes(result)+b'\xc3'

class Sources:
 def __init__(self,dll,exe):
  if dll.sha256!=SOURCE_SHA or exe.sha256!=SOURCE_SHA256:raise ValueError('selection resources require both pinned source images')
  self.dll,self.exe=dll,exe;self.derived={};self.constructions=[];self.sequence_constructions=[]
  for character,name,indices,custom in SELECTION:
   constructor,table_slot,destination=SEQUENCE_LAYOUT[character]
   code=source_sequence_bytes(table_slot,destination,indices,custom)
   if exe.read(constructor,len(code))!=code:raise ValueError('selection sequence initializer byte proof changed')
   self.sequence_constructions.append({'character':character,'constructor_address':hex(constructor),'constructor_bytes':code.hex(),'action_export':'___'+name+'_ACTIONS','source_indices':list(indices),'custom_ordinals':[row[0] for row in custom],'terminated':True})
   model=dll.unpack('<I',export(dll,'___'+name+'_OBJECTS'))[0]
   for ordinal,constructor,slot,destination,motion in custom:
    instructions=(b'\xa1'+struct.pack('<I',slot)+b'\x8b\x08\x89\x0d'+struct.pack('<I',destination)+b'\xc7\x05'+struct.pack('<II',destination+4,motion)+b'\xc3')
    if exe.read(constructor,len(instructions))!=instructions:raise ValueError('selection action constructor byte proof changed')
    logical=struct.pack('<II',model,motion);self.derived[destination]=logical
    self.constructions.append({'character':character,'ordinal':ordinal,'model_export':'___'+name+'_OBJECTS','model_index':0,'constructor_address':hex(constructor),'constructor_bytes':instructions.hex(),'source_slot_provenance':hex(slot),'destination_provenance':hex(destination),'model_provenance':hex(model),'motion_provenance':hex(motion),'initialized_action_words':logical.hex(),'origin':'statically reconstructed source initializer, not original zero-filled BSS bytes'})
 def read(self,address,size):
  for start,data in self.derived.items():
   if start<=address and address+size<=start+len(data):return data[address-start:address-start+size]
  return (self.dll if address>=self.dll.base else self.exe).read(address,size)
 def unpack(self,fmt,address):return struct.unpack(fmt,self.read(address,struct.calcsize(fmt)))
 def provenance(self,address,size):
  if address in self.derived:return {'source_component':'constructed_action','direct_image_bytes':False,'constructor_proof':next(x for x in self.constructions if int(x['destination_provenance'],16)==address)}
  return {'source_component':'character_dll' if address>=self.dll.base else 'game_assets','direct_image_bytes':True}

def build(dll,exe):
 sources=Sources(dll,exe);imp=Importer(sources);actions=[];seen=set()
 def source_action(character,name,index):
  key=(0,character,index)
  if key in seen:return
  table=export(dll,'___'+name+'_ACTIONS');address=dll.unpack('<I',table+index*4)[0];actions.append((*key,imp.action(address)));seen.add(key)
 for character,name,indices in TITLE_ACTIONS:
  for index in indices:source_action(character,name,index)
 sequences=[]
 for character,name,indices,custom in SELECTION:
  sequence=[]
  for index in indices:source_action(character,name,index);sequence.append([0,character,index])
  for ordinal,constructor,slot,destination,motion in custom:
   key=(1,character,ordinal);actions.append((*key,imp.action(destination)));seen.add(key);sequence.append(list(key))
  sequences.append({'character':character,'actions':sequence})
 imp.question_object=imp.object(0x10d7774)
 imp.verify_round_trip()
 return imp,actions,sequences,sources

def import_selection_actions(model_dll,game_exe,output):
 model_dll,game_exe,output=Path(model_dll).resolve(),Path(game_exe).resolve(),Path(output).resolve()
 if output==model_dll or output==game_exe or game_exe.parent in output.parents or model_dll.parent in output.parents:raise ValueError('selection output must be outside owned source directories')
 dll,exe=PEImage(model_dll),PEImage(game_exe);imp,actions,sequences,sources=build(dll,exe)
 output.parent.mkdir(parents=True,exist_ok=True);fd,name=tempfile.mkstemp(prefix='.selection-actions-',dir=output.parent);os.close(fd);temporary=Path(name)
 try:
  digest=imp.write(temporary,actions,selection=True);os.replace(temporary,output)
 finally:
  if temporary.exists():temporary.unlink()
 descriptor_words=[(0x49a9b0,0x91cb58,0x3c5ff94,0x7f0738),(0x461700,0x91a9c8,0x3c5ff84,0x7f0744),(0x47a770,0x91bd20,0x3c5ffa4,0x7f0750),(0x48abf0,0x91c800,0x3c5ffd8,0x7f075c),(0x490a00,0x91c910,0x3c5ffc4,0x7f0768),(0x483430,0x91c560,0x3c5ffb4,0x7f0778),(0x5125a0,0,0,0)]
 descriptor_raw=exe.read(0x7f0788,112)
 if b''.join(struct.pack('<4I',*row) for row in descriptor_words)!=descriptor_raw:raise ValueError('selection actor declaration byte proof changed')
 positions=exe.read(0x7f07f8,84);characters=exe.read(0x7ef6b4,14);sonic_aim=exe.read(0x10db828,4)
 if struct.pack('<21I',*struct.unpack('<21I',positions))!=positions or struct.pack('<7H',*struct.unpack('<7H',characters))!=characters:raise ValueError('preview declaration inverse mismatch')
 frames=[]
 for address,count in [(0x7f0738,3),(0x7f0744,3),(0x7f0750,3),(0x7f075c,3),(0x7f0768,4),(0x7f0778,3)]:
  raw=exe.read(address,4*count);values=list(struct.unpack('<'+'i'*count,raw));frames.append({'values':values,'source_address':hex(address),'source_bytes':raw.hex()})
 metadata={'version':1,'pack_schema':'SDXACT03','characters':list(struct.unpack('<7H',characters)),'positions_words':[list(struct.unpack_from('<3I',positions,12*i)) for i in range(7)],'sonic_aim_y':struct.unpack('<i',sonic_aim)[0],'sequences':[{**row,'link_frames':frames[i]['values']} for i,row in enumerate(sequences)],'actor_kinds':['sonic','miles','knuckles','amy','big','gamma','question_mark'],'question_object':imp.question_object,'question_menu_texture':2}
 proof={'schema':'SDXACT03','source_dll_sha256':dll.sha256,'source_game_sha256':exe.sha256,'pack_sha256':digest,'actions':actions,'records':imp.evidence,'constructed_actions':sources.constructions,'sequence_constructions':sources.sequence_constructions,'preview_declaration_records':[{'address':'0x7f0788','bytes':descriptor_raw.hex()},{'address':'0x7f07f8','bytes':positions.hex()},{'address':'0x7ef6b4','bytes':characters.hex()},{'address':'0x10db828','bytes':sonic_aim.hex()},*frames],'round_trip_exact':True,'limits':['All original model/motion records invert byte-for-byte to their source image. Seven derived NJS_ACTION records invert to the statically reconstructed initializer result, not to original zero-filled BSS storage.','This resource graph contains no native player/camera/factory behavior or source executable code. Actual lower callers remain separate required native implementations.']}
 atomic_json(output.with_suffix('.provenance.json'),proof);atomic_json(output.parent/'selection-preview.json',metadata)
 return {'records':len(imp.records),'bytes':output.stat().st_size,'actions':len(actions),'pack_sha256':digest,'metadata':metadata}
if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('model_dll');p.add_argument('game_exe');p.add_argument('output');a=p.parse_args();result=import_selection_actions(a.model_dll,a.game_exe,a.output);print(json.dumps({k:v for k,v in result.items() if k!='metadata'}))
