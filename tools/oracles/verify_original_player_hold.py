"""Execute actual43b9c0 against native task/player/collision ownership.

Fully known fixture memory is separate from game files. Actual unmapped-null
reads stop at their source instruction; they never count as successful zero.
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
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,DATA,STACK,STOP

TASK=DATA+0x100;PLAYER=DATA+0x300;HELD=DATA+0x600;PRIMARY=DATA+0x700
MOTION=DATA+0x800;COLLISION=DATA+0x900;HIT=DATA+0xa00;OWNER=DATA+0xb00;ACTOR=DATA+0xc00
TABLE=0x3b3cdf0
def sha(data):return hashlib.sha256(data).hexdigest()

class HoldOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f);self.machine.mem_map(0x3b3c000,0x2000)
        self.fault=0
        def stop(machine,access,address,size,value,user):
            self.fault=machine.reg_read(UC_X86_REG_EIP);return False
        self.machine.hook_add(unicorn.UC_HOOK_MEM_READ_UNMAPPED,stop)

    def run(self,f):
        m=self.machine;self.fault=0
        def u32(at,value):m.mem_write(at,struct.pack('<I',value))
        m.mem_write(TASK+4,struct.pack('<H',f[0]&65535));m.mem_write(TASK+8,bytes((f[1]&255,f[2]&255)))
        u32(TABLE+(f[1]&255)*4,PLAYER if f[4] else 0);u32(PLAYER+0x68,HELD if f[5] else 0)
        u32(HELD+0x10,(0,0x40b570,0x51f680)[f[6]]);u32(HELD+0x24,MOTION if f[7] else 0)
        u32(HELD+0x20,PRIMARY if f[8] else 0);u32(PRIMARY+0x38,COLLISION if f[9] else 0)
        m.mem_write(PRIMARY+2,bytes((f[3]&255,)));u32(TASK+0x38,COLLISION if f[10] else 0)
        m.mem_write(COLLISION+4,struct.pack('<H',f[11]&65535));u32(COLLISION+0xa4,HIT if f[12] else 0)
        u32(HIT+0x9c,OWNER if f[13] else 0);u32(OWNER+0x20,ACTOR if f[14] else 0)
        m.mem_write(ACTOR+2,bytes((f[3]&255,)));m.mem_write(ACTOR+4,struct.pack('<H',f[15]&65535))
        m.mem_write(STACK,struct.pack('<II',STOP,TASK));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x43b9c0,STOP,count=500)
        except unicorn.UcError:
            if not self.fault:raise
        if self.fault:return dict(boundary=self.fault,flags=None)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Original hold did not return through cdecl')
        return dict(boundary=0,flags=m.reg_read(UC_X86_REG_EAX))

def fixtures():
    base=[0,0,0,4,1,1,2,1,1,1,1,5,1,1,1,0x40]
    for held in (False,True):
        for character in range(256):
            for actor in range(256):
                f=base.copy();f[0]=0x800 if held else 0;f[2]=character;f[3]=actor;yield f
    for index in range(256):
        f=base.copy();f[0]=0x800;f[1]=index;yield f
    for held in (False,True):
        for field in (4,5,6,7,8,9,10,12,13,14):
            for character in (0,7,8,255):
                f=base.copy();f[0]=0x800 if held else 0;f[2]=character;f[field]=0;yield f
        f=base.copy();f[0]=0x800 if held else 0;f[6]=1;yield f
    rng=random.Random(0x43b9c0)
    for _ in range(2000):
        f=base.copy();f[0]=rng.randrange(65536);f[1]=rng.randrange(256);f[2]=rng.randrange(256)
        f[3]=rng.randrange(256);f[11]=rng.randrange(65536);f[15]=rng.randrange(65536);yield f

def main():
    parser=argparse.ArgumentParser();parser.add_argument('exe',type=pathlib.Path)
    parser.add_argument('native_library',type=pathlib.Path);parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args();image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    lib=ctypes.CDLL(str(args.native_library.resolve()));probe=lib.sadx_native_player_hold
    probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    root=pathlib.Path(__file__).resolve().parents[2]
    report=dict(source_sha256=image.sha256,profile='actual43b9c0/no-overlays/read-only/native-Task-and-WorkArena',
        source_body=dict(address='0x43b9c0',size=0x141,sha256=sha(image.read(0x43b9c0,0x141))),
        source_tables=dict(address='0x43bb04',bytes=image.read(0x43bb04,40).hex()),
        native_library_sha256=sha(args.native_library.read_bytes()),
        native_sources={name:sha((root/name).read_bytes()) for name in ('src/player/player_hold.hpp','src/player/player_hold.cpp',
            'src/engine/task.hpp','src/engine/task.cpp','src/player/player_task.hpp','src/player/player_task.cpp','src/engine/task_work.hpp','src/engine/task_work.cpp',
            'src/player/player_task.hpp','src/player/player_physics.hpp','src/player/player_position.hpp',
            'tests/probes/native_player_hold_probe.cpp','tools/oracles/verify_original_player_hold.py')},
        cases=0,fault_cases=0,mismatches=[],limits=[
            'Fixtures explicitly supply observed memory facts; they are not an initialized game scene',
            'Unknown fields and live typed-state precedence are native owner tests, not invented oracle memory',
            'No executable, held task, controller, render or collision producer is run as a game',
            'Actual Windows reference/gameplay/physical controllers remain unverified'])
    oracle=HoldOriginal(image);start=time.monotonic();digest=hashlib.sha256()
    for index,f in enumerate(fixtures()):
        digest.update(struct.pack('<16I',*f));expected=oracle.run(f);out=(ctypes.c_uint32*6)()
        if probe((ctypes.c_uint32*16)(*f),out):raise RuntimeError(f'Native hold probe failed fixture{index}')
        actual=dict(boundary=out[1],flags=out[4] if out[3] else None)
        if bool(out[0])!=bool(expected['boundary']):
            report['mismatches'].append(dict(case=index,input=f,field='success/failure',source=expected,native=list(out)))
        elif actual!=expected:report['mismatches'].append(dict(case=index,input=f,source=expected,native=actual))
        report['cases']+=1;report['fault_cases']+=bool(expected['boundary'])
    report['case_sha256']=digest.hexdigest();report['seconds']=time.monotonic()-start
    report['passed']=not report['mismatches'];args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ('cases','fault_cases','seconds','passed')}))
    return int(not report['passed'])

if __name__=='__main__':raise SystemExit(main())
