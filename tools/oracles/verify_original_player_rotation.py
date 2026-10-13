"""Four actual rotation helpers with real GetAnalog/matrix/CRT instructions.

No overlays. Explicit fixture/runtime state; Bochs FPATAN candidate is compared
to Unicorn's FPATAN model, not measured Intel hardware. Raw NaN bits retained.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import DATA,STACK,STOP
from verify_original_player_coordinates import CoordinatesOriginal,UNIT_ADDRESS,MATRIX,TASK,fword

PLAYER=DATA+0x1000
MOTION=DATA+0x1800
FUNCTIONS=(0x443c50,0x443e60,0x4491e0,0x449380)
FLOAT_CONSTANTS=(0x7dfff0,0x7dd568,0x7dd550,0x7e0070,0x7e0118,0x7dcc98,0x7dcb10,0x7e011c,0x7e0120,0x7dce68,0x7dcb2c,0x7e0164,0x7dccf4,0x7e0160,0x7e0170,0x7dcb60)
DOUBLE_CONSTANTS=(0x7e0168,0x7dccd0,0x7dcce8)
SEED=0x443e604491e0
def sha(b):return hashlib.sha256(b).hexdigest()

class OriginalRotation(CoordinatesOriginal):
    def __init__(self,image,cw,unit):
        super().__init__(image,cw,unit)
        # CoordinatesOriginal maps required global matrix sections. Additional
        # actual gravity/input globals are file-backed or BSS in the same PE.
        for a in (0x3b0f000,0x3b22000,0x3b3c000,0x3b0e000):
            self.machine.mem_map(a,0x1000)

    def run(self,w,cw):
        m=self.machine
        task=bytearray(64);struct.pack_into('<H',task,4,w[1]);struct.pack_into('<3I',task,0x14,*w[2:5]);m.mem_write(TASK,bytes(task))
        p=bytearray(0x240);struct.pack_into('<H',p,0xa,w[12]);struct.pack_into('<I',p,0x1c,w[9]);struct.pack_into('<3I',p,0x38,*w[6:9]);struct.pack_into('<I',p,0x5c,w[10]);struct.pack_into('<33I',p,0x9c,*w[23:56]);m.mem_write(PLAYER,bytes(p))
        motion=bytearray(0x50);struct.pack_into('<3I',motion,4,*w[13:16]);struct.pack_into('<I',motion,0x20,w[16]);struct.pack_into('<I',motion,0x3c,w[17]);m.mem_write(MOTION,bytes(motion))
        m.mem_write(0x3b0f0f8,struct.pack('<3I',*w[18:21]));m.mem_write(0x3b22dcc,struct.pack('<H',1));m.mem_write(0x3b3cdf0,struct.pack('<I',PLAYER));m.mem_write(0x3b0e7a0,struct.pack('<II',0,w[21]))
        m.mem_write(MATRIX,struct.pack('<96I',*w[58:]));
        for a,v in ((0x3d0f818,MATRIX+(w[56]-1)*64),(0x3d0fa28,6),(0x3d0fa2c,w[56]),(0x3d0f7ac,w[57])):m.mem_write(a,struct.pack('<I',v))
        if w[0]==0:
            args=(STOP,MOTION,w[5]);regs={UC_X86_REG_ESI:TASK,UC_X86_REG_EDI:PLAYER}
        elif w[0]==1:
            args=(STOP,MOTION,w[5]);regs={UC_X86_REG_ESI:PLAYER,UC_X86_REG_EBX:TASK}
        elif w[0]==2:
            args=(STOP,TASK,MOTION,PLAYER);regs={}
        else:
            args=(STOP,PLAYER);regs={UC_X86_REG_EAX:TASK,UC_X86_REG_ESI:MOTION}
        m.mem_write(STACK,struct.pack('<'+'I'*len(args),*args))
        for r,v in regs.items():m.reg_write(r,v)
        m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(FUNCTIONS[w[0]],STOP,count=30000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Source rotation ABI return failed')
        return [*struct.unpack('<3I',m.mem_read(TASK+0x14,12)),*struct.unpack('<3I',m.mem_read(PLAYER+0x38,12)),struct.unpack('<I',m.mem_read(MOTION+0x20,4))[0],struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0],m.reg_read(UC_X86_REG_FPSW)&0x3f,0,0,*struct.unpack('<96I',m.mem_read(MATRIX,384))]

def cases(params,unit):
    result=[];rng=random.Random(SEED);identity=list(struct.unpack('<16I',unit))
    def case(op,yaw,target,flags,speed,dotp,friction,world,gravity,depth=1,norm=0):
        return [op,flags,0,yaw,0,target,*map(fword,speed),fword(dotp),0,1,0,*map(fword,world),yaw,fword(friction),*map(fword,gravity),fword(1),1,*params,depth,norm,*identity*6]
    for op in range(4):
        for yaw in (0,1,65535,0x8000,0xffffffff):
            for target in (0,4095,4096,4097,8192,8193,32768):
                for flags in (0,1,0x100):
                    for dotp in (0,0.7,1):
                        result.append(case(op,yaw,target,flags,(3,1,-2),dotp,0.5,(2,1,3),(0,-1,0)))
        for _ in range(300):
            w=case(op,rng.getrandbits(32),rng.getrandbits(32),rng.choice((0,1,0x100)),tuple(rng.uniform(-30,30) for _ in range(3)),rng.uniform(-1,1),rng.uniform(-1,2),tuple(rng.uniform(-10,10) for _ in range(3)),tuple(rng.uniform(-1,1) for _ in range(3)),rng.choice((1,2,5,6)),rng.randrange(2));w[2]=rng.getrandbits(32);w[4]=rng.getrandbits(32);result.append(w)
    for op in range(4):
        for raw in (0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0xffc54321):
            w=case(op,0,100,0,(3,1,-2),1,1,(2,1,3),(0,-1,0));w[6]=raw;result.append(w)
        for timer in (0,1,0xffff):
            for attributes in (0,0x4000):
                w=case(op,0,16384,1,(3,1,-2),0.4,1,(0,0,0),(0,-0.5,0.5));w[10]=attributes;w[12]=timer;result.append(w)
    for active in (0,1):
        for friction in (-1,0,0.5,1,2):
            for dotp in (0.4,0.4000001):
                w=case(0,0,16384,1,(3,1,-2),dotp,friction,(2,1,3),(0,-1,0));w[21]=fword(active);result.append(w)
    for op in (2,3):
        w=case(op,0,1000,0,(3,1,-2),1,1,(0,0,0),(0,-0.5,0.5));w[23+9]=fword(-1);result.append(w)
    return result

def main():
    parser=argparse.ArgumentParser();parser.add_argument('exe',type=pathlib.Path);parser.add_argument('native_library',type=pathlib.Path);parser.add_argument('--output',type=pathlib.Path,required=True);args=parser.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires Unicorn2.1.4')
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source image changed')
    table=image.read(TABLE_ADDRESS,4100);unit=image.read(UNIT_ADDRESS,64);constants=b''.join(image.read(a,4) for a in FLOAT_CONSTANTS)+b''.join(image.read(a,8) for a in DOUBLE_CONSTANTS)
    fixtures=cases(list(struct.unpack('<33I',image.read(0x9154e8,132))),unit)
    blob=b''.join(struct.pack('<154I',*w) for w in fixtures);args.output.parent.mkdir(parents=True,exist_ok=True);blob_path=args.output.with_suffix('.cases.bin');blob_path.write_bytes(blob)
    library=ctypes.CDLL(str(args.native_library.resolve()));fn=library.sadx_native_player_rotation;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t]*3+[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    buffers=[(ctypes.c_ubyte*len(v)).from_buffer_copy(v) for v in (table,unit,constants)]
    report=dict(profile='four original rotation bodies with real input/transforms/Ninja/CRT; no overlays',source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),table_sha256=sha(table),unit_sha256=sha(unit),parameter_row_sha256=sha(image.read(0x9154e8,132)),constants_sha256=sha(constants),constant_addresses=dict(binary32=list(map(hex,FLOAT_CONSTANTS)),binary64=list(map(hex,DOUBLE_CONSTANTS))),case_blob=dict(path=str(blob_path),sha256=sha(blob),record='154 little-endian uint32 words'),seed=hex(SEED),source_functions={hex(a):dict(size=b-a,sha256=sha(image.read(a,b-a))) for a,b in ((0x443c50,0x443dea),(0x443e60,0x443f47),(0x4491e0,0x449373),(0x449380,0x4494ef))},control_words=[],limits=['Explicit runtime/matrix/input fixtures, not startup/mod execution','Selected Bochs P5/P6 FPATAN versus Unicorn, not Intel hardware','Exception flags are separately unverified','Quiet NaNs/infinities/subnormals/signed zero in local speed X; signaling-NaN FLD is a separately retained emulator limitation','Direction-reset/resource queries are not part of this comparison'])
    for cw in (p|r|0x7f for p in (0,0x200,0x300) for r in (0,0x400,0x800,0xc00)):
        original=OriginalRotation(image,cw,unit);item=dict(control_word=hex(cw),cases=len(fixtures),output_mismatches=0,flag_mismatches=0,examples=[],flag_examples=[])
        for index,w in enumerate(fixtures):
            expected=original.run(w,cw);out=(ctypes.c_uint32*107)();status=fn(buffers[0],4100,buffers[1],64,buffers[2],88,(ctypes.c_uint32*154)(*w),154,cw,out,107)
            if status or out[9]:raise RuntimeError(f'Native failed case{index}: status{status},failure{out[9]}')
            actual=list(out);different=actual[:8]+actual[9:]!=expected[:8]+expected[9:];item['output_mismatches']+=different;item['flag_mismatches']+=actual[8]!=expected[8]
            if different:item['examples'].append(dict(case=index,function=hex(FUNCTIONS[w[0]]),original=list(map(hex,expected)),native=list(map(hex,actual))))
            if actual[8]!=expected[8] and len(item['flag_examples'])<5:item['flag_examples'].append(dict(case=index,original=hex(expected[8]),native=hex(actual[8])))
        report['control_words'].append(item);print(json.dumps({k:v for k,v in item.items() if k not in ('examples','flag_examples')}),flush=True)
    report['stored_values_and_matrix_match']=all(x['output_mismatches']==0 for x in report['control_words']);args.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['stored_values_and_matrix_match'])
if __name__=='__main__':raise SystemExit(main())
