"""ghDrawPvrTextureS actual instructions, with the pinned GVR format-mask byte patch.

The existing unmodified PVR judge supplies initialization, texture metadata and capture.
Only its source entry is redirected to the scaled sibling, with the two additional arguments.
"""
import argparse,ctypes,hashlib,json,random
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import STACK
from verify_original_pvr_draw import Source as OriginalSource
class Source(OriginalSource):
    def __init__(self,image):
        self.scaled=False
        super().__init__(image)
        self.m.mem_write(0x43434a,b'\x00')
        self.clean=self.m.context_save()
    def hook(self,m,address,size,user):
        if self.scaled and address==0x4338d0:
            self.put(STACK+20,self.scales[0]);self.put(STACK+24,self.scales[1])
            m.reg_write(UC_X86_REG_EIP,0x433f80)
            return
        super().hook(m,address,size,user)
    def scaled_run(self,v):
        self.m.context_restore(self.clean)
        self.scales=v[14:16];self.scaled=True
        return super().run(v[:14])
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('exe',type=Path);p.add_argument('native_library',type=Path)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--seed',type=int,default=423723);p.add_argument('--cases',type=int,default=1500)
    a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise ValueError('requires pinned source')
    source=Source(image);lib=ctypes.CDLL(str(a.native_library.resolve()));native=lib.sadx_native_pvr_scaled
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;native.restype=None;rng=random.Random(a.seed);cases=[]
    floats=[0,0x80000000,0x3f800000,0xbf800000,0x3f000000,0x7f800000,0xff800000,0x7fc12345,0x7f812345,1,0x7f7fffff]
    for cw in (127,639,895,1151,1663,1919,2175,2687,2943,3199,3711,3967):
        for anchor in range(9):
            for flip in range(4):
                for width,height in ((1,3),(255,257),(0xffffffff,0x80000001)):
                    cases.append([(flip<<30)|(anchor<<24)|3,0x42c80000,0xc2480000,0x3f99999a,width,height,97,0xffffffff,0,cw,
                                  0xffffffff,0xff123456,0x80123456,0,0x3f19999a,0x3ef00000])
    for anchor in range(9):
        for axis in (1,2,3,14,15):
            for value in floats:
                row=[(anchor<<24)|1,0x42c80000,0xc2480000,0x3f99999a,513,257,91,0,0,127,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0x3f800000,0x3f800000]
                row[axis]=value;cases.append(row)
    for _ in range(a.cases):
        row=[(rng.randrange(4)<<30)|(rng.randrange(9)<<24)|rng.randrange(8)]+[rng.choice(floats) for _ in range(3)]
        row += [rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.choice([0,0,0,1]),127]
        row += [rng.getrandbits(32) for _ in range(4)]+[rng.choice(floats),rng.choice(floats)]
        cases.append(row)
    bad=[];flags=0
    for index,row in enumerate(cases):
        expected=source.scaled_run(row);out=(ctypes.c_uint32*32)();native((ctypes.c_uint32*16)(*row),out)
        if out[31]!=expected[31]:flags+=1
        diff=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if i!=31 and x!=y]
        if diff:
            bad.append({'case':index,'input':row,'diff':diff})
            if len(bad)==8:break
    root=Path(__file__).resolve().parents[2];paths=['src/engine/pvr_draw.hpp','src/engine/pvr_draw.cpp','tests/probes/native_pvr_scaled_probe.cpp']
    paths += ['src/cpu/fidelity_float.hpp', 'src/cpu/x87_math.hpp', 'src/cpu/x87_math.cpp', 'src/cpu/crt_integer.hpp', 'src/cpu/crt_integer.cpp']
    report={'passed':not bad and not flags,'values_match':not bad,'cases':len(cases),'mismatches':len(bad),'first_mismatches':bad,'fp_flag_mismatches':flags,'seed':a.seed,
            'source_sha256':image.sha256,'source_ranges':{'0x433f80-0x434370':hashlib.sha256(image.read(0x433f80,0x3f0)).hexdigest()},
            'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'native_library_sha256':hashlib.sha256(a.native_library.read_bytes()).hexdigest(),
            'limits':['Effective GVR_Init43434a mask byte applied; alpha submission1 for every format. Texture metadata is fixture-supplied; device pixels not claimed. Valid anchors0..8; unknown anchors read uninitialized source stack and native reports an explicit boundary. Low-six x87 flags are compared separately.']}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
