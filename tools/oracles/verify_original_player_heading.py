"""Actual AdjustAngle/heading helper plus real coordinate/matrix instructions.

No overlays. Angles, matrix state and FPU modes are explicit experiments;
this is not a full game frame or a native Windows hardware measurement.
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
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_ESP,
    UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)
from extract import SOURCE_SHA256, TABLE_ADDRESS
from pe_image import PEImage
from verify_original_math import Original, DATA, STACK, STOP
from verify_original_player_coordinates import CoordinatesOriginal, UNIT_ADDRESS, MATRIX, TASK, fword

PLAYER = DATA + 0x1000
SEED = 0x443df0


def sha(data): return hashlib.sha256(data).hexdigest()


def angle_fixtures():
    angles = [0, 1, 8191, 8192, 16383, 16384, 32767, 32768, 49152, 65535, 65536, 0xffffffff, 0x80000000, 0x7fffffff]
    steps = [0, 1, 7, 8192, 32767, 32768, 65535, 0xffffffff, 0x80000000, 0x7fffffff]
    cases = [(a,b,s) for a in angles for b in angles for s in steps]
    rng = random.Random(SEED)
    cases.extend(tuple(rng.getrandbits(32) for _ in range(3)) for _ in range(3000))
    return cases


def heading_fixtures(unit):
    cases = []; words = list(struct.unpack('<16I',unit)); rng=random.Random(SEED)
    for yaw in (0, 1, 8192, 16384, 32768, 49152, 65535, 65536, 0xffffffff):
        for target in (0, 16384, 32768, 0xffffffff):
            for depth in (1, 2, 5, 6):
                for normalize in (0, 1):
                    cases.append([0,yaw,0,target,fword(1),fword(0),fword(0),depth,normalize,0,*words*6])
    for _ in range(1000):
        frame_words = list(map(fword, (rng.uniform(-5,5) for _ in range(96))))
        cases.append([*[rng.getrandbits(32) for _ in range(4)],
            *map(fword,(rng.uniform(-100,100) for _ in range(3))),rng.choice((1,2,5,6)),rng.randrange(2),0,*frame_words])
    for raw in (0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345):
        for component in range(3):
            case=[123,456,789,23456,fword(1),fword(2),fword(3),1,1,0,*words*6]
            case[4+component]=raw;cases.append(case)
    return cases


class OriginalHeading(CoordinatesOriginal):
    def run_heading(self, words, cw):
        machine=self.machine
        task=bytearray(64);struct.pack_into('<3I',task,0x14,*words[:3])
        machine.mem_write(TASK,bytes(task));machine.mem_write(PLAYER+0x38,struct.pack('<3I',*words[4:7]))
        machine.mem_write(MATRIX,struct.pack('<96I',*words[10:]))
        for address,value in ((0x3d0f818,MATRIX+(words[7]-1)*64),(0x3d0fa28,6),
                              (0x3d0fa2c,words[7]),(0x3d0f7ac,words[8])):
            machine.mem_write(address,struct.pack('<I',value))
        # The source reads target at entry ESP+8. Entry ESP+4 is an unused
        # stack argument alongside the EAX/EDI register owners.
        machine.mem_write(STACK,struct.pack('<3I',STOP,0,words[3]))
        machine.reg_write(UC_X86_REG_EAX,PLAYER);machine.reg_write(UC_X86_REG_EDI,TASK)
        machine.reg_write(UC_X86_REG_ESP,STACK);machine.reg_write(UC_X86_REG_FPCW,cw)
        machine.reg_write(UC_X86_REG_FPSW,0);machine.reg_write(UC_X86_REG_FPTAG,0xffff)
        machine.emu_start(0x443df0,STOP,count=10000)
        if machine.reg_read(UC_X86_REG_ESP)!=STACK+4: raise RuntimeError('Source heading ABI return failed')
        yaw=struct.unpack('<I',machine.mem_read(TASK+0x18,4))[0]
        velocity=struct.unpack('<3I',machine.mem_read(PLAYER+0x38,12))
        depth=struct.unpack('<I',machine.mem_read(0x3d0fa2c,4))[0]
        frames=struct.unpack('<96I',machine.mem_read(MATRIX,384))
        return [yaw,*velocity,depth,machine.reg_read(UC_X86_REG_FPSW)&0x3f,0,*frames]


def main():
    parser=argparse.ArgumentParser();parser.add_argument('exe',type=pathlib.Path)
    parser.add_argument('native_library',type=pathlib.Path);parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires pinned Unicorn2.1.4')
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source executable address map mismatch')
    table,unit=image.read(TABLE_ADDRESS,4100),image.read(UNIT_ADDRESS,64)
    library=ctypes.CDLL(str(args.native_library.resolve()))
    library.sadx_native_adjust_angle.argtypes=[ctypes.c_uint32]*3;library.sadx_native_adjust_angle.restype=ctypes.c_uint32
    library.sadx_native_player_heading.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t]*2+[
        ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t]
    library.sadx_native_player_heading.restype=ctypes.c_int
    angles=angle_fixtures();headings=heading_fixtures(unit)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    blob=b''.join(struct.pack('<106I',*case) for case in headings)
    fixture_path=args.output.with_suffix('.cases.bin');fixture_path.write_bytes(blob)
    angle_blob=b''.join(struct.pack('<3I',*case) for case in angles)
    angle_path=args.output.with_suffix('.angle-cases.bin');angle_path.write_bytes(angle_blob)
    report=dict(profile='actual438350/443df0 plus real coordinate/matrix callees; no overlays',
        source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),
        angle_case_blob=dict(path=str(angle_path.resolve()),sha256=sha(angle_blob),size=len(angle_blob)),
        heading_case_blob=dict(path=str(fixture_path.resolve()),sha256=sha(blob),size=len(blob)),
        source_functions={hex(a):dict(size=b-a,sha256=sha(image.read(a,b-a))) for a,b in
            ((0x438350,0x438392),(0x443df0,0x443e57))},
        angle_cases=len(angles),angle_mismatches=[],control_words=[],limits=[
            'Explicit seeded source matrix/depth/normalization and experimental FPU modes',
            'No hardware flags/deferred delivery, startup, updated mods or complete game frames'])
    original=Original(image,0x027f)
    for index,case in enumerate(angles):
        original.machine.mem_write(STACK,struct.pack('<4I',STOP,*case))
        original.machine.reg_write(UC_X86_REG_ESP,STACK);original.machine.emu_start(0x438350,STOP,count=100)
        if original.machine.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Source AdjustAngle ABI return failed')
        expected=original.machine.reg_read(UC_X86_REG_EAX);actual=library.sadx_native_adjust_angle(*case)
        if actual!=expected:report['angle_mismatches'].append(dict(case=index,words=list(map(hex,case)),expected=hex(expected),actual=hex(actual)))
    buffers=[(ctypes.c_ubyte*len(data)).from_buffer_copy(data) for data in (table,unit)]
    started=time.monotonic()
    for cw in (p|r|0x7f for p in (0,0x200,0x300) for r in (0,0x400,0x800,0xc00)):
        original=OriginalHeading(image,cw,unit);item=dict(control_word=hex(cw),cases=len(headings),
            output_mismatches=0,flag_mismatches=0,examples=[])
        for index,words in enumerate(headings):
            expected=original.run_heading(words,cw);out=(ctypes.c_uint32*103)()
            status=library.sadx_native_player_heading(buffers[0],4100,buffers[1],64,(ctypes.c_uint32*106)(*words),106,cw,out,103)
            if status or out[6]:raise RuntimeError(f'Native heading rejected case{index}:status{status},failure{out[6]}')
            actual=list(out);different=actual[:5]+actual[6:]!=expected[:5]+expected[6:];flags=actual[5]!=expected[5]
            item['output_mismatches']+=different;item['flag_mismatches']+=flags
            if different or (flags and len(item['examples'])<5):
                item['examples'].append(dict(case=index,original=list(map(hex,expected)),native=list(map(hex,actual))))
        report['control_words'].append(item);print(json.dumps({k:v for k,v in item.items() if k!='examples'}),flush=True)
    report['angle_and_stored_heading_matrix_match']=not report['angle_mismatches'] and all(i['output_mismatches']==0 for i in report['control_words'])
    report['emulator_exception_flags_match']=all(i['flag_mismatches']==0 for i in report['control_words'])
    report['duration_seconds']=time.monotonic()-started;args.output.write_text(json.dumps(report,indent=2)+'\n')
    if not report['angle_and_stored_heading_matrix_match']:raise SystemExit(1)


if __name__=='__main__':main()
