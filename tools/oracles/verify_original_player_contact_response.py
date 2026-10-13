"""Compare the five actual contact-response bodies and native owners.

Original numeric callees/CRT/sine table execute without overlays. Fixtures are
explicit contact payloads and policy1, not an initialized scene or Windows run.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDI,
    UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG)
from extract import SOURCE_SHA256,TABLE_ADDRESS
from pe_image import PEImage
from verify_original_math import Original,DATA,STACK,STOP
ENTRY=(0x43cce0,0x43cf90,0x43d180,0x43d8b0,0x43d360)
END=(0x43cf89,0x43d178,0x43d356,0x43dc99,0x43d8ad)
TASK=DATA+0x100;WORLD=DATA+0x200;PLAYER=DATA+0x300;CONTACT=DATA+0x600
CRT=0x3c837a8;RESPONSES=0x3b47008;GRAVITY=0x3b0f0f8
def sha(b):return hashlib.sha256(b).hexdigest()
def fw(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def vector(v):return list(map(fw,v))
def payload(normal,point):return [0,0,*vector(normal),*vector(normal),*vector(point),*vector(point),*vector((0,0,0)),*vector((0,0,0)),0,0]

class ResponseOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f)
        for a in (0x2bcc000,0x3c83000,0x3b47000,0x3b0f000):self.machine.mem_map(a,0x1000)
        self.machine.mem_write(0x2bcc000,image.read(0x2bcc000,0x1000))
    def run(self,kind,w,responses,cw):
        m=self.machine
        def words(at,v):m.mem_write(at,struct.pack('<'+str(len(v))+'I',*v))
        m.mem_write(CONTACT,bytes(0x84));words(CONTACT+4,w[22:25]);words(CONTACT+0x1c,w[25:28]);words(CONTACT+0x10,w[28:31])
        words(CONTACT+0x60,w[31:34]);words(CONTACT+0x6c,w[34:37]);words(CONTACT+0x78,w[37:40])
        m.mem_write(CONTACT+0x28,struct.pack('<H',w[0]));words(RESPONSES,[word for r in responses for word in r])
        words(WORLD+4,w[16:19]);words(WORLD+0x20,[w[6]]);words(PLAYER+0x38,w[13:16]);words(PLAYER+0x44,w[19:22])
        words(PLAYER+0x60,[w[4]]);words(PLAYER+0xc4,[w[7]]);words(PLAYER+0xc0,[w[8]]);words(PLAYER+0xcc,[w[9]])
        words(TASK+0x18,[w[5]]);words(GRAVITY,w[10:13]);words(CRT,[w[3]])
        words(STACK,[STOP,TASK,WORLD,PLAYER,w[1]]);m.reg_write(UC_X86_REG_ESP,STACK)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EDI,UC_X86_REG_ESI):m.reg_write(reg,CONTACT)
        m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(ENTRY[kind],STOP,count=100000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Source contact-response ABI mismatch')
        def read(at,n=3):return list(struct.unpack('<'+str(n)+'I',m.mem_read(at,n*4)))
        return [m.reg_read(UC_X86_REG_EAX),m.reg_read(UC_X86_REG_FPCW),m.reg_read(UC_X86_REG_FPSW)&63,
            *read(CONTACT+0x10),*read(CONTACT+0x6c),*read(WORLD+4),*read(PLAYER+0x44),*read(TASK+0x18,1),*read(WORLD+0x20,1)]

def fixtures():
    base=[1,0,1,1,0,0x1000,0x5000,fw(2),fw(1),fw(4),*vector((0,-1,0)),
        *vector((3,0,0)),*vector((-5,1,2)),*vector((0,1,0)),*vector((0,0,0)),
        *vector((0,1,0)),*vector((-4,0,0)),*vector((0,0,0)),*vector((-3,1,2)),*vector((.5,0,-.25))]
    yield base.copy()[:0]+[0]+base[1:],[payload((1,0,0),(1,0,0))]
    normals=((1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1),(.6,.8,0))
    for normal in normals:
        for distance in (.0,.25,1.,5.,25.):
            for displacement in ((-4,0,0),(4,0,0),(0,-4,0),(0,4,0),(0,0,-4),(-4,2,1),(0,0,0)):
                for count in (1,2,16):
                    w=base.copy();w[0]=count;w[28:31]=vector(displacement)
                    yield w,[payload(normal,(distance,0,0)) for _ in range(count)]
    rng=random.Random(0x43cce0)
    for _ in range(250):
        w=base.copy();w[0]=rng.choice((1,2,3,16));w[1]=rng.choice((0,0x4000,0x800000));w[4]=rng.choice((0,0x4000))
        w[13:19]=vector((rng.uniform(-7,7),0,0))+vector(tuple(rng.uniform(-7,7) for _ in range(3)))
        w[28:31]=vector(tuple(rng.uniform(-6,6) for _ in range(3)));w[34:37]=vector(tuple(rng.uniform(-6,6) for _ in range(3)))
        yield w,[payload(rng.choice(normals),tuple(rng.uniform(-3,3) for _ in range(3))) for _ in range(w[0])]
    for word in (0,0x80000000,1,0x80000001,0x7f800000,0xff800000,
                 0x7fc12345,0xffc54321,0x7f812345,0xff854321,0x7f7fffff):
        for field in (25,28,16):
            w=base.copy();w[field]=word;yield w,[payload((1,0,0),(1,0,0))]
        w=base.copy();r=payload((1,0,0),(1,0,0));r[5]=word;yield w,[r]

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path)
    p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args();image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    addresses=((0x7dcb2c,4),(0x7dcb10,4),(0x7dd570,4),(0x7e0080,4),(0x7dd58c,4),(0x7dcb0c,4),(0x7e6c30,4),(0x7dccd0,8),(0x7dccc8,8),(0x7e0b40,8))
    constants=b''.join(image.read(a,n) for a,n in addresses);sine=image.read(TABLE_ADDRESS,4100)
    args.output.with_suffix('.constants.bin').write_bytes(constants)
    lib=ctypes.CDLL(str(args.native_library.resolve()));init=lib.sadx_native_contact_response_init
    init.argtypes=[ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t]
    if init((ctypes.c_float*1025).from_buffer_copy(sine),(ctypes.c_ubyte*52).from_buffer_copy(constants),52):raise RuntimeError('Native constants rejected')
    fn=lib.sadx_native_contact_response;fn.argtypes=[ctypes.c_uint,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
    cases=list(fixtures());oracle=ResponseOriginal(image);start=time.monotonic();root=pathlib.Path(__file__).resolve().parent.parent
    case_blob=b''.join(struct.pack('<40I',*w)+struct.pack('<I',len(rows))+b''.join(struct.pack('<22I',*row) for row in rows) for w,rows in cases)
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),
        profile='five actual source contact-response bodies with real callees, no overlays; explicit fixtures/policy1',
        source_functions={hex(a):dict(size=b-a,sha256=sha(image.read(a,b-a))) for a,b in zip(ENTRY,END)},
        constants=dict(bytes=constants.hex(),sha256=sha(constants),fields=[hex(a) for a,n in addresses]),
        cases_per_entry_and_cw=len(cases),case_sha256=sha(case_blob),
        native_sources={name:sha((root/name).read_bytes()) for name in ('src/player/player_contact_response.hpp','src/player/player_contact_response.cpp','tests/probes/native_player_contact_response_probe.cpp','tools/oracles/verify_original_player_contact_response.py')},
        native_shared_sources={name:sha((root/name).read_bytes()) for name in ('src/ninja/ninja_math.cpp','src/player/player_rotation.cpp','src/player/player_projection.cpp',
            'src/player/player_plane.cpp','src/player/player_acceleration_alternate_math.cpp','src/cpu/crt_acos.cpp','src/cpu/crt_asin.cpp',
            'src/cpu/crt_integer.cpp','src/cpu/x87_math.cpp','src/cpu/x87_transcendental.cpp','src/player/player_contact.hpp','src/player/player_mesh_query.hpp',
            'src/player/player_task.hpp','src/player/player_physics.hpp','src/player/player_acceleration.hpp')},
        profiles=[],mismatches=[],limits=['Explicit fixtures/policy1/12 CW profiles do not establish actual Windows startup/mods/scene state',
            'Unobserved contact/resource/scratch boundaries belong to owner tests; source initialization is not fabricated',
            'Bochs FPATAN model versus Unicorn host-double transcendental and status differences remain separate',
            'Actual Intel hardware, complete collision producers and physical controllers remain unverified'])
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        for kind in range(5):
            item=dict(entry=hex(ENTRY[kind]),cw=hex(cw),cases=len(cases),value_mismatches=0,flag_mismatches=0,flag_examples=[])
            for index,(w,responses) in enumerate(cases):
                out=(ctypes.c_uint32*64)();raw=[word for response in responses for word in response]
                status=fn(kind,cw,(ctypes.c_uint32*40)(*w),(ctypes.c_uint32*len(raw))(*raw),out)
                if status or out[0] or not out[3]:raise RuntimeError(f'Native rejected entry{kind} case{index}:status{status} failure{list(out[:7])}')
                expected=oracle.run(kind,w,responses,cw)
                actual=[out[4],out[5],out[6],*out[7:21]]
                if actual[:2]+actual[3:]!=expected[:2]+expected[3:]:
                    item['value_mismatches']+=1;report['mismatches'].append(dict(entry=hex(ENTRY[kind]),cw=hex(cw),case=index,source=expected,native=actual))
                if actual[2]!=expected[2]:
                    item['flag_mismatches']+=1
                    if len(item['flag_examples'])<6:item['flag_examples'].append(dict(case=index,source=hex(expected[2]),native=hex(actual[2])))
            report['profiles'].append(item);print(json.dumps({k:v for k,v in item.items() if k!='flag_examples'}),flush=True)
    report['stored_values_return_control_match']=all(not x['value_mismatches'] for x in report['profiles'])
    report['passed']=report['stored_values_return_control_match'] and all(not x['flag_mismatches'] for x in report['profiles'])
    report['seconds']=time.monotonic()-start;args.output.write_text(json.dumps(report,indent=2)+'\n')
    return int(not report['stored_values_return_control_match'])
if __name__=='__main__':raise SystemExit(main())
