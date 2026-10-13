"""Actual auxiliary-effect initialization, node destruction and supported update prefix.

Executes original4a2b60/4a2ae0,441970,4144e0,4f88a0,4a0720,40b310,
4a2570 prefix/CRT_ftol/FreeTask. No source callee is substituted.
Unlifted update owners and unknown native facts are retained prefix boundaries.
"""
import argparse,ctypes,json,pathlib,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,STACK,STOP,DATA
from verify_original_task_work import ARENA,SIZE,GLOBALS,call,sha
TABLE=0x9275a0
POOL=ARENA+32
REGISTERED=DATA+0x100

def float_word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def fixtures():
    cases=[]
    def add(name,op=0,**kw):
        w=[op,2,0,1,0,1,0,float_word(0),0,0,2,2,1,0,1,0xa5]
        for key,value in kw.items():w[int(key[1:])]=value
        cases.append(dict(name=name,words=w))
    for char in(0,2,3,5,7,1,6,255):add('registered character selection',w2=char)
    for char in(0,2,7,5,6,0xffff):add('known-null registration fallback',w1=1,w4=char)
    add('unknown registration',w1=0);add('unknown fallback',w1=1,w3=0);add('missing table',w5=0);add('null primary initializer stores callbacks',w14=0)
    for nodes in range(5):add('real node unlink/free',op=1,w6=nodes)
    add('null destructor primary',op=1,w14=0)
    add('update missing registration',op=2,w1=0)
    add('update observed null registration schedules destruction',op=2,w1=1)
    add('update unknown selection',op=2,w10=0)
    add('update known null selection schedules destruction',op=2,w10=1)
    add('null plus nonzero variant is unsafe source pointer',op=2,w10=1,w9=1)
    add('unknown live secondary mode',op=2,w12=0)
    for mode in(0,1,255):
        for player_mode in(0,0x27):add('source variant committed before required update callee',op=2,w8=mode,w13=player_mode)
    add('null update primary',op=2,w14=0)
    for raw in(0x80000000,1,0x80000001,0x7fc12345,0xffc54321,0x7f812345):add('source ftol nonfinite/subnormal index',op=2,w7=raw)
    return cases

class ShadowOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f);m=self.machine
        for page,size in((ARENA,0x1001000),(GLOBALS,0x1000),(0x38f6000,0x1000),(0x3b42000,0x1000),(0x3b22000,0x1000),(0x927000,0x2000)):
            if not any(start<=page and page+size-1<=end for start,end,perms in m.mem_regions()):m.mem_map(page,size)
        m.mem_write(TABLE,image.read(TABLE,0x438));m.reg_write(UC_X86_REG_EDX,ARENA);call(m,0x40b1ce,[])
        m.mem_write(STACK,struct.pack('<2I',0,STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40b466,STOP,count=100000)
        self.initial=bytes(m.mem_read(ARENA,SIZE));self.globals=bytes(m.mem_read(GLOBALS,0x1000));self.target=0;self.stopped=0
        def code(machine,address,size,user):
            if address==self.target:self.stopped=address;machine.emu_stop()
        m.hook_add(unicorn.UC_HOOK_CODE,code)
    def run(self,w,cw):
        m=self.machine;self.target=0;self.stopped=0;m.mem_write(ARENA,self.initial);m.mem_write(GLOBALS,self.globals)
        m.mem_write(0x3b42000,bytes(0x1000));task=call(m,0x40b860,[2,2,0x4a2b60]);primary=struct.unpack('<I',m.mem_read(task+32,4))[0]
        m.mem_write(primary,bytes([w[15]&255])*64);m.mem_write(REGISTERED,bytes(64));m.mem_write(REGISTERED+9,bytes([w[2]&255]));m.mem_write(REGISTERED+1,bytes([w[13]&255]));m.mem_write(0x3b42e10,struct.pack('<I',REGISTERED if w[1]==2 else 0));m.mem_write(0x3b22dc0,struct.pack('<H',w[4]&65535))
        nodes=[call(m,0x40b220,[52])for _ in range(w[6])]
        for n in nodes:m.mem_write(n,bytes(52))
        if w[0]!=0:
            m.mem_write(primary+0x10,struct.pack('<I',nodes[0]if nodes else 0))
            for i,n in enumerate(nodes):m.mem_write(n+0x2c,struct.pack('<2I',nodes[i-1]if i else 0,nodes[i+1]if i+1<len(nodes)else 0))
        if w[0]==0:
            if not w[14]:self.target=0x4a2afd
            elif not w[1]or(w[1]==1 and not w[3]):self.target=0x4a2b21
            elif not w[5]:self.target=0x4a2b26
        elif w[0]==1:
            if not w[14]:self.target=0x4a0728
        else:
            m.mem_write(primary+0x34,struct.pack('<I',w[7]));m.mem_write(primary+1,bytes([w[8]&255]));m.mem_write(primary+3,bytes([w[9]&255]));m.mem_write(primary+8,struct.pack('<I',TABLE+w[11]*216 if w[10]==2 else 0))
            if not w[14]:self.target=0x4a25aa
            elif not w[1]:self.target=0x4a25b5
            elif w[1]!=1:
                if w[10]==0:self.target=0x4a25cb
                elif w[10]==1 and w[9]:self.target=0x4a25ce
                elif w[10]==2:
                    if not w[12]:self.target=0x4a25d9
                    else:
                        variant=(2 if w[8]==0 else 1)if w[13]==0x27 else 0
                        self.target=(0x4a262c if variant else 0x4a2600)if w[8]else(0x4a264d if variant else 0x4a2687)
        if not w[14]:m.mem_write(task+32,bytes(4))
        m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<2I',STOP,task));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start((0x4a2b60,0x4a0720,0x4a2570)[w[0]],STOP,count=200000)
        if self.target and self.stopped!=self.target:raise RuntimeError('Expected source prefix boundary not reached')
        header=struct.unpack('<3I',m.mem_read(task+16,12)) # exec/display/destructor
        return dict(memory=bytes(m.mem_read(ARENA,SIZE)),boundary=self.stopped,callbacks=[header[0],header[2],header[1]],free_head=struct.unpack('<I',m.mem_read(0x3abdbe8,4))[0],cw=m.reg_read(UC_X86_REG_FPCW),flags=m.reg_read(UC_X86_REG_FPSW)&63)

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires Unicorn2.1.4')
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source identity changed')
    table=image.read(TABLE,0x438);cases=fixtures();blob=b''.join(struct.pack('<16I',*c['words'])for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_player_auxiliary_effect;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t];fn.restype=ctypes.c_int
    native_table=(ctypes.c_ubyte*len(table)).from_buffer_copy(table);memory=(ctypes.c_ubyte*SIZE)();original=ShadowOriginal(image);root=pathlib.Path(__file__).resolve().parents[2]
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),table_sha256=sha(table),case_blob=dict(path=str(a.output.with_suffix('.cases.bin')),sha256=sha(blob),cases=len(cases),record='16 LE uint32 words'),source_functions={hex(x):dict(size=y-x,sha256=sha(image.read(x,y-x)))for x,y in((0x4a2b60,0x4a2b69),(0x4a2ae0,0x4a2b5b),(0x441970,0x44198a),(0x4144e0,0x4144e8),(0x4f88a0,0x4f88a3),(0x4a0720,0x4a075e),(0x4a2570,0x4a2a02),(0x644340,0x6443bf))},native_sources={n:sha((root/n).read_bytes())for n in('src/player/player_auxiliary_effect.cpp','src/player/player_auxiliary_effect.hpp','src/engine/task.cpp','src/engine/task.hpp','src/engine/task_work.cpp','src/player/player_startup.cpp','tests/probes/native_player_auxiliary_effect_probe.cpp','tools/oracles/verify_original_player_auxiliary_effect.py')},profiles=[],limits=['Exact initializers/getters/destructor and update prefix; no source callee overlays','Unlifted update calls and unknown facts stop before the required source instruction; no full update/renderer or playable-game claim','Heap/task initialization starts after unlifted acquisition, as explicit fixture','Task pool payload excluded from arena byte comparison because native graph owns it; live callback identities compared separately, pool semantics have separate instruction oracle','Shadow source pointer bytes in probe snapshot encode the authoritative owned table selection; production never casts them','All FP status differences retained; Unicorn FLD SNaN conversion is not the hardware specification','No original Windows hardware or initialized mods runtime observed'])
    for cw in(pc|rc|0x7f for pc in(0,0x200,0x300)for rc in(0,0x400,0x800,0xc00)):
        profile=dict(cw=hex(cw),cases=len(cases),state_mismatches=0,boundary_mismatches=0,callback_mismatches=0,control_mismatches=0,flag_mismatches=0,examples=[])
        for index,c in enumerate(cases):
            expected=original.run(c['words'],cw);out=(ctypes.c_uint32*16)();status=fn(native_table,len(table),(ctypes.c_uint32*16)(*c['words']),16,cw,out,16,memory,SIZE)
            if status:raise RuntimeError(f'Native probe status {status}, case {index}')
            actual=bytes(memory);a_bytes=actual[:32]+actual[32+0x34000:];e_bytes=expected['memory'][:32]+expected['memory'][32+0x34000:];different=[]
            for field,x,y in(('state',e_bytes,a_bytes),('boundary',expected['boundary'],out[1]),('callback',expected['callbacks'],list(out)[4:7]),('control',expected['cw'],out[11]),('flag',expected['flags'],out[12])):
                if x!=y:profile[field+'_mismatches']+=1;different.append(field)
            if different and len(profile['examples'])<20:
                profile['examples'].append(dict(case=index,name=c['name'],fields=different,native=list(out),source_boundary=hex(expected['boundary']),source_callbacks=list(map(hex,expected['callbacks'])),source_flags=hex(expected['flags']),first_byte_offsets=[hex(i)for i,(x,y)in enumerate(zip(e_bytes,a_bytes))if x!=y][:12]))
        report['profiles'].append(profile);print(json.dumps({k:v for k,v in profile.items()if k!='examples'}),flush=True)
    report['supported_state_boundaries_callbacks_control_match']=all(not any(p[k]for k in('state_mismatches','boundary_mismatches','callback_mismatches','control_mismatches'))for p in report['profiles']);report['strict_emulator_match']=report['supported_state_boundaries_callbacks_control_match']and all(not p['flag_mismatches']for p in report['profiles']);a.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['supported_state_boundaries_callbacks_control_match'])
if __name__=='__main__':raise SystemExit(main())
