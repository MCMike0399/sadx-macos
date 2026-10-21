"""MSG ARGB4444 memory-image descriptor/acquisition against original SONIC instructions.
The platform upload remains an explicit receipt; actual byte conversion and immutable
native snapshots are verified separately by the resource unit, not by these receipts.
"""
import ctypes, json, random, sys
from pathlib import Path
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--seed',type=int,default=7861760);parser.add_argument('--cases',type=int,default=600)
args=parser.parse_args()
sys.argv=[__file__,str(args.native_library.resolve()),str(args.output),str(args.seed),str(args.cases),str(args.exe)]

import unicorn
from unicorn.x86_const import UC_X86_REG_ESP
from pe_image import PEImage
from verify_original_native_texture_lifetime import Source

class MessageSource(Source):
    def __init__(self, image):
        super().__init__(image)
        original_start = self.m.emu_start
        def bounded_start(*args, **kwargs):
            # Retain the shared source helper's inputs/comparisons while replacing
            # its legacy instruction cap with the project's wall-clock limit.
            kwargs.pop('count', None)
            kwargs['timeout'] = 5000000
            return original_start(*args, **kwargs)
        self.m.emu_start = bounded_start
        self.m.hook_add(unicorn.UC_HOOK_CODE, self.set_format, begin=0x77f7f0, end=0x77f7f0)
    def set_format(self,m,at,size,unused):
        descriptor = self.get(m.reg_read(UC_X86_REG_ESP) + 4)
        # The MSG caller njSetTextureInfo passes type2306, shifted into texsurface.Type.
        self.put(descriptor + 4, 2306 << 16)

def run(seed, repetitions):
    source = MessageSource(PEImage(sys.argv[5]))
    library = ctypes.CDLL(sys.argv[1]); rng = random.Random(seed)
    mismatches=[]
    for case in range(repetitions):
        count=case%5; gbix=rng.getrandbits(32); w=rng.randrange(1,17); h=rng.randrange(1,17)
        resident=case%2; refs=rng.choice([0,1,2,65535]); oldwidth=rng.getrandbits(32)
        values=[count,gbix,w,h,resident,refs,oldwidth]
        original=[1,gbix,0,0,w,refs,oldwidth,count,resident,h]
        expected=source.prepare(original)
        out=(ctypes.c_uint32*32)()
        library.sadx_message_image_probe((ctypes.c_uint32*7)(*values),out)
        if list(out)!=expected:
            mismatches.append({'case':case,'input':values,'differences':[(i,a,b) for i,(a,b) in enumerate(zip(out,expected)) if a!=b]})
    return {'seed':seed,'cases':repetitions,'mismatches':mismatches,'platform_upload':'receipt only; no original byte conversion claim'}
report=run(int(sys.argv[3]),int(sys.argv[4]));Path(sys.argv[2]).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'cases':report['cases'],'mismatches':len(report['mismatches'])}));sys.exit(bool(report['mismatches']))
