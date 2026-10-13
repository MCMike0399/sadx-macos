"""Explicit scalar opcode probe: emulator comparison is not hardware proof."""
import argparse,ctypes,hashlib,json,pathlib,random,time
import unicorn
from unicorn.x86_const import UC_X86_REG_XMM0,UC_X86_REG_XMM1,UC_X86_REG_MXCSR,UC_X86_REG_EAX
OPS=[('ADDSS','f30f58c1'),('SUBSS','f30f5cc1'),('MULSS','f30f59c1'),('DIVSS','f30f5ec1'),('MINSS','f30f5dc1'),('CVTSS2SD','f30f5ac0'),('CVTSD2SS','f20f5ac0'),('SQRTSD','f20f51c0'),('CVTTSS2SI','f30f2cc0')]
def main():
    p=argparse.ArgumentParser();p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires pinned Unicorn 2.1.4')
    lib=ctypes.CDLL(str(a.native_library.resolve()));native=lib.sadx_native_sse_math
    native.argtypes=[ctypes.c_uint,ctypes.c_uint64,ctypes.c_uint64,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint64),ctypes.POINTER(ctypes.c_uint32)];native.restype=ctypes.c_int
    m=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32);m.mem_map(0x10000,4096)
    edges32=[0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x80800000,0x3f000000,0x3f800000,0xbf800000,0x7f7fffff,0x7f800000,0xff800000,0x7f800001,0x7fc00001,0x7fc12345,0xff800001]
    edges64=[0,0x8000000000000000,1,0x8000000000000001,0xfffffffffffff,0x10000000000000,0x36a0000000000000,0x3800000000000000,0x3ff0000000000000,0xbff0000000000000,0x7fefffffffffffff,0x7ff0000000000000,0x7ff0000000000001,0x7ff8000000000001]
    rng=random.Random(0x535345);reports=[];start=time.monotonic()
    for operation,(name,code) in enumerate(OPS):
        size=64 if operation in (6,7) else 32;edges=edges64 if size==64 else edges32
        pairs=[(x,y) for x in edges for y in edges] if operation<5 else [(x,0) for x in edges]
        pairs += [(rng.getrandbits(size),rng.getrandbits(size)) for _ in range(2000)]
        m.mem_write(0x10000,bytes.fromhex(code));report={'operation':name,'opcode':code,'evaluations':0,'value_mismatches':0,'mxcsr_mismatches':0,'first_value_mismatches':[],'difference_classes':{},'unclassified_mismatches':[]} 
        for rounding in range(4):
            for daz in range(2):
                for ftz in range(2):
                    mxcsr=0x1f80|(rounding<<13)|(daz<<6)|(ftz<<15)
                    for x,y in pairs:
                        m.reg_write(UC_X86_REG_XMM0,x);m.reg_write(UC_X86_REG_XMM1,y);m.reg_write(UC_X86_REG_MXCSR,mxcsr)
                        m.emu_start(0x10000,0x10000+len(bytes.fromhex(code)),count=1)
                        expected=m.reg_read(UC_X86_REG_EAX) if operation==8 else m.reg_read(UC_X86_REG_XMM0)&((1<<(64 if operation in (5,7) else 32))-1)
                        status=m.reg_read(UC_X86_REG_MXCSR);out=ctypes.c_uint64();flags=ctypes.c_uint32()
                        if native(operation,x,y,mxcsr,ctypes.byref(out),ctypes.byref(flags)):raise RuntimeError('masked operation failed')
                        report['evaluations']+=1
                        if expected!=out.value:
                            report['value_mismatches']+=1
                            nan=lambda v:(v&0x7fffffff)>0x7f800000
                            den=lambda v:0<(v&0x7fffffff)<0x800000
                            if operation<4 and nan(x) and nan(y):reason='emulator_x87_NaN_priority'
                            elif operation in (0,1,2,3,6) and ftz and den(expected) and out.value==(expected&0x80000000):reason='emulator_FTZ_wrong_status_owner'
                            elif operation==4 and daz and (den(x) or den(y)):reason='emulator_MINSS_returns_unconverted_operand'
                            else:reason='unclassified'
                            report['difference_classes'][reason]=report['difference_classes'].get(reason,0)+1
                            if reason=='unclassified' and len(report['unclassified_mismatches'])<20:report['unclassified_mismatches'].append({'a':hex(x),'b':hex(y),'mxcsr':hex(mxcsr),'oracle':hex(expected),'native':hex(out.value)})
                            if len(report['first_value_mismatches'])<20:report['first_value_mismatches'].append({'a':hex(x),'b':hex(y),'mxcsr':hex(mxcsr),'oracle':hex(expected),'native':hex(out.value)})
                        if status!=flags.value:report['mxcsr_mismatches']+=1
        reports.append(report);print(json.dumps(report),flush=True)
    result={'native_probe_sha256':hashlib.sha256(a.native_library.read_bytes()).hexdigest(),'unicorn_version':unicorn.__version__,'scope':'single explicit scalar opcodes; emulator values only, no physical Intel hardware claim','results':reports,'seconds':time.monotonic()-start,'passed':all(not(r['value_mismatches'] or r['mxcsr_mismatches']) for r in reports)}
    a.output.write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
