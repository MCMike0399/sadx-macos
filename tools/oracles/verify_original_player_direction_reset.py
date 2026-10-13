"""Complete43c130 with real path, plane, CRT and vector-angle instructions.

No path/angle callbacks replaced. Supported fixture resources and explicit
runtime inputs; unknown/native-null ownership states are separate owner tests.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_MXCSR
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import DATA,STACK,STOP
from verify_original_player_path import source_setup,seed_path,path_constants,point_words,write_blob,fword,sha
TASK=DATA+0x1800;PLAYER=DATA+0x2000;MOTION=DATA+0x2600;CONTACT=DATA+0x3000;FRAME=STACK-0x90
SEED=0x43c13049c330
def reset_constants(image):return path_constants(image)+image.read(0x7dcb0c,4)+image.read(0x7dccd0,8)+image.read(0x7dccc8,8)+image.read(0x7e0b40,8)
class ResetOriginal:
    def __init__(self,image,cw):
        self.machine=source_setup(image,cw).machine
        for page in (0x3b0e000,0x3b22000,0x3b3c000):self.machine.mem_map(page,0x1000)
    def run(self,w,cw):
        from verify_original_player_path import PATH
        m=self.machine;seed_path(m,w[25],w[26],w[28],w[36:]);task=bytearray(64);struct.pack_into('<H',task,4,w[0]);struct.pack_into('<I',task,0x10,PATH);struct.pack_into('<3I',task,0x14,*w[1:4]);struct.pack_into('<3I',task,0x20,*w[5:8]);m.mem_write(TASK,bytes(task));player=bytearray(0x240);struct.pack_into('<I',player,0x18,w[10]);struct.pack_into('<3I',player,0x38,*w[11:14]);struct.pack_into('<I',player,0x64,CONTACT);struct.pack_into('<I',player,0xd0,w[18]);m.mem_write(PLAYER,bytes(player));motion=bytearray(0x50);struct.pack_into('<3I',motion,4,*w[14:17]);struct.pack_into('<I',motion,0x20,w[17]);m.mem_write(MOTION,bytes(motion));m.mem_write(CONTACT+0x1c,struct.pack('<3I',*w[21:24]));m.mem_write(0x3b22dcc,struct.pack('<H',1));m.mem_write(0x3b3cdf0,struct.pack('<I',PLAYER));m.mem_write(0x3b0e7a0,struct.pack('<II',0,w[24]));m.mem_write(0x3d0f400,struct.pack('<I',w[30]));m.mem_write(0x2bcc034,struct.pack('<H',w[33]));m.mem_write(0x3c837a8,struct.pack('<I',w[35]));m.mem_write(FRAME,bytes(0x90));m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_EAX,PLAYER);m.reg_write(UC_X86_REG_EBX,MOTION);m.reg_write(UC_X86_REG_EDI,TASK);m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_MXCSR,w[31]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.emu_start(0x43c130,STOP,count=50000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Original reset ABI mismatch')
        return dict(flags=struct.unpack('<H',m.mem_read(TASK+4,2))[0],yaw=struct.unpack('<I',m.mem_read(TASK+0x18,4))[0],target=struct.unpack('<I',m.mem_read(MOTION+0x20,4))[0],speed=struct.unpack('<I',m.mem_read(PLAYER+0x38,4))[0],distance=struct.unpack('<I',m.mem_read(PLAYER+0x18,4))[0],cw=m.reg_read(UC_X86_REG_FPCW),fp_flags=m.reg_read(UC_X86_REG_FPSW)&0x3f,mxcsr=m.reg_read(UC_X86_REG_MXCSR),status=list(struct.unpack('<20I',m.mem_read(FRAME+0x40,80))),projected_delta=list(struct.unpack('<3I',m.mem_read(FRAME+0x10,12))),projected_velocity=list(struct.unpack('<3I',m.mem_read(FRAME+0x34,12))))
def fixtures():
    rng=random.Random(SEED);out=[]
    def case(type,distance,speed,cpu,position=(0,0,0),normal=(0,1,0),world=(1,0,0),yaw=0,target=0,active=1):
        points=[v for i in range(4) for v in point_words((i*2000-3000,i*4000-6000),4,(i*4,i,0))]
        return [0x2000,0,yaw,0,1,*map(fword,position),2,1,fword(distance),fword(speed),0,0,*map(fword,world),target,fword(1),1,1,*map(fword,normal),fword(active),type,3,4,fword(12),1,cpu,0x1f80,1,0x027f,1,1,*points]
    for type in (0,1):
        for distance in (-3,0,1,4,8,12,14):
            for speed in (0,0.5,2):
                for cpu in (0,1):
                    for active in (0,1):out.append(case(type,distance,speed,cpu,active=active))
    for _ in range(500):
        out.append(case(rng.randrange(2),rng.uniform(-12,20),rng.uniform(-4,8),rng.randrange(2),tuple(rng.uniform(-5,5) for _ in range(3)),tuple(rng.uniform(-1,1) for _ in range(3)),tuple(rng.uniform(-5,5) for _ in range(3)),rng.getrandbits(32),rng.getrandbits(32),rng.randrange(2)))
    out.extend(case(0,0,2,1,world=v) for v in ((0,0,0),(0,1,0)))
    return out
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires Unicorn2.1.4')
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source image changed')
    table=image.read(TABLE_ADDRESS,4100);unit=image.read(0x389d650,64);constants=reset_constants(image);cases=fixtures();args.output.parent.mkdir(parents=True,exist_ok=True)
    lib=ctypes.CDLL(str(args.native_library.resolve()));fn=lib.sadx_native_player_direction_reset;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t]*3+[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int;buffers=[(ctypes.c_ubyte*len(v)).from_buffer_copy(v) for v in (table,unit,constants)]
    report=dict(profile='complete43c130 with actual path/plane/normalize/angle/CRT/input; no overlays',source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),constants_sha256=sha(constants),table_sha256=sha(table),unit_sha256=sha(unit),seed=hex(SEED),case_blob=write_blob(args.output.with_suffix('.cases.bin'),cases),source_functions={hex(a):dict(size=b-a,sha256=sha(image.read(a,b-a))) for a,b in ((0x43c130,0x43c2bf),(0x788d40,0x788de6))},control_words=[],limits=['Caller-supplied finite path/task/contact/runtime fixtures, not captured game ticks','Both actual CRT floor routes and explicitly selected acos matherr bypass1','Work/vector checks cover observed source stores; undefined scratch is not fabricated','Original hardware flags and mod/startup FP state remain unverified'])
    for cw in (p|r|0x7f for p in (0,0x200,0x300) for r in (0,0x400,0x800,0xc00)):
        original=ResetOriginal(image,cw);item=dict(control_word=hex(cw),cases=len(cases),value_control_mismatches=0,flag_mismatches=0,examples=[],flag_examples=[])
        for index,w in enumerate(cases):
            expected=original.run(w,cw);out=(ctypes.c_uint32*50)();status=fn(buffers[0],4100,buffers[1],64,buffers[2],120,(ctypes.c_uint32*len(w))(*w),len(w),cw,out,50)
            if status or out[0]:raise RuntimeError(f'Native rejected case{index}: status{status},failure{out[0]},source{out[1]:x},nested{out[2]:x}')
            actual=[out[3],out[4],out[5],out[6],out[8],out[9]];desired=[expected[k] for k in ('flags','yaw','target','speed','distance','cw')];different=actual!=desired
            for k in range(20):
                if out[12]&(1<<k):different|=out[13+k]!=expected['status'][k]
            if out[33]:different|=list(out[34:37])!=expected['projected_delta']
            if out[37]:different|=list(out[38:41])!=expected['projected_velocity']
            flags=out[10]!=expected['fp_flags'] or out[11]!=expected['mxcsr'];item['value_control_mismatches']+=different;item['flag_mismatches']+=flags
            if different:item['examples'].append(dict(case=index,original=expected,native=list(out)))
            if flags and len(item['flag_examples'])<5:item['flag_examples'].append(dict(case=index,original_flags=expected['fp_flags'],native_flags=out[10],original_mxcsr=hex(expected['mxcsr']),native_mxcsr=hex(out[11])))
        report['control_words'].append(item);print(json.dumps({k:v for k,v in item.items() if k not in ('examples','flag_examples')}),flush=True)
    report['state_work_vectors_and_control_match']=all(i['value_control_mismatches']==0 for i in report['control_words']);args.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['state_work_vectors_and_control_match'])
if __name__=='__main__':raise SystemExit(main())
