"""Original43cb90 instructions against native material/parameter stores.

Source defaults and incoming work are explicit fixture seeds. Missing native
facts stop the original at the corresponding read; these are knowledge
boundaries, not an invented source index validation. No math calls are overlaid.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG)
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,DATA,STACK,STOP

CONSTANTS=((0x7dcc98,4),(0x7dd588,4),(0x7e0060,4),(0x7e007c,4),(0x7dd584,4),
           (0x43ccac,4),(0x43ccb9,4),(0x43ccc6,4),(0x43ccd2,4),(0x43ccda,4))
TASK=DATA+0x100;PLAYER=DATA+0x200;MOTION=DATA+0x500
def sha(data):return hashlib.sha256(data).hexdigest()
def word(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def boundary(w):
    char,shadow,_,incoming,*_=w
    reads=((0x43cba6 if char==5 else 0x43cbbd,1),(0x43cbd9,2),(0x43cbf5,4)) if shadow&2 else (
        ((0x43cc1e,1),(0x43cc3a,2),(0x43cc52,4)) if shadow&0x400000 else ((0x43cc6c,1),(0x43cc82,2),(0x43cc98,4)))
    for address,mask in reads:
        if not w[11]&mask:return address
    if w[12]:return 0
    low=incoming&255;slot=0 if low&8 else 1 if low&4 else 2 if low&16 else 3 if low&128 else 4
    return (0x43cca9,0x43ccb6,0x43ccc3,0x43cccf,0x43ccd7)[slot]

class AttributesOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f);self.stop_at=0
        self.machine.mem_map(0x915000,0x1000)
        self.machine.mem_write(0x9154e8,image.read(0x9154e8,8*132))
        def hook(machine,address,size,user):
            if address==self.stop_at:machine.emu_stop()
        self.machine.hook_add(unicorn.UC_HOOK_CODE,hook,begin=0x43cb90,end=0x43ccde)
    def run(self,w,cw):
        m=self.machine;self.stop_at=boundary(w)
        m.mem_write(TASK,bytes(64));m.mem_write(TASK+9,bytes([w[0]]))
        m.mem_write(PLAYER,bytes(512));m.mem_write(PLAYER+0x1e4,struct.pack('<I',w[1]));m.mem_write(PLAYER+0x5c,struct.pack('<I',w[2]))
        for offset,value in zip((0x114,0xf4,0xd8),w[4:7]):m.mem_write(PLAYER+offset,struct.pack('<I',value))
        m.mem_write(MOTION,bytes(64));m.mem_write(MOTION+0x3c,struct.pack('<I',w[7]))
        # These typed caller seeds model the three source default fields only.
        for address,value in zip((0x915560,0x915540,0x915524),w[8:11]):
            if w[0]<8:m.mem_write(address+w[0]*132,struct.pack('<I',value))
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        for register,value in ((UC_X86_REG_EAX,PLAYER),(UC_X86_REG_EBX,w[3]),(UC_X86_REG_ECX,TASK),(UC_X86_REG_ESI,MOTION if w[12] else 0)):
            m.reg_write(register,value)
        m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(0x43cb90,STOP,count=2000)
        if not self.stop_at and m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Source material ABI did not return')
        values=[struct.unpack('<I',m.mem_read(PLAYER+o,4))[0] for o in (0x5c,0x114,0xf4,0xd8)]
        values += [struct.unpack('<I',m.mem_read(MOTION+0x3c,4))[0]]
        return dict(boundary=self.stop_at,state=values,flags=m.reg_read(UC_X86_REG_FPSW)&63,cw=m.reg_read(UC_X86_REG_FPCW))

def fixtures(image):
    cases=[]
    def add(char,shadow,incoming,defaults,mask=7,motion=1):
        cases.append([char,shadow,0x12340000,incoming,*map(word,(11,12,13,14)),*defaults,mask,motion])
    for char in range(8):
        defaults=[image.unpack('<I',address+char*132)[0] for address in (0x915560,0x915540,0x915524)]
        for shadow in (0,2,0x400000,0x400002):
            for incoming in range(256):add(char,shadow,incoming,defaults)
            for incoming in (0x80000000,0xffffffff,0x12340080,0x12340000):add(char,shadow,incoming,defaults)
    raw=(0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,
         0x7fc12345,0xffc12345,0x7f800123,0xff800123)
    for char in (0,5):
        for shadow in (0,2,0x400000,0x400002):
            for value in raw:add(char,shadow,0x80,[value,value,value])
            for mask in range(7):add(char,shadow,4,list(map(word,(2,3,4))),mask)
            for incoming in (8,4,16,128,0):add(char,shadow,incoming,list(map(word,(2,3,4))),motion=0)
    for char in (8,127,255):
        for shadow in (0,2,0x400000):add(char,shadow,0x80000080,[0,0,0],mask=0)
    rng=random.Random(0x43cb90)
    for _ in range(300):add(rng.randrange(8),rng.choice((0,2,0x400000,0x400002)),rng.getrandbits(32),[rng.getrandbits(32) for _ in range(3)])
    return cases

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    data=b''.join(image.read(address,size) for address,size in CONSTANTS);constants=(ctypes.c_uint8*40).from_buffer_copy(data)
    library=ctypes.CDLL(str(a.library.resolve()));probe=library.sadx_native_contact_attributes
    probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    cases=fixtures(image);blob=b''.join(struct.pack('<13I',*w) for w in cases);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parent.parent
    report=dict(source_sha256=image.sha256,profile='actual43cb90/native-arm64; source default/read/null-motion fixture boundaries',
        native_library_sha256=sha(a.library.read_bytes()),function_sha256=sha(image.read(0x43cb90,0x14f)),constants_sha256=sha(data),
        native_sources={name:sha((root/name).read_bytes()) for name in ('src/player/player_contact_attributes.cpp','src/player/player_contact_attributes.hpp','tests/probes/native_player_contact_attributes_probe.cpp','tools/oracles/verify_original_player_contact_attributes.py')},
        case_sha256=sha(blob),profiles=[],mismatches=[],limits=['No original instructions or math calls overlaid','Default table fields are explicit selected fixture values; actual initialized modded runtime is not observed','Missing default facts and null motion stop at original source instruction before the read/store','All byte0..255 friction priorities and nonfinites are included; Unicorn flags remain a weaker oracle','Full scene contact, hardware input and speedrun timing are not exercised'])
    oracle=AttributesOriginal(image)
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        item=dict(cw=hex(cw),cases=len(cases),state_mismatches=0,boundary_mismatches=0,control_mismatches=0,flag_mismatches=0)
        for index,w in enumerate(cases):
            expected=oracle.run(w,cw);out=(ctypes.c_uint32*9)();status=probe(constants,(ctypes.c_uint32*13)(*w),cw,out)
            if status:raise RuntimeError(f'Native material failedcase{index}: {status}')
            actual=dict(boundary=out[1],state=list(out[2:7]),flags=out[7],cw=out[8]);different=[]
            for field,counter in (('boundary','boundary_mismatches'),('state','state_mismatches'),('cw','control_mismatches'),('flags','flag_mismatches')):
                if actual[field]!=expected[field]:item[counter]+=1;different.append(field)
            if any(field!='flags' for field in different):report['mismatches'].append(dict(cw=hex(cw),case=index,inputs=list(map(hex,w)),fields=different,source=expected,native=actual))
        report['profiles'].append(item);print(json.dumps(item),flush=True)
    report['state_boundaries_control_match']=all(not any(p[key] for key in ('state_mismatches','boundary_mismatches','control_mismatches')) for p in report['profiles'])
    report['full_emulator_match']=report['state_boundaries_control_match'] and all(not p['flag_mismatches'] for p in report['profiles'])
    a.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['state_boundaries_control_match'])
if __name__=='__main__':raise SystemExit(main())
