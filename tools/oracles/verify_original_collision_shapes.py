"""Actual shape-position, normal-dispatch and matrix primitive instructions.

No geometry/normal replies or replacement arithmetic. Explicit actor/CCL/matrix
fixtures are native-owned state mirrored into isolated instruction memory.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import DATA,STACK,STOP
from verify_original_player_coordinates import CoordinatesOriginal,UNIT_ADDRESS,MATRIX,TASK,fword
OTHER_TASK=DATA+0x800;OTHER_WORK=DATA+0x900
COLLISION,HIT,OWN_INFO,HIT_INFO,OUTPUT= [DATA+x for x in (0x1000,0x1200,0x1800,0x2000,0x3000)]
FUNCTIONS=[0x419030,0x419270,0x4193a0,0x419900,0x41bed0,0x781c10,0x781670,0x784c70]
SEED=0x41bc00419270
class Source(CoordinatesOriginal):
    def __init__(self,image,cw,unit):
        super().__init__(image,cw,unit);self.stores=0
        self.machine.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.written,begin=OUTPUT,end=OUTPUT+11)
    def written(self,m,access,address,size,value,data):
        if OUTPUT<=address<OUTPUT+12:self.stores|=1<<((address-OUTPUT)//4)
    def run(self,w,cw):
        m=self.machine
        task=bytearray(64);struct.pack_into('<H',task,4,w[1]);struct.pack_into('<3I',task,0x14,*w[2:5]);struct.pack_into('<3I',task,0x20,*w[5:8]);struct.pack_into('<I',task,0x38,COLLISION);m.mem_write(TASK,bytes(task))
        other=bytearray(64);struct.pack_into('<3I',other,0x14,w[133],w[19],w[134]);struct.pack_into('<3I',other,0x20,*w[16:19]);m.mem_write(OTHER_WORK,bytes(other));m.mem_write(OTHER_TASK+0x20,struct.pack('<I',OTHER_WORK if w[26] else 0))
        shape=bytearray(48);shape[1]=w[15];struct.pack_into('<I',shape,4,w[8]);struct.pack_into('<3I',shape,8,*w[9:12]);struct.pack_into('<I',shape,0x14,w[12]);struct.pack_into('<I',shape,0x1c,w[13]);struct.pack_into('<I',shape,0x28,w[14])
        own=bytearray(48);struct.pack_into('<I',own,4,w[27]);struct.pack_into('<3I',own,8,*w[28:31]);struct.pack_into('<I',own,0x14,w[31]);m.mem_write(OWN_INFO,bytes(own))
        index=w[20]&65535;index=index if index<32768 else index-65536
        m.mem_write(HIT_INFO+48*index,bytes(shape))
        collision=bytearray(0xa8);struct.pack_into('<I',collision,0xc,OWN_INFO if w[23] else 0);struct.pack_into('<H',collision,0xa2,index&65535);struct.pack_into('<I',collision,0xa4,HIT if w[22] else 0);m.mem_write(COLLISION,bytes(collision))
        hit=bytearray(0xa8);struct.pack_into('<I',hit,0xc,HIT_INFO if w[24] else 0);struct.pack_into('<H',hit,6,w[21]&65535);struct.pack_into('<I',hit,0x9c,OTHER_TASK if w[25] else 0);m.mem_write(HIT,bytes(hit))
        m.mem_write(OUTPUT,struct.pack('<3I',*w[130:133]));self.stores=0
        m.mem_write(MATRIX,struct.pack('<96I',*w[34:130]));
        for a,v in ((0x3d0f818,MATRIX+(w[32]-1)*64),(0x3d0fa28,w[135]),(0x3d0fa2c,w[32]),(0x3d0f7ac,w[33])):m.mem_write(a,struct.pack('<I',v))
        if w[0]<=2:
            args=(STOP,TASK if w[26] else 0,HIT_INFO+48*index,OUTPUT);regs={};adjust=4
        elif w[0]==3:
            args=(STOP,);regs={UC_X86_REG_ECX:TASK+0x20,UC_X86_REG_EAX:OTHER_WORK+0x20,UC_X86_REG_ESI:w[19],UC_X86_REG_EDI:OUTPUT};adjust=4
        elif w[0]==4:
            args=(STOP,TASK,OUTPUT);regs={};adjust=4
        elif w[0]==5:
            args=(STOP,);regs={UC_X86_REG_ECX:TASK+0x20};adjust=4
        elif w[0]==6:
            args=(STOP,OUTPUT);regs={UC_X86_REG_ECX:0,UC_X86_REG_EDX:TASK+0x20};adjust=8
        else:
            args=(STOP,*w[5:8]);regs={UC_X86_REG_ECX:0};adjust=16
        m.mem_write(STACK,struct.pack('<'+'I'*len(args),*args))
        for register,value in regs.items():m.reg_write(register,value)
        m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(FUNCTIONS[w[0]],STOP,count=30000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+adjust:raise RuntimeError(f'Original mode{w[0]} ABI did not return')
        result=[*struct.unpack('<3I',m.mem_read(OUTPUT,12)),m.reg_read(UC_X86_REG_EAX) if w[0]==4 else 0xdeadbeef,0,0,self.stores if w[0]==4 else 0,struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0],m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW),*struct.unpack('<96I',m.mem_read(MATRIX,384))]
        return result

def fixtures(unit):
    rng=random.Random(SEED);identity=list(struct.unpack('<16I',unit))*6;result=[]
    def base(mode):
        return [mode,0,0,0,0,*map(fword,(10,20,30)),0,*map(fword,(1,2,3)),fword(1),fword(2),0,0,*map(fword,(1,2,3)),0,0,1,1,1,1,1,1,0,*map(fword,(1,2,3)),fword(1),1,0,*identity,*map(fword,(101,102,103)),0,0,6]
    for mode in range(8):
        for attributes in (0,0x20,0x200,0x8000,0x8200):
            for angle in (0,1,16384,32768,65536,0xffffffff):
                for depth in (1,6):
                    w=base(mode);w[8]=attributes;w[2:5]=[angle,angle,angle];w[32]=depth;w[19]=angle;result.append(w)
        for _ in range(350):
            w=base(mode);w[2:5]=[rng.getrandbits(32) for _ in range(3)];w[5:8]=[fword(rng.uniform(-100,100)) for _ in range(3)];w[8]=rng.choice((0,0x20,0x200,0x8000,0x8200));w[9:14]=[fword(rng.uniform(-20,20)) for _ in range(5)];w[14]=rng.getrandbits(32);w[15]=rng.randrange(10);w[16:19]=[fword(rng.uniform(-50,50)) for _ in range(3)];w[19]=rng.getrandbits(32);w[20]=rng.choice((0,1,15,16));w[21]=rng.choice((0,1,15,16));w[27]=rng.choice((0,0x20,0x200,0x8000));w[28:32]=[fword(rng.uniform(-20,20)) for _ in range(4)];w[32]=rng.choice((1,2,5,6));w[33]=rng.randrange(2);w[133:135]=[rng.getrandbits(32) for _ in range(2)]
            # Random retained frames distinguish push-unit from copy-current.
            w[34:130]=[fword(rng.uniform(-3,3)) for _ in range(96)];result.append(w)
        if mode!=4:
            for capacity in (1,2,6):
                w=base(mode);w[135]=capacity;w[32]=capacity;result.append(w)
        if mode<=2:
            w=base(mode);w[8]=0x20;w[26]=0;result.append(w)
        for raw in (0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0xffc54321):
            w=base(mode);w[8]=0x20;w[27]=0x20;w[rng.choice((5,6,7,9,10,11))]=raw;result.append(w)
    for form in range(10):
        for attributes in (0x20,0,0x8000,0x200):
            for index,count in ((0,0),(0,1),(1,1),(2,1),(-1,0),(16,16)):
                w=base(4);w[8]=attributes;w[27]=0x20;w[15]=form;w[20]=index;w[21]=count;result.append(w)
    for bits in ((0,1,1),(1,0,1),(1,1,0)):
        w=base(4);w[22:25]=bits;result.append(w)
    w=base(4);w[8]=0x20;w[26]=0;result.append(w)
    return [[x&0xffffffff for x in w] for w in result]

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires Unicorn2.1.4')
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source identity mismatch')
    table=image.read(TABLE_ADDRESS,4100);unit=image.read(UNIT_ADDRESS,64);constants=image.read(0x7dcb2c,4)+image.read(0x7dcb10,4);cases=fixtures(unit)
    blob=b''.join(struct.pack('<136I',*w) for w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_collision_shapes;fn.argtypes=[ctypes.POINTER(ctypes.c_uint8)]*3+[ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    buffers=[(ctypes.c_uint8*len(b)).from_buffer_copy(b) for b in (table,unit,constants)];reports=[];start=time.monotonic()
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        source=Source(image,cw,unit);r={'cw':hex(cw),'cases':len(cases),'value_mismatches':0,'flag_mismatches':0,'first_value_mismatches':[],'first_flag_mismatches':[],'modes':{str(i):{'cases':0,'value_mismatches':0} for i in range(8)}}
        for index,w in enumerate(cases):
            expected=source.run(w,cw);output=(ctypes.c_uint32*106)();status=fn(*buffers,(ctypes.c_uint32*136)(*w),cw,output)
            if status:raise RuntimeError(f'Native rejectedcase{index}mode{w[0]}status{status}')
            actual=list(output);different=actual[:8]+actual[9:]!=expected[:8]+expected[9:];r['value_mismatches']+=different;r['flag_mismatches']+=actual[8]!=expected[8];r['modes'][str(w[0])]['cases']+=1;r['modes'][str(w[0])]['value_mismatches']+=different
            if different and len(r['first_value_mismatches'])<15:r['first_value_mismatches'].append({'case':index,'input':w,'different':[(i,hex(expected[i]),hex(actual[i])) for i in range(106) if i!=8 and expected[i]!=actual[i]]})
            if actual[8]!=expected[8] and len(r['first_flag_mismatches'])<5:r['first_flag_mismatches'].append({'case':index,'source':hex(expected[8]),'native':hex(actual[8])})
        reports.append(r);print(json.dumps({k:v for k,v in r.items() if not k.startswith('first_') and k!='modes'}),flush=True)
    sha=lambda b:hashlib.sha256(b).hexdigest()
    result={'source_sha256':image.sha256,'native_library_sha256':sha(a.native_library.read_bytes()),'sine_sha256':sha(table),'unit_sha256':sha(unit),'axis_constants_sha256':sha(constants),'cases_sha256':sha(blob),'seed':hex(SEED),'results':reports,'seconds':time.monotonic()-start,'stored_state_matches':all(not r['value_mismatches'] for r in reports),'passed':all(not(r['value_mismatches'] or r['flag_mismatches']) for r in reports),'scope':'4real shape primitives, real41bc00+41bed0, 3real matrix primitives; no arithmetic/normal replies or geometry replacements. Native live-task views seeded explicitly; hardware/runtime unknown.'}
    result['limits']=([result['limits']] if isinstance(result.get('limits'),str) else list(result.get('limits') or []))+['Flag difference: the native x87 model sets the precision bit (PE 0x20) of the status word on inexact results; Unicorn 2.1.4 does not raise x87 exception bits for these instructions (source 0x0). All compared values match; the hardware flags need the Intel capture (M4).']
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    return int(not result['stored_state_matches'])
if __name__=='__main__':raise SystemExit(main())
