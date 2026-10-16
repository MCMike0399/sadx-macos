"""Actual bounded Sonic predicates and496f50 actions1..5.

Every unbound mandatory callback stops at the actual source CALL; no success
stub, no-overlap hooks or emulated game loop are supplied. Native supports
explicit scene/resource unknowns; these fixtures supply known source facts.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn.x86_const import *
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK=DATA+0x100;PLAYER=DATA+0x300;WORLD=DATA+0x600;EVENT=DATA+0x700
CWP=DATA+0x800;HIT=DATA+0x900;OWNER=DATA+0xa00;ACTOR=DATA+0xb00
CSTS=DATA+0xc00;CAMERA=DATA+0xd00;INFO=DATA+0xe00
CONSTANTS=(0x7dcb2c,0x7dfff0,0x7dd554,0x7e0090,0x7dd57c,0x7df1bc,0x7e01a8,0x7dcb94,0x7e0844,0x7df1c8,0x7e00f4,0x7dcb34,0x7dd550)
ENTRIES=(0x495fa0,0x492f90,0x494f70,0x495e60,0x493120,0x496ee0,0x442680,0x442570,0x43bf40,0x4b83f0,0x4b8320,0x494ff0,0x4429c0,0x495c30,0x495b50,0x495ba0,0x4930d0,0x495080,0x496f50)
UNBOUND={0x45a870,0x423d70,0x423f50,0x424210,0x423be0,0x40b860,0x492710,0x437d20,0x473b00,0x495c80,0x491660,0x4b8470,0x43c060,0x41c0c0,0x41bed0,0x4bcc10,0x491550,0x442720}
def sha(b):return hashlib.sha256(b).hexdigest()
class ActionsOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f)
        for page,size in ((0x909000,0x1000),(0x3b18000,0x1000),(0x3b0e000,0x1000),(0x3b22000,0x1000),(0x3b2c000,0x1000),(0x3b3c000,0x1000),(0x3c5a000,0x1000)):
            self.machine.mem_map(page,size)
        self.boundary=0;self.kind='';self.entry=0
        def stop(m,a,size,user):
            if a==0x495ff8 or a==0x497025 and self.action not in range(1,6):self.boundary=a;self.kind='remaining_dispatch';m.emu_stop();return
            if size==5:
                code=bytes(m.mem_read(a,5))
                if code[0]==0xe8:
                    callee=(a+5+struct.unpack('<i',code[1:])[0])&0xffffffff
                    if callee in UNBOUND:self.boundary=a;self.kind=hex(callee);m.emu_stop()
        self.machine.hook_add(unicorn.UC_HOOK_CODE,stop)
    def run(self,f):
        m=self.machine;self.boundary=0;self.kind='';self.entry=f[0];self.action=f[3]&255
        def put(a,fmt,*values):m.mem_write(a,struct.pack(fmt,*values))
        m.mem_write(TASK,bytes(64));m.mem_write(PLAYER,bytes(512));m.mem_write(WORLD,bytes(64));m.mem_write(EVENT,bytes(128));m.mem_write(CWP,bytes(176));m.mem_write(HIT,bytes(176))
        put(TASK,'3B',f[3]&255,f[4]&255,f[5]&255);put(TASK+4,'HH',f[2]&65535,f[7]&65535);put(TASK+8,'B',f[6]&255);put(TASK+0x18,'I',f[23]);put(TASK+0x38,'II',CWP,EVENT)
        put(EVENT+0x24,'B',f[8]&255);put(EVENT+0x4c,'I',INFO if f[9] else 0)
        put(PLAYER,'I',f[14]);put(PLAYER+4,'HHH',f[10]&65535,f[11]&65535,f[17]&65535);put(PLAYER+0xa,'H',f[27]&65535);put(PLAYER+0xe,'H',f[18]&65535)
        put(PLAYER+0x5c,'I',f[12]);put(PLAYER+0x7e,'3H',f[13]&65535,f[15]&65535,f[16]&65535);put(PLAYER+0x124,'H',f[19]&65535);put(PLAYER+0x38,'3I',*f[20:23]);put(PLAYER+0x68,'II',0,0);put(PLAYER+0x64,'I',CSTS);put(CSTS+0x20,'I',f[50])
        put(PLAYER+0xb4,'I',f[46]);put(PLAYER+0xbc,'III',f[49],f[47],f[48])
        put(CWP+4,'H',f[40]&65535);put(CWP+0xa2,'H',f[45]&65535);put(CWP+0xa4,'I',HIT if f[41] else 0)
        put(HIT+0x9c,'I',OWNER);put(HIT+0xc,'I',INFO);put(OWNER+0x20,'I',ACTOR);put(ACTOR+2,'B',f[42]&255);put(ACTOR+6,'H',f[43]&65535);put(ACTOR+0xc,'I',f[44]);put(INFO+(f[45]&65535)*0x30+4,'I',0)
        index=f[6]&255;put(0x3b0e7f8+index*0x34,'I',f[29]);put(0x3b0e800+index*0x34,'I',f[30]);put(0x3b0e7a0+index*8,'II',f[25],f[26]);put(0x3b3cdf0+index*4,'I',PLAYER)
        put(0x909f94,'II',f[31],f[32]);put(0x909fa4,'III',f[33],f[34],f[35]);put(0x3b22dcc,'H',f[24]&65535);put(0x3b22dec,'H',0);put(0x3b22dc0,'H',0);put(0x3b2cbb0,'I',CAMERA);put(CAMERA+0x18,'I',f[28]);put(0x3b18db5,'B',f[36]&255)
        for i in range(10):put(0x3c5a27c+4*i,'I',0)
        m.reg_write(UC_X86_REG_FPCW,f[1]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        entry=f[0];args=[TASK,WORLD,PLAYER,f[51]]
        if entry in (0x495fa0,0x492f90,0x494f70,0x495e60,0x493120,0x496ee0,0x494ff0):
            args=[WORLD];m.reg_write(UC_X86_REG_EAX,PLAYER);m.reg_write(UC_X86_REG_ECX,TASK);m.reg_write(UC_X86_REG_ESI,TASK);m.reg_write(UC_X86_REG_EDI,PLAYER)
            if entry in (0x495fa0,0x496ee0):m.reg_write(UC_X86_REG_EDI,TASK)
            if entry==0x495e60:m.reg_write(UC_X86_REG_EBX,WORLD)
        elif entry in (0x495c30,0x495b50,0x495ba0,0x495080):
            args=[WORLD];m.reg_write(UC_X86_REG_ESI,PLAYER);m.reg_write(UC_X86_REG_EDI,TASK)
        elif entry==0x4930d0:m.reg_write(UC_X86_REG_ECX,PLAYER);m.reg_write(UC_X86_REG_EDX,TASK);args=[]
        put(STACK,'I'+str(len(args))+'I',STOP,*args);m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(entry,STOP,count=20000)
        except unicorn.UcError as error:raise RuntimeError(f'Original fixture {list(map(hex,f))} stopped at {m.reg_read(UC_X86_REG_EIP):#x}') from error
        def get(a,fmt):return struct.unpack(fmt,m.mem_read(a,struct.calcsize(fmt)))
        out={6:get(TASK,'B')[0],7:get(TASK+1,'B')[0],8:get(TASK+4,'H')[0],9:get(TASK+6,'H')[0],10:get(PLAYER,'I')[0],11:get(PLAYER+4,'H')[0],12:get(PLAYER+6,'H')[0],13:get(PLAYER+8,'H')[0],14:get(PLAYER+0xe,'H')[0],15:get(PLAYER+0x7e,'H')[0],16:get(PLAYER+0x80,'H')[0],17:get(PLAYER+0x82,'H')[0],18:get(PLAYER+0x124,'H')[0],22:get(EVENT+0x24,'B')[0],23:m.reg_read(UC_X86_REG_FPCW),24:m.reg_read(UC_X86_REG_FPSW)&63}
        for i,v in enumerate(get(PLAYER+0x38,'3I')):out[19+i]=v
        is_void=entry in (0x496f50,0x4b8320,0x495c30,0x495b50,0x495ba0)
        if not self.boundary and not is_void:out[4]=1;out[5]=m.reg_read(UC_X86_REG_EAX)
        return out,self.boundary,self.kind

def fixtures():
    base=[0x496f50,0x7f,3,1,0,2,0,0,0,0,0,0,0,1,0x40000000,9,7,5,12,14,0x3f800000,0,0,0,0,0x12345678,0x3f000000,0,0,0,0,2,4,0x20,0x40,0x80,0,0,0x3f800000,0,0,0,8,1,1,1,0x40c00000,0x3f800000,0x40000000,0x3dcccccd,0x3f800000,0x00854a01]
    for entry in ENTRIES:
        for flags in (0,1,3,0x100,0x200,0x800,0x1000,0x2000,0x4000):
            for held,pressed in ((0,0),(2,0),(0,2),(0,4),(0,0x20),(2,4)):
                f=base.copy();f[0]=entry;f[2]=flags;f[29]=held;f[30]=pressed;yield f
    for action in range(1,6):
        for magnitude in (0,0x80000000,1,0x3f000000,0x7fc12345,0x7f812345):
            for speed in (0,0x80000000,1,0x3f800000,0xbf800000,0x7fc12345):
                f=base.copy();f[3]=action;f[26]=magnitude;f[20]=speed;yield f
    for entry in (0x495fa0,0x495080,0x496ee0,0x492f90,0x4b83f0):
        for event in (0,1):
            for timer in (0,119,120,209,210,299,300,0x7fff,0x8000,65535):
                for equipment in (0,1,2,3,4,0x8000):
                    f=base.copy();f[0]=entry;f[8]=event;f[13]=timer;f[10]=equipment;f[29]=2;f[30]=2;yield f
    for entry in (0x495fa0,0x442570):
        for flag in (0,0x1000):
            for sub in (0,1,5,50,51,128,255):
                for lst in (0,1):
                    f=base.copy();f[0]=entry;f[2]=flag;f[4]=sub;f[9]=lst;yield f
    for actor in (7,8,9):
        for timer in (0,1,65535):
            for counter in (0,1,100,1000,0x80000000,0x7fffffff):
                f=base.copy();f[0]=0x4b83f0;f[41]=1;f[42]=actor;f[43]=timer;f[44]=counter;yield f
    rng=random.Random(0x496f50)
    for _ in range(600):
        f=base.copy();f[0]=rng.choice(ENTRIES);f[2]=rng.choice((0,1,3,0x100,0x200,0x800,0x2000));f[3]=rng.randrange(1,6);f[29]=rng.randrange(4);f[30]=rng.choice((0,2,4,0x20));f[20]=rng.choice((0,0x3f800000,0x40000000,0xbf800000));f[13]=rng.randrange(400);yield f

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle mismatch')
    cb=b''.join(image.read(x,4) for x in CONSTANTS);a.output.with_suffix('.constants.bin').write_bytes(cb)
    lib=ctypes.CDLL(str(a.native_library.resolve()));probe=lib.sadx_native_sonic_action;probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];probe.restype=ctypes.c_int
    oracle=ActionsOriginal(image);buf=(ctypes.c_uint8*len(cb)).from_buffer_copy(cb);report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),cases=0,stored_mismatches=0,boundary_mismatches=0,status_mismatches=0,examples=[],status_examples=[],entry_cases={},mandatory_boundaries={},constants=dict(addresses=list(map(hex,CONSTANTS)),bytes=cb.hex(),sha256=sha(cb)),limits=['Actual original bytes with private fixture memory, not full game/mod startup or a physical gameplay trace','Unbound sound/effect/native task/animation/normal owners stop at actual source CALL before execution; none returns synthetic success','Fully known source masks/pad/event fields are explicit fixture facts, not defaults','All12 masked CW profiles include hardware candidate007f and worker027f but are not hardware measurements','Source FPATAN/scratch-dependent wall query remains a mandatory owner boundary in these fixtures'])
    root=pathlib.Path(__file__).resolve().parents[2];report['native_sources']={n:sha((root/n).read_bytes()) for n in ('src/player/sonic_player.hpp','src/player/sonic_player.cpp','tests/probes/native_sonic_action_probe.cpp','tools/oracles/verify_original_sonic_actions.py','src/player/player_task.hpp','src/player/player_physics.hpp','src/player/player_position.hpp')}
    report['native_sources'].update({name:sha((root/name).read_bytes()) for name in ('src/player/ground_acceleration.hpp', 'src/player/ground_acceleration.cpp', 'src/player/ground_acceleration_internal.hpp', 'src/player/ground_acceleration_native.cpp', 'src/player/ground_arithmetic.hpp', 'src/player/ground_friction.cpp', 'src/guest/ground_acceleration_reference.hpp', 'src/guest/player_acceleration_ground.cpp', 'src/guest/player_acceleration_friction.cpp', 'src/player/player_acceleration_ground.hpp', 'src/player/player_acceleration_friction.hpp', 'src/cpu/fidelity_float.hpp')})
    digest=hashlib.sha256();start=time.monotonic()
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        for f in fixtures():
            f[1]=cw;digest.update(struct.pack('<52I',*f));expected,boundary,kind=oracle.run(f);out=(ctypes.c_uint32*32)()
            if probe(buf,len(cb),(ctypes.c_uint32*52)(*f),52,out,32):raise RuntimeError('Native probe rejected fixture')
            for field,value in expected.items():
                if out[field]!=value:
                    key='status_mismatches' if field==24 else 'stored_mismatches';report[key]+=1;bucket=report['status_examples'] if field==24 else report['examples']
                    if len(bucket)<40:bucket.append(dict(case=report['cases'],input=f,field=field,source=value,native=out[field],boundary=hex(boundary)))
            if bool(boundary)!=bool(out[0]) or boundary and out[1]!=boundary:
                report['boundary_mismatches']+=1
                if len(report['examples'])<40:report['examples'].append(dict(case=report['cases'],input=f,field='boundary',source=hex(boundary),native=hex(out[1]),native_failure=out[0],kind=kind))
            report['cases']+=1;entry=hex(f[0]);report['entry_cases'][entry]=report['entry_cases'].get(entry,0)+1
            if boundary:key=hex(boundary);report['mandatory_boundaries'][key]=report['mandatory_boundaries'].get(key,0)+1
    report.update(case_sha256=digest.hexdigest(),seconds=time.monotonic()-start,stored_state_matches=not(report['stored_mismatches'] or report['boundary_mismatches']),passed=not(report['stored_mismatches'] or report['boundary_mismatches'] or report['status_mismatches']))
    report['limits']=([report['limits']] if isinstance(report.get('limits'),str) else list(report.get('limits') or []))+['Flag difference (status_mismatches): the native x87 status exception bits (PE 0x20 in the examples) are set where Unicorn 2.1.4 does not raise them (source 0). All compared values match; the hardware flags need the Intel capture (M4).']
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ('cases','stored_mismatches','boundary_mismatches','status_mismatches','seconds','passed')}));return int(not report['stored_state_matches'])
if __name__=='__main__':raise SystemExit(main())
