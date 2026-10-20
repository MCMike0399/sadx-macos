"""Exact Input DLL Init narrowing instructions, with getInt results as caller inputs.

No INI parser/import/Windows startup executes. Key WORD store+MOVSX, signed-WORD
clamps, byte threshold store and signed keyboard-player clamp are actual code.
"""
import argparse,ctypes,hashlib,json,pathlib,random
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDI,UC_X86_REG_ESP
from pe_image import PEImage
from verify_original_native_input import Source,KEYS,DATA,STACK,get,put,SHA

def main():
    parser=argparse.ArgumentParser();parser.add_argument('input_dll',type=pathlib.Path);parser.add_argument('native_library',type=pathlib.Path);parser.add_argument('--output',type=pathlib.Path,required=True);parser.add_argument('--seed',type=int,default=20261021);args=parser.parse_args()
    image=PEImage(args.input_dll)
    if image.sha256!=SHA:raise RuntimeError('Pinned DLL identity mismatch')
    source=Source(image,KEYS).m
    native=ctypes.CDLL(str(args.native_library.resolve())).sadx_native_input_configuration
    native.argtypes=[ctypes.POINTER(ctypes.c_int32),ctypes.POINTER(ctypes.c_uint32)];native.restype=ctypes.c_int
    edges=[-2147483648,-65537,-65536,-65535,-32769,-32768,-1,0,1,7,8,255,256,32767,32768,65535,65536,65537,2147483647]
    cases=[[value,value,value,value,value] for value in edges]
    rng=random.Random(args.seed)
    cases += [[rng.randint(-2147483648,2147483647) for _ in range(5)] for _ in range(2000)]
    bad=[]
    for number,(key,left,right,threshold,player) in enumerate(cases):
        source.reg_write(UC_X86_REG_EAX,key&0xffffffff);source.emu_start(0x10006f39,0x10006f3f,count=2)
        source.emu_start(0x1000521c,0x10005223,count=2);expected=[source.reg_read(UC_X86_REG_EAX)]
        source.reg_write(UC_X86_REG_ESP,STACK);source.reg_write(UC_X86_REG_EDI,DATA+0x400)
        put(source,STACK+12,4,left);put(source,STACK+16,4,right)
        source.emu_start(0x100077dd,0x10007863,count=40)
        expected += [get(source,DATA+0x402,2),get(source,DATA+0x406,2)]
        source.reg_write(UC_X86_REG_EAX,threshold&0xffffffff);source.emu_start(0x10007bc2,0x10007bc5,count=2)
        expected += [get(source,DATA+0x409,1)]
        source.reg_write(UC_X86_REG_EAX,player&0xffffffff);source.emu_start(0x10006989,0x100069a0,count=20)
        expected += [source.reg_read(UC_X86_REG_EAX)]
        output=(ctypes.c_uint32*5)()
        if not native((ctypes.c_int32*5)(key,left,right,threshold,player),output):raise RuntimeError('Native config rejected')
        if list(output)!=expected:bad.append({'case':number,'input':cases[number],'expected':expected,'actual':list(output)})
    report={'source_sha256':image.sha256,'seed':args.seed,'cases':len(cases),'mismatch_count':len(bad),'mismatches':bad[:20],'passed':not bad,'scope':__doc__,'cases_sha256':hashlib.sha256(json.dumps(cases).encode()).hexdigest(),'native_probe_sha256':hashlib.sha256(args.native_library.read_bytes()).hexdigest()}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','mismatch_count','passed']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
