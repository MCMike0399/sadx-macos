"""Lift retained 7887d0 arithmetic into direct native source expressions.

The resulting core has no instruction interpreter, guest memory or address
dispatch. Regeneration requires the retained REA evidence for the pinned EXE.
"""
import hashlib
import json
import pathlib
import re

ROOT=pathlib.Path(__file__).resolve().parent.parent
EVIDENCE=ROOT.parent/'reference/mesh-transform-dependencies/plane-intersection.json'

class Emitter:
    def __init__(self):
        self.stack=[];self.lines=[];self.next=0;self.spills=set()
    def value(self,expression,address):
        name=f'v{self.next}';self.next+=1
        self.lines.append(f'    const auto {name}={expression}; // {address}')
        return name
    def memory(self,arg):
        match=re.fullmatch(r'dword \[(esi|edi|eax)(?:\+(0x[0-9a-f]+|[0-9]+))?\]',arg)
        if match:
            index=int(match[2] or '0',0)//4
            if index>=6:raise ValueError('Invalid plane field')
            owner={'esi':'first.','edi':'second.','eax':'output->'}[match[1]]
            return owner+('point.' if index<3 else 'normal.')+'xyz'[index%3]
        match=re.fullmatch(r'dword \[esp\+0x24\+(var_[0-9A-F]+|arg_0)\]',arg)
        if match:
            name='spill_'+match[1].lower()
            if name not in self.spills:raise ValueError('Read before spill: '+name)
            return name
        if arg=='dword [aImm32dll+16]':return 'constants.axis.one'
        raise ValueError('Unrecognized memory: '+arg)
    def store(self,arg,address,pop):
        if arg.startswith('st'):
            index=int(arg[2:]);self.stack[index]=self.stack[0]
        elif arg.startswith('dword [esp'):
            match=re.search(r'(var_[0-9A-F]+|arg_0)',arg);name='spill_'+match[1].lower()
            prefix='' if name in self.spills else 'float '
            self.spills.add(name);self.lines.append(f'    {prefix}{name}=fp.to_float({self.stack[0]}); // {address}')
        else:self.lines.append(f'    {self.memory(arg)}=fp.to_float({self.stack[0]}); // {address}')
        if pop:self.stack.pop(0)
    def instruction(self,line):
        address,instruction=line.split(': ',1);parts=instruction.split(' ',1)
        op=parts[0];arg=parts[1] if len(parts)>1 else ''
        if op=='fld':
            self.stack.insert(0,self.stack[int(arg[2:])] if arg.startswith('st') else
                              self.value(f'fp.from_float({self.memory(arg)})',address))
        elif op in ('fst','fstp'):self.store(arg,address,op=='fstp')
        elif op=='fxch':
            index=int(arg.split(', ')[1][2:]);self.stack[0],self.stack[index]=self.stack[index],self.stack[0]
        elif op in ('fadd','fsub','fmul'):
            method={'fadd':'add','fsub':'subtract','fmul':'multiply'}[op]
            expression=(f'fp.{method}({self.stack[0]},{self.stack[int(arg[2:])]})' if arg.startswith('st') else
                        f'fp.{method}_float_memory({self.stack[0]},{self.memory(arg)})')
            self.stack[0]=self.value(expression,address)
        elif op in ('faddp','fsubp'):
            index=int(arg[2:]);method={'faddp':'add','fsubp':'subtract'}[op]
            self.stack[index]=self.value(f'fp.{method}({self.stack[index]},{self.stack[0]})',address)
            self.stack.pop(0)
        elif op=='fdivr':
            self.stack[0]=self.value(f'fp.reverse_divide_float_memory({self.stack[0]},{self.memory(arg)})',address)
        else:raise ValueError('Unrecognized arithmetic: '+line)
        if len(self.stack)>8:raise ValueError('Invalid source stack')

def main():
    record=json.loads(EVIDENCE.read_text())
    evidence=record.get('structuredContent')
    if not evidence:
        evidence=json.loads(next(item['text'] for item in record['content'] if item['type']=='text'))
    if evidence['subject']['digest']['sha256']!='6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74':
        raise ValueError('Unexpected source identity')
    emitter=Emitter()
    for line in evidence['normalized_result']['assembly']:
        address=int(line.split(':')[0],16)
        if 0x788831<=address<=0x7889d1:emitter.instruction(line)
    if emitter.stack:raise ValueError('Unbalanced intersection arithmetic')
    text='''#include "player_plane.hpp"
#include "player_direction_reset.hpp"
namespace sadx {
bool source_planes_parallel(const NinjaPlane& first,const NinjaPlane& second,
        const PlayerPlaneConstants& constants,X87Context& fp) {
    // Source795823/29/2d cached FLDs precede the dot-product spill.
    const auto z=fp.from_float(second.normal.z);
    const auto y=fp.from_float(second.normal.y);
    const auto x=fp.from_float(second.normal.x);
    const auto first_normal=first.normal; // source MOV snapshots
    const auto zz=fp.multiply_float_memory(fp.from_float(second.normal.z),first.normal.z);
    const auto yy=fp.multiply_float_memory(fp.from_float(second.normal.y),first.normal.y);
    const auto zy=fp.add(zz,yy);
    const auto xx=fp.multiply_float_memory(fp.from_float(first.normal.x),second.normal.x);
    const float dot=fp.to_float(fp.add(zy,xx));
    const auto x2=fp.multiply(x,x),y2=fp.multiply(y,y);
    const auto xy=fp.add(x2,y2);
    const float second_square=fp.to_float(fp.add(xy,fp.multiply(z,z)));
    const float second_length=fp.to_float(signed_sqrt_extended(second_square,fp));
    const auto a=fp.multiply_float_memory(fp.from_float(first_normal.x),first_normal.x);
    const auto b=fp.multiply_float_memory(fp.from_float(first_normal.y),first_normal.y);
    const auto ab=fp.add(a,b);
    const auto c=fp.multiply_float_memory(fp.from_float(first_normal.z),first_normal.z);
    const float first_square=fp.to_float(fp.add(ab,c));
    const auto denominator=fp.multiply_float_memory(signed_sqrt_extended(first_square,fp),second_length);
    const auto ratio=fp.reverse_divide_float_memory(denominator,dot);
    const auto cosine=fp.from_canonical_ext80(ratio.sign_exponent()&0x7fffU,ratio.significand());
    return fp.compare_float_memory(cosine,constants.parallel_cosine)==X87Comparison::greater;
}
X87Value intersect_source_planes(const NinjaPlane& first,const NinjaPlane& second,NinjaPlane* output,
        const PlayerPlaneConstants& constants,X87Context& fp) {
    if(source_planes_parallel(first,second,constants,fp)) {
        const auto point=first.point;
        if(output)*output={{0,0,0},{0,0,0}};
        return project_point_onto_plane(point,second.point,second.normal,nullptr,constants.distance_scale,fp);
    }
    if(!output)return fp.from_float(constants.axis.zero);
'''
    text+=f'    // Static lift from REA evidence SHA256 {hashlib.sha256(EVIDENCE.read_bytes()).hexdigest()}\n'
    text+='\n'.join(emitter.lines)
    text+='\n    return fp.from_float(constants.axis.zero);\n}\n} // namespace sadx\n'
    (ROOT/'src/player/player_plane.cpp').write_text(text)
    print(f'Emitted {len(emitter.lines)} direct arithmetic expressions/stores')
if __name__=='__main__':main()
