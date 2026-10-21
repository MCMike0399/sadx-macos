"""Actual403db0: native owned interval sizes, pointer offset and no cursor/end mutation.
Out-of-owned-range source pointer wrapping is outside native resource API.
"""
import ctypes,json,random,struct,sys
from pathlib import Path
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()

sys.path.insert(0,str(Path(__file__).parent))
from verify_original_late_sprite import Source,root,PEImage,STACK,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP
image=PEImage(args.exe);source=Source(image);m=source.m
U=ctypes.c_uint32;fn=ctypes.CDLL(str(args.native_library.resolve())).sadx_native_end_borrow;fn.argtypes=[U,ctypes.POINTER(U)]
rng=random.Random(403);bad=[]
for size in [0,1,3,20,0x20020,0x88000]+[rng.randrange(0x88001) for _ in range(200)]:
 m.context_restore(source.clean);m.mem_write(0x3ab9874,struct.pack('<I',0x88000));m.mem_write(STACK,struct.pack('<2I',STOP,size));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x403db0,STOP,timeout=0)
 expected=[m.reg_read(UC_X86_REG_EAX),0,0x88000,0];out=(U*4)();assert fn(size,out)==1
 if list(out)!=expected:bad.append([size,expected,list(out)])
report={'cases':206,'mismatch_count':len(bad),'passed':not bad,'mismatches':bad,'source_sha256':image.sha256,'limits':__doc__};args.output.write_text(json.dumps(report,indent=2)+'\n');print(report);assert not bad
