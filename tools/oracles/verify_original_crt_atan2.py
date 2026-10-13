"""Execute actual645270 classifier/math/return; stop at real64b518 dispatch.

No CRT body, FPATAN, classifier or handler is replaced. Unicorn2.1.4 uses
weaker host-double FPATAN/flag behavior; retain differences rather than adapt
the native integer Bochs model to those defects. This is not a Windows run.
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
from unicorn.x86_const import (UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_FP0,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG)
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,CODE,DATA,STACK,STOP

BOUNDARY=0x64b518
POLICY=0x3c837a8
SCRATCH=0x3c83960
COPY_FLAG=0x3c83968

def sha(data):return hashlib.sha256(data).hexdigest()
def bits(value):return struct.unpack('<Q',struct.pack('<d',value))[0]

class Atan2Original(Original):
    def __init__(self,image):
        super().__init__(image,0x37f)
        self.machine.mem_map(0x2bcc000,0x1000)
        self.machine.mem_write(0x2bcc000,image.read(0x2bcc000,0x1000))
        self.machine.mem_map(0x3c83000,0x1000)
        # Real cdecl return, explicit caller cleanup, then unrounded ext80 store.
        self.machine.mem_write(CODE,b'\x83\xc4\x10\xdb\x3d'+struct.pack('<I',DATA)+b'\xc3')
        self.boundary=0
        self.fpatan=None;self.seed=None
        def stop(machine,address,size,user):
            self.boundary=address;machine.emu_stop()
        self.machine.hook_add(unicorn.UC_HOOK_CODE,stop,begin=BOUNDARY,end=BOUNDARY)
        def handoff(machine,address,size,user):
            # UC_X86_REG_FP0..7 are physical regs; FPSW.TOP selects ST0.
            top=(machine.reg_read(UC_X86_REG_FPSW)>>11)&7
            sig,se=machine.reg_read(UC_X86_REG_FP0+top)
            self.fpatan=[hex(sig),hex(se)]
            if self.seed is not None:
                machine.reg_write(UC_X86_REG_FP0+top,self.seed)
                tag=machine.reg_read(UC_X86_REG_FPTAG)
                machine.reg_write(UC_X86_REG_FPTAG,tag&~(3<<(top*2)))
        self.machine.hook_add(unicorn.UC_HOOK_CODE,handoff,begin=0x64b2ad,end=0x64b2ad)

    def run(self,y,x,cw,policy,seed=None):
        m=self.machine;self.boundary=0;self.fpatan=None;self.seed=seed
        m.mem_write(POLICY,struct.pack('<I',policy));m.mem_write(SCRATCH,b'\x00'*9)
        m.mem_write(STACK,struct.pack('<IQQI',CODE,y,x,STOP))
        m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPSW,0)
        m.reg_write(UC_X86_REG_FPTAG,0xffff);m.reg_write(UC_X86_REG_FPCW,cw)
        m.emu_start(0x645270,STOP,count=4000)
        result=dict(boundary=self.boundary,cw=m.reg_read(UC_X86_REG_FPCW),
            flags=m.reg_read(UC_X86_REG_FPSW)&63,value=None,pending=None,error=None,
            scratch=hex(struct.unpack('<Q',m.mem_read(SCRATCH,8))[0]) if policy==0 else None,
            copy_flag=m.mem_read(COPY_FLAG,1)[0],fpatan=self.fpatan)
        if self.boundary:
            frame=m.reg_read(UC_X86_REG_EBP)
            result['pending']=hex(struct.unpack('<Q',m.mem_read(frame-0x76,8))[0])
            result['error']=struct.unpack('<I',m.mem_read(frame-0x8e,4))[0]
            # Arguments remain their exact original bits in the source record.
            if struct.unpack('<QQ',m.mem_read(frame-0x86,16))!=(y,x):
                raise RuntimeError('Source did not preserve both original matherr arguments')
        else:
            if m.reg_read(UC_X86_REG_ESP)!=STACK+24:
                raise RuntimeError('Source atan2 did not return through cdecl ABI')
            sig,se=struct.unpack('<QH',m.mem_read(DATA,10));result['value']=[hex(sig),hex(se)]
        return result

def fixtures():
    raw=list(map(bits,[0.,-0.,0.125,-0.125,1.,-1.,2.,-2.,float('inf'),-float('inf')]))
    raw += [1,0x8000000000000001,0xfffffffffffff,0x800fffffffffffff,
        0x7fefffffffffffff,0xffefffffffffffff,0x7ff8000000000123,
        0xfff8000000000123,0x7ff0000000000123,0xfff0000000000123,
        0x7ff8000000000456,0x7ff0000000000456]
    cases=[(y,x) for y in raw for x in raw]
    rng=random.Random(0x645270)
    cases += [(bits(rng.uniform(-100,100)),bits(rng.uniform(-100,100))) for _ in range(400)]
    cases += [(rng.getrandbits(64),rng.getrandbits(64)) for _ in range(300)]
    return [(y,x,p) for y,x in cases for p in (0,1)]

def main():
    parser=argparse.ArgumentParser();parser.add_argument('exe',type=pathlib.Path)
    parser.add_argument('native_library',type=pathlib.Path)
    parser.add_argument('--output',type=pathlib.Path,required=True);args=parser.parse_args()
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':
        raise RuntimeError('Source/oracle identity mismatch')
    if image.read(0x2bcc170,16)!=bytes.fromhex('056174616e3200000000000000021000'):
        raise RuntimeError('Source atan2 descriptor changed')
    fixture=fixtures();lib=ctypes.CDLL(str(args.native_library.resolve()));probe=lib.sadx_native_crt_atan2
    probe.argtypes=[ctypes.c_uint64,ctypes.c_uint64,ctypes.c_uint16,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)]
    probe.restype=ctypes.c_int
    root=pathlib.Path(__file__).resolve().parent.parent
    report=dict(profile='actual645270/two-argument-classifier/body/return-or-real-matherr-call/no-overlays',
        source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),
        descriptor=dict(address=hex(0x2bcc170),bytes=image.read(0x2bcc170,80).hex(),name='atan2',argument_count=2,operation=16),
        source_functions={hex(lo):dict(size=hi-lo,sha256=sha(image.read(lo,hi-lo))) for lo,hi in
            ((0x645270,0x64527a),(0x64b29c,0x64b328),(0x64b3a4,0x64b592),(0x6489c7,0x648b2a))},
        native_sources={name:sha((root/name).read_bytes()) for name in
            ('src/cpu/crt_atan2.hpp','src/cpu/crt_atan2.cpp','tests/probes/native_crt_atan2_probe.cpp','tools/oracles/verify_original_crt_atan2.py')},
        cases=len(fixture),case_sha256=sha(b''.join(struct.pack('<QQI',*c) for c in fixture)),
        profiles=[],mismatches=[],suffix_profiles=[],suffix_mismatches=[],limits=[
            'Caller-selected policy0/1 and12 masked CW profiles are experimental inputs, not actual initialized Windows state',
            'Source and native stop before real64b518 handler; no fake callback is installed',
            'Unicorn FPATAN uses host-double results and incomplete flags; native Bochs P5/P6 remains explicit',
            'Primary comparisons have no replacement/overlay; supplementary suffix profiles explicitly seed ST0 at64b2ad with the native extended intermediate after actual source prefix/FPATAN',
            'Suffix seeding validates only quadrant/return/dispatch scalar instructions; it does not prove Intel FPATAN agreement',
            'The two-argument64dacb dispatcher, errno/matherr callbacks and physical Intel Ultra behavior remain unverified',
            'Source stack/TOP/C1/deferred trap timing and native full-game collision behavior remain outside this comparison'])
    start=time.monotonic();original=Atan2Original(image)
    fields=('value','boundary','cw','pending','error','scratch','copy_flag','fpatan','flags')
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        item=dict(cw=hex(cw),cases=len(fixture),**{field+'_mismatches':0 for field in fields})
        suffix=dict(cw=hex(cw),cases=0,**{field+'_mismatches':0 for field in fields if field!='fpatan'})
        item['unclassified_nonflag_mismatches']=0
        for index,(y,x,policy) in enumerate(fixture):
            expected=original.run(y,x,cw,policy);out=(ctypes.c_uint32*24)()
            status=probe(y,x,cw,policy,out)
            if status or out[0] not in (0,2):
                raise RuntimeError(f'Native atan2 failed case{index}: status{status},failure{out[0]}')
            actual=dict(boundary=out[1],cw=out[2],flags=out[3],
                value=[hex(out[4]|out[5]<<32),hex(out[6])] if out[7] else None,
                pending=hex(out[8]|out[9]<<32) if out[10] else None,
                error=out[11] if out[15] else None,
                scratch=hex(out[12]|out[13]<<32) if out[14] else None,copy_flag=out[16],
                fpatan=[hex(out[20]|out[21]<<32),hex(out[22])] if out[23] else None)
            differences=[field for field in fields if actual[field]!=expected[field]]
            for field in differences:item[field+'_mismatches']+=1
            suffix_differences=[]
            if actual['fpatan'] is not None:
                seeded=original.run(y,x,cw,policy,tuple(int(word,16) for word in actual['fpatan']))
                suffix['cases']+=1
                suffix_differences=[field for field in fields if field!='fpatan' and actual[field]!=seeded[field]]
                for field in suffix_differences:suffix[field+'_mismatches']+=1
                if suffix_differences:report['suffix_mismatches'].append(dict(case=index,y=hex(y),x=hex(x),
                    policy=policy,cw=hex(cw),fields=suffix_differences,source_seeded=seeded,native=actual))
            nonflags=[field for field in differences if field!='flags']
            attributed=bool(nonflags) and actual['fpatan']!=expected['fpatan'] and not any(
                field!='flags' for field in suffix_differences) and all(
                field in ('value','pending','scratch','fpatan') for field in nonflags)
            if nonflags and not attributed:item['unclassified_nonflag_mismatches']+=1
            if differences:report['mismatches'].append(dict(case=index,y=hex(y),x=hex(x),
                policy=policy,cw=hex(cw),fields=differences,source=expected,native=actual,
                classification='different_fpatan_models/source_scalar_suffix_agrees' if attributed else None))
        report['profiles'].append(item);print(json.dumps(item),flush=True)
        report['suffix_profiles'].append(suffix)
    report['returned_values_and_boundaries_match']=all(not any(p[field+'_mismatches']
        for field in fields if field!='flags') for p in report['profiles'])
    report['full_emulator_match']=report['returned_values_and_boundaries_match'] and all(
        not p['flags_mismatches'] for p in report['profiles'])
    report['local_control_and_suffix_matches']=all(not p['unclassified_nonflag_mismatches'] for p in report['profiles']) and all(
        not any(p[field+'_mismatches'] for field in fields if field not in ('flags','fpatan')) for p in report['suffix_profiles'])
    report['seconds']=time.monotonic()-start
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    return int(not report['local_control_and_suffix_matches'])

if __name__=='__main__':raise SystemExit(main())
