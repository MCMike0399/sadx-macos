"""Compare the actual 0x644340 routine to its native conversion reconstruction.

All source instructions execute. The harness supplies a binary64 load into
ST0; it does not establish arbitrary extended-register input or hardware flags.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, CODE, DATA, FIRST, STACK, STOP


def double_bits(value):
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('exe',type=pathlib.Path)
    parser.add_argument('native_library',type=pathlib.Path)
    parser.add_argument('--output',required=True,type=pathlib.Path)
    args=parser.parse_args()
    if unicorn.__version__ != '2.1.4':
        raise RuntimeError('This oracle profile requires Unicorn 2.1.4')
    image=PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise RuntimeError('Executable address map identity differs')
    library=ctypes.CDLL(str(args.native_library.resolve()))
    probe=library.sadx_native_crt_integer
    probe.argtypes=[ctypes.c_uint64,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint64),ctypes.POINTER(ctypes.c_uint16)]
    probe.restype=ctypes.c_int
    values=[0.,-0.,0.25,-0.25,0.5,-0.5,1.25,-1.25,1.75,-1.75,
            16383.999,-16383.999,65536.,-65536.,4294967296.,-4294967296.,
            float(2**63),-float(2**63),float('inf'),-float('inf')]
    # Store cases as bits so NaN payloads/signs and JSON remain unambiguous.
    cases=[double_bits(value) for value in values]
    cases.extend([1,0x8000000000000001,0x7ff8000000000123,0xfff8000000000123,
                  0x7ff0000000000001,0xfff0000000000001])
    generator=random.Random(0x644340)
    cases.extend(double_bits(generator.uniform(-1e8,1e8)) for _ in range(2000))
    cases.extend(generator.getrandbits(64) for _ in range(1000))
    case_bytes=b''.join(struct.pack('<Q',value) for value in cases)
    report=dict(source_sha256=image.sha256,native_library_sha256=hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
                source_function_address='0x644340',source_function_sha256=hashlib.sha256(image.read(0x644340,0x75)).hexdigest(),
                input_case_sha256=hashlib.sha256(case_bytes).hexdigest(),profiles=[],
                limits=['Binary64 initial load, not every possible extended register input',
                        'Original instructions execute in Unicorn, not native x86 hardware',
                        'Exception flags are reported independently; emulator flag behavior is not a hardware oracle'])
    for precision in (0,0x200,0x300):
        for rounding in (0,0x400,0x800,0xc00):
            control_word=precision|rounding|0x7f
            original=Original(image,control_word)
            # FNCLEX; FLD qword input; call original; FNSTSW to memory; RET.
            prefix=b'\xdb\xe2\xdd\x05'+struct.pack('<I',FIRST)
            call=b'\xe8'+struct.pack('<i',0x644340-(CODE+len(prefix)+5))
            suffix=b'\xdd\x3d'+struct.pack('<I',DATA)+b'\xc3'
            original.machine.mem_write(CODE,prefix+call+suffix)
            item=dict(control_word=hex(control_word),cases=len(cases),value_mismatches=0,flag_mismatches=0,examples=[])
            for index,input_bits in enumerate(cases):
                original.machine.mem_write(FIRST,struct.pack('<Q',input_bits))
                original.machine.mem_write(STACK,struct.pack('<I',STOP))
                original.machine.reg_write(UC_X86_REG_ESP,STACK)
                original.machine.emu_start(CODE,STOP,count=1000)
                actual=original.machine.reg_read(UC_X86_REG_EAX) | (original.machine.reg_read(UC_X86_REG_EDX)<<32)
                actual_flags=struct.unpack('<H',original.machine.mem_read(DATA,2))[0]&0x3f
                native_result=ctypes.c_uint64(); native_flags=ctypes.c_uint16()
                status=probe(input_bits,control_word,ctypes.byref(native_result),ctypes.byref(native_flags))
                if status:
                    raise RuntimeError('Native probe rejected valid masked control profile')
                mismatch=actual!=native_result.value
                item['value_mismatches']+=mismatch
                item['flag_mismatches']+=actual_flags!=native_flags.value
                if mismatch and len(item['examples'])<10:
                    item['examples'].append(dict(case=index,input_bits=hex(input_bits),original=hex(actual),native=hex(native_result.value)))
            report['profiles'].append(item)
            print(json.dumps(item),flush=True)
    report['tested_values_match']=all(not item['value_mismatches'] for item in report['profiles'])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    if not report['tested_values_match']:
        raise SystemExit(1)


if __name__=='__main__':
    main()
