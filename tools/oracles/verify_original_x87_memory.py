"""Focused FLD32/64 + arithmetic-memory instruction fixtures, not game startup.

Inputs remain raw bits, including SNaNs. Immutable x86 instruction wrappers
are test fixtures. Their forms match source x87 memory arithmetic; source
njAddVector's first FLD/FADD/FSTP sequence is independently pinned below.
Extended results and FCOM condition codes are strict; exception differences
are reported separately because Unicorn is not original x86 hardware.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import struct
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_FPCW
from pe_image import PEImage
from extract import SOURCE_SHA256

CODE=0x10000000
DATA=0x11000000
FIRST=DATA+0x100
SECOND=DATA+0x200
RESULT=DATA+0x300
STATUS=DATA+0x400
STOP=CODE+0xff0
STACK=DATA+0xf000
OPS=('add','multiply','subtract','reverse_subtract','divide','reverse_divide','compare')
OP_FIELDS=(0,1,4,5,6,7,2)


class Instructions:
    def __init__(self):
        self.uc=Uc(UC_ARCH_X86,UC_MODE_32)
        self.uc.mem_map(CODE,0x1000)
        self.uc.mem_map(DATA,0x10000)
        self.addresses={}
        self.instructions={}
        for width in (32,64):
            for operation,field in enumerate(OP_FIELDS):
                address=CODE+len(self.addresses)*64
                # FNINIT; FLD m32/64; arithmetic m32/64; FNSTSW AX;
                # MOV status,EAX; FSTP m80; RET. FNINIT resets sticky flags.
                # FLDCW follows FNINIT and reads the explicit fixture CW.
                wrapper=(b'\xdb\xe3\xd9\x2d'+struct.pack('<I',DATA)+
                         bytes((0xd9 if width==32 else 0xdd,0x05))+struct.pack('<I',FIRST)+
                         bytes((0xd8 if width==32 else 0xdc,0x05|(field<<3)))+
                         struct.pack('<I',SECOND)+b'\xdf\xe0\xa3'+struct.pack('<I',STATUS)+
                         b'\xdb\x3d'+struct.pack('<I',RESULT)+b'\xc3')
                self.uc.mem_write(address,wrapper)
                self.addresses[width,operation]=address
                self.instructions[f'{width}/{OPS[operation]}']=wrapper.hex()

    def call(self,first,second,width,operation,cw):
        self.uc.mem_write(DATA,struct.pack('<H',cw))
        self.uc.mem_write(FIRST,struct.pack('<Q',first))
        self.uc.mem_write(SECOND,struct.pack('<Q',second))
        self.uc.mem_write(STACK,struct.pack('<I',STOP))
        self.uc.reg_write(UC_X86_REG_ESP,STACK)
        self.uc.emu_start(self.addresses[width,operation],STOP,count=32)
        significand,exponent=struct.unpack('<QH',self.uc.mem_read(RESULT,10))
        status,=struct.unpack('<H',self.uc.mem_read(STATUS,2))
        return (significand&0xffffffff,significand>>32,exponent,
                status&0x4500 if operation==6 else 0,status&0x3f)


def fixtures(width):
    # Both signs, signaling/quiet NaNs with differing payload magnitudes,
    # subnormals, finite arithmetic/reversal, signed zero and infinity.
    if width==32:
        values=(0,0x80000000,1,0x80000001,0x007fffff,0x00800000,
                0x3f800000,0xc0000000,0x40a00000,0x3eaaaaab,
                0x7f800000,0xff800000,0x7f812345,0xff812345,
                0x7fc12345,0xffc12345,0x7fffffff,0xffc00000)
    else:
        values=(0,0x8000000000000000,1,0x8000000000000001,
                0x000fffffffffffff,0x0010000000000000,0x3ff0000000000000,
                0xc000000000000000,0x4014000000000000,0x3fd5555555555555,
                0x7ff0000000000000,0xfff0000000000000,0x7ff0123456789abc,
                0xfff0123456789abc,0x7ff8123456789abc,0xfff8123456789abc,
                0x7fffffffffffffff,0xfff8000000000000)
    return [(first,second) for first in values for second in values]


def signaling_nan(bits,width):
    exponent,fraction,quiet=((0x7f800000,0x007fffff,0x00400000) if width==32 else
        (0x7ff0000000000000,0x000fffffffffffff,0x0008000000000000))
    return bits&exponent==exponent and bits&fraction!=0 and bits&quiet==0


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('exe',type=pathlib.Path)
    parser.add_argument('native_library',type=pathlib.Path)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    if unicorn.__version__!='2.1.4': raise RuntimeError('Requires Unicorn2.1.4')
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256: raise RuntimeError('Source executable changed')
    source=image.read(0x785170,25)
    if source.hex()!='d901d802d919d94204d84104d95904d94208d84108d95908c3':
        raise RuntimeError('Source njAddVector instruction sequence changed')
    library=ctypes.CDLL(str(args.native_library.resolve()))
    native=library.sadx_native_x87_memory
    native.argtypes=[ctypes.c_uint64,ctypes.c_uint64,ctypes.c_uint,ctypes.c_uint,
                     ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)]
    native.restype=ctypes.c_int
    original=Instructions()
    report=dict(profile='immutable-x87-memory-instruction-fixtures/unicorn-2.1.4/native-arm64-ext80',
                source_sha256=image.sha256,njAddVector_address='0x785170',
                njAddVector_bytes=source.hex(),njAddVector_sha256=hashlib.sha256(source).hexdigest(),
                native_library_sha256=hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
                instruction_wrappers=original.instructions,control_words=[],
                limits=['Focused instruction fixtures, not complete original game execution',
                        'Explicit experimental control words, not captured running-game state',
                        'Unicorn exception flags are recorded separately; hardware is unverified',
                        'All exception masks enabled; native typed unmasked failures tested separately'])
    case_bytes=b''.join(struct.pack('<IQQ',width,a,b)
                        for width in (32,64) for a,b in fixtures(width))
    cases_path=args.output.with_suffix('.cases.bin')
    cases_path.parent.mkdir(parents=True,exist_ok=True)
    cases_path.write_bytes(case_bytes)
    report['cases_path']=str(cases_path)
    report['cases_sha256']=hashlib.sha256(case_bytes).hexdigest()
    failed=False
    mismatches=[]
    output=(ctypes.c_uint32*5)()
    for precision in (0,0x200,0x300):
        for rounding in (0,0x400,0x800,0xc00):
            cw=precision|rounding|0x7f
            item=dict(control_word=hex(cw),cases=0,value_condition_mismatches=0,
                      register_load_snan_cases=0,register_load_snan_mismatches=0,
                      other_cases=0,other_mismatches=0,
                      exception_flag_mismatches=0,examples=[],flag_examples=[])
            for width in (32,64):
                for a,b in fixtures(width):
                    for operation in range(7):
                        actual=original.call(a,b,width,operation,cw)
                        if native(a,b,width,operation,cw,output): raise RuntimeError('Native rejected fixture')
                        expected=tuple(output)
                        item['cases']+=1
                        category='register_load_snan' if signaling_nan(a,width) else 'other'
                        item[category+'_cases']+=1
                        example=dict(width=width,operation=OPS[operation],first=hex(a),memory=hex(b),
                                     original=[hex(x) for x in actual],native=[hex(x) for x in expected])
                        if actual[:4]!=expected[:4]:
                            item['value_condition_mismatches']+=1
                            item[category+'_mismatches']+=1
                            mismatches.append(dict(control_word=hex(cw),**example))
                            if len(item['examples'])<20: item['examples'].append(example)
                        if actual[4]!=expected[4]:
                            item['exception_flag_mismatches']+=1
                            if len(item['flag_examples'])<10: item['flag_examples'].append(example)
            failed|=item['value_condition_mismatches']!=0
            report['control_words'].append(item)
            print(json.dumps({k:v for k,v in item.items() if k not in ('examples','flag_examples')}),flush=True)
    report['stored_ext80_and_comparison_conditions_match']=not failed
    report['non_signaling_register_load_profile_matches']=all(
        item['other_mismatches']==0 for item in report['control_words'])
    mismatch_path=args.output.with_suffix('.mismatches.json')
    mismatch_path.write_text(json.dumps(mismatches,indent=2)+'\n')
    report['mismatch_path']=str(mismatch_path)
    report['mismatch_sha256']=hashlib.sha256(mismatch_path.read_bytes()).hexdigest()
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    return int(failed)


if __name__=='__main__': raise SystemExit(main())
