"""Original77fa10 exact-GBIX search, unchanged reference count and descriptor.
Upload is a receipt; palette-bank selection executes in both implementations.
The complete selector and initialization have separate source judges; this oracle
compares reload lookup, unchanged count and descriptor fields.
"""
import ctypes,json,random,struct,sys
from pathlib import Path
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--seed',type=int,default=7862800);parser.add_argument('--cases',type=int,default=700)
args=parser.parse_args()
sys.argv=[__file__,str(args.native_library.resolve()),str(args.output),str(args.seed),str(args.cases),str(args.exe)]

import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
from pe_image import PEImage
from verify_original_native_texture_lifetime import Source,POOL,BASE,STACK,STOP
source=Source(PEImage(sys.argv[5]));m=source.m
# Exact reload passes Memory in ECX and pixels in EDX; leave native platform conversion separate.
def upload(m,at,size,unused):
    esp=m.reg_read(UC_X86_REG_ESP)
    m.reg_write(UC_X86_REG_EIP,source.get(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
m.hook_add(unicorn.UC_HOOK_CODE,upload,begin=0x78cbd0,end=0x78cbd0)
library=ctypes.CDLL(sys.argv[1]);rng=random.Random(int(sys.argv[3]));bad=[]
for case in range(int(sys.argv[4])):
    gbix=rng.randrange(1,0xffffff00);w=rng.randrange(1,17);h=rng.randrange(1,17);refs=rng.randrange(0,65536);missing=case%2
    values=[gbix,w,h,refs,missing];out=(ctypes.c_uint32*32)()
    library.sadx_message_reload_probe((ctypes.c_uint32*5)(*values),out)
    m.context_restore(source.clean);source.put(0x3d0f80c,3);source.put(0x3d0f844,POOL)
    for index in range(3):
        words=[0xffffffff,0xffffffff]+[0]*14+[0xffff0000]
        if index==0:
            words[0]=gbix;words[6]=2306<<16;words[8]=268435456;words[9]=w;words[10]=h;words[11]=w*h*2;words[16]|=refs
        m.mem_write(POOL+index*68,struct.pack('<17I',*words))
    source.put(STACK,STOP);source.put(STACK+4,gbix+missing);source.put(STACK+8,BASE+0x7000)
    m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x77fa10,STOP,timeout=5000000)
    assert m.reg_read(UC_X86_REG_EIP)==STOP
    expected=[m.reg_read(UC_X86_REG_EAX),source.get(POOL+64,2),source.get(POOL+24),source.get(POOL+32),source.get(POOL+36),source.get(POOL+40),source.get(POOL+44)]+[0]*25
    if list(out)!=expected: bad.append({'case':case,'input':values,'expected':expected,'native':list(out)})
report={'cases':case+1,'mismatches':bad,'upload':'receipt','palette_bank':'executed; separately judged by verify_palette.py and verify_palette_initial.py'}
Path(sys.argv[2]).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':case+1,'mismatches':len(bad)}));sys.exit(bool(bad))
