import ctypes,json,struct,sys
from pathlib import Path
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--seed',type=int,default=0);parser.add_argument('--cases',type=int,default=1)
args=parser.parse_args()
sys.argv=[__file__,str(args.native_library.resolve()),str(args.output),str(args.seed),str(args.cases),str(args.exe)]

from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
from verify_original_native_texture_lifetime import Source,STACK,STOP
from pe_image import PEImage
s=Source(PEImage(sys.argv[5]));m=s.m
s.put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x78d0d0,STOP,timeout=5000000)
assert m.reg_read(UC_X86_REG_EIP)==STOP
expected=list(struct.unpack('<512I',m.mem_read(0x3d11f80,2048)))+[s.get(0x3d128a8)]+list(struct.unpack('<255I',m.mem_read(0x3cfc000,1020)))
out=(ctypes.c_uint32*768)();ctypes.CDLL(sys.argv[1]).sadx_palette_initial_probe(out)
bad=[(i,a,b) for i,(a,b) in enumerate(zip(out,expected)) if a!=b]
Path(sys.argv[2]).write_text(json.dumps({'cases':1,'compared_words':768,'mismatches':bad},indent=2)+'\n');print(json.dumps({'cases':1,'mismatches':len(bad)}));sys.exit(bool(bad))
