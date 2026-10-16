"""Original Sonic control prefix and explicitly seeded local-speed suffix.

No predicate, animation, task initializer or model owner is faked. Actual
mandatory call instructions terminate control fixtures before execution.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn.x86_const import *
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TASK=DATA+0x100;PLAYER=DATA+0x300;WORLD=DATA+0x600;OBJECT=DATA+0x700
COLLISION=DATA+0x800;INFO=DATA+0x900;OWNER=DATA+0xa00;ACTOR=DATA+0xb00
CONSTANTS=(0x7dcb2c,0x7dfff0,0x7dd554,0x7e0090,0x7dd57c,0x7df1bc,0x7e01a8,0x7dcb94,0x7e0844,0x7df1c8,0x7e00f4,0x7dcb34,0x7dd550)
def sha(b):return hashlib.sha256(b).hexdigest()
class SonicOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f)
        for page in (0x3abd000,0x3b29000,0x3b22000,0x3b0f000,0x3c55000):self.machine.mem_map(page,0x1000)
        self.table=struct.unpack('<88I',image.read(0x49bfbc,352));self.boundary=0
        def stop(m,a,size,user):
            if a in self.stops:self.boundary=a;m.emu_stop()
        self.machine.hook_add(unicorn.UC_HOOK_CODE,stop)
    def run(self,f):
        m=self.machine;self.boundary=0
        def put(a,fmt,*v):m.mem_write(a,struct.pack(fmt,*v))
        m.mem_write(TASK,bytes(64));m.mem_write(PLAYER,bytes(512));m.mem_write(WORLD,bytes(64))
        put(TASK,'BB',f[3]&255,0);put(TASK+2,'B',f[4]&255);put(TASK+4,'HH',f[2]&65535,f[5]&65535)
        put(TASK+0x14,'3I',*f[14:17]);put(TASK+0x38,'I',COLLISION if f[30] else 0)
        put(PLAYER+4,'HH',f[7]&65535,f[6]&65535);put(PLAYER+0x7e,'H',f[8]&65535)
        put(PLAYER+0x5c,'II',f[9],f[10]);put(PLAYER+0x38,'3I',0x42a20000,0x42a40000,0x42a60000);put(PLAYER+0x1c,'I',0x42a80000)
        put(PLAYER+0xd8,'I',f[20]);put(PLAYER+0x50,'3I',*f[21:24]);put(0x3b0f0f8,'3I',*f[24:27])
        put(WORLD,'I',PLAYER if f[35] else 0);put(WORLD+4,'3I',*f[11:14]);put(WORLD+0x1c,'3I',*f[17:20])
        put(OBJECT+0x20,'II',TASK,WORLD);put(COLLISION+0xc,'I',INFO);put(COLLISION+0x9c,'I',OWNER);put(OWNER+0x20,'I',ACTOR)
        put(ACTOR+0x14,'I',f[33]);put(ACTOR+0x1c,'I',f[34]);put(INFO+3,'B',f[31]&255);put(INFO+0x34,'I',f[32])
        put(0x3b22dcc,'H',f[27]&65535);put(0x3b29d0c,'H',f[28]&65535);put(0x3b29d08,'H',f[29]&65535)
        put(0x3abdf60,'II',0xaaaaaaaa,0xbbbbbbbb);put(0x3c55d45,'B',0xcc)
        m.reg_write(UC_X86_REG_FPCW,f[1]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        if f[0]==0:
            put(STACK,'9I',0,0,0,*f[11:14],STOP,0,0);entry=0x43eecc;self.stops=set()
            m.reg_write(UC_X86_REG_EBP,PLAYER);m.reg_write(UC_X86_REG_ESI,TASK);m.reg_write(UC_X86_REG_EDI,WORLD)
        elif f[0]==1:
            put(STACK,'4I',STOP,TASK,WORLD,PLAYER if f[35] else 0);entry=0x496f50
            self.stops={0x497025,0x492da6,0x496f70}
        else:
            put(STACK,'II',STOP,OBJECT);entry=0x49a9b0
            self.stops=set(self.table)|{0x497025,0x492da6,0x496f70}
        m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(entry,STOP,count=5000)
        def get(a,fmt):return struct.unpack(fmt,m.mem_read(a,struct.calcsize(fmt)))
        out={2:get(TASK+4,'H')[0],3:get(TASK,'B')[0],4:get(TASK+6,'H')[0],5:get(PLAYER+6,'H')[0],6:get(PLAYER+0x7e,'H')[0],
             16:get(PLAYER+0x1c,'I')[0],17:m.reg_read(UC_X86_REG_FPSW)&63,18:m.reg_read(UC_X86_REG_FPCW),23:get(INFO+3,'B')[0],24:get(INFO+0x34,'I')[0]}
        for i,v in enumerate(get(PLAYER+0x38,'3I')):out[7+i]=v
        for i,v in enumerate(get(TASK+0x14,'3I')):out[10+i]=v
        for i,v in enumerate(get(WORLD+0x1c,'3I')):out[13+i]=v
        if f[0]==2:
            out[19]=int(get(0x3abdf60,'I')[0]==TASK);out[20]=int(get(0x3abdf64,'I')[0]==(PLAYER if f[35] else 0))
            val=get(0x3c55d45,'B')[0];out[21]=int(val!=0xcc);out[22]=val if val!=0xcc else 0
            out[25]=self.boundary if self.boundary in self.table else 0
        return out,self.boundary

class GroundOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f)
        for page in (0x3b0e000,0x3b22000,0x3b2c000,0x3b3c000):self.machine.mem_map(page,0x1000)
        self.boundary=0;self.stops={0x44bb98,0x44bbd7,0x44bbed,0x44bd5a,0x44bc19}
        def stop(m,a,size,user):
            if a in self.stops:self.boundary=a;m.emu_stop()
        self.machine.hook_add(unicorn.UC_HOOK_CODE,stop)
    def run(self,f):
        m=self.machine;self.boundary=0
        def put(a,fmt,*v):m.mem_write(a,struct.pack(fmt,*v))
        m.mem_write(TASK,bytes(64));m.mem_write(PLAYER,bytes(512));m.mem_write(WORLD,bytes(64))
        put(TASK+4,'H',f[1]&65535);put(TASK+0x18,'I',f[12]);put(TASK+0x3c,'I',DATA+0xc00)
        put(DATA+0xc24,'B',f[2]&255);put(PLAYER+0x5c,'I',f[8]);put(PLAYER+0x38,'I',f[9]);put(PLAYER+0xa,'H',f[5]&65535)
        put(WORLD+0x20,'I',f[10]);put(0x3b22dcc,'H',f[6]&65535);put(0x3b22dec,'H',f[7]&65535);put(0x3b22dc0,'H',0)
        put(0x3b0e7a0,'II',f[4],f[3]);put(0x3b3cdf0,'I',PLAYER);put(0x3b2cbb0,'I',DATA+0xd00);put(DATA+0xd18,'I',f[11])
        put(STACK,'4I',STOP,TASK,WORLD,PLAYER);m.reg_write(UC_X86_REG_ESP,STACK)
        m.reg_write(UC_X86_REG_FPCW,f[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(0x44bb60,STOP,count=1000)
        return [struct.unpack('<I',m.mem_read(WORLD+0x20,4))[0],m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW),f[1]&65535],self.boundary

def ground_fixtures(cw):
    for flags in (0,1,3,0x100,0x2000,0x4000,0x6000):
        for event in (0,1):
            for magnitude in (0,0x80000000,1,0x3f000000,0x7fc12345,0x7f812345):
                for timer in (0,1):
                    for attributes in (0,0x20000):
                        yield [cw,flags,event,magnitude,0x12345678,timer,0,0,attributes,0x7f812345,0x87654321,0x76543210,0xabcdef00]

def fixtures():
    base=[0,0x37f,0,0,2,0,0,0,0,0,0,0x3dcccccd,0x3f800000,0xbdcccccd,0xfedc1234,0x87654321,0xdeadbeef,0x12345678,0xffeeccaa,0xaaaabbbb,0x3dcccccd,0,0x3f800000,0,0,0xbf800000,0,0,0,0,1,0xac,0x12345678,0xffff4321,0xabcdef00,1]
    for flags in (0,1,2,3):
        for prior in (0,4,8,12):
            for attrs in (0,0x4000):
                for bits in (0,0x80000000,1,0x80000001,0x3dcccccd,0xbdcccccd,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
                    f=base.copy();f[2]=flags;f[9]=attrs;f[10]=prior;f[11]=bits;f[13]=bits;yield f
    rng=random.Random(0x49a9b0)
    for _ in range(500):
        f=base.copy();f[2]=rng.randrange(4);f[9]=rng.choice((0,0x4000));f[10]=rng.randrange(16)
        f[11]=struct.unpack('<I',struct.pack('<f',rng.uniform(-2,2)))[0];f[13]=struct.unpack('<I',struct.pack('<f',rng.uniform(-2,2)))[0];yield f
    for action in range(256):
        for presence in (0,1):
            f=base.copy();f[0]=1;f[3]=action;f[35]=presence;yield f
    for flags in (0,4,0x8000,0xffff):
        for taskid in (0,2,255):
            for timer in (0,1,65535):
                for pflags in (0,0x8000):
                    f=base.copy();f[0]=1;f[2]=flags;f[4]=taskid;f[5]=timer;f[6]=pflags;yield f
    for action in range(256):
        for forced in (0,1):
            f=base.copy();f[0]=2;f[3]=action;f[28]=forced;yield f
    for action in (6,44,45,72,86,87,255):
        for flags in (0,0x100,0x20,0x120):
            for pflags,equipment in ((0,0),(0x8000,0),(0,0x8000)):
                f=base.copy();f[0]=2;f[3]=action;f[2]=flags;f[6]=pflags;f[7]=equipment;f[28]=int(action!=86);f[8]=65535;f[12]=0xbf800000;yield f

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args()
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    cb=b''.join(image.read(a,4) for a in CONSTANTS);args.output.with_suffix('.constants.bin').write_bytes(cb)
    args.output.with_suffix('.dispatch.bin').write_bytes(image.read(0x49bfbc,352))
    lib=ctypes.CDLL(str(args.native_library.resolve()));probe=lib.sadx_native_sonic_player;probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    oracle=SonicOriginal(image);report=dict(source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),profile='actual Sonic control prefix plus explicitly seeded post-transform43eecc suffix',cases=0,by_entry=[0,0,0],stored_mismatches=0,status_mismatches=0,examples=[],stored_examples=[],source_ranges={hex(a):dict(size=n,sha256=sha(image.read(a,n))) for a,n in ((0x49a9b0,0x160c),(0x496f50,0x3900),(0x43ee70,0x191),(0x4915f0,0x69),(0x44bb60,0x20a))},constants=dict(addresses=list(map(hex,CONSTANTS)),bytes=cb.hex(),sha256=sha(cb)),dispatch_sha256=sha(image.read(0x49bfbc,352)),limits=['No full Sonic action, animation, model, sound, collision or task initialization is executed as a game','Control comparisons stop before mandatory callbacks or at the remaining predicate jump; those paths are incomplete','Ground-input cases execute actual GetAnalog and stop at mandatory rotation/raw-input owners; their matrix resources are not consumed',
            'Local-speed cases seed the transformed stack vector at43eecc; they do not establish the producer43ed20/43ec00 equivalence','Unicorn is an instruction oracle; physical Windows control/status and hardware equivalence remain unverified','Native full-angle unavailable low16 stops before the high16 source write; known fixtures cover fully captured angles'])
    root=pathlib.Path(__file__).resolve().parents[2];report['native_sources']={str(name):sha((root/name).read_bytes()) for name in map(pathlib.Path,('src/player/sonic_player.hpp','src/player/sonic_player.cpp','tests/probes/native_sonic_player_probe.cpp','tools/oracles/verify_original_sonic_player.py'))}
    report['native_sources'].update({name:sha((root/name).read_bytes()) for name in ('src/player/ground_acceleration.hpp', 'src/player/ground_acceleration.cpp', 'src/player/ground_acceleration_internal.hpp', 'src/player/ground_acceleration_native.cpp', 'src/player/ground_arithmetic.hpp', 'src/player/ground_friction.cpp', 'src/guest/ground_acceleration_reference.hpp', 'src/guest/player_acceleration_ground.cpp', 'src/guest/player_acceleration_friction.cpp', 'src/player/player_acceleration_ground.hpp', 'src/player/player_acceleration_friction.hpp', 'src/cpu/fidelity_float.hpp')})
    start=time.monotonic();digest=hashlib.sha256();buf=(ctypes.c_uint8*len(cb)).from_buffer_copy(cb)
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        for f in fixtures():
            f[1]=cw;digest.update(struct.pack('<36I',*f));expected,boundary=oracle.run(f);out=(ctypes.c_uint32*40)()
            if probe(buf,len(cb),(ctypes.c_uint32*36)(*f),out):raise RuntimeError('Native probe rejected source fixture')
            for field,value in expected.items():
                if out[field]!=value:
                    key='status_mismatches' if field==17 else 'stored_mismatches';report[key]+=1
                    if len(report['examples'])<30:report['examples'].append(dict(case=report['cases'],entry=f[0],field=field,source=value,native=out[field],boundary=hex(boundary),input=f))
            report['cases']+=1;report['by_entry'][f[0]]+=1
    ground=GroundOriginal(image);gp=lib.sadx_native_sonic_ground_input;gp.argtypes=probe.argtypes;gp.restype=ctypes.c_int
    report['ground_prefix_cases']=0;report['ground_prefix_stored_mismatches']=0;report['ground_prefix_status_mismatches']=0
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        for f in ground_fixtures(cw):
            expected,boundary=ground.run(f);out=(ctypes.c_uint32*8)()
            digest.update(struct.pack('<13I',*f))
            if gp(buf,len(cb),(ctypes.c_uint32*13)(*f),out):raise RuntimeError('Native ground prefix rejected fixture')
            for i,value in enumerate(expected):
                if out[3+i]!=value:
                    key='status_mismatches' if i==1 else 'stored_mismatches';report[key]+=1;report['ground_prefix_'+key]+=1
                    if i!=1 and len(report['stored_examples'])<30:report['stored_examples'].append(dict(ground_input=f,field=i,source=value,native=out[3+i],boundary=hex(boundary)))
                    if len(report['examples'])<30:report['examples'].append(dict(ground_input=f,field=i,source=value,native=out[3+i],boundary=hex(boundary)))
            if out[1]!=boundary:
                report['stored_mismatches']+=1;report['ground_prefix_stored_mismatches']+=1
                if len(report['stored_examples'])<30:report['stored_examples'].append(dict(ground_input=f,field='boundary',source=hex(boundary),native=hex(out[1])))
                if len(report['examples'])<30:report['examples'].append(dict(ground_input=f,field='boundary',source=hex(boundary),native=hex(out[1])))
            report['cases']+=1;report['ground_prefix_cases']+=1
    report.update(case_sha256=digest.hexdigest(),seconds=time.monotonic()-start,stored_state_matches=report['stored_mismatches']==0,passed=report['stored_mismatches']==0 and report['status_mismatches']==0)
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ('cases','by_entry','stored_mismatches','status_mismatches','seconds','passed')}));return int(not report['stored_state_matches'])
if __name__=='__main__':raise SystemExit(main())
