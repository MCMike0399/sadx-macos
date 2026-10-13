"""Exact coordinate/matrix instruction comparison; no rotation test doubles.

Matrix storage/depth/normalization and input vectors/angles are explicit test
seeds. Source unit matrix and quarter table are loaded from the supplied PE.
This does not execute startup, full gameplay or original x86 hardware.
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
from unicorn import UC_PROT_READ,UC_PROT_WRITE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from extract import SOURCE_SHA256,TABLE_ADDRESS
from pe_image import PEImage
from verify_original_math import Original,DATA,STACK,STOP

MATRIX=DATA+0x4000
VECTOR=DATA+0x200
TASK=DATA+0x100
UNIT_ADDRESS=0x389D650
GLOBALS=0x3D0F000
SEED=0x43EC00


def sha(data):return hashlib.sha256(data).hexdigest()
def fword(value):return struct.unpack('<I',struct.pack('<f',value))[0]


def fixtures(unit):
    words=list(struct.unpack('<16I',unit));cases=[]
    for mode in range(6):
        for angle in (0,1,15,16,16384,32768,49152,65535,65536,-1,-65536,-2147483648,2147483647):
            for vector in ((1,2,3),(0,0,0),(-0.0,1,-0.0)):
                for flag in ((0,1) if mode in (0,1,5) else (0,)):
                    cases.append([mode,angle&0xffffffff,angle&0xffffffff,angle&0xffffffff,
                                  *map(fword,vector),1,flag,0,*words*6])
    randoms=random.Random(SEED)
    for _ in range(1500):
        mode=randoms.randrange(6);framewords=[]
        for _ in range(6):framewords.extend(map(fword,(randoms.uniform(-4,4) for _ in range(16))))
        angles=[randoms.randrange(-2147483648,2147483648)&0xffffffff for _ in range(3)]
        cases.append([mode,*angles,*map(fword,(randoms.uniform(-100,100) for _ in range(3))),
                      randoms.choice((1,2,5,6)),randoms.choice((0,1,0x80000000)),
                      int(mode==1 and randoms.randrange(10)==0),*framewords])
    for raw in (1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345):
        for mode in (0,1,5):
            for index in range(3):
                for flag in (0,1):
                    case=[mode,123,456,789,fword(1),fword(2),fword(3),1,flag,0,*words*6]
                    case[4+index]=raw;cases.append(case)
    return cases


class CoordinatesOriginal(Original):
    def __init__(self,image,control_word,unit):
        super().__init__(image,control_word)
        self.machine.mem_map(0x389D000,0x1000,UC_PROT_READ|UC_PROT_WRITE)
        self.machine.mem_write(UNIT_ADDRESS,unit)
        self.machine.mem_map(GLOBALS,0x1000,UC_PROT_READ|UC_PROT_WRITE)

    def run(self,case,cw):
        mode=case[0];depth=case[7];current=MATRIX+(depth-1)*64
        self.machine.mem_write(MATRIX,struct.pack('<96I',*case[10:106]))
        self.machine.mem_write(VECTOR,struct.pack('<3I',*case[4:7]))
        task=bytearray(64);struct.pack_into('<3I',task,0x14,*case[1:4]);self.machine.mem_write(TASK,bytes(task))
        for address,value in ((0x3d0f818,current),(0x3d0fa28,6),(0x3d0fa2c,depth),(0x3d0f7ac,case[8])):
            self.machine.mem_write(address,struct.pack('<I',value))
        if mode in (0,1):
            address=(0x43ec00,0x43ec90)[mode]
            self.machine.mem_write(STACK,struct.pack('<3I',STOP,0 if case[9] else TASK,VECTOR))
        elif mode in (2,3,4):
            address={2:0x784ee0,3:0x784e20,4:0x784fb0}[mode]
            self.machine.mem_write(STACK,struct.pack('<I',STOP))
            self.machine.reg_write(UC_X86_REG_ECX,current)
            self.machine.reg_write(UC_X86_REG_EDX,case[mode-1])
        else:
            address=0x784d60
            self.machine.mem_write(STACK,struct.pack('<2I',STOP,VECTOR))
            self.machine.reg_write(UC_X86_REG_ECX,current)
            self.machine.reg_write(UC_X86_REG_EDX,VECTOR)
        self.machine.reg_write(UC_X86_REG_ESP,STACK)
        self.machine.reg_write(UC_X86_REG_FPCW,cw)
        self.machine.reg_write(UC_X86_REG_FPSW,self.machine.reg_read(UC_X86_REG_FPSW)&~0x3f)
        self.machine.emu_start(address,STOP,count=5000)
        expected_stack=STACK+(8 if mode==5 else 4)
        if self.machine.reg_read(UC_X86_REG_ESP)!=expected_stack:
            raise RuntimeError('Original matrix function failed to return with expected ABI')
        vector=struct.unpack('<3I',self.machine.mem_read(VECTOR,12))
        depth=struct.unpack('<I',self.machine.mem_read(0x3d0fa2c,4))[0]
        frames=struct.unpack('<96I',self.machine.mem_read(MATRIX,384))
        return vector,depth,frames,self.machine.reg_read(UC_X86_REG_FPSW)&0x3f


def main():
    parser=argparse.ArgumentParser();parser.add_argument('exe',type=pathlib.Path)
    parser.add_argument('native_library',type=pathlib.Path);parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires Unicorn2.1.4 profile')
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Unknown original function-address identity')
    table=image.read(TABLE_ADDRESS,4100);unit=image.read(UNIT_ADDRESS,64);cases=fixtures(unit)
    blob=b''.join(struct.pack('<106I',*case) for case in cases);path=args.output.with_suffix('.cases.bin')
    path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(blob)
    library=ctypes.CDLL(str(args.native_library.resolve()))
    library.sadx_native_coordinates.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,
        ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t]
    library.sadx_native_coordinates.restype=ctypes.c_int
    table_buffer=(ctypes.c_ubyte*4100).from_buffer_copy(table);unit_buffer=(ctypes.c_ubyte*64).from_buffer_copy(unit)
    report=dict(profile='exact-coordinate-and-Ninja-matrix-i386/native-arm64/source-data-no-rotation-stubs',
        source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),
        table_sha256=sha(table),unit_matrix_sha256=sha(unit),seed=hex(SEED),
        case_blob=dict(path=str(path.resolve()),sha256=sha(blob),size=len(blob),record='106 little-endian uint32 words'),
        source_functions={hex(a):dict(size=b-a,sha256=sha(image.read(a,b-a))) for a,b in
          ((0x43ec00,0x43ec8a),(0x43ec90,0x43ed17),(0x781c80,0x781cbc),(0x7875f0,0x78760d),
           (0x784ee0,0x784fa1),(0x784e20,0x784edf),(0x784fb0,0x78506f),(0x784d60,0x784e1e),(0x785140,0x785162))},
        control_words=[],limits=['Actual original task/world functions and all called Ninja primitives execute without overlays',
          'Matrix storage/depth/capacity/normalization and angles/vectors are explicit test seeds',
          'General floor-normal angle helper is not exercised by this profile',
          'FP flags are reported separately; original hardware flag/delivery equivalence is unverified',
          'Full gameplay/runtime/mods and actual startup control state are not executed'])
    started=time.monotonic()
    for cw in (p|q|0x7f for p in (0,0x200,0x300) for q in (0,0x400,0x800,0xc00)):
        original=CoordinatesOriginal(image,cw,unit);item=dict(control_word=hex(cw),cases=len(cases),
            vector_mismatches=0,depth_mismatches=0,frame_mismatches=0,exception_flag_mismatches=0,examples=[])
        for i,case in enumerate(cases):
            vector,depth,frames,flags=original.run(case,cw);out=(ctypes.c_uint32*102)()
            status=library.sadx_native_coordinates(table_buffer,4100,unit_buffer,64,
                     (ctypes.c_uint32*106)(*case),106,cw,out,102)
            if status or out[5]:raise RuntimeError(f'Native coordinate probe rejected case{i}:status{status},failure{out[5]}')
            vm=tuple(out[:3])!=vector;dm=out[3]!=depth;fm=tuple(out[6:])!=frames;em=out[4]!=flags
            item['vector_mismatches']+=vm;item['depth_mismatches']+=dm;item['frame_mismatches']+=fm;item['exception_flag_mismatches']+=em
            if (vm or dm or fm or em) and len(item['examples'])<10:
                item['examples'].append(dict(case=i,mode=case[0],vector=[list(map(hex,vector)),list(map(hex,out[:3]))],
                    depth=[depth,out[3]],frame_mismatch=fm,exception_flags=[hex(flags),hex(out[4])]))
        report['control_words'].append(item)
        print(f"CW={cw:#x}: cases{len(cases)}, vectors{item['vector_mismatches']}, depths{item['depth_mismatches']}, "
              f"frames{item['frame_mismatches']}, flag differences{item['exception_flag_mismatches']}",flush=True)
    report['stored_values_and_stack_match']=all(not any(item[k] for k in ('vector_mismatches','depth_mismatches','frame_mismatches'))
        for item in report['control_words'])
    report['emulator_exception_flags_match']=all(not item['exception_flag_mismatches'] for item in report['control_words'])
    report['duration_seconds']=time.monotonic()-started;args.output.write_text(json.dumps(report,indent=2)+'\n')
    if not report['stored_values_and_stack_match']:raise SystemExit(1)


if __name__=='__main__':main()
