"""Actual njRotateV and the seeded matrix portion of alternate20000.

The second mode executes original0x44b453..0x44b50d, with preceding raw-stick
angle and retained-ST0 explicitly seeded. It is not the complete acceleration
body; its peer-owned original-body harness composes that earlier producer.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import CODE,DATA,STACK,STOP
from verify_original_player_coordinates import CoordinatesOriginal,UNIT_ADDRESS,MATRIX,TASK,fword
AXIS=DATA+0x200
PLAYER=DATA+0x1000
CAMERA=DATA+0x2000
FRAME=STACK-0x100
SEED=0x788df044b453
def sha(v):return hashlib.sha256(v).hexdigest()
class OriginalAxis(CoordinatesOriginal):
    def __init__(self,image,cw,unit):
        super().__init__(image,cw,unit);self.machine.mem_map(0x3b2c000,0x1000)
        a=CODE+0x500;wrapper=b'\xd9\x05'+struct.pack('<I',AXIS+12)+b'\xe9'+struct.pack('<i',0x44b453-(a+11))
        self.machine.mem_write(a,wrapper)
    def run(self,w,cw):
        m=self.machine;m.mem_write(MATRIX,struct.pack('<96I',*w[13:]));m.mem_write(AXIS,struct.pack('<4I',*w[1:4],w[10]));
        for a,v in ((0x3d0f818,MATRIX+(w[11]-1)*64),(0x3d0fa28,6),(0x3d0fa2c,w[11]),(0x3d0f7ac,w[12])):m.mem_write(a,struct.pack('<I',v))
        m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        if w[0]==0:
            m.mem_write(STACK,struct.pack('<II',STOP,w[4]));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_ECX,0);m.reg_write(UC_X86_REG_EDX,AXIS);m.emu_start(0x788df0,STOP,count=30000)
            if m.reg_read(UC_X86_REG_ESP)!=STACK+8:raise RuntimeError('njRotateV ABI mismatch')
            vectors=[0]*9;mask=0
        else:
            task=bytearray(64);struct.pack_into('<3I',task,0x14,*w[5:8]);m.mem_write(TASK,bytes(task));m.mem_write(PLAYER+0x50,struct.pack('<3I',*w[1:4]));
            camera=bytearray(64);struct.pack_into('<II',camera,0x14,w[8],w[9]);m.mem_write(CAMERA,bytes(camera));m.mem_write(0x3b2cbb0,struct.pack('<I',CAMERA));
            m.mem_write(FRAME,bytes(0x80));m.reg_write(UC_X86_REG_ESP,FRAME);m.reg_write(UC_X86_REG_EAX,w[4]);m.reg_write(UC_X86_REG_ESI,PLAYER);m.reg_write(UC_X86_REG_EDI,TASK);m.emu_start(CODE+0x500,0x44b50d,count=30000)
            if m.reg_read(UC_X86_REG_ESP)!=FRAME-8:raise RuntimeError('Projection seeded-frame ABI mismatch')
            vectors=[*struct.unpack('<3I',m.mem_read(FRAME+0x3c,12)),*struct.unpack('<3I',m.mem_read(FRAME+0x48,12)),*struct.unpack('<3I',m.mem_read(FRAME+0x30,12))];mask=7
        return [0,0,struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0],m.reg_read(UC_X86_REG_FPSW)&0x3f,*vectors,mask,*struct.unpack('<96I',m.mem_read(MATRIX,384))]
def fixtures(unit):
    identity=list(struct.unpack('<16I',unit));cases=[];rng=random.Random(SEED)
    for mode in (0,1):
        for axis in ((0,0,0),(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1),(2,3,4),(0.3,-0.7,0.2)):
            for angle in (0,1,8192,16384,32768,49152,65535,0xffffffff):
                for depth in (1,5,6):cases.append([mode,*map(fword,axis),angle,0,12345,0,4567,8910,fword(0),depth,0,*identity*6])
        for _ in range(400):
            framewords=list(map(fword,(rng.uniform(-3,3) for _ in range(96))));cases.append([mode,*map(fword,(rng.uniform(-2,2) for _ in range(3))),*[rng.getrandbits(32) for _ in range(6)],fword(rng.uniform(-1,1)),rng.choice((1,2,5,6)),rng.randrange(2),*framewords])
        for raw in (0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0xffc12345):
            for component in range(3):
                w=[mode,fword(0.3),fword(0.4),fword(0.5),12345,111,222,333,444,555,0,1,0,*identity*6];w[1+component]=raw;cases.append(w)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires Unicorn2.1.4')
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source image changed')
    table=image.read(TABLE_ADDRESS,4100);unit=image.read(UNIT_ADDRESS,64);constants=image.read(0x7dcb2c,4)+image.read(0x7dcb10,4);cases=fixtures(unit)
    args.output.parent.mkdir(parents=True,exist_ok=True);blob=b''.join(struct.pack('<109I',*w) for w in cases);path=args.output.with_suffix('.cases.bin');path.write_bytes(blob)
    lib=ctypes.CDLL(str(args.native_library.resolve()));fn=lib.sadx_native_axis_projection;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t]*3+[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    buffers=[(ctypes.c_ubyte*len(b)).from_buffer_copy(b) for b in (table,unit,constants)]
    report=dict(profile='actual axis body + explicitly seeded original matrix projection tail; no overlays',source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),table_sha256=sha(table),unit_sha256=sha(unit),constants_sha256=sha(constants),case_blob=dict(path=str(path),sha256=sha(blob),record='109 little-endian uint32 words'),seed=hex(SEED),source_functions={hex(a):dict(size=b-a,sha256=sha(image.read(a,b-a))) for a,b in ((0x788df0,0x78931e),(0x44b453,0x44b50d))},control_words=[],limits=['Raw-stick conversion and retained ST0 are explicitly seeded preceding producers','The projection comparison ends before vector-angle438410; no complete acceleration claim','Explicit matrix/runtime/FPU fixtures, not startup, mods or captured gameplay','Quiet NaNs/Inf/subnormals/signed-zero axis fixtures; initial-SNaN FLD remains separate emulator limitation','Hardware exception delivery and full x87 status are unverified'])
    for cw in (p|r|0x7f for p in (0,0x200,0x300) for r in (0,0x400,0x800,0xc00)):
        original=OriginalAxis(image,cw,unit);item=dict(control_word=hex(cw),cases=len(cases),output_mismatches=0,flag_mismatches=0,examples=[],flag_examples=[])
        for index,w in enumerate(cases):
            expected=original.run(w,cw);out=(ctypes.c_uint32*110)();status=fn(buffers[0],4100,buffers[1],64,buffers[2],8,(ctypes.c_uint32*109)(*w),109,cw,out,110)
            if status or out[0]:raise RuntimeError(f'Native failedcase{index}:status{status},failure{out[0]}')
            actual=list(out);different=actual[:3]+actual[4:]!=expected[:3]+expected[4:];item['output_mismatches']+=different;item['flag_mismatches']+=actual[3]!=expected[3]
            if different:item['examples'].append(dict(case=index,mode=w[0],original=list(map(hex,expected)),native=list(map(hex,actual))))
            if actual[3]!=expected[3] and len(item['flag_examples'])<5:item['flag_examples'].append(dict(case=index,original=hex(expected[3]),native=hex(actual[3])))
        report['control_words'].append(item);print(json.dumps({k:v for k,v in item.items() if k not in ('examples','flag_examples')}),flush=True)
    report['stored_vectors_and_matrix_match']=all(i['output_mismatches']==0 for i in report['control_words']);args.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['stored_vectors_and_matrix_match'])
if __name__=='__main__':raise SystemExit(main())
