"""Original WriteAnalogs and real Input2.6.1 return-hook instructions.

The two module images are isolated emulator mappings with explicit test-owned
globals/pads. Updated mode applies the actual Init operand fix and Jumps table
target before any execution, not a replacement high-level hook test double.
This executes no Windows startup/imports, physical input or game frame loop.
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
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original, CODE, DATA, STACK, STOP

INPUT_SHA='c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5'
PAD, CAMERA=DATA+0x100,DATA+0x800
CONSTANTS=[(0x7dccf4,4),(0x7dccf0,4),(0x7dcce0,4),(0x7dcb10,4),
           (0x7dccd0,8),(0x7dccc8,8),(0x7dcce8,8)]

def word(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def case(x,y,control,camera,enabled):
    values=[control,camera,0x80012345,x,y,0,0]+[enabled,1,1,0,0,0,0,0]
    for slot in range(8):values += [slot%2,1,x if slot==0 else 127,y if slot==0 else 0,word(.375+slot/16)]
    return values
def cases():
    output=[]
    for x in (-32768,-127,-126,-100,-13,-12,-1,0,1,12,13,100,126,127,32767):
        for y in (-32768,-127,-126,-100,-13,-12,-1,0,1,12,13,100,126,127,32767):
            for control in (0,1,2):
                for camera in (0,1):
                    for enabled in (0,1,2):output.append(case(x,y,control,camera,enabled))
    rng=random.Random(0x40f170)
    for _ in range(1000):
        item=case(rng.randrange(-32768,32768),rng.randrange(-32768,32768),rng.randrange(3),rng.randrange(2),rng.randrange(3))
        item[2]=rng.getrandbits(32);item[5:7]=[rng.randrange(-32768,32768) for _ in range(2)]
        for slot in range(8):
            item[7+slot]=rng.randrange(3);base=15+slot*5
            item[base:base+5]=[rng.randrange(3),rng.randrange(3),rng.randrange(-32768,32768),rng.randrange(-32768,32768),rng.getrandbits(32)]
        output.append(item)
    return output

class OriginalProducer(Original):
    def __init__(self,image,mod_image,cw,updated):
        super().__init__(image,cw)
        for address in (0x909000,0x3b0e000,0x3b2c000):self.machine.mem_map(address,0x1000)
        for section in mod_image.sections:
            if section['name'] not in ('.text','.rdata','.data'):continue
            length=(max(section['raw_size'],section['virtual_size'])+4095)&~4095
            self.machine.mem_map(mod_image.base+section['rva'],length)
            self.machine.mem_write(mod_image.base+section['rva'],mod_image.slice(section['file_offset'],section['raw_size']))
        if updated:
            # Real Input2.6.1 Init redirects the original enabled-table operand.
            self.machine.mem_write(0x40f30c,struct.pack('<I',0x1000e73c))
            # Real exported Jumps pair maps the return instruction to this hook.
            self.machine.mem_write(0x40f343,b'\xe9'+struct.pack('<i',0x10004b70-(0x40f343+5)))
        self.machine.mem_write(CODE+0x200,b'\xdb\xe2')
    def run(self,values,cw):
        control,camera,yaw=values[:3]
        self.machine.mem_write(0x909fb0,bytes([control]))
        self.machine.mem_write(0x909fb4,bytes(values[7:15]))
        self.machine.mem_write(0x1000e73c,bytes(values[7:15]))
        self.machine.mem_write(0x3b2cbb0,struct.pack('<I',CAMERA if camera else 0))
        self.machine.mem_write(CAMERA+0x18,struct.pack('<I',yaw))
        for slot in range(2):
            self.machine.mem_write(0x3b0e77c+slot*4,struct.pack('<I',PAD+slot*0x80))
            self.machine.mem_write(PAD+slot*0x80+0x1c,struct.pack('<hh',*values[3+slot*2:5+slot*2]))
        for slot in range(8):
            connected,keyboard,x,y,magnitude=values[15+slot*5:20+slot*5];offset=slot*0x60
            self.machine.mem_write(0x1000e98c+offset,bytes([connected]));self.machine.mem_write(0x1000e99c+offset,bytes([keyboard]))
            self.machine.mem_write(0x1000e96c+offset,struct.pack('<hh',x,y));self.machine.mem_write(0x1000e994+offset,struct.pack('<I',magnitude))
        self.machine.mem_write(STACK,struct.pack('<I',STOP));self.machine.reg_write(UC_X86_REG_ESP,STACK)
        self.machine.reg_write(UC_X86_REG_FPCW,cw);self.machine.emu_start(CODE+0x200,CODE+0x202,count=1)
        self.machine.emu_start(0x40f170,STOP,count=3000)
        if self.machine.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Original producer did not return with expected ABI')
        output=list(struct.unpack('<16I',self.machine.mem_read(0x3b0e368,64)))
        output+=list(struct.unpack('<16I',self.machine.mem_read(0x3b0e7a0,64)))
        return output+[self.machine.reg_read(UC_X86_REG_FPSW)&0x3f,0,0,0]

def main():
    parser=argparse.ArgumentParser();parser.add_argument('exe',type=pathlib.Path);parser.add_argument('input_mod',type=pathlib.Path)
    parser.add_argument('native_library',type=pathlib.Path);parser.add_argument('--output',type=pathlib.Path,required=True);args=parser.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires pinned Unicorn2.1.4')
    image,mod_image=PEImage(args.exe),PEImage(args.input_mod)
    if image.sha256!=SOURCE_SHA256 or mod_image.sha256!=INPUT_SHA:raise RuntimeError('Instruction/module identity mismatch')
    constants=b''.join(image.read(address,size) for address,size in CONSTANTS);buffer=(ctypes.c_uint8*40).from_buffer_copy(constants)
    library=ctypes.CDLL(str(args.native_library.resolve()));library.sadx_native_player_input_producer.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.c_uint16,ctypes.c_bool,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t]
    library.sadx_native_player_input_producer.restype=ctypes.c_int
    fixtures=cases();blob=b''.join(struct.pack('<55I',*[x&0xffffffff for x in item]) for item in fixtures)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.with_suffix('.cases.bin').write_bytes(blob)
    report={'profile':'original WriteAnalogs / actual Input2.6.1 hook instructions / native BochsP5P6 model',
        'source_sha256':image.sha256,'input_mod_sha256':mod_image.sha256,'constants_sha256':hashlib.sha256(constants).hexdigest(),
        'native_library_sha256':hashlib.sha256(args.native_library.read_bytes()).hexdigest(),'fixture_sha256':hashlib.sha256(blob).hexdigest(),
        'fixture_count':len(fixtures),'runs':[],'limitations':['No Windows startup or physical input','Explicit experimental CW/global/pad seeds',
            'BochsP5P6 FPATAN vs Unicorn2.1.4 host-double transcendental oracle; hardware Intel Ultra parity unverified',
            'Unicorn x87 exception omissions retained as a separate mismatch','Prepared controller/keyboard states do not verify their upstream SSE producer']}
    started=time.monotonic();failed=False
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        for updated in (False,True):
            original=OriginalProducer(image,mod_image,cw,updated);run={'control_word':hex(cw),'updated_hook':updated,'cases':len(fixtures),'value_mismatches':0,'exception_mismatches':0,'examples':[]}
            for values in fixtures:
                expected=original.run(values,cw);raw=(ctypes.c_uint32*55)(*[x&0xffffffff for x in values]);output=(ctypes.c_uint32*36)()
                code=library.sadx_native_player_input_producer(raw,55,buffer,40,cw,updated,output,36)
                if code:raise RuntimeError('Native producer probe rejected fully supplied seed: '+str(code))
                actual=list(output)
                mismatch=actual[:32]+actual[33:]!=expected[:32]+expected[33:]
                if mismatch:
                    run['value_mismatches']+=1;failed=True
                    if len(run['examples'])<4:run['examples'].append({'words':[hex(x&0xffffffff) for x in values],'expected':[hex(x) for x in expected[:32]],'actual':[hex(x) for x in actual[:32]]})
                if actual[32]!=expected[32]:run['exception_mismatches']+=1
            report['runs'].append(run);print(json.dumps({k:v for k,v in run.items() if k!='examples'}),flush=True)
    report['value_outputs_passed']=not failed;report['exception_flags_match']=all(r['exception_mismatches']==0 for r in report['runs'])
    report['passed']=report['value_outputs_passed'] and report['exception_flags_match'];report['elapsed_seconds']=time.monotonic()-started
    report['limits']=([report['limits']] if isinstance(report.get('limits'),str) else list(report.get('limits') or []))+['Flag difference (exception_mismatches): the native x87/MXCSR exception bits are set where Unicorn 2.1.4 does not raise them; values, boundaries and call order match. All compared values match; the hardware flags need the Intel capture (M4).']
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
