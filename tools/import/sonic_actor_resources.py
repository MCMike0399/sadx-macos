"""Static source declaration import for the real Sonic selected-menu actor.
Only linear MOV/XOR data declarations are folded; no source code is executed.
The existing SDXACT03 graph/namespace remains one pack; legacy fixtures unchanged.
"""
import hashlib,json,os,struct,sys,tempfile
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(Path(__file__).resolve().parent),str(ROOT/'tools/lib'),str(ROOT/'tools/import')]
import capstone
from capstone.x86_const import X86_OP_REG,X86_OP_IMM,X86_OP_MEM
from character_actions import export,SOURCE_SHA
from selection_actions import build
from pe_image import PEImage
from extract import SOURCE_SHA256
from file_select_display import atomic_json
CONSTRUCTORS=(('sonic',0x7d0b50,0x7d14cf,0x3c55e28,37,'210dc49bca4bfb1cb7454e45a5987cd991a463b391470eff9a47f2424d09d4ce'),('metal',0x7d18f0,0x7d1aa6,0x3c55d68,8,'5eaaad3ef86086fc38d78059bcf66c4eacfaece8914a6e2fedf5acf9663ba16b'))
def digest(b):return hashlib.sha256(b).hexdigest()
def bss(image,address,size):
 for s in image.sections:
  start=image.base+s['rva']
  if start+s['raw_size']<=address and address+size<=start+s['virtual_size']:return {'section':s['name'],'raw_end':hex(start+s['raw_size']),'virtual_end':hex(start+s['virtual_size'])}
 raise ValueError('Sonic join declarations must originate in source zero-fill storage')
REG={}
for root,parts in [('eax',['eax','ax','al','ah']),('ebx',['ebx','bx','bl','bh']),('ecx',['ecx','cx','cl','ch']),('edx',['edx','dx','dl','dh'])]:
 for name,bits,shift in zip(parts,[32,16,8,8],[0,0,0,8]):REG[name]=(root,bits,shift)
for root in ['esi','edi','ebp','esp']:REG[root]=(root,32,0)
def declarations(dll,exe,spec):
 name,start,end,destination,count,expected=spec;code=exe.read(start,end-start)
 if digest(code)!=expected:raise ValueError('Sonic weld initializer bytes changed')
 initial=bss(exe,destination,count*24);buffer=bytearray(count*24);written=set();registers={k:(0,0) for k in ['eax','ebx','ecx','edx','esi','edi','ebp','esp']};stack=[];reads=[];writes=[];objects=export(dll,'___SONIC_OBJECTS');decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);decoder.detail=True;instructions=list(decoder.disasm(code,start))
 if not instructions or instructions[-1].mnemonic!='ret' or sum(i.size for i in instructions)!=len(code):raise ValueError('incomplete initializer declaration body')
 def reg_read(register):
  root,width,shift=REG[decoder.reg_name(register)];value,known=registers[root];mask=((1<<width)-1)<<shift
  if known&mask!=mask:raise ValueError('initializer reads an indeterminate register')
  return (value>>shift)&((1<<width)-1)
 def reg_write(register,value):
  root,width,shift=REG[decoder.reg_name(register)];mask=((1<<width)-1)<<shift
  # Never invent the upper bits of byte-register temporaries.
  old,known=registers[root];registers[root]=((old&~mask)|((value<<shift)&mask),known|mask)
 def operand(op):
  if op.type==X86_OP_REG:return reg_read(op.reg)
  if op.type==X86_OP_IMM:return op.imm&((1<<(8*op.size))-1)
  if op.type!=X86_OP_MEM or op.mem.index:raise ValueError('unsupported initializer declaration source')
  address=op.mem.disp+(reg_read(op.mem.base) if op.mem.base else 0)
  if address==0x38f6e80 and op.size==4:value=objects
  elif objects<=address and address+op.size<=objects+79*4:value=int.from_bytes(dll.read(address,op.size),'little')
  else:raise ValueError('initializer reads beyond declared model export')
  reads.append({'address':hex(address),'size':op.size,'value':hex(value)});return value
 for i in instructions:
  ops=i.operands
  if i.mnemonic=='push':
   if len(ops)!=1 or ops[0].type!=X86_OP_REG:raise ValueError('unsupported declaration register preservation')
   root,_,_=REG[decoder.reg_name(ops[0].reg)];stack.append(registers[root]);continue
  if i.mnemonic=='pop':
   if len(ops)!=1 or ops[0].type!=X86_OP_REG or not stack:raise ValueError('unbalanced declaration register preservation')
   root,_,_=REG[decoder.reg_name(ops[0].reg)];registers[root]=stack.pop();continue
  if i.mnemonic=='ret':
   if stack:raise ValueError('unbalanced declaration body')
   continue
  if i.mnemonic=='xor' and len(ops)==2 and ops[0].type==X86_OP_REG and ops[1].type==X86_OP_REG and ops[0].reg==ops[1].reg:reg_write(ops[0].reg,0);continue
  if i.mnemonic!='mov' or len(ops)!=2:raise ValueError('initializer contains behavior beyond static declarations')
  value=operand(ops[1]);dst=ops[0]
  if dst.type==X86_OP_REG:reg_write(dst.reg,value);continue
  if dst.type!=X86_OP_MEM or dst.mem.base or dst.mem.index:raise ValueError('unsupported declaration destination')
  offset=dst.mem.disp-destination
  if offset<0 or offset+dst.size>len(buffer):raise ValueError('initializer writes outside its declaration owner')
  raw=(value&((1<<(8*dst.size))-1)).to_bytes(dst.size,'little');buffer[offset:offset+dst.size]=raw;written.update(range(offset,offset+dst.size));writes.append({'instruction':hex(i.address),'destination':hex(dst.mem.disp),'bytes':raw.hex()})
 return bytes(buffer),{'name':name,'address':hex(start),'size':len(code),'sha256':expected,'code_bytes':code.hex(),'origin':'statically folded linear source data declarations; not original BSS payload bytes','initial_zero_fill':initial,'written_bytes':len(written),'reads':reads,'writes':writes}
def build_sonic_actor_resources(dll,exe):
 imp,actions,sequences,sources=build(dll,exe);table=export(dll,'___SONIC_OBJECTS');model_addresses=list(dll.unpack('<79I',table));models=[{'index':i,'object':imp.object(model_addresses[i])} for i in [5,6,22]];action_table=export(dll,'___SONIC_ACTIONS');initial_action=imp.action(dll.unpack('<I',action_table+4)[0]);pairs=[];pair_ids={};proofs=[];lists={}
 for spec in CONSTRUCTORS:
  raw,proof=declarations(dll,exe,spec);proofs.append(proof);entries=[]
  for i in range(spec[4]):
   obj,src,dst,vertices,mode,srcnode,dstnode,original,indices=struct.unpack_from('<IIIBbBBII',raw,24*i)
   if original:raise ValueError('source constructor requires an unsupported preallocated join buffer')
   pair_id=0
   if indices:
    key=(indices,vertices)
    if key not in pair_ids:
     pair_ids[key]=len(pairs)+1;pairs.append({'id':len(pairs)+1,'values':[list(x) for x in struct.iter_unpack('<HH',exe.read(indices,vertices*4))]})
    pair_id=pair_ids[key]
   elif vertices and mode in [1,2,3]:raise ValueError('missing source vertex-pair declaration')
   entry={'object':imp.object(obj),'source_object':imp.object(src),'destination_object':imp.object(dst),'vertex_count':vertices,'input_mode':mode,'source_node':srcnode,'destination_node':dstnode,'original_vertices':0,'vertex_pairs':pair_id};entries.append(entry)
   restored=struct.pack('<IIIBbBBII',*[imp.source_addresses[x] if x else 0 for x in [entry['object'],entry['source_object'],entry['destination_object']]],vertices,mode,srcnode,dstnode,0,indices)
   if restored!=raw[24*i:24*(i+1)]:raise ValueError('Sonic join record failed inverse declaration proof')
  if entries[-1]['object']!=0 or any(e['object']==0 for e in entries[:-1]):raise ValueError('Sonic join terminator/count differs from declared source owner')
  lists[spec[0]]=entries
 direct_code=bytes.fromhex('a1806e8f038b88c8000000890d585dc5038b90d000000089155c5dc5038b80cc000000a3605dc503c3')
 if exe.read(0x7d0af0,len(direct_code))!=direct_code:raise ValueError('Sonic direct-ahead declaration bytes changed')
 direct_ahead=[imp.object(model_addresses[i]) for i in [50,52,51]]
 parameters=exe.read(0x9154e8,8*132)
 collision=exe.read(0x91cbf0,2*48);metadata={'version':1,'pack_schema':'SDXACT03','objects':models,'initial_animation_action':initial_action,'initial_animation_export_index':1,'direct_ahead_objects':direct_ahead,'player_parameter_words':[list(struct.unpack_from('<33I',parameters,i*132)) for i in range(8)],'joins':lists,'vertex_pairs':pairs,'collision_words':list(struct.unpack('<24I',collision))}
 return imp,actions,metadata,proofs,pair_ids,table,action_table,direct_code,parameters,collision

def import_sonic_actor_resources(model_dll,game_exe,output):
 model_dll=Path(model_dll).resolve();game_exe=Path(game_exe).resolve();output=Path(output).resolve()
 if output.is_relative_to(model_dll.parent) or output.is_relative_to(game_exe.parent):raise ValueError('Sonic actor output must be outside owned source directories')
 dll=PEImage(model_dll);exe=PEImage(game_exe)
 if dll.sha256!=SOURCE_SHA or exe.sha256!=SOURCE_SHA256:raise ValueError('unsupported Sonic actor source hash')
 imp,actions,metadata,proofs,pair_ids,table,action_table,direct_code,parameters,collision=build_sonic_actor_resources(dll,exe)
 output.parent.mkdir(parents=True,exist_ok=True);fd,tmp=tempfile.mkstemp(prefix=output.name+'.',dir=output.parent);os.close(fd)
 try:pack_hash=imp.write(Path(tmp),actions,selection=True);os.replace(tmp,output)
 finally:
  if os.path.exists(tmp):os.unlink(tmp)
 proof={'version':1,'source_dll_sha256':dll.sha256,'source_game_sha256':exe.sha256,'pack_sha256':pack_hash,'records':imp.evidence,'round_trip_exact':all(e['round_trip_exact'] for e in imp.evidence),'declaration_constructions':proofs,'source_declarations':[{'name':'SonicDirectAheadModels','source_address':'0x7d0af0','bytes':direct_code.hex()},{'name':'PlayerParameterTable','source_address':'0x9154e8','bytes':parameters.hex()},{'name':'SONIC_OBJECTS','source_address':hex(table),'bytes':dll.read(table,79*4).hex()},{'name':'initial_SONIC_ACTIONS1','source_address':hex(action_table+4),'bytes':dll.read(action_table+4,4).hex()},{'name':'Sonic_Collision','source_address':'0x91cbf0','bytes':collision.hex()}],'vertex_pair_sources':[{'id':pair_ids[k],'source_address':hex(k[0]),'count':k[1],'bytes':exe.read(k[0],k[1]*4).hex()} for k in pair_ids],'limits':['Extends existing one SDXACT03 graph with actual source-requested Sonic model slots5/6/22, actual initialStand action1 and vertex-join/collision declarations; no live actor or renderer claim.','Only model slots5/6/22 requested by this constructor are declared; other native object-index requests must report a missing asset. Full149-entry Sonic motion payload set is not imported here:10 source motions use MDATA3(type7), distinct from this pack decoder MDATA2; do not fabricate missing payloads or substitute clearpose. Existing source147-entry animation-table initializer requires its actual resource binding.','Source initializer byte folding is limited linear declaration extraction, no source x86 execution; all record references invert into reconstructed declaration bytes and constructor hashes.']}
 atomic_json(output.with_suffix('.provenance.json'),proof);atomic_json(output.with_name('sonic-actor.json'),metadata);return {'metadata':metadata,'proof':proof,'records':len(imp.records),'sha256':pack_hash,'bytes':output.stat().st_size}
if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('model_dll',type=Path);p.add_argument('game_exe',type=Path);p.add_argument('output',type=Path);a=p.parse_args();r=import_sonic_actor_resources(a.model_dll,a.game_exe,a.output);print(json.dumps({k:r[k] for k in ['records','sha256','bytes']}))
