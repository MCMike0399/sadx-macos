"""Interpret only the actual initializer's MOV/XOR integer instructions.

LLVM disassembly is byte-located evidence, independently checked against the
pinned PE range and existing147-row recipe. No source function is executed.
"""
import argparse,hashlib,json,pathlib,re
import sys,pathlib as _pl;sys.path[:0]=[str(_pl.Path(__file__).resolve().parents[2]/'tools'/d) for d in('lib','oracles')]  # run from anywhere
from pe_image import PEImage
from extract import SOURCE_SHA256
BASE=0x3c56210
REGS={'eax':('eax',0,4),'ecx':('ecx',0,4),'edx':('edx',0,4),'ebx':('ebx',0,4),'esi':('esi',0,4),'edi':('edi',0,4),'ebp':('ebp',0,4),'al':('eax',0,1),'ah':('eax',1,1),'ax':('eax',0,2),'cl':('ecx',0,1),'ch':('ecx',1,1),'cx':('ecx',0,2),'dl':('edx',0,1),'dh':('edx',1,1),'dx':('edx',0,2),'bl':('ebx',0,1),'bh':('ebx',1,1),'bx':('ebx',0,2),'si':('esi',0,2),'di':('edi',0,2),'bp':('ebp',0,2)}
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('assembly',type=pathlib.Path);p.add_argument('--header',type=pathlib.Path,required=True);p.add_argument('--metadata',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
 if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source identity')
 registers={r:[None]*4 for r in ('eax','ecx','edx','ebx','esi','edi','ebp')};ops=[];loads=0;writes=0
 def get(register):
  r,start,size=REGS[register];return registers[r][start:start+size]
 def setreg(register,value):
  r,start,size=REGS[register];assert len(value)==size;registers[r][start:start+size]=value
 for line in a.assembly.read_text().splitlines():
  match=re.fullmatch(r'\s*([0-9a-f]+): ([0-9a-f ]+)\t([a-z]+)\s*(.*?)\s*(?:#.*)?',line)
  if not match:continue
  address=int(match[1],16);code=bytes.fromhex(match[2]);mnemonic=match[3];operands=match[4].strip()
  if image.read(address,len(code))!=code:raise RuntimeError('LLVM/source bytes mismatch')
  if mnemonic in ('pushl','popl','retl'):continue
  if mnemonic=='xorl':
   left,right=operands.split(', ')
   if left!=right:raise RuntimeError('Unsupported XOR')
   setreg(left[1:],[0]*4);continue
  if mnemonic not in ('movl','movw','movb'):raise RuntimeError('Unsupported source instruction')
  width={'movl':4,'movw':2,'movb':1}[mnemonic];left,right=operands.split(', ')
  if left=='0x38f6e7c':setreg(right[1:],[('base',n)for n in range(4)]);continue
  export=re.fullmatch(r'(0x[0-9a-f]+)?\(%eax\)',left)
  if export:
   if get('eax')!=[('base',n)for n in range(4)]:raise RuntimeError('Unresolved export base')
   offset=int(export[1],16)if export[1]else 0
   if offset%4 or width!=4:raise RuntimeError('Unaligned action read')
   ops.append(dict(address=address,read=True,slot=loads,index=offset//4,record=0,field=0,bits=0));setreg(right[1:],[('action',loads,n)for n in range(4)]);loads+=1;continue
  if left.startswith('$'):value=int(left[1:],0);values=[(value>>(n*8))&255 for n in range(width)]
  elif left.startswith('%'):values=get(left[1:])
  else:raise RuntimeError('Unresolved source operand:'+left)
  if right.startswith('%'):setreg(right[1:],values);continue
  target=int(right,16);row,field=divmod(target-BASE,16)
  if not 0<=row<147 or field not in (0,4,5,6,8,12):raise RuntimeError('Unexpected field address')
  if all(isinstance(v,int)for v in values):bits=sum(v<<(n*8)for n,v in enumerate(values));slot=65535
  elif width==4 and all(v==('action',values[0][1],n)for n,v in enumerate(values)):bits=0;slot=values[0][1]
  else:raise RuntimeError('Unknown partial register')
  ops.append(dict(address=address,read=False,slot=slot,index=0,record=row,field=field,bits=bits));writes+=1
 if writes!=882:raise RuntimeError('Incomplete source coverage')
 header=['#pragma once','#include <array>','#include <cstdint>','namespace sadx {','struct SonicAnimationOperation {std::uint32_t address;bool read;std::uint16_t slot,index,record;std::uint8_t field;std::uint32_t bits;};',f'inline constexpr std::array<SonicAnimationOperation,{len(ops)}> sonic_animation_operations{{{{']
 for op in ops:header.append('    {'+hex(op['address'])+','+('true'if op['read']else'false')+','+','.join(hex(op[k])for k in ('slot','index','record','field','bits'))+'},')
 header+=['}};','}'];a.header.write_text('\n'.join(header)+'\n');sha=lambda b:hashlib.sha256(b).hexdigest();a.metadata.write_text(json.dumps(dict(source_sha256=image.sha256,assembly_sha256=sha(a.assembly.read_bytes()),generated_header_sha256=sha(a.header.read_bytes()),operations=len(ops),export_reads=loads,field_writes=writes,operations_detail=ops),indent=2)+'\n');print(json.dumps(dict(operations=len(ops),export_reads=loads,field_writes=writes)))
if __name__=='__main__':main()
