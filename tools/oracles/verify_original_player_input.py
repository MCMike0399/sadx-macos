"""Compare recovered GetAnalog against supplied i386 bytes, using explicit seeds.

No PE startup, mod DLL execution, Windows process, physical input or frame-loop
is exercised. Forced angle-output FPATAN and CRT _ftol execute the real source
instructions; native constants are read from the same source PE ranges.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct
import time

import unicorn
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original, CODE, DATA, STACK, STOP

TASK, EVENT, PLAYER, CAMERA = DATA+0x100, DATA+0x200, DATA+0x400, DATA+0x800
ANGLE, MAGNITUDE = DATA+0xc00, DATA+0xc04

def words_float(value):
    return struct.unpack('<I',struct.pack('<f',value))[0]

def forced_angle_path(words, mask):
    flags, _, _, _, _, stage, act, current = words[:8]
    return bool(mask&1 and not flags&0x4000 and flags&0x2000 and current==0 and
                ((stage==2 and act==2) or (stage==4 and act==0)))

def cases():
    result=[]
    magnitudes=[0,0x80000000,1,0x80000001,0x007fffff,0x807fffff,words_float(.25),
                words_float(-1),0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345]
    for flags in (0,0x100,0x2000,0x2100,0x4000,0x6000):
        for stage,act in ((1,0),(2,2),(2,1),(4,0),(4,1),(9,2),(9,3),(9,-1),(-1,2)):
            for character in (0,1):
                for magnitude in magnitudes:
                    for timer in (0,1,65535):
                        for mask in range(4):
                            item=[flags,3,character,0x87654321,255,stage,act,0,0xfedcba98,magnitude,timer,0x80000000]
                            result.append((item,mask,'systematic'))
    generator=random.Random(0x43bdc0)
    for _ in range(1000):
        item=[generator.choice((0,0x100,0x2000,0x2100,0x4000)),generator.randrange(8),
              generator.randrange(8),generator.getrandbits(32),generator.randrange(256),
              generator.choice((-32768,-1,0,1,2,4,9,32767)),generator.choice((-32768,-1,0,1,2,3,32767)),
              generator.randrange(8),generator.getrandbits(32),generator.getrandbits(32),
              generator.randrange(65536),generator.getrandbits(32)]
        mask=generator.randrange(4)
        result.append((item,mask,'raw-random'))
    return result

class OriginalInput(Original):
    def __init__(self,image,control_word):
        super().__init__(image,control_word)
        # Sparse game-global pages. Values are explicitly seeded, not claimed
        # to reflect BSS startup or actual runtime state.
        for page in (0x3b0e000,0x3b22000,0x3b2c000,0x3b3c000):
            self.machine.mem_map(page,0x1000)
        self.machine.mem_write(CODE+0x200,b'\xdb\xe2') # actual FNCLEX, not just accessor reset
        self.comparison_status=None
        self.machine.hook_add(UC_HOOK_CODE,self.record_comparison_status,begin=0x43be87,end=0x43be87)
    def record_comparison_status(self,machine,address,size,user_data):
        # Exactly after original FNSTSW AX, before TEST AH,0x44.
        self.comparison_status=machine.reg_read(UC_X86_REG_EAX)&0xffff
    def run(self,words,mask,control_word):
        flags,index,character,yaw,event,stage,act,current,input_angle,magnitude,timer,camera_yaw=words
        task=bytearray(64); struct.pack_into('<H',task,4,flags&0xffff)
        task[8]=index; task[9]=character; struct.pack_into('<I',task,0x18,yaw)
        struct.pack_into('<I',task,0x3c,EVENT)
        self.machine.mem_write(TASK,bytes(task));self.machine.mem_write(EVENT+0x24,bytes([event]))
        for address,value in ((0x3b22dcc,stage),(0x3b22dec,act),(0x3b22dc0,current)):
            self.machine.mem_write(address,struct.pack('<H',value&0xffff))
        self.machine.mem_write(0x3b0e7a0+index*8,struct.pack('<II',input_angle,magnitude))
        self.machine.mem_write(0x3b3cdf0+index*4,struct.pack('<I',PLAYER))
        self.machine.mem_write(PLAYER+0xa,struct.pack('<H',timer))
        self.machine.mem_write(0x3b2cbb0,struct.pack('<I',CAMERA))
        self.machine.mem_write(CAMERA+0x18,struct.pack('<I',camera_yaw))
        self.machine.mem_write(ANGLE,struct.pack('<II',0xa1b2c3d4,0xdeadbeef))
        self.machine.mem_write(STACK,struct.pack('<4I',STOP,TASK,ANGLE if mask&1 else 0,MAGNITUDE if mask&2 else 0))
        self.machine.reg_write(UC_X86_REG_ESP,STACK);self.machine.reg_write(UC_X86_REG_FPCW,control_word)
        self.machine.emu_start(CODE+0x200,CODE+0x202,count=1)
        self.comparison_status=None
        self.machine.emu_start(0x43bdc0,STOP,count=500)
        if self.machine.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Original GetAnalog ABI return mismatch')
        angle,magnitude=struct.unpack('<II',self.machine.mem_read(ANGLE,8))
        accessor_flags=self.machine.reg_read(UC_X86_REG_FPSW)&0x3f
        flags=accessor_flags if self.comparison_status is None else self.comparison_status&0x3f
        return [self.machine.reg_read(UC_X86_REG_EAX),angle,magnitude,flags,0],accessor_flags,self.comparison_status

def main():
    parser=argparse.ArgumentParser();parser.add_argument('exe',type=pathlib.Path)
    parser.add_argument('native_library',type=pathlib.Path);parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires pinned Unicorn2.1.4')
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source executable identity mismatch')
    library=ctypes.CDLL(str(args.native_library.resolve()));library.sadx_native_player_input.argtypes=[
        ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_double),ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];library.sadx_native_player_input.restype=ctypes.c_int
    constant_addresses=(0x7e0050,0x7e0048,0x7dccd0,0x7dcce8)
    constants=(ctypes.c_double*4)(*[struct.unpack('<d',image.read(address,8))[0] for address in constant_addresses])
    fixtures=cases();blob=b''.join(struct.pack('<13I',*[x&0xffffffff for x in words],mask) for words,mask,_ in fixtures)
    fixture_path=args.output.with_suffix('.cases.bin');fixture_path.parent.mkdir(parents=True,exist_ok=True);fixture_path.write_bytes(blob)
    report={'profile':'GetAnalog exact i386 bytes / native arm64 / explicit external globals',
            'source_sha256':image.sha256,'source_function_va':'0x43bdc0',
            'source_function_sha256':hashlib.sha256(image.read(0x43bdc0,0x174)).hexdigest(),
            'native_library_sha256':hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
            'forced_angle_constants':[{'address':hex(address),'binary64_hex':image.read(address,8).hex()} for address in constant_addresses],
            'fixture_sha256':hashlib.sha256(blob).hexdigest(),'fixture_count':len(fixtures),
            'forced_angle_case_count':sum(forced_angle_path(words,mask) for words,mask,_ in fixtures),
            'control_words':[],'limitations':['Unicorn oracle, not original x86 hardware',
                'Explicit experimental FPCWs and seeded globals; actual runtime state is unobserved',
                'No Windows startup, updated mod DLL execution or physical game input',
                'Forced-angle FPATAN uses an explicit Bochs P5/P6 model; Intel Ultra hardware is unverified',
                'Native InputState/WriteAnalogs producer and complete gameplay are not verified']}
    failure=False;started=time.monotonic()
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        original=OriginalInput(image,cw);item={'control_word':hex(cw),'cases':len(fixtures),'mismatches':0,
            'value_mismatches':0,'exception_mismatches':0,'accessor_instruction_exception_mismatches':0,'examples':[]}
        for words,mask,label in fixtures:
            expected,accessor_flags,comparison_status=original.run(words,mask,cw);output=(ctypes.c_uint32*5)();raw=(ctypes.c_uint32*12)(*[x&0xffffffff for x in words])
            code=library.sadx_native_player_input(raw,12,cw,mask,constants,4,output,5)
            if code:raise RuntimeError('Native query probe rejected a complete seed: '+str(code))
            actual=list(output)
            if actual[:3]+actual[4:]!=expected[:3]+expected[4:]:item['value_mismatches']+=1
            if actual[3]!=expected[3]:item['exception_mismatches']+=1
            if accessor_flags!=expected[3]:item['accessor_instruction_exception_mismatches']+=1
            if actual!=expected:
                item['mismatches']+=1;failure=True
                if len(item['examples'])<8:item['examples'].append({'label':label,'words':[hex(x&0xffffffff) for x in words],
                    'outputs_requested':mask,'expected':[hex(x) for x in expected],'actual':[hex(x) for x in actual],
                    'unicorn_fpsw_accessor_flags':hex(accessor_flags),'source_fnstsw_ax':hex(comparison_status) if comparison_status is not None else None})
        report['control_words'].append(item);print(json.dumps({k:v for k,v in item.items() if k!='examples'}),flush=True)
    report['elapsed_seconds']=time.monotonic()-started;report['passed']=not failure
    report['value_outputs_passed']=all(item['value_mismatches']==0 for item in report['control_words'])
    report['exception_flags_match']=all(item['exception_mismatches']==0 for item in report['control_words'])
    report['exception_oracle_note']='Unicorn2.1.4 FNSTSW/accessor omit tested denormal/invalid flags; native architecture flags retained; actual PC exception parity unverified.'
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    if failure:raise SystemExit(1)

if __name__=='__main__':main()
