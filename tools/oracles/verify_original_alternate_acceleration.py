"""Actual attribute4000 body and actual input/heading/rotation/matrix callees.

No source callee is overlaid or replaced. State is explicitly seeded; this is
not Windows startup, physical input, loaded mods or a hardware observation.
"""
import argparse,ctypes,hashlib,json,pathlib,struct
import unicorn
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import native_source,Original,DATA,STACK,STOP
from verify_original_ground_acceleration import cases,CONSTANTS

TASK=DATA+0x100;PLAYER=DATA+0x300;MOTION=DATA+0x700;EVENT=DATA+0xc00;MATRIX=DATA+0x4000;UNIT=0x389d650
ROTATION_FLOATS=(0x7dfff0,0x7dd568,0x7dd550,0x7e0070,0x7e0118,0x7dcc98,0x7dcb10,0x7e011c,0x7e0120,0x7dce68,0x7dcb2c,0x7e0164,0x7dccf4,0x7e0160,0x7e0170,0x7dcb60)
ROTATION_DOUBLES=(0x7e0168,0x7dccd0,0x7dcce8)
def sha(data):return hashlib.sha256(data).hexdigest()
class AlternateOriginal(Original):
    def __init__(self,image,cw,unit):
        super().__init__(image,cw);self.calls=[];self.unit=unit
        self.trace={}
        for page in (0x3b0e000,0x3b0f000,0x3b22000,0x3b3c000,0x3d0f000,0x389d000):self.machine.mem_map(page,0x1000)
        self.machine.mem_write(UNIT,unit)
        self.machine.mem_map(0x2bcc000,0x1000);self.machine.mem_write(0x2bcc000,image.read(0x2bcc000,0x1000));self.machine.mem_map(0x3c83000,0x1000);self.machine.mem_map(0x3b2c000,0x1000)
        def call(m,a,s,u):
            sp=m.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',m.mem_read(sp,4))[0]
            if self.bounds[0]<=ret<self.bounds[1]:self.calls.append(a)
        for target in (0x443c50,0x443df0,0x443e60,0x4491e0):self.machine.hook_add(UC_HOOK_CODE,call,begin=target,end=target)
        def trace(m,a,s,u):
            if a==0x44b453:self.trace['raw_angle']=hex(m.reg_read(UC_X86_REG_EAX))
            elif a==0x785170:
                sp=m.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',m.mem_read(sp,4))[0]
                if ret in (0x4477aa,0x447dd6):
                    self.trace['vector_add_pointer']=m.reg_read(UC_X86_REG_ECX)
                    self.trace['vector_add_inputs']=[list(map(hex,struct.unpack('<3I',m.mem_read(m.reg_read(r),12)))) for r in (UC_X86_REG_ECX,UC_X86_REG_EDX)]
            elif a in (0x4477aa,0x447dd6):
                if 'vector_add_pointer' in self.trace:self.trace['vector_add_output']=list(map(hex,struct.unpack('<3I',m.mem_read(self.trace['vector_add_pointer'],12))))
            elif a==0x43ec00:
                sp=m.reg_read(UC_X86_REG_ESP);ret,task,vector=struct.unpack('<3I',m.mem_read(sp,12))
                if ret in (0x4477bb,0x447de7):
                    self.trace['gravity_pointer']=vector;self.trace['gravity_before_transform']=list(map(hex,struct.unpack('<3I',m.mem_read(vector,12))))
            elif a in (0x4477bb,0x447de7):
                if 'gravity_pointer' in self.trace:self.trace['gravity_after_transform']=list(map(hex,struct.unpack('<3I',m.mem_read(self.trace['gravity_pointer'],12))))
            elif a==0x438410:
                sp=m.reg_read(UC_X86_REG_ESP);ptrs=struct.unpack('<3I',m.mem_read(sp+4,12));self.trace['angle_vectors']=[list(map(hex,struct.unpack('<3I',m.mem_read(p,12)))) for p in ptrs]
            else:self.trace['projected_angle']=hex(m.reg_read(UC_X86_REG_EAX))
        for a in (0x44b453,0x438410,0x44b51d,0x43ec00,0x4477bb,0x447de7,0x785170,0x4477aa,0x447dd6):self.machine.hook_add(UC_HOOK_CODE,trace,begin=a,end=a)
    def run(self,w,cw):
        m=self.machine;self.calls=[];self.trace={};self.bounds=((0x447010,0x44750c),(0x447510,0x447b7e),(0x447b90,0x447e54),(0x44b0e0,0x44b9b2))[w[63]]
        task=bytearray(64);struct.pack_into('<H',task,4,w[0]);struct.pack_into('<3I',task,0x14,*w[1:4]);m.mem_write(TASK,bytes(task))
        m.mem_write(TASK+0x20,struct.pack('<3I',*w[70:73]))
        if w[63] in (1,2):
            m.mem_write(TASK+0x3c,struct.pack('<I',EVENT));m.mem_write(EVENT+0x24,bytes([w[64]]));m.mem_write(EVENT+0x28,struct.pack('<5I',*w[65:70]))
        player=bytearray(0x200);struct.pack_into('<I',player,0x1c,w[6]);struct.pack_into('<3I',player,0x2c,*w[21:24]);struct.pack_into('<3I',player,0x38,*w[18:21]);struct.pack_into('<I',player,0x5c,w[7]);struct.pack_into('<33I',player,0x9c,*w[30:63]);m.mem_write(PLAYER,bytes(player))
        m.mem_write(PLAYER+0x50,struct.pack('<3I',*w[77:80]));m.mem_write(0x3b0e80c,struct.pack('<2H',w[73]&0xffff,w[74]&0xffff));m.mem_write(0x3b2cbb0,struct.pack('<I',EVENT));m.mem_write(EVENT+0x14,struct.pack('<2I',w[75],w[76]));m.mem_write(0x3c837a8,struct.pack('<I',1))
        motion=bytearray(64);struct.pack_into('<3I',motion,4,*w[12:15]);struct.pack_into('<3I',motion,0x10,*w[15:18]);struct.pack_into('<I',motion,0x20,w[8]);struct.pack_into('<3I',motion,0x34,*w[9:12]);m.mem_write(MOTION,bytes(motion))
        m.mem_write(0x3b0f0f8,struct.pack('<3I',*w[24:27]));m.mem_write(0x3b22dcc,struct.pack('<H',1));m.mem_write(0x3b0e7a0,struct.pack('<2I',w[4],w[5]));m.mem_write(0x3b3cdf0,struct.pack('<I',PLAYER));m.mem_write(PLAYER+0xa,b'\0\0')
        m.mem_write(MATRIX,self.unit*3)
        for address,value in ((0x3d0f818,MATRIX),(0x3d0fa28,3),(0x3d0fa2c,1),(0x3d0f7ac,0)):m.mem_write(address,struct.pack('<I',value))
        m.mem_write(STACK,struct.pack('<4I',STOP,TASK,MOTION,PLAYER));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        entry=(0x447010,0x447510,0x447b90,0x44b0e0)[w[63]]
        if w[63]==3:m.mem_write(STACK,struct.pack('<2I',STOP,MOTION));m.reg_write(UC_X86_REG_EAX,PLAYER);m.reg_write(UC_X86_REG_ECX,TASK)
        m.emu_start(entry,STOP,count=20000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Alternate cdecl return mismatch')
        values=list(struct.unpack('<6I',m.mem_read(PLAYER+0x2c,24)))+list(struct.unpack('<3I',m.mem_read(MOTION+4,12)))+[struct.unpack('<I',m.mem_read(MOTION+0x20,4))[0]]+list(struct.unpack('<2I',m.mem_read(MOTION+0x34,8)))+list(struct.unpack('<3I',m.mem_read(TASK+0x14,12)))+[struct.unpack('<H',m.mem_read(TASK+4,2))[0],m.mem_read(EVENT+0x24,1)[0] if w[63] in (1,2) else 0x100]
        return values,self.calls,m.reg_read(UC_X86_REG_FPSW)&0x3f,m.reg_read(UC_X86_REG_FPCW)
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--variant',choices=['attribute4000','eventground','eventair','attribute20000'],default='attribute4000');p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args()
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    table=image.read(TABLE_ADDRESS,4100);unit=image.read(UNIT,64);parameters=list(struct.unpack('<33I',image.read(0x9154e8,132)));base_cases=cases(parameters)
    variant={'attribute4000':0,'eventground':1,'eventair':2,'attribute20000':3}[args.variant];fixtures=[]
    if not variant:fixtures=[w+[0,0,0,0,0,0,0,0,0,0] for w in base_cases]
    elif variant in (1,2):
        import random
        rng=random.Random(0x447510+variant)
        for i in range(1800):
            w=base_cases[rng.randrange(len(base_cases))].copy();w[0]=0x4000
            mode=rng.choice((0,1,2,3,4,5,255))
            vector=[struct.unpack('<I',struct.pack('<f',rng.uniform(-10,10)))[0] for _ in range(3)]
            position=[struct.unpack('<I',struct.pack('<f',rng.uniform(-10,10)))[0] for _ in range(3)]
            speed=struct.unpack('<I',struct.pack('<f',rng.choice((0.,-1.,.5,2.,10.))))[0]
            step=struct.unpack('<I',struct.pack('<f',rng.choice((0.,-.5,.03,.5))))[0]
            fixtures.append(w+[variant,mode,*vector,speed,step,*position])
        for mode in (1,2,3,4):
            for raw in (0,0x80000000,1,0x80000001,0x3f800000,0x40000000,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345):
                w=base_cases[-1].copy();w[0]=0x4000;fixtures.append(w+[variant,mode,raw,raw,raw,raw,raw,0,0,0])
    else:
        import random
        rng=random.Random(0x44b0e0)
        for w in base_cases:
            w=w.copy();w[0]=0 # Reset-direction owner is a separate source dependency.
            fixtures.append(w+[variant,0,0,0,0,0,0,0,0,0])
    import random
    rng=random.Random(0x788df0)
    for index,w in enumerate(fixtures):
        normal=(0.,1.,0.)
        if variant==3 and index%3:
            components=[rng.uniform(-1,1) for _ in range(3)];length=sum(v*v for v in components)**.5;normal=tuple(v/length for v in components)
        w.extend([rng.randrange(65536),rng.randrange(65536),rng.randrange(65536),rng.randrange(65536),*[struct.unpack('<I',struct.pack('<f',v))[0] for v in normal]])
    # Task flags0/2000 are legitimate inputs to this selected variant. Other
    # source task-bit branches are covered separately without unknown scratch.
    rotation=b''.join(image.read(a,4) for a in ROTATION_FLOATS)+b''.join(image.read(a,8) for a in ROTATION_DOUBLES)
    event=image.read(0x7dcb10,4)+image.read(0x7dccd0,8)+image.read(0x7dccc8,8)
    projection=image.read(0x7dcb2c,4)+image.read(0x7dcb10,4)+image.read(0x7dcb0c,4)+image.read(0x7dccd0,8)+image.read(0x7dccc8,8)
    lib=ctypes.CDLL(str(args.library.resolve()));probe=lib.sadx_native_alternate_acceleration
    lib.sadx_native_alternate_acceleration_compiler.argtypes=[];lib.sadx_native_alternate_acceleration_compiler.restype=ctypes.c_char_p
    leaf=lib.sadx_native_event_vector_add;leaf.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];leaf.restype=ctypes.c_int
    probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];probe.restype=ctypes.c_int
    tb=(ctypes.c_uint8*4100).from_buffer_copy(table);ub=(ctypes.c_uint8*64).from_buffer_copy(unit);cb=(ctypes.c_uint32*24)(*[image.unpack('<I',a)[0] for a in CONSTANTS]);rb=(ctypes.c_uint8*88).from_buffer_copy(rotation);eb=(ctypes.c_uint8*20).from_buffer_copy(event);pb=(ctypes.c_uint8*28).from_buffer_copy(projection)
    blob=b''.join(struct.pack('<80I',*w) for w in fixtures);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.with_suffix('.cases.bin').write_bytes(blob)
    address,end=((0x447010,0x44750c),(0x447510,0x447b7e),(0x447b90,0x447e54),(0x44b0e0,0x44b9b2))[variant]
    root=pathlib.Path(__file__).resolve().parents[2]
    names=('player_acceleration_alternate','player_acceleration_alternate_math','player_acceleration_friction','player_acceleration_ground','player_rotation','player_projection','player_coordinates','ninja_matrix','player_heading','player_input','player_state','crt_acos','crt_asin','crt_integer','x87_math','x87_transcendental','ninja_math')
    report=dict(profile=f'actual {args.variant}/actual-yaw-matrix-callees/no-overlays',source_sha256=image.sha256,source_function=dict(address=hex(address),end_exclusive=hex(end),sha256=sha(image.read(address,end-address))),native_library_sha256=sha(args.library.read_bytes()),fixture_sha256=sha(blob),fixture_record='80 little-endian uint32 words',fixture_count=len(fixtures),
        native_sources={f'src/{name}.{ext}':sha((native_source(root,f'src/{name}.{ext}')).read_bytes()) for name in names for ext in ('cpp','hpp')},
        native_contracts={f'src/{name}.hpp':sha((native_source(root,f'src/{name}.hpp')).read_bytes()) for name in ('player_acceleration','player_physics','player_task','player_rotation_result')},
        native_compiler=lib.sadx_native_alternate_acceleration_compiler().decode(),declared_build_profile='arm64 C++17 O2 ffp-contract=off fno-fast-math native integer SoftFloat/Bochs',
        native_probe_sha256=sha((root/'tests/probes/native_alternate_acceleration_probe.cpp').read_bytes()),verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),
        source_artifacts=dict(quarter_table_sha256=sha(table),unit_matrix_sha256=sha(unit),parameter_row_sha256=sha(image.read(0x9154e8,132)),rotation_constants_sha256=sha(rotation),event_constants_sha256=sha(event),projection_constants_sha256=sha(projection)),
        control_words=[],mismatches=[],limits=['Explicit source data/states and experimental masked CW profiles, not observed startup state','Actual source yaw/input/vector/matrix callees execute when selected; no no-op or identity replacement','Attribute20000 profiles use task flags0; direction-reset owner remains a separate required boundary','Source CRT matherr-control1 is an explicit dispatcher-bypass test seed; actual initialized control is unobserved','Unicorn flags/transcendental results remain weaker than original Intel hardware','Raw/nonfinite encodings are retained without caller-value normalization','No physical controls, loaded mods or complete game/collision path'])
    report['native_sources'].update({name:sha((root/name).read_bytes()) for name in ('src/player/ground_acceleration.hpp', 'src/player/ground_acceleration.cpp', 'src/player/ground_acceleration_internal.hpp', 'src/player/ground_acceleration_native.cpp', 'src/player/ground_arithmetic.hpp', 'src/player/ground_friction.cpp', 'src/guest/ground_acceleration_reference.hpp', 'src/guest/player_acceleration_ground.cpp', 'src/guest/player_acceleration_friction.cpp', 'src/player/player_acceleration_ground.hpp', 'src/player/player_acceleration_friction.hpp', 'src/cpu/fidelity_float.hpp')})
    report['source_ranges']={f'{address:#x}-{end:#x}':sha(image.read(address,end-address))}
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        original=AlternateOriginal(image,cw,unit);item=dict(cw=hex(cw),cases=len(fixtures),value_mismatches=0,call_mismatches=0,flag_mismatches=0,cw_mismatches=0)
        for index,w in enumerate(fixtures):
            values,calls,flags,restored=original.run(w,cw);out=(ctypes.c_uint32*58)();status=probe(tb,4100,ub,64,cb,24,rb,88,eb,20,pb,28,(ctypes.c_uint32*80)(*w),80,cw,out,58)
            if status or out[12]:raise RuntimeError(f'Native rejected case{index}:status{status},failure{out[12]},site{out[13]:#x}')
            actual=list(out[:12])+list(out[40:45]);differences=[]
            for label,a,b in [('value',values,actual),('call',calls,list(out[18:18+out[17]])),('flag',flags,out[15]),('cw',restored,out[16])]:
                if a!=b:item[label+'_mismatches']+=1;differences.append(label)
            if differences:
                replay=None
                if 'value' in differences and 'vector_add_inputs' in original.trace:
                    inputs=[(ctypes.c_uint32*3)(*[int(x,16) for x in v]) for v in original.trace['vector_add_inputs']];leaf_out=(ctypes.c_uint32*4)()
                    if leaf(*inputs,cw,leaf_out):raise RuntimeError('Leaf replay failed')
                    replay=dict(native_output=list(map(hex,leaf_out[:3])),native_flags=hex(leaf_out[3]),original_output=original.trace['vector_add_output'])
                report['mismatches'].append(dict(cw=hex(cw),case=index,fields=differences,input=list(map(hex,w)),original=list(map(hex,values)),native=list(map(hex,actual)),calls=[list(map(hex,calls)),list(map(hex,out[18:18+out[17]]))],flags=[hex(flags),hex(out[15])],original_projection=original.trace,native_projection=list(map(hex,out[45:58])),vector_add_replay=replay))
        report['control_words'].append(item);print(json.dumps(item),flush=True)
    report['values_calls_cw_match']=all(not p['value_mismatches'] and not p['call_mismatches'] and not p['cw_mismatches'] for p in report['control_words']);args.output.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['values_calls_cw_match'] else 1
if __name__=='__main__':raise SystemExit(main())
