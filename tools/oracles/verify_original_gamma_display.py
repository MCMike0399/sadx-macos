"""Judge E102Display's topology prefix through its first palette-call boundary.

Every data word and the completed failure prefix are compared. This integer
prefix is deliberately separate from later placement, arithmetic and rendering.
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

BASE, STACK, STOP = 0x11000000, 0x12000800, 0x13000000
TABLE, WELD = 0x3a8af6c, 0x3c5455c
ENTRY, END = 0x47fd50, 0x47fdbf
STEPS = {0x47fd59: 1, 0x47fd5c: 2, 0x47fd5f: 3, 0x47fd61: 4,
         0x47fd68: 5, 0x47fd6d: 6, 0x47fd80: 7, 0x47fd93: 7,
         0x47fd83: 8, 0x47fd96: 8, 0x47fd86: 9, 0x47fd99: 9,
         0x47fd89: 10, 0x47fd9c: 10, 0x47fd8e: 11, 0x47fda1: 11,
         0x47fda4: 12, 0x47fda7: 13, 0x47fdaa: 14, 0x47fdaf: 15,
         0x47fdb2: 16, 0x47fdb5: 17, 0x47fdba: 18}
SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'

def packed(values): return struct.pack('<' + 'I' * len(values), *values)


class Source:
    def __init__(self, image):
        assert image.sha256 == SHA and uc.__version__ == '2.1.4'
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
        if address == END:
            m.emu_stop(); return
        if address not in STEPS: return
        self.step = STEPS[address]
        if self.case[2] == self.step:
            m.mem_write(self.case[3], packed([self.case[4]]))
        if self.case[1] == self.step:
            self.failure = self.step; m.emu_stop()
    def invalid(self, m, access, address, size, value, user):
        self.failure = self.step; return False
    def run(self, case):
        self.case, self.step, self.failure = case, 0, 0
        m = self.m
        m.mem_write(BASE, packed(case[8:]))
        m.mem_write(TABLE, packed([case[8]])); m.mem_write(WELD, packed([case[9]]))
        m.mem_write(STACK, packed([STOP, case[0]]))
        m.reg_write(UC_X86_REG_ESP, STACK)
        try: m.emu_start(ENTRY, STOP, count=200)
        except uc.UcError:
            if not self.failure: raise
        assert self.failure or m.reg_read(UC_X86_REG_EIP) == END
        words = list(struct.unpack('<1024I', m.mem_read(BASE, 4096)))
        words[0] = struct.unpack('<I', m.mem_read(TABLE, 4))[0]
        words[1] = struct.unpack('<I', m.mem_read(WELD, 4))[0]
        return words + [self.failure, self.step]


def fixtures(count, seed):
    rng = random.Random(seed)
    cases = []
    def make(upgraded=False, deny=0, mutation=0, address=0, value=0):
        memory = [rng.getrandbits(32) for _ in range(1024)]
        def put(address, value): memory[(address-BASE)//4] = value
        task, motion, player, extra = [BASE + n for n in (256, 512, 768, 1536)]
        table, other = BASE + 2048, BASE + 2176
        objects = [BASE + 2560 + 64*i for i in range(8)]
        put(task+32, BASE+1600); put(task+36, motion); put(motion, player)
        put(player+4, (rng.getrandbits(32) & ~512) | (512 if upgraded else 0))
        put(player+124, extra)
        for current in (table, other):
            for index, obj in zip((14,16,18,20,22,23), objects): put(current+index*4, obj)
        for i, obj in enumerate(objects):
            put(obj+44, objects[(i+1)%len(objects)]); put(obj+48, objects[(i+2)%len(objects)])
        memory[0], memory[1] = table, rng.getrandbits(32)
        case = [task, deny, mutation, address, value, 0, 0, 0] + memory
        cases.append(case)
        return case
    for upgraded in (False, True):
        make(upgraded)
        for deny in range(1,19): make(upgraded,deny)
        # Table replacement before either store must honor the source's retained
        # first pointer and subsequent reloads. Values are all mapped identities.
        for step in range(1,19):
            make(upgraded,mutation=step,address=TABLE,value=BASE+2176)
        for offset in (0,32,36):
            c=make(upgraded); c[8+(256+offset)//4]=0
        # A selected sibling can alias a table row or destination object.
        for a,b in [(14,22),(16,22),(18,23),(20,23),(22,23)]:
            c=make(upgraded);c[8+(2048+a*4)//4]=c[8+(2048+b*4)//4]
        for row in (14,16,18,20,22,23):
            c=make(upgraded);c[8+(2048+row*4)//4]=0
    for _ in range(count):
        c=make(bool(rng.getrandbits(1)),rng.randrange(19))
        if rng.randrange(4)==0:
            row=rng.choice((14,16,18,20,22,23))
            c[8+(2048+row*4)//4]=rng.choice([0,BASE+4092,BASE+2600,BASE+2200])
    return cases


def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('probe')
    p.add_argument('--cases',type=int,default=1024);p.add_argument('--seed',type=lambda s:int(s,0),default=0x47fd50)
    p.add_argument('--body',action='store_true');p.add_argument('--output',default='build/oracles/gamma_display.json');args=p.parse_args()
    image=PEImage(args.exe); source=Source(image)
    library=ctypes.CDLL(str(Path(args.probe).resolve()));native=library.sadx_native_gamma_display
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];native.restype=ctypes.c_int
    failures=[]; observations=[]; failure_prefixes=0
    cases=fixtures(args.cases,args.seed)
    for index,case in enumerate(cases):
        expected=source.run(case);output=(ctypes.c_uint32*1026)()
        assert native((ctypes.c_uint32*1032)(*case),output)==1
        actual=list(output);failure_prefixes+=bool(expected[1024])
        observations.append({'case':index,'input_sha256':hashlib.sha256(packed(case)).hexdigest(),
            'original_sha256':hashlib.sha256(packed(expected)).hexdigest(),
            'native_sha256':hashlib.sha256(packed(actual)).hexdigest(),
            'failure_prefix':expected[1024]})
        if expected!=actual:
            failures.append({'case':index,'different_words':[i for i,(a,b) in enumerate(zip(expected,actual)) if a!=b]})
    path=Path(args.output);path.parent.mkdir(parents=True,exist_ok=True)
    observation_path=path.with_suffix('.observations.json');observation_path.write_text(json.dumps(observations)+'\n')
    report={'cases':len(cases),'seed':args.seed,'value_failures':len(failures),'failure_prefixes':failure_prefixes,
        'status':'fail' if failures else 'pass','failures':failures[:20],
        'source_ranges':{f'{ENTRY:#x}-{END:#x}':hashlib.sha256(image.read(ENTRY,END-ENTRY)).hexdigest()},
        'scope':'integer topology prefix only; stops before first palette call; no placement or rendering claim',
        'observations_sha256':hashlib.sha256(observation_path.read_bytes()).hexdigest()}
    if args.body:
        from gamma_display_body_judge import judge
        report['body'] = judge(image, library, args.cases, args.seed ^ 0xabc, path)
        if report['body']['value_failures']: report['status'] = 'fail'
    # Normalised verdict fields: values exact and no failure means pass.
    report['values_match'] = report['status'] != 'fail'
    report['passed'] = report['values_match']
    path.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(report['status'] == 'fail')
if __name__=='__main__':raise SystemExit(main())
