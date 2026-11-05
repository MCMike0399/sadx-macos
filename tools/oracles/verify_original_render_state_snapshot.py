"""Original-instruction judge for the seven reached integer render-state leaves.

SetMaterial's existing queue clamp/device implementation is a separate regression;
its scratch-store prefix is covered by the display caller qualification.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys
import unicorn as uc
from unicorn.x86_const import *
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'lib'))
from pe_image import PEImage

ENTRIES = (0x4395c0,0x4395d0,0x439520,0x439540,0x439590,0x77ff60,0x7802d0)
ENDS = (0x4395cb,0x4395db,0x439537,0x43955d,0x4395b7,0x77ff6a,0x7802d7)
GLOBALS = (0x3d0f9c8,0x3d0f840,0x3d0f9c4,0x3b2d3c4,0x3b2d3c8,0x3b2d3c0,0x3d08844)
STACK, STOP = 0x11000800, 0x12000000

def packed(values): return struct.pack('<'+'I'*len(values),*values)


def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('probe')
    p.add_argument('--cases',type=int,default=1024);p.add_argument('--seed',type=lambda s:int(s,0),default=0x439520)
    p.add_argument('--output',default='build/oracles/render_state_snapshot.json');args=p.parse_args()
    image=PEImage(args.exe)
    assert image.sha256=='6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
    assert uc.__version__=='2.1.4'
    m=uc.Uc(uc.UC_ARCH_X86,uc.UC_MODE_32)
    pages={STACK&~4095,STOP,*[a&~4095 for a in GLOBALS],*[a&~4095 for a in ENTRIES]}
    for page in pages:m.mem_map(page,4096)
    for start,end in zip(ENTRIES,ENDS):m.mem_write(start,image.read(start,end-start))
    lib=ctypes.CDLL(str(Path(args.probe).resolve()));native=lib.sadx_native_render_state_snapshot
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];native.restype=ctypes.c_int
    rng=random.Random(args.seed);cases=[]
    for op in range(7):
        for edge in (0,1,0xffffffff,0x80000000,0x00800000,0x00080000):
            cases.append([op]+[edge]*8)
    cases += [[rng.randrange(7)]+[rng.getrandbits(32) for _ in range(8)] for _ in range(args.cases)]
    failures=[];observations=[]
    for index,case in enumerate(cases):
        for address,value in zip(GLOBALS,case[1:7]+[0]):m.mem_write(address,packed([value]))
        m.mem_write(STACK,packed([STOP,*case[7:9]]));m.reg_write(UC_X86_REG_ESP,STACK)
        m.reg_write(UC_X86_REG_ECX,0x47f990 if case[7] else 0)
        m.emu_start(ENTRIES[case[0]],STOP,count=50)
        assert m.reg_read(UC_X86_REG_EIP)==STOP
        expected=[struct.unpack('<I',m.mem_read(a,4))[0] for a in GLOBALS]
        actual=(ctypes.c_uint32*7)();assert native((ctypes.c_uint32*9)(*case),actual)==1
        actual=list(actual)
        observations.append({'case':index,'input_sha256':hashlib.sha256(packed(case)).hexdigest(),
            'original_sha256':hashlib.sha256(packed(expected)).hexdigest(),
            'native_sha256':hashlib.sha256(packed(actual)).hexdigest()})
        if expected!=actual:failures.append({'case':index,'original':expected,'native':actual})
    output=Path(args.output);output.parent.mkdir(parents=True,exist_ok=True)
    rows=output.with_suffix('.observations.json');rows.write_text(json.dumps(observations)+'\n')
    report={'cases':len(cases),'value_failures':len(failures),'status':'fail' if failures else 'pass',
        'failures':failures[:20],'seed':args.seed,
        'source_ranges':{f'{a:#x}-{b:#x}':hashlib.sha256(image.read(a,b-a)).hexdigest() for a,b in zip(ENTRIES,ENDS)},
        'observations_sha256':hashlib.sha256(rows.read_bytes()).hexdigest(),
        'scope':'seven integer leaves; native object callback domain is None/Gamma; SetMaterial excluded',
        'values_match':not failures,'passed':not failures}
    output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(bool(failures))
if __name__=='__main__':raise SystemExit(main())
