"""Ground local body with real input/coordinate/cross callees.

Five rotation/reset callees are explicit controlled no-mutation test boundaries
in both executions. Fast yaw executes actual original/native heading owners.
This establishes caller arithmetic/selection/stores and that actual heading
integration, not the behavior of controlled callees or full native movement.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn import UC_HOOK_CODE,UC_PROT_READ,UC_PROT_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import native_source,Original,DATA,STACK,STOP

TASK=DATA+0x100;PLAYER=DATA+0x300;MOTION=DATA+0x700;COLLISION=DATA+0xa00;MATRIX=DATA+0x4000
UNIT=0x389d650
CONTROLLED=(0x4491e0,0x43c130,0x449380,0x443e60,0x443c50)
CONSTANTS=(0x7dcb2c,0x7dcb34,0x7dcc98,0x7dccfc,0x7dce5c,0x7dd540,0x7dd550,0x7dd568,
    0x7dd578,0x7dd584,0x7dfff0,0x7e0064,0x7e0068,0x7e0088,0x7e00e4,0x7e0124,
    0x7e0138,0x7e0148,0x7e014c,0x7e0178,0x7e019c,0x7e01a0,0x7e01a4,0x7e01a8)
def sha(data):return hashlib.sha256(data).hexdigest()
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def cases(parameters):
    rng=random.Random(0x44c270);result=[]
    for _ in range(1800):
        active=rng.choice((0,1));flags=rng.choice((0,0x2000)) if active else 0
        words=[flags,rng.randrange(-65536,65536)&0xffffffff,rng.randrange(65536),rng.randrange(65536),rng.randrange(65536),
            bits(rng.choice((.2,.5,.7,1.,-1.))) if active else 0,bits(rng.uniform(-1,1)),rng.choice((0,4,0x800000)),
            rng.randrange(65536),bits(123.),bits(456.),bits(rng.uniform(.2,2)),
            *map(bits,(rng.uniform(-10,10) for _ in range(3))),*map(bits,(rng.uniform(-2,2) for _ in range(3))),
            *map(bits,(rng.uniform(-2,14),rng.uniform(-2,2),rng.uniform(-2,2))),
            bits(111),bits(222),bits(333),*map(bits,(rng.uniform(-1,1) for _ in range(3))),
            *map(bits,(rng.uniform(-1,1) for _ in range(3))),*parameters]
        result.append(words)
    # Independently selected parameter values exercise the contract, including
    # unusual ordering/signs. The native owner must preserve them, not silently
    # substitute Sonic defaults or normalize parameter relationships.
    for _ in range(300):
        words=result[rng.randrange(1800)].copy()
        for parameter,low,high in ((4,-2,16),(9,-2,6),(10,-2,12),(11,-3,13),(13,-2,15),
                                   (15,-.5,.5),(17,-2,2),(21,-.2,.2),(22,-.2,.2),
                                   (23,-.2,.2),(24,-2,2),(25,-2,2),(26,-2,2),(30,-2,2)):
            words[30+parameter]=bits(rng.uniform(low,high))
        result.append(words)
    for speed in (-2.,0.,.5,1.,2.,3.,8.,12.):
        for dot in (-.5,-.3,-.1,0.,.4,.7,.71,.87,.94,.96,1.):
            for active in (0,1):
                words=[0,0,0,0,16384,bits(.5) if active else 0,bits(dot),4,0,bits(123),bits(456),bits(1),
                    bits(1),bits(0),bits(1),bits(.1),bits(-.2),bits(.3),bits(speed),bits(0),bits(.25),
                    bits(111),bits(222),bits(333),bits(0),bits(-1),bits(0),bits(0),bits(1),bits(0),*parameters]
                result.append(words)
    base=[0,0,0,0,16384,bits(.5),bits(.75),4,0,bits(123),bits(456),bits(1),
          bits(1),bits(0),bits(1),bits(.1),bits(-.2),bits(.3),bits(3),bits(0),bits(.25),
          bits(111),bits(222),bits(333),bits(0),bits(-1),bits(0),bits(0),bits(1),bits(0),*parameters]
    for raw in (0,0x80000000,1,0x80000001,0x7f800000,0xff800000,
                0x7fc12345,0xffc54321,0x7f812345,0xff812345):
        for index in (5,6,11,12,13,14,15,16,17,18,19,20,24,25,26,27,28,29,51,54,56,60):
            words=base.copy();words[index]=raw;result.append(words)
    return result
class GroundOriginal(Original):
    def __init__(self,image,cw,unit):
        super().__init__(image,cw);self.calls=[]
        for page in (0x3b0e000,0x3b0f000,0x3b22000,0x3b3c000,0x3d0f000,0x389d000):self.machine.mem_map(page,0x1000)
        self.machine.mem_write(UNIT,unit);self.unit=unit
        def boundary(machine,address,size,user):
            self.calls.append(address);sp=machine.reg_read(UC_X86_REG_ESP)
            target=struct.unpack('<I',machine.mem_read(sp,4))[0]
            machine.reg_write(UC_X86_REG_ESP,sp+4);machine.reg_write(UC_X86_REG_EIP,target)
        for address in CONTROLLED:self.machine.hook_add(UC_HOOK_CODE,boundary,begin=address,end=address)
        self.machine.hook_add(UC_HOOK_CODE,lambda machine,address,size,user:self.calls.append(address),begin=0x443df0,end=0x443df0)
    def run(self,w,cw):
        m=self.machine;self.calls=[];task=bytearray(64);struct.pack_into('<H',task,4,w[0]);struct.pack_into('<3I',task,0x14,*w[1:4]);m.mem_write(TASK,bytes(task))
        player=bytearray(0x200);struct.pack_into('<I',player,0x1c,w[6]);struct.pack_into('<3I',player,0x2c,*w[21:24]);struct.pack_into('<3I',player,0x38,*w[18:21]);struct.pack_into('<I',player,0x5c,w[7]);struct.pack_into('<I',player,0x64,COLLISION);struct.pack_into('<33I',player,0x9c,*w[30:63]);m.mem_write(PLAYER,bytes(player))
        motion=bytearray(64);struct.pack_into('<3I',motion,4,*w[12:15]);struct.pack_into('<3I',motion,0x10,*w[15:18]);struct.pack_into('<I',motion,0x20,w[8]);struct.pack_into('<3I',motion,0x34,*w[9:12]);m.mem_write(MOTION,bytes(motion));m.mem_write(COLLISION+0x1c,struct.pack('<3I',*w[27:30]))
        m.mem_write(0x3b0f0f8,struct.pack('<3I',*w[24:27]));m.mem_write(0x3b22dcc,struct.pack('<H',1));m.mem_write(0x3b0e7a0,struct.pack('<2I',w[4],w[5]));m.mem_write(0x3b3cdf0,struct.pack('<I',PLAYER));m.mem_write(PLAYER+0xa,b'\0\0')
        m.mem_write(MATRIX,self.unit*3)
        for address,value in ((0x3d0f818,MATRIX),(0x3d0fa28,3),(0x3d0fa2c,1),(0x3d0f7ac,0)):m.mem_write(address,struct.pack('<I',value))
        m.mem_write(STACK,struct.pack('<4I',STOP,TASK,MOTION,PLAYER));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(0x44c270,STOP,count=20000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Ground cdecl return mismatch')
        values=list(struct.unpack('<6I',m.mem_read(PLAYER+0x2c,24)))+list(struct.unpack('<3I',m.mem_read(MOTION+4,12)))+[struct.unpack('<I',m.mem_read(MOTION+0x20,4))[0]]+list(struct.unpack('<2I',m.mem_read(MOTION+0x34,8)))+list(struct.unpack('<3I',m.mem_read(TASK+0x14,12)))
        return values,self.calls,m.reg_read(UC_X86_REG_FPSW)&0x3f,m.reg_read(UC_X86_REG_FPCW)
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args()
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    table=image.read(TABLE_ADDRESS,4100);unit=image.read(UNIT,64);parameters=list(struct.unpack('<33I',image.read(0x9154e8,132)));fixtures=cases(parameters)
    lib=ctypes.CDLL(str(args.library.resolve()));probe=lib.sadx_native_ground_acceleration
    lib.sadx_native_ground_acceleration_compiler.argtypes=[];lib.sadx_native_ground_acceleration_compiler.restype=ctypes.c_char_p
    probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];probe.restype=ctypes.c_int
    tb=(ctypes.c_uint8*4100).from_buffer_copy(table);ub=(ctypes.c_uint8*64).from_buffer_copy(unit);cb=(ctypes.c_uint32*24)(*[image.unpack('<I',a)[0] for a in CONSTANTS])
    blob=b''.join(struct.pack('<63I',*w) for w in fixtures);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parents[2]
    names=('player_acceleration_ground','player_coordinates','ninja_matrix','player_heading','player_input','player_state','ninja_math','x87_math','x87_transcendental','crt_integer')
    report=dict(profile='ground local body/real input-matrix-cross-fast-yaw/five controlled rotation no-mutation boundaries',source_sha256=image.sha256,source_function=dict(address='0x44c270',size=0xb75,sha256=sha(image.read(0x44c270,0xb75))),native_library_sha256=sha(args.library.read_bytes()),fixture_sha256=sha(blob),controlled_callees=list(map(hex,CONTROLLED)),real_callees=['0x43ec00','0x43bdc0','0x7889f0','0x7871f0','0x4383b0','0x443df0','0x438350','0x43ec90'],
        native_sources={f'src/{name}.{ext}':sha((native_source(root,f'src/{name}.{ext}')).read_bytes()) for name in names for ext in ('hpp','cpp')},
        native_task_owner_sha256=sha((root/'src/player/player_task.hpp').read_bytes()),
        native_probe_sha256=sha((root/'tests/probes/native_ground_acceleration_probe.cpp').read_bytes()),
        verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),
        native_compiler=lib.sadx_native_ground_acceleration_compiler().decode(),
        declared_build_profile='arm64 C++17 O2 ff-contract=off fno-fast-math native SoftFloat/Bochs',
        quarter_table_sha256=sha(table),unit_matrix_sha256=sha(unit),parameter_row_sha256=sha(image.read(0x9154e8,132)),
        source_constants={hex(a):image.read(a,4).hex() for a in CONSTANTS},
        control_words=[],mismatches=[],limits=['No original behavior of the five rotation/reset callees is established by controlled boundaries','Fast yaw executes actual original/native heading and both matrix transforms','Matrix/input/physics/gravity/collision state is explicit seeded state, not observed running-game state','No Windows startup, mods, physical input, collision response or full gameplay','Unicorn x87 flags are weaker than hardware; no native approximation is introduced to match them'])
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        original=GroundOriginal(image,cw,unit);item=dict(cw=hex(cw),cases=len(fixtures),value_mismatches=0,call_mismatches=0,flag_mismatches=0,cw_mismatches=0,actual_fast_yaw_cases=0,cases_without_controlled_callees=0)
        for index,w in enumerate(fixtures):
            values,calls,flags,restored=original.run(w,cw);out=(ctypes.c_uint32*43)();status=probe(tb,4100,ub,64,cb,24,(ctypes.c_uint32*63)(*w),63,cw,out,43)
            item['actual_fast_yaw_cases']+=0x443df0 in calls
            item['cases_without_controlled_callees']+=not any(address in CONTROLLED for address in calls)
            if status or out[12]:raise RuntimeError(f'Native rejected case{index}: status{status},failure{out[12]},boundary{out[13]:#x}')
            differences=[]
            actual_values=list(out[:12])+list(out[40:43])
            for label,a,b in [('value',values,actual_values),('call',calls,list(out[18:18+out[17]])),('flag',flags,out[15]),('cw',restored,out[16])]:
                if a!=b:item[label+'_mismatches']+=1;differences.append(label)
            if differences:report['mismatches'].append(dict(cw=hex(cw),case=index,fields=differences,input=list(map(hex,w)),original=list(map(hex,values)),native=list(map(hex,actual_values)),calls=[list(map(hex,calls)),list(map(hex,out[18:18+out[17]]))],flags=[hex(flags),hex(out[15])]))
        report['control_words'].append(item);print(json.dumps(item),flush=True)
    report['local_values_and_call_selections_match']=all(not p['value_mismatches'] and not p['call_mismatches'] and not p['cw_mismatches'] for p in report['control_words']);args.output.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['local_values_and_call_selections_match'] else 1
if __name__=='__main__':raise SystemExit(main())
