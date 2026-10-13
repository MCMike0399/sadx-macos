"""Real air body, GetAnalog, regular yaw and coordinate callees; no overlays.

External task/player/global/FP/matrix seeds are explicit experiments. Event
flag4000 is verified separately by its owning event body, not this harness.
"""
import argparse,ctypes,hashlib,json,pathlib,struct,time
import unicorn
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from extract import SOURCE_SHA256,TABLE_ADDRESS,PLAYER_ROTATION_CONSTANTS
from pe_image import PEImage
from verify_original_math import Original,DATA,STACK,STOP
from verify_original_ground_acceleration import cases
TASK=DATA+0x100;PLAYER=DATA+0x300;MOTION=DATA+0x700;MATRIX=DATA+0x4000;UNIT=0x389d650

def sha(data):return hashlib.sha256(data).hexdigest()
class AirOriginal(Original):
    def __init__(self,image,cw,unit):
        super().__init__(image,cw);self.calls=0;self.unit=unit
        for page in (0x3b0e000,0x3b0f000,0x3b22000,0x3b3c000,0x3d0f000,0x389d000):self.machine.mem_map(page,0x1000)
        self.machine.mem_write(UNIT,unit)
        self.machine.hook_add(UC_HOOK_CODE,self.count,begin=0x443c50,end=0x443c50)
    def count(self,m,a,s,u):self.calls+=1
    def run(self,w,cw):
        m=self.machine;self.calls=0
        task=bytearray(64);struct.pack_into('<H',task,4,w[0]);struct.pack_into('<3I',task,0x14,*w[1:4]);m.mem_write(TASK,bytes(task))
        player=bytearray(0x200);struct.pack_into('<I',player,0x1c,w[6]);struct.pack_into('<3I',player,0x2c,*w[21:24]);struct.pack_into('<3I',player,0x38,*w[18:21]);struct.pack_into('<I',player,0x5c,w[7]);struct.pack_into('<33I',player,0x9c,*w[30:]);m.mem_write(PLAYER,bytes(player))
        motion=bytearray(64);struct.pack_into('<3I',motion,4,*w[12:15]);struct.pack_into('<3I',motion,0x10,*w[15:18]);struct.pack_into('<I',motion,0x20,w[8]);struct.pack_into('<3I',motion,0x34,*w[9:12]);m.mem_write(MOTION,bytes(motion))
        m.mem_write(0x3b0f0f8,struct.pack('<3I',*w[24:27]));m.mem_write(0x3b22dcc,struct.pack('<H',1));m.mem_write(0x3b22dec,b'\0\0')
        m.mem_write(0x3b0e7a0,struct.pack('<2I',*w[4:6]));m.mem_write(0x3b3cdf0,struct.pack('<I',PLAYER))
        m.mem_write(MATRIX,self.unit*3)
        for address,value in ((0x3d0f818,MATRIX),(0x3d0fa28,3),(0x3d0fa2c,1),(0x3d0f7ac,0)):m.mem_write(address,struct.pack('<I',value))
        m.mem_write(STACK,struct.pack('<4I',STOP,TASK,MOTION,PLAYER));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(0x44b9c0,STOP,count=10000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Source air ABI mismatch')
        a=list(struct.unpack('<6I',m.mem_read(PLAYER+0x2c,24)))
        a.append(struct.unpack('<I',m.mem_read(MOTION+0x20,4))[0]);a.extend(struct.unpack('<3I',m.mem_read(TASK+0x14,12)))
        a.extend((self.calls,struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0],m.reg_read(UC_X86_REG_FPSW)&0x3f,m.reg_read(UC_X86_REG_FPCW),0))
        a.extend(struct.unpack('<3I',m.mem_read(MOTION+0x34,12)));a.extend(struct.unpack('<6I',m.mem_read(MOTION+4,24)));a.extend((struct.unpack('<H',m.mem_read(TASK+4,2))[0],struct.unpack('<I',m.mem_read(PLAYER+0x5c,4))[0]));a.extend(struct.unpack('<48I',m.mem_read(MATRIX,192)))
        return a

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args()
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    table=image.read(TABLE_ADDRESS,4100);unit=image.read(UNIT,64);constants=b''.join(image.read(a,s) for a,s in PLAYER_ROTATION_CONSTANTS)
    fixtures=[w for w in cases(list(struct.unpack('<33I',image.read(0x9154e8,132)))) if not w[0]&0x4000]
    blob=b''.join(struct.pack('<63I',*w) for w in fixtures);path=args.output.with_suffix('.cases.bin');path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(blob)
    lib=ctypes.CDLL(str(args.native_library.resolve()));fn=lib.sadx_native_air_acceleration;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t]*3+[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    buffers=[(ctypes.c_ubyte*len(b)).from_buffer_copy(b) for b in (table,unit,constants)]
    report=dict(profile='complete source air body with real input/yaw/matrix callees; no overlays',source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),case_blob=dict(path=str(path),sha256=sha(blob)),constants_sha256=sha(constants),control_words=[],limits=['Explicit source parameters/runtime/FP/matrix experiments; event4000 body excluded','Source native function composition, not a complete character/game tick','Hardware FP flags, timing, real mod startup and physical input remain unverified'])
    started=time.monotonic()
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        original=AirOriginal(image,cw,unit);item=dict(cw=hex(cw),cases=len(fixtures),stored_and_call_mismatches=0,flag_mismatches=0,examples=[])
        for i,w in enumerate(fixtures):
            expected=original.run(w,cw);out=(ctypes.c_uint32*74)();status=fn(buffers[0],4100,buffers[1],64,buffers[2],88,(ctypes.c_uint32*63)(*w),63,cw,out,74)
            if status or out[14]:raise RuntimeError(f'Native air rejected case{i}:status{status},failure{out[14]}')
            actual=list(out);diff=actual[:12]+actual[13:]!=expected[:12]+expected[13:];flags=actual[12]!=expected[12]
            item['stored_and_call_mismatches']+=diff;item['flag_mismatches']+=flags
            if diff or (flags and len(item['examples'])<4):item['examples'].append(dict(case=i,expected=list(map(hex,expected)),actual=list(map(hex,actual))))
        report['control_words'].append(item);print(json.dumps({k:v for k,v in item.items() if k!='examples'}),flush=True)
    report['stored_values_calls_and_matrix_match']=all(i['stored_and_call_mismatches']==0 for i in report['control_words']);report['exception_flags_match']=all(i['flag_mismatches']==0 for i in report['control_words']);report['elapsed_seconds']=time.monotonic()-started;args.output.write_text(json.dumps(report,indent=2)+'\n')
    if not report['stored_values_calls_and_matrix_match']:raise SystemExit(1)
if __name__=='__main__':main()
