"""SelectBank exact first-match scan and miss preservation, including canonical bank−1.
The full table is source-initialized bank−1,slot ordinal,colors0; versions are BSS0.
"""
import ctypes,json,random,sys
from pathlib import Path
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--seed',type=int,default=7868288);parser.add_argument('--cases',type=int,default=800)
args=parser.parse_args()
sys.argv=[__file__,str(args.native_library.resolve()),str(args.output),str(args.seed),str(args.cases),str(args.exe)]

from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX,UC_X86_REG_EIP
from verify_original_native_texture_lifetime import Source,STACK,STOP
from pe_image import PEImage
s=Source(PEImage(sys.argv[5]));m=s.m
library=ctypes.CDLL(sys.argv[1]);rng=random.Random(int(sys.argv[3]));bad=[]
for case in range(int(sys.argv[4])):
    values=[rng.randrange(128),rng.choice([0xffffffff,0,5,20]),rng.randrange(128),rng.choice([0xffffffff,0,5,20]),rng.choice([0xffffffff,0,5,20,777])]
    out=(ctypes.c_uint32*1)();library.sadx_palette_probe((ctypes.c_uint32*5)(*values),out)
    m.context_restore(s.clean)
    for i in range(128):
        s.put(0x3d11f80+i*16,0xffffffff);s.put(0x3d11f80+i*16+4,i);s.put(0x3d11f80+i*16+8,0)
    for index,bank,color in [(values[0],values[1],1),(values[2],values[3],2)]:
        s.put(0x3d11f80+index*16,bank);s.put(0x3d11f80+index*16+8,color)
    s.put(0x3d128a8,3);s.put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_ECX,values[4]);m.emu_start(0x78cf80,STOP,timeout=5000000)
    assert m.reg_read(UC_X86_REG_EIP)==STOP
    if out[0]!=s.get(0x3d128a8):bad.append({'input':values,'native':out[0],'expected':s.get(0x3d128a8)})
Path(sys.argv[2]).write_text(json.dumps({'cases':case+1,'mismatches':bad},indent=2)+'\n');print(json.dumps({'cases':case+1,'mismatches':len(bad)}));sys.exit(bool(bad))
