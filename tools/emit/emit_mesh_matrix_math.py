"""Generate static native C++ expressions from pinned straight-line FP evidence.

The generated runtime contains no opcode table/interpreter or guest memory.
All temporary spills and source operations become direct X87Context calls.
"""
import hashlib,pathlib,re

ROOT=pathlib.Path(__file__).resolve().parents[2]
REFERENCE=ROOT.parent/'_reference/sadx'

class Emitter:
    def __init__(self,snapshot=False,seed=()):
        self.stack=list(seed);self.lines=[];self.next=0;self.locals=set();self.snapshot=snapshot
    def value(self,expression,address):
        name=f'v{self.next}';self.next+=1;self.lines.append(f'    [[maybe_unused]] const auto {name}={expression}; // {address}');return name
    def memory(self,operand):
        m=re.fullmatch(r'dword \[(?:eax|ebx)(?:\+(0x[0-9a-f]+|[0-9]+))?\]',operand)
        if m:return f'matrix[{int(m[1] or "0",0)//4}]'
        m=re.fullmatch(r'dword \[esp\+0x[0-9a-f]+\+var_([0-9A-F]+)\]',operand)
        if m:
            number=int(m[1],16)
            if self.snapshot and number>=0x64:return f'saved[{(0xa0-number)//4}]'
            name=f'spill_{m[1].lower()}'
            if name not in self.locals:raise ValueError(f'Read before source spill: {name}')
            return name
        raise ValueError(f'Unsupported FP memory operand: {operand}')
    def instruction(self,line):
        address,text=line.split(': ',1);parts=text.split(' ',1);op=parts[0];arg=parts[1] if len(parts)>1 else ''
        if not op.startswith('f'):return
        if op=='fld':
            if re.fullmatch('st[0-7]',arg):self.stack.insert(0,self.stack[int(arg[2:])])
            else:self.stack.insert(0,self.value(f'fp.from_float({self.memory(arg)})',address))
        elif op=='fstp':
            if arg=='st0':self.stack.pop(0)
            elif arg.startswith('dword [esp'):
                m=re.search('var_([0-9A-F]+)',arg);name=f'spill_{m[1].lower()}'
                prefix='' if name in self.locals else 'float ';self.locals.add(name)
                self.lines.append(f'    {prefix}{name}=fp.to_float({self.stack.pop(0)}); // {address}')
            else:self.lines.append(f'    {self.memory(arg)}=fp.to_float({self.stack.pop(0)}); // {address}')
        elif op=='fxch':
            n=int(arg.split(', ')[1][2:]);self.stack[0],self.stack[n]=self.stack[n],self.stack[0]
        elif op=='fchs':self.stack[0]=self.value(f'{self.stack[0]}.negated()',address)
        elif op in ('fadd','fsub','fmul'):
            method={'fadd':'add','fsub':'subtract','fmul':'multiply'}[op]
            if re.fullmatch('st[0-7]',arg):expression=f'fp.{method}({self.stack[0]},{self.stack[int(arg[2:])]})'
            else:expression=f'fp.{method}_float_memory({self.stack[0]},{self.memory(arg)})'
            self.stack[0]=self.value(expression,address)
        elif op in ('faddp','fsubp','fmulp'):
            n=int(arg[2:]);method={'faddp':'add','fsubp':'subtract','fmulp':'multiply'}[op]
            self.stack[n]=self.value(f'fp.{method}({self.stack[n]},{self.stack[0]})',address);self.stack.pop(0)
        else:raise ValueError(f'Unsupported FP instruction: {line}')
        if len(self.stack)>8:raise ValueError('Source arithmetic exceeded x87 stack')

def main():
    determinant=REFERENCE/'mesh-math-dependencies/matrix-determinant.asm'
    inverse=REFERENCE/'scene-mesh-dependencies/point-787630.asm'
    if len(determinant.read_text().splitlines())!=119 or len(inverse.read_text().splitlines())!=467:raise ValueError('Evidence coverage changed')
    output=['// Generated static source-ordered math; see tools/emit/emit_mesh_matrix_math.py.',
            f'// Determinant evidence SHA256 {hashlib.sha256(determinant.read_bytes()).hexdigest()}',
            f'// Inverse evidence SHA256 {hashlib.sha256(inverse.read_bytes()).hexdigest()}',
            '#include "player_mesh_matrix.hpp"','#include <cstring>','namespace sadx {',
            'X87Value mesh_matrix_determinant(const NinjaMatrix& matrix,X87Context& fp) {']
    e=Emitter()
    for line in determinant.read_text().splitlines():e.instruction(line)
    if len(e.stack)!=1:raise ValueError('Determinant return stack mismatch')
    output+=e.lines+[f'    return {e.stack[0]};','}',
        'bool invert_mesh_matrix(NinjaMatrix& matrix,const CoordinateAxisConstants& constants,X87Context& fp) {',
        '    const auto determinant=mesh_matrix_determinant(matrix,fp);',
        '    const auto zero=fp.from_float(constants.zero);',
        '    if(fp.compare_quiet(determinant,zero)==X87Comparison::equal) {',
        '        constexpr std::uint32_t bits=0x7f7fc99eU;float sentinel;std::memcpy(&sentinel,&bits,4);',
        '        matrix.fill(sentinel);return false;',
        '    }',
        '    const auto inverse_det=fp.reverse_divide_float_memory(determinant,constants.one);',
        '    const auto saved=matrix;']
    e=Emitter(snapshot=True,seed=('inverse_det',))
    for line in inverse.read_text().splitlines():
        address=int(line.split(':')[0],16)
        if 0x787674<=address<=0x787c51:e.instruction(line)
    if e.stack:raise ValueError('Inverse source return stack mismatch')
    output+=e.lines+['    return true;','}',
        'void scale_mesh_matrix_vector(NinjaMatrix& matrix,const NinjaVector& input,X87Context& fp) {',
        '    const auto x=fp.from_float(input.x); // source788a51',
        '    const auto y=fp.from_float(input.y); // source788a57',
        '    const float z=input.z; // source788a5a/5d MOV preserves signaling encoding',
        '    for(unsigned column=0;column<4;++column)',
        '        matrix[column]=fp.to_float(fp.multiply_float_memory(x,matrix[column]));',
        '    for(unsigned column=0;column<4;++column)',
        '        matrix[4+column]=fp.to_float(fp.multiply_float_memory(y,matrix[4+column]));',
        '    for(unsigned column=0;column<4;++column)',
        '        matrix[8+column]=fp.to_float(fp.multiply_float_memory(fp.from_float(z),matrix[8+column]));',
        '}','} // namespace sadx','']
    (ROOT/'src/player/player_mesh_matrix.cpp').write_text('\n'.join(output))
    print(f'Emitted {len(e.lines)} direct inverse expressions/stores; no runtime instruction interpreter')
if __name__=='__main__':main()
