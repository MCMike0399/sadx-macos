"""Actual PSetPosition local instructions with explicit mandatory owner replies.

Scene/normal/hold owners are controlled fixture boundaries, never fake geometry.
Unavailable owners stop at their real CALL site; local math and matrix/CRT
callees execute actual bytes. This is not whole-game collision conformance.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_player_coordinates import CoordinatesOriginal,UNIT_ADDRESS,MATRIX,TASK,fword
from verify_original_player_rotation import FLOAT_CONSTANTS,DOUBLE_CONSTANTS
from verify_original_math import DATA,STACK,STOP
PLAYER,MOTION=DATA+0x1000,DATA+0x1800
COLLISION,HIT,OWN_INFO,HIT_INFO,OTHER_TASK,ACTOR= [DATA+x for x in (0x2200,0x2300,0x2400,0x2800,0x2c00,0x3000)]
CONST=[(0x7e012c,4),(0x7e01ac,4),(0x7dccd0,8),(0x7dccc8,8),(0x7dcce8,8)]
SEED=0x44cdf0
class OriginalPosition(CoordinatesOriginal):
    def __init__(self,image,cw,unit):
        super().__init__(image,cw,unit)
        for addr in (0x3b22000,0x3c83000,0x2bcc000):self.machine.mem_map(addr,4096)
        self.machine.mem_write(0x2bcc000,image.read(0x2bcc000,4096))
        self.machine.hook_add(unicorn.UC_HOOK_CODE,self.owner_call)
    def owner_call(self,m,address,size,data):
        if address not in (0x44ce8a,0x44d1ea,0x44d1bf,0x44d39f,0x44d3a9,0x44d4b1,0x44d4bb):return
        w=self.w
        if address==0x44ce8a:
            if not w[26]:self.boundary=address;m.emu_stop();return
            ptr=struct.unpack('<I',m.mem_read(m.reg_read(UC_X86_REG_ESP)+4,4))[0]
            m.mem_write(ptr,struct.pack('<3I',*w[22:25]))
        elif address==0x44d1ea:
            if not w[27]:self.boundary=address;m.emu_stop();return
            m.reg_write(UC_X86_REG_EAX,w[25])
        elif address==0x44d1bf:
            self.boundary=address;m.emu_stop();return
        else:
            if not w[28]:self.boundary=address;m.emu_stop();return
            flags=struct.unpack('<H',m.mem_read(TASK+4,2))[0]
            if w[30]==1:flags|=1
            elif w[30]==2:flags&=~1
            elif w[30]==3:flags&=~2
            m.mem_write(TASK+4,struct.pack('<H',flags));m.mem_write(PLAYER+0x5c,struct.pack('<I',0xabcdef));m.reg_write(UC_X86_REG_EAX,w[29])
        # This simulates only the explicit dependency reply, not the callee's
        # algorithm. Original caller stack and all subsequent bytes are real.
        m.reg_write(UC_X86_REG_EIP,address+5)
    def run(self,w,cw):
        m=self.machine;self.w=w;self.boundary=0
        task=bytearray(64);struct.pack_into('<H',task,4,w[0]);struct.pack_into('<3I',task,0x14,*w[1:4]);struct.pack_into('<3I',task,0x20,*w[4:7]);struct.pack_into('<I',task,0x38,COLLISION);m.mem_write(TASK,bytes(task))
        p=bytearray(0x240);struct.pack_into('<3I',p,0x38,*w[7:10]);struct.pack_into('<3f',p,0x50,.1,.2,.3);struct.pack_into('<I',p,0x5c,w[170]);struct.pack_into('<33I',p,0x9c,*w[39:72]);m.mem_write(PLAYER,bytes(p))
        motion=bytearray(0x50);struct.pack_into('<3I',motion,4,*w[10:13]);m.mem_write(MOTION,bytes(motion))
        collision=bytearray(0xa8);struct.pack_into('<H',collision,4,w[13]);struct.pack_into('<I',collision,0xc,OWN_INFO);struct.pack_into('<H',collision,0xa2,w[14]&65535);struct.pack_into('<I',collision,0xa4,HIT if w[33] else 0);m.mem_write(COLLISION,bytes(collision))
        hit=bytearray(0xa8);struct.pack_into('<H',hit,2,w[38]&65535);struct.pack_into('<I',hit,0xc,HIT_INFO);struct.pack_into('<I',hit,0x9c,OTHER_TASK if w[34] else 0);m.mem_write(HIT,bytes(hit))
        m.mem_write(OTHER_TASK+0x20,struct.pack('<I',ACTOR if w[35] else 0));m.mem_write(ACTOR+2,bytes([w[36]]));m.mem_write(ACTOR+0x18,struct.pack('<I',w[37]))
        index=w[14]&65535;index=index if index<32768 else index-65536
        own=bytearray(48);own[3]=w[17];struct.pack_into('<I',own,0x14,w[20]);m.mem_write(OWN_INFO,bytes(own));m.mem_write(OWN_INFO+48*index+1,bytes([w[15]]))
        info=bytearray(48);info[2]=w[16];info[3]=w[18];struct.pack_into('<I',info,4,w[19]);struct.pack_into('<I',info,0x14,w[21]);m.mem_write(HIT_INFO+48*index,bytes(info))
        m.mem_write(0x3b22dcc,struct.pack('<H',w[31]&65535));m.mem_write(0x3b22dec,struct.pack('<H',w[32]&65535));m.mem_write(0x3c837a8,struct.pack('<I',1))
        m.mem_write(MATRIX,struct.pack('<96I',*w[74:170]));
        for a,v in ((0x3d0f818,MATRIX+(w[72]-1)*64),(0x3d0fa28,6),(0x3d0fa2c,w[72]),(0x3d0f7ac,w[73])):m.mem_write(a,struct.pack('<I',v))
        m.mem_write(STACK,struct.pack('<4I',STOP,TASK,MOTION,PLAYER));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(0x44cdf0,STOP,count=50000)
        if not self.boundary and m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('PSetPosition ABI did not return')
        scratch=struct.unpack('<I',m.mem_read(STACK-0x20,4))[0]
        output=[struct.unpack('<H',m.mem_read(TASK+4,2))[0],*struct.unpack('<3I',m.mem_read(TASK+0x14,12)),*struct.unpack('<3I',m.mem_read(TASK+0x20,12)),*struct.unpack('<3I',m.mem_read(PLAYER+0x38,12)),*struct.unpack('<3I',m.mem_read(MOTION+4,12)),*struct.unpack('<3I',m.mem_read(PLAYER+0x44,12)),*struct.unpack('<3I',m.mem_read(PLAYER+0x50,12)),*struct.unpack('<2I',m.mem_read(PLAYER+0x5c,8)),m.reg_read(UC_X86_REG_EAX) if not self.boundary else 0xdeadbeef,scratch,self.boundary,struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0],m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW),int(bool(self.boundary)),*struct.unpack('<96I',m.mem_read(MATRIX,384))]
        return output

def fixtures(params,unit):
    rng=random.Random(SEED);frames=list(struct.unpack('<16I',unit))*6;out=[]
    def base():
        return [0,0,0,0,*map(fword,(10,20,30)),*map(fword,(3,0,0)),*map(fword,(3,0,0)),0,0,0,7,12,0,0,fword(1),fword(3),*map(fword,(-1,0,0)),0,1,1,0,9,0,1,0,1,1,1,12,0x4000,1,*params,1,0,*frames,0x12345678]
    for flags in (0,1,2,3,4,0x101):
        for collision in (0,1,4,0x10,0x14,0x15):
            for enabled in (0,1):
                for form in (0,1,2,3,4):
                    w=base();w[0]=flags;w[13]=collision;w[15]=form;w[28]=enabled;out.append(w)
    for speed in (-1,0,1,3,5,8,10,14,15,16,30):
        for normal in ((-1,0,0),(-.5,0,.866),(0,0,0),(0,1,0),(1,2,3)):
            for held in (0,0x400):
                for own_a,hit_a in ((1,3),(3,1),(0,0)):
                    w=base();w[7]=fword(speed);w[13]=0x14;w[20]=fword(own_a);w[21]=fword(hit_a);w[22:25]=map(fword,normal);w[25]=held;w[28]=1;out.append(w)
    for _ in range(1200):
        w=base();w[0]=rng.choice((0,1,2,3,4,0x101));w[1:4]=[rng.getrandbits(32) for _ in range(3)];w[7:13]=[fword(rng.uniform(-20,20)) for _ in range(6)];w[13]=rng.choice((0,1,0x14,0x15));w[14]=rng.choice((0,1,15,16));w[15]=rng.randrange(5);w[16]=rng.randrange(8);w[17:19]=[rng.randrange(256),rng.randrange(256)];w[19]=rng.choice((0,0x2000));w[20:25]=[fword(rng.uniform(-4,4)) for _ in range(5)];w[25]=rng.choice((0,0x400));w[28]=rng.randrange(2);w[30]=rng.randrange(4);w[31:33]=rng.choice(((1,0),(9,2),(9,3),(-1,-1)));w[33:36]=[rng.randrange(2) for _ in range(3)];
        # A selected wall record requires actual known non-null hit info.
        if w[13]&0x10:w[33]=1
        w[36]=rng.choice((0,12,13));w[37]=rng.getrandbits(32);w[38]=rng.choice((0,1,0xffff));w[72]=rng.choice((1,2,5,6));w[73]=rng.randrange(2);out.append(w)
    for raw in (0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0xffc54321):
        for index in (7,8,9,10,11,12,22,23,24,20,21):
            w=base();w[13]=0x14;w[28]=1;w[index]=raw;out.append(w)
    for flags in (0,1,2):
        for normal_owner,hold_owner in ((0,1),(1,0)):
            w=base();w[0]=flags;w[13]=0x14;w[26]=normal_owner;w[27]=hold_owner;out.append(w)
    return [[x&0xffffffff for x in w] for w in out]

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires Unicorn2.1.4')
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source identity mismatch')
    table=image.read(TABLE_ADDRESS,4100);unit=image.read(UNIT_ADDRESS,64);rotation=b''.join(image.read(addr,4) for addr in FLOAT_CONSTANTS)+b''.join(image.read(addr,8) for addr in DOUBLE_CONSTANTS);position=b''.join(image.read(addr,n) for addr,n in CONST)
    cases=fixtures(list(struct.unpack('<33I',image.read(0x9154e8,132))),unit);blob=b''.join(struct.pack('<171I',*w) for w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_player_position;fn.argtypes=[ctypes.POINTER(ctypes.c_uint8)]*4+[ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    buffers=[(ctypes.c_uint8*len(b)).from_buffer_copy(b) for b in (table,unit,rotation,position)];reports=[];start=time.monotonic()
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        original=OriginalPosition(image,cw,unit);r={'cw':hex(cw),'cases':len(cases),'value_mismatches':0,'flag_mismatches':0,'first_value_mismatches':[],'first_flag_mismatches':[]}
        for index,w in enumerate(cases):
            expected=original.run(w,cw);out=(ctypes.c_uint32*124)();status=fn(*buffers,(ctypes.c_uint32*171)(*w),cw,out)
            if status:raise RuntimeError(f'Native failedcase{index} status{status}')
            actual=list(out);actual[27]=int(bool(actual[27]));different=actual[:25]+actual[26:]!=expected[:25]+expected[26:]
            r['value_mismatches']+=different;r['flag_mismatches']+=actual[25]!=expected[25]
            if different and len(r['first_value_mismatches'])<10:r['first_value_mismatches'].append({'case':index,'input':w,'different':[(i,hex(expected[i]),hex(actual[i])) for i in range(124) if i!=25 and expected[i]!=actual[i]]})
            if actual[25]!=expected[25] and len(r['first_flag_mismatches'])<5:r['first_flag_mismatches'].append({'case':index,'oracle':hex(expected[25]),'native':hex(actual[25])})
        reports.append(r);print(json.dumps({k:v for k,v in r.items() if not k.startswith('first_')}),flush=True)
    formfn=lib.sadx_native_position_form
    formfn.argtypes=[ctypes.c_int32,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,ctypes.POINTER(ctypes.c_int32)];formfn.restype=ctypes.c_int
    form_result={'cases':0,'value_mismatches':0}
    machine=original.machine
    for index in (-2147483648,-1,*range(17),2147483647):
        for present in (0,1):
            for work_present in (0,1):
                for value in (0,1,2,3,4,5,6,7,128,255):
                    machine.mem_write(TASK+0x38,struct.pack('<I',COLLISION if work_present else 0));machine.mem_write(COLLISION+0xc,struct.pack('<I',OWN_INFO))
                    if 0<=index<16:machine.mem_write(OWN_INFO+index*48+1,bytes([value]))
                    machine.mem_write(STACK,struct.pack('<3I',STOP,TASK if present else 0,index&0xffffffff));machine.reg_write(UC_X86_REG_ESP,STACK)
                    machine.emu_start(0x41c1c0,STOP,count=50)
                    expected=machine.reg_read(UC_X86_REG_EAX);out=ctypes.c_int32()
                    if formfn(index,present,work_present,value,ctypes.byref(out)):raise RuntimeError('Native CCL form getter failed known fixture')
                    form_result['cases']+=1;form_result['value_mismatches']+=(out.value&0xffffffff)!=expected
    sha=lambda b:hashlib.sha256(b).hexdigest()
    result={'source_sha256':image.sha256,'source_instructions_sha256':sha(image.read(0x44cdf0,0x6eb)),'native_library_sha256':sha(a.native_library.read_bytes()),'constants_sha256':sha(position),'rotation_constants_sha256':sha(rotation),'sine_sha256':sha(table),'unit_sha256':sha(unit),'cases_sha256':sha(blob),'seed':hex(SEED),'ccl_form_actual_instruction_comparison':form_result,'results':reports,'seconds':time.monotonic()-start,'stored_values_match':all(not(r['value_mismatches']) for r in reports),'passed':all(not(r['value_mismatches'] or r['flag_mismatches']) for r in reports) and not form_result['value_mismatches'],'scope':'actual1771-byte local body and real math/matrix/CRT; explicit fixture replies or stop boundaries for normal/hold/scene/reaction; not actual geometry or fullcollision'}
    result['limits']=([result['limits']] if isinstance(result.get('limits'),str) else list(result.get('limits') or []))+['Flag difference: the native x87 model sets the precision bit (PE 0x20) of the status word on inexact results; Unicorn 2.1.4 does not raise x87 exception bits for these instructions (oracle 0x0). All compared values match; the hardware flags need the Intel capture (M4).']
    a.output.write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
