"""Pin the actual7cec90 descriptor initializer as native data recipes.

Consumes complete retained Ghidra source statements, checks all147 records and
field addresses, and pins exact executable function bytes. This emits source
constants; verification executes the original integer-only producer separately.
"""
import argparse,hashlib,json,pathlib,re,sys
from pe_image import PEImage
from extract import SOURCE_SHA256
BASE=0x3c56210;COUNT=147
FIELDS={0:'action_index',4:'object_count',5:'mode',6:'next',8:'transition_bits',12:'step_bits'}
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('pseudocode',type=pathlib.Path);p.add_argument('--header',type=pathlib.Path,required=True);p.add_argument('--metadata',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
 if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source mismatch')
 rows=[{}for _ in range(COUNT)];statements=[]
 for line in a.pseudocode.read_text().splitlines():
  if '='not in line:continue
  match=re.fullmatch(r'\s*_?DAT_([0-9a-f]+) = (DAT_038f6e7c\[(0x[0-9a-f]+|[0-9]+)\]|\*DAT_038f6e7c|0x[0-9a-f]+|[0-9]+);',line)
  if not match:raise RuntimeError('Unrecognized source assignment:'+line)
  address=int(match[1],16);offset=address-BASE;record,field=divmod(offset,16)
  if record<0 or record>=COUNT or field not in FIELDS or field in rows[record]:raise RuntimeError('Unexpected/duplicate source field')
  expression=match[2];value=int(match[3],0)if match[3]else 0 if expression=='*DAT_038f6e7c'else int(expression,0)
  if field==0 and not expression.startswith(('DAT_038f6e7c[','*DAT_038f6e7c')):
   if value!=0:raise RuntimeError('Unresolved non-null literal action')
   value=None
  if field!=0 and expression.startswith(('DAT_038f6e7c[','*DAT_038f6e7c')):raise RuntimeError('Unexpected pointer coercion')
  rows[record][field]=value;statements.append(dict(address=hex(address),field=FIELDS[field],value=value))
 if any(set(row)!=set(FIELDS)for row in rows):raise RuntimeError('Incomplete source table')
 def sha(b):return hashlib.sha256(b).hexdigest()
 header=['#pragma once','#include <array>','#include <cstdint>','namespace sadx {','struct SonicAnimationRecipe {bool action_export;std::uint16_t action_index;std::uint8_t object_count,mode;std::uint16_t next;std::uint32_t transition_bits,step_bits;};','// Actual7cec90 source assignments. Source digest and generator metadata are retained.','inline constexpr std::array<SonicAnimationRecipe,147> sonic_animation_recipes{{']
 for row in rows:header.append('    {'+('true,' if row[0] is not None else 'false,')+','.join(hex(row[f] if row[f] is not None else 0)for f in FIELDS)+'},')
 header+=['}};','}'];a.header.write_text('\n'.join(header)+'\n')
 meta=dict(source_sha256=image.sha256,procedure_address='0x7cec90',source_bytes=7674,procedure_sha256=sha(image.read(0x7cec90,7674)),source_pseudocode_sha256=sha(a.pseudocode.read_bytes()),generated_header_sha256=sha(a.header.read_bytes()),table_address=hex(BASE),records=COUNT,assignment_count=len(statements),action_export_indices=sorted(set(row[0]for row in rows if row[0] is not None)),statements=statements,limits=['Recipes are derived from retained source statements and require original-instruction producer verification','147 records are recovered from actual writes; SDK count146 is a locator-only discrepancy','Actual DLL action identities must come from the selected export loader; no zero-fill table or Windows loader execution is assumed'])
 a.metadata.write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps({k:meta[k]for k in ('records','assignment_count','procedure_sha256','generated_header_sha256')}))
if __name__=='__main__':main()
