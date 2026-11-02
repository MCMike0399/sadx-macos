"""Complete Gamma fan body plus the original _ftol and njSin callees.

Compare every word of player/extra storage, including deliberate aliases and
ordered failure prefixes. FP exception differences retain individual identities.
The argument taskwk is unused in the original; playerwk and E102WK are captured.
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
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage

ENTRY, END = 0x4c2e90, 0x4c2f42
BASE, STACK, STOP = 0x11000000, 0x12000800, 0x13000000
STEPS = {0x4c2e96: 1, 0x4c2e99: 2, 0x4c2eeb: 3, 0x4c2ef9: 4,
         0x788290: 5, 0x4c2f01: 6, 0x4c2f04: 7, 0x4c2f2b: 8, 0x4c2f3d: 8}
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'

def packed(values): return struct.pack('<' + 'I' * len(values), *values)
def bits(value): return struct.unpack('<I', struct.pack('<f', value))[0]

class Source:
    def __init__(self, image):
        assert image.sha256 == SOURCE_SHA and uc.__version__ == '2.1.4'
        self.m = m = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        for section in image.sections:
            if section['name'] not in ('.text', '.rdata', '.data'): continue
            address = image.base + section['rva']
            size = (max(section['raw_size'], section['virtual_size']) + 4095) & ~4095
            m.mem_map(address, size)
            m.mem_write(address, image.slice(section['file_offset'], section['raw_size']))
        for page in (BASE, STACK & ~4095, STOP): m.mem_map(page, 4096)
        m.hook_add(uc.UC_HOOK_CODE, self.instruction)
        m.hook_add(uc.UC_HOOK_MEM_INVALID, self.invalid)
    def instruction(self, m, address, size, user):
        if address not in STEPS: return
        self.step = STEPS[address]
        if self.case[4] == self.step and self.case[5] < 1024:
            m.mem_write(BASE + self.case[5] * 4, packed([self.case[6]]))
        if self.case[3] == self.step:
            self.failure = self.step; m.emu_stop()
    def invalid(self, m, access, address, size, value, user):
        self.failure = self.step; return False
    def run(self, case):
        self.case, self.step, self.failure = case, 0, 0
        m = self.m
        m.mem_write(BASE, packed(case[8:]))
        m.mem_write(STACK, packed([STOP, 0, (BASE + case[1] * 4) & 0xffffffff,
                                   (BASE + case[2] * 4) & 0xffffffff]))
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.reg_write(UC_X86_REG_FPCW, case[0])
        m.reg_write(UC_X86_REG_FPSW, 0)
        m.reg_write(UC_X86_REG_FPTAG, 0xffff)
        try: m.emu_start(ENTRY, STOP, count=2000)
        except uc.UcError:
            if not self.failure: raise
        assert self.failure or m.reg_read(UC_X86_REG_EIP) == STOP
        return list(struct.unpack('<1024I', m.mem_read(BASE, 4096))) + [self.failure, self.step,
                                                                m.reg_read(UC_X86_REG_FPSW) & 63]

def fixtures(count, seed):
    rng = random.Random(seed); cases = []
    def add(x=0.0, z=0.0, phase=0, cw=127, deny=0, mutation=0, word=114,
            value=0x3f800000, player=100, extra=500):
        memory = [rng.getrandbits(32) for _ in range(1024)]
        for index, value_bits in [(player+14,bits(x)),(player+16,bits(z)),(extra+7,phase)]:
            if index < 1024: memory[index] = value_bits
        cases.append([cw, player, extra, deny, mutation, word, value, 0] + memory)
    edges = [0.0,-0.0,0.5,-0.5,1.0,-1.0,1.5,-1.5,2.0,-2.0,2.5,-2.5,20.0,-20.0,
             1e30,-1e30,float('inf'),float('-inf')]
    for x in edges:
        for z in [-2.0,0.0,1.0]:
            for phase in [0,0x7fffffff,0xffffffff]: add(x,z,phase)
    for raw in [1,0x80000001,0x7f812345,0xff812345,0x7fc12345,0xffc12345,0x7f7fffff]:
        value = struct.unpack('<f',packed([raw]))[0]
        # Set raw bits afterward so Python does not quiet signaling NaNs.
        add(value,0); cases[-1][8+114] = raw
        add(0,value); cases[-1][8+116] = raw
    for cw in [127,639,895,1151,1663,1919,2175,2687,2943,3199,3711,3967]:
        for deny in range(9): add(1.25,-0.125,0x8000,cw,deny)
    for mutation in range(1,9):
        for word in [114,116,506,507,510]:
            add(1.5,0.25,0x7fffffff,mutation=mutation,word=word,value=0xbf400000)
    for extra in [107,108,109,110,100,1023,1024]:
        for deny in range(9): add(1.5,0.25,extra=extra,deny=deny)
    for player in [1008,1009,1010,1024,0xffffffff,0xfffffff2,0xfbc00000]: add(player=player)
    for extra in [0xffffffff,0xfffffff9,0xfbc00000]: add(extra=extra)
    for _ in range(count):
        add(rng.uniform(-8,8),rng.uniform(-4,4),rng.getrandbits(32),
            cw=rng.choice([127,639,895]),deny=rng.randrange(9),mutation=rng.randrange(9),
            word=rng.choice([114,116,506,507,510]),value=rng.getrandbits(32),
            extra=rng.choice([500,100,107,108,109,110]))
    return cases

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe'); p.add_argument('probe'); p.add_argument('--output',default='build/oracles/gamma_fan.json')
    p.add_argument('--cases',type=int,default=512); p.add_argument('--seed',type=lambda x:int(x,0),default=0x4c2e90)
    a=p.parse_args(); image=PEImage(a.exe); source=Source(image)
    native=ctypes.CDLL(str(Path(a.probe).resolve())).sadx_native_gamma_fan
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
    native.restype=ctypes.c_int
    cases=fixtures(a.cases,a.seed); failures=[]; partials=[]; original_stream=bytearray(); native_stream=bytearray(); prefixes=0
    for index,case in enumerate(cases):
        original=source.run(case); output=(ctypes.c_uint32*1027)()
        assert native((ctypes.c_uint32*len(case))(*case),output)==1
        actual=list(output); original_stream.extend(packed(original));native_stream.extend(packed(actual))
        differences=[i for i in range(1026) if original[i]!=actual[i]]
        prefixes+=bool(original[1024])
        if differences: failures.append({'case_id':index,'config':case[:8],'words':differences,
            'original':[original[i] for i in differences],'native':[actual[i] for i in differences]})
        if original[-1]!=actual[-1]: partials.append({'case_id':index,'original_flags':original[-1],'native_flags':actual[-1]})
    report={'source_sha256':image.sha256,'source_ranges':{f'{ENTRY:#x}-{END:#x}':hashlib.sha256(image.read(ENTRY,END-ENTRY)).hexdigest(),
            '0x644340-0x6443b5':hashlib.sha256(image.read(0x644340,0x75)).hexdigest(),
            '0x788290-0x7883bd':hashlib.sha256(image.read(0x788290,0x12d)).hexdigest()},
        'cases':len(cases),'failure_prefixes':prefixes,'value_failures':len(failures),'mismatches':failures,'fp_partials':partials,
        'values_match':not failures,'passed':not failures and not partials,'seed':a.seed,
        'original_sha256':hashlib.sha256(original_stream).hexdigest(),'native_sha256':hashlib.sha256(native_stream).hexdigest(),
        'limits':['All 1024 player/extra words and ordered failure prefixes; exact hardware exception PCs unverified.',
                  'Original _ftol and njSin execute; individual x87 exception partials remain explicit.',
                  'No geometry, display, particle lifecycle or complete actor-frame claim.']}
    out=Path(a.output);out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(report,indent=2)+'\n')
    out.with_suffix('.original.bin').write_bytes(original_stream);out.with_suffix('.native.bin').write_bytes(native_stream)
    print(json.dumps({k:report[k] for k in ['cases','failure_prefixes','value_failures']}), 'FP partials',len(partials))
    return 1 if failures else 2 if partials else 0
if __name__=='__main__': raise SystemExit(main())
