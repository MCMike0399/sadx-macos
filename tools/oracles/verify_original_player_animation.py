"""Original44a800 numeric/control paths and43f230/446910, real source tails.

Typed source table fixture is separate from the actual PE resource decoder. Actual
43f9f0 runs with zero object count; The emulator stops before43f2a0; native diagnostics retain its real nested resource failure.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn.x86_const import *
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,DATA,STACK,STOP
STATE=DATA+0x100;TABLE=DATA+0x200;ACTION=DATA+0x300;MOTION=DATA+0x400
BUFFER=DATA+0x500;BMOTION=DATA+0x520;SPEED=DATA+0x600;WORK=DATA+0x604
CONSTANTS=(0x7dcb2c,0x7dcb10,0x7dcb7c)
def sha(b):return hashlib.sha256(b).hexdigest()
class AnimationOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f);self.boundary=0
        def stop(m,address,size,user):
            if address in (0x446996,0x4469a0):self.boundary=address;m.emu_stop()
        self.machine.hook_add(unicorn.UC_HOOK_CODE,stop)
    def run(self,f):
        m=self.machine;self.boundary=0
        def put(a,fmt,*v):m.mem_write(a,struct.pack(fmt,*v))
        m.mem_write(STATE,bytes(48));put(STATE,'7H',*[x&65535 for x in (f[0],0,f[1],f[2],f[3],f[4],f[5])]);put(STATE+0xe,'H',f[6]&65535);put(STATE+0x10,'II',f[7],f[8]);put(STATE+0x18,'4I',SPEED,WORK,TABLE,0);put(STATE+0x28,'I',BUFFER)
        put(BUFFER,'II',0x12340000,BMOTION);put(BMOTION,'I I H H',0,2,3,2);put(SPEED,'II',f[9],f[10])
        for i in range(4):
            put(TABLE+16*i,'I B b H I I',ACTION+8*i,0,(f[13+i]&255)-256 if f[13+i]&128 else f[13+i]&255,f[17+i]&65535,f[21+i],f[25+i]);put(ACTION+8*i,'II',0x12340000 if i<2 else 0x23450000,MOTION+12*i);put(MOTION+12*i,'I I H H',0,f[29+i],f[33+i]&65535,2)
        m.reg_write(UC_X86_REG_FPCW,f[11]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        put(STACK,'II',STOP,STATE);m.reg_write(UC_X86_REG_ESP,STACK)
        entry=(0x44a800,0x43f230,0x446910)[f[12]]
        if f[12]:m.reg_write(UC_X86_REG_ECX,STATE);m.reg_write(UC_X86_REG_EDX,f[1])
        m.emu_start(entry,STOP,count=10000)
        returned=m.reg_read(UC_X86_REG_EIP)==STOP
        words=struct.unpack('<8H',m.mem_read(STATE,16));frames=struct.unpack('<II',m.mem_read(STATE+16,8))
        return [words[0],words[2],words[3],words[4],words[5],words[6],words[7],*frames,m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW)],self.boundary,returned

def fixtures():
    base=[3,0,0,7,1,65535,0xffff,0,0x3f000000,0x3f800000,0x40000000,0x37f,0,3,4,6,5,1,2,3,0,0x3f800000,0x3e800000,0x3f000000,0x3f800000,0x3f000000,0x3f800000,0x3dcccccd,0xbf800000,10,11,12,13,3,7,3,3]
    for mode in range(1,13):
        for frame in (0,0x80000000,0x3f000000,0x3f800000,0x41200000,0xc1200000,0x42c80000,0xc2c80000,0x7fc12345):
            for frame_count in (1,2,10,0x7fffffff,0x80000000,0xffffffff):
                f=base.copy();f[0]=mode;f[7]=frame;f[29]=frame_count;yield f
    for mode in range(1,13):
        for current in (0,1):
            for transition in (0x3f800000,0x3e800000,0xbf800000,0x7fc12345):
                f=base.copy();f[0]=mode;f[2]=current;f[21]=transition;yield f
    for entry in (1,2):
        for mode in range(13):
            f=base.copy();f[12]=entry;f[0]=mode;yield f
    rng=random.Random(0x44a800)
    for _ in range(600):
        f=base.copy();f[0]=rng.randrange(1,13);f[7]=struct.unpack('<I',struct.pack('<f',rng.uniform(-40,40)))[0];f[25]=struct.unpack('<I',struct.pack('<f',rng.uniform(-2,2)))[0];f[2]=rng.randrange(2);yield f

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    cb=b''.join(image.read(x,4)for x in CONSTANTS);a.output.with_suffix('.constants.bin').write_bytes(cb);constants=(ctypes.c_float*3).from_buffer_copy(cb)
    lib=ctypes.CDLL(str(a.native_library.resolve()));probe=lib.sadx_native_player_animation;probe.argtypes=[ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    oracle=AnimationOriginal(image);report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),cases=0,stored_mismatches=0,status_mismatches=0,boundary_mismatches=0,examples=[],status_examples=[],source_ranges={hex(x):dict(bytes=n,sha256=sha(image.read(x,n)))for x,n in ((0x44a800,0x641),(0x43f230,0x6b),(0x446910,0x40),(0x446950,0x57),(0x43f9f0,0x9a))},constants=dict(addresses=list(map(hex,CONSTANTS)),sha256=sha(cb)),limits=['Typed four-record action/motion fixture is not the real initialized147-record Sonic table','Source numeric paths execute real43f9f0 with object_count0; nonzero key buffers/allocation lifecycle are not proved by this experiment','Original43f2a0 blend CALL stops before execution; the separate producer verifier covers its body and the native path retains synthetic resource unavailability','An emulator is the instruction oracle, not physical Windows hardware or game/mod execution'])
    root=pathlib.Path(__file__).resolve().parent.parent;report['native_sources']={n:sha((root/n).read_bytes())for n in ('src/player/player_animation.hpp','src/player/player_animation.cpp','tests/probes/native_player_animation_probe.cpp','tools/oracles/verify_original_player_animation.py')}
    start=time.monotonic();digest=hashlib.sha256()
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300)for rc in (0,0x400,0x800,0xc00)):
        for f in fixtures():
            f[11]=cw;digest.update(struct.pack('<37I',*f));expected,boundary,returned=oracle.run(f);out=(ctypes.c_uint32*15)()
            if probe(constants,(ctypes.c_uint32*37)(*f),out):raise RuntimeError('Native rejected fixture')
            for field,value in enumerate(expected):
                if out[field]!=value:
                    key='status_mismatches' if field==9 else 'stored_mismatches';report[key]+=1;bucket=report['status_examples']if field==9 else report['examples']
                    if len(bucket)<30:bucket.append(dict(case=report['cases'],input=f,field=field,source=value,native=out[field],boundary=hex(boundary)))
            if returned!=bool(out[13]) or boundary and out[12]!=boundary:
                report['boundary_mismatches']+=1
                if len(report['examples'])<30:report['examples'].append(dict(case=report['cases'],input=f,field='boundary',source=hex(boundary),returned=returned,native=list(out)))
            report['cases']+=1
    report.update(case_sha256=digest.hexdigest(),seconds=time.monotonic()-start,stored_state_matches=not(report['stored_mismatches']or report['boundary_mismatches']),passed=not(report['stored_mismatches']or report['boundary_mismatches']or report['status_mismatches']));a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('cases','stored_mismatches','boundary_mismatches','status_mismatches','seconds','passed')}));return int(not report['stored_state_matches'])
if __name__=='__main__':raise SystemExit(main())
