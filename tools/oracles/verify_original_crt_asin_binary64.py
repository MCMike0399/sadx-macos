"""Compare original 0x645034 binary64 asin entry with the native core.

Runs original classifier/body/return instructions. Policy0 stops at the actual
matherr call, preserving the spill and CW; no handler or FPATAN is replaced.
Unicorn FPATAN/flags remain weaker evidence than Intel hardware observations.
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
from unicorn.x86_const import (UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original, CODE, DATA, STACK, STOP

BOUNDARY=0x648b6b
POLICY=0x3c837a8

def sha(data): return hashlib.sha256(data).hexdigest()
def bits(value): return struct.unpack('<Q',struct.pack('<d',value))[0]

class AsinOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f)
        self.machine.mem_map(0x2bcc000,0x1000)
        self.machine.mem_write(0x2bcc000,image.read(0x2bcc000,0x1000))
        self.machine.mem_map(0x3c83000,0x1000)
        self.machine.mem_write(CODE,b'\x83\xc4\x08\xdb\x3d'+struct.pack('<I',DATA)+b'\xc3')
        self.boundary=0
        def stop(machine,address,size,user):
            self.boundary=address;machine.emu_stop()
        self.machine.hook_add(unicorn.UC_HOOK_CODE,stop,begin=BOUNDARY,end=BOUNDARY)

    def run(self,raw,cw,policy):
        machine=self.machine;self.boundary=0
        machine.mem_write(POLICY,struct.pack('<I',policy))
        machine.mem_write(STACK,struct.pack('<IQI',CODE,raw,STOP))
        machine.reg_write(UC_X86_REG_ESP,STACK)
        machine.reg_write(UC_X86_REG_FPSW,0)
        machine.reg_write(UC_X86_REG_FPTAG,0xffff)
        machine.reg_write(UC_X86_REG_FPCW,cw)
        machine.emu_start(0x645034,STOP,count=2000)
        result=dict(boundary=self.boundary,cw=machine.reg_read(UC_X86_REG_FPCW),
                    flags=machine.reg_read(UC_X86_REG_FPSW)&63,value=None,pending=None)
        if self.boundary:
            result['pending']=hex(struct.unpack('<Q',machine.mem_read(machine.reg_read(UC_X86_REG_EBP)-8,8))[0])
        else:
            if machine.reg_read(UC_X86_REG_ESP)!=STACK+16:
                raise RuntimeError('Original binary64 asin did not return with source ABI')
            sig,se=struct.unpack('<QH',machine.mem_read(DATA,10))
            result['value']=[hex(sig),hex(se)]
        return result

def main():
    parser=argparse.ArgumentParser();parser.add_argument('exe',type=pathlib.Path)
    parser.add_argument('native_library',type=pathlib.Path)
    parser.add_argument('--output',type=pathlib.Path,required=True);args=parser.parse_args()
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':
        raise RuntimeError('Source/oracle identity mismatch')
    cases=list(map(bits,[0.,-0.,0.5,-0.5,1.,-1.,1.5,-1.5,float('inf'),-float('inf')]))
    cases += [1,0x8000000000000001,0xfffffffffffff,0x800fffffffffffff,
              0x7ff8000000000123,0xfff8000000000123,0x7ff0000000000123,0xfff0000000000123]
    for base in (bits(1.),bits(-1.),bits(0.5),bits(-0.5)):
        cases += [(base+delta)&0xffffffffffffffff for delta in range(-3,4)]
    rng=random.Random(0x645034)
    cases += [bits(rng.uniform(-1.25,1.25)) for _ in range(400)]
    cases += [rng.getrandbits(64) for _ in range(200)]
    fixture=[(raw,1) for raw in cases]+[(raw,0) for raw in cases[:46]]
    library=ctypes.CDLL(str(args.native_library.resolve()))
    probe=library.sadx_native_crt_asin_binary64
    probe.argtypes=[ctypes.c_uint64,ctypes.c_uint16,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)]
    probe.restype=ctypes.c_int
    root=pathlib.Path(__file__).resolve().parents[2]
    report=dict(profile='actual645034/classifier/body/return-or-real-matherr-boundary/no-overlays',
        source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),
        source_functions={hex(lo):dict(size=hi-lo,sha256=sha(image.read(lo,hi-lo))) for lo,hi in
            ((0x645034,0x6450eb),(0x648b47,0x648c65))},
        native_sources={name:sha((root/name).read_bytes()) for name in
            ('src/cpu/crt_asin.cpp','src/cpu/crt_asin.hpp','src/cpu/crt_argument.cpp','src/cpu/crt_argument.hpp',
             'src/cpu/crt_inverse_internal.hpp','tests/probes/native_crt_asin_binary64_probe.cpp',
             'tools/oracles/verify_original_crt_asin_binary64.py')},
        cases=len(fixture),case_sha256=sha(b''.join(struct.pack('<QI',*case) for case in fixture)),
        profiles=[],mismatches=[],limits=[
            'Explicit policy1 bypass and policy0 boundary profiles do not establish initialized CRT state',
            'No original handler is executed or replaced; original instructions execute until return or dispatcher',
            'Native FPATAN uses Bochs P5/P6; Unicorn uses host-double math with incomplete flags',
            'Intel Ultra results and full-game geometry/mod/controller behavior remain unverified'])
    start=time.monotonic();original=AsinOriginal(image)
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        item=dict(cw=hex(cw),cases=len(fixture),value_mismatches=0,boundary_mismatches=0,
                  control_mismatches=0,pending_mismatches=0,flag_mismatches=0)
        for index,(raw,policy) in enumerate(fixture):
            expected=original.run(raw,cw,policy);out=(ctypes.c_uint32*12)()
            status=probe(raw,cw,policy,out)
            if status or out[0] not in (0,2):
                raise RuntimeError(f'Native asin failed case{index}: status{status}, failure{out[0]}')
            actual=dict(boundary=out[1],cw=out[2],flags=out[3],
                value=[hex(out[4]|out[5]<<32),hex(out[6])] if out[7] else None,
                pending=hex(out[8]|out[9]<<32) if out[10] else None)
            differences=[]
            for field,counter in (('value','value_mismatches'),('boundary','boundary_mismatches'),
                ('cw','control_mismatches'),('pending','pending_mismatches'),('flags','flag_mismatches')):
                if actual[field]!=expected[field]:
                    item[counter]+=1;differences.append(field)
            if differences:
                report['mismatches'].append(dict(case=index,input=hex(raw),policy=policy,cw=hex(cw),
                    fields=differences,source=expected,native=actual))
        report['profiles'].append(item);print(json.dumps(item),flush=True)
    report['returned_values_and_boundaries_match']=all(not any(profile[key] for key in
        ('value_mismatches','boundary_mismatches','control_mismatches','pending_mismatches'))
        for profile in report['profiles'])
    report['full_emulator_match']=report['returned_values_and_boundaries_match'] and all(
        not profile['flag_mismatches'] for profile in report['profiles'])
    report['seconds']=time.monotonic()-start
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    return int(not report['returned_values_and_boundaries_match'])

if __name__=='__main__': raise SystemExit(main())
