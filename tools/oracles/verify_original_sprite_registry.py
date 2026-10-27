"""Original RegisterSprite instructions vs typed native state and actual arena.

CAlloc is intercepted: this proves the constructor, not the lower allocator.
No definition bytes are mapped. Source read/write ordering is separately asserted.
A separate instrumented native copy checks five store prefixes; product source has no observer.
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
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/lib'))
from pe_image import PEImage

START, END = 0x456c50, 0x456ca1
ALLOC = 0x40b2d0
HEAD, COUNT = 0x3c4858c, 0x3c48590
BUFFER, STACK, STOP = 0x11000000, 0x12000800, 0x13000000
MASK = 0xffffffff


def rounded_size(size):
    return (size + 3) & MASK & ~3


def allocation_succeeds(size, fail):
    size = rounded_size(size)
    return not fail and 0 < size <= 0xffffffc0 and ((size + 63) & ~31) <= 0x1000000


class Source:
    def __init__(self, image):
        if image.sha256 != '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74':
            raise RuntimeError('Original PE identity mismatch')
        self.code = image.read(START, END - START)
        if hashlib.sha256(self.code).hexdigest() != '0fa30c3ff99ef4a7d0f07319edd21692228239ed62574cfb08cee6a3c936526f':
            raise RuntimeError('Original constructor range identity mismatch')
        m = self.m = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        for address in (START, ALLOC, HEAD, BUFFER, STACK, STOP):
            m.mem_map(address & ~4095, 4096)
        m.mem_write(START, self.code)
        m.hook_add(uc.UC_HOOK_CODE, self.instruction)
        m.hook_add(uc.UC_HOOK_MEM_READ, self.read)
        m.hook_add(uc.UC_HOOK_MEM_WRITE, self.write)

    def word(self, address):
        return struct.unpack('<I', self.m.mem_read(address, 4))[0]

    def put(self, address, value):
        self.m.mem_write(address, struct.pack('<I', value))

    def read(self, m, access, address, size, value, user):
        if address in (COUNT, HEAD):
            self.reads.append((address, size))

    def write(self, m, access, address, size, value, user):
        if address in (COUNT, HEAD) or BUFFER <= address < BUFFER + 28:
            self.writes.append((address, size, value & MASK))

    def instruction(self, m, address, size, user):
        phases = {0x456c88: 0, 0x456c8d: 1, 0x456c93: 2, 0x456c9b: 3, 0x456c9f: 4}
        if address in phases:
            phase = phases[address]
            self.prefixes += [phase, 1 if self.word(HEAD) == BUFFER else self.word(HEAD),
                              self.word(COUNT), self.word(BUFFER),
                              self.word(BUFFER + 16) if phase == 4 else 0,
                              self.word(BUFFER + 24) - BUFFER if phase >= 3 and self.case[1] else 0]
        if address != ALLOC:
            return
        sp = m.reg_read(UC_X86_REG_ESP)
        self.args = [self.word(sp + 4), self.word(sp + 8)]
        self.put(COUNT, self.case[4])
        self.put(HEAD, self.case[5])
        # The separate lower allocator is represented by a bounded actual-arena
        # contract. Only bytes observable by this constructor need backing here.
        answer = BUFFER if allocation_succeeds(self.args[1], self.case[6]) else 0
        if answer:
            m.mem_write(BUFFER, bytes(min(rounded_size(self.args[1]), 4096)))
        m.reg_write(UC_X86_REG_EAX, answer)
        m.reg_write(UC_X86_REG_ESP, sp + 4)
        m.reg_write(UC_X86_REG_EIP, self.word(sp))

    def run(self, case):
        self.case = case
        definition, extra, count, head, post_count, post_head, fail = case
        self.put(COUNT, count)
        self.put(HEAD, head)
        self.m.mem_write(BUFFER, bytes([0xff]) * 4096)
        self.m.mem_write(STACK, struct.pack('<3I', STOP, definition, extra))
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.args, self.reads, self.writes, self.prefixes = [], [], [], []
        self.m.emu_start(START, STOP, count=100)
        assert self.m.reg_read(UC_X86_REG_EIP) == STOP
        assert self.m.reg_read(UC_X86_REG_ESP) == STACK + 4
        result = self.m.reg_read(UC_X86_REG_EAX)
        expected_reads, expected_writes = [], []
        if result:
            expected_reads = [(COUNT, 4), (HEAD, 4)]
            expected_writes = [(BUFFER, 4, post_head), (HEAD, 4, BUFFER),
                               (COUNT, 4, (post_count + 1) & MASK)]
            if extra:
                expected_writes.append((BUFFER + 24, 4, BUFFER + 28))
            expected_writes.append((BUFFER + 16, 4, definition))
        assert self.reads == expected_reads, (case, self.reads)
        assert self.writes == expected_writes, (case, self.writes)
        record = list(struct.unpack('<7I', self.m.mem_read(BUFFER, 28))) if result else [0] * 7
        if result and record[6]:
            record[6] -= BUFFER
        return [bool(self.args), *(self.args or [0, 0]), self.word(COUNT),
                1 if self.word(HEAD) == BUFFER else self.word(HEAD), bool(result), *record,
                rounded_size(self.args[1]) if result else 0]


def cases(seed, count, holdout):
    if not holdout:
        for definition in (0, 0xdeadbeef):
            for extra in (0, 1, 2, 3, 4, 28, 4096, 0xffffffe0, 0xffffffe4,
                          0xffffffe5, 0xffffffe8, 0xfffffffc, 0xffffffff, 0x80000000):
                for fail in (0, 1):
                    for after in (0, MASK):
                        yield [definition, extra, 13, 0x20000000, after, 0x21000000, fail]
    rng = random.Random(seed)
    for _ in range(count):
        extra = rng.choice([rng.randrange(257), rng.randrange(0xffffffe0, 1 << 32),
                            rng.randrange(1 << 32)])
        yield [rng.choice([0, rng.randrange(0x30000000, 0x31000000)]), extra,
               rng.getrandbits(32), rng.choice([0, 0x20000000]), rng.getrandbits(32),
               rng.choice([0, 0x21000000]), rng.randrange(2)]


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--exe', required=True)
    p.add_argument('--probe', default=str(ROOT / 'build/native-sprite-registry-probe.dylib'))
    p.add_argument('--output', default=str(ROOT / 'build/oracles/sprite_registry.json'))
    p.add_argument('--prefix-probe')
    p.add_argument('--holdout', action='store_true')
    p.add_argument('--seed', type=int, default=45650)
    p.add_argument('--count', type=int, default=1024)
    a = p.parse_args()
    image = PEImage(a.exe)
    source = Source(image)
    native = ctypes.CDLL(a.probe).sadx_native_sprite_registry
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32)] * 2
    prefix_native = None
    if a.prefix_probe:
        prefix_native = ctypes.CDLL(a.prefix_probe).sadx_native_sprite_registry
        prefix_native.argtypes = native.argtypes
    mismatch = []
    n = 0
    for case in cases(a.seed, a.count, a.holdout):
        expected = source.run(case)
        actual = (ctypes.c_uint32 * 14)()
        assert native((ctypes.c_uint32 * 7)(*case), actual) == 1
        n += 1
        prefix_match = True
        if prefix_native:
            observed = (ctypes.c_uint32 * 45)()
            assert prefix_native((ctypes.c_uint32 * 7)(*case), observed) == 1
            prefix_match = (list(observed[:14]) == expected and
                            list(observed[14:14 + observed[44] * 6]) == source.prefixes and
                            observed[44] * 6 == len(source.prefixes))
        if list(actual) != expected or not prefix_match:
            mismatch.append({'case': case, 'source': expected, 'native': list(actual), 'prefix_match': prefix_match})
            if len(mismatch) == 8:
                break
    result = {'slug': 'sprite_registry', 'cases': n, 'holdout': a.holdout,
              'seed': a.seed, 'prefixes_match': not mismatch if prefix_native else None, 'passed': not mismatch, 'values_match': not mismatch,
              'mismatches': mismatch, 'mismatch_count': len(mismatch),
              'source_ranges': {f'{START:#x}-{END:#x}': {'size': END - START}},
              'range_sha256': hashlib.sha256(source.code).hexdigest(),
              'limits': ['CAlloc intercepted with actual-arena size/failure contract; lower allocator not judged.',
                         'Source instruction read/store order asserted; five native prefixes use a test-only instrumented copy when requested. Before-store uninitialized fields are excluded from prefix snapshots; final bytes remain compared.',
                         'Opaque unmapped definition identity only; no payload, import, Gamma binding or rendering claim.',
                         'Wrapped undersized allocations retain interior identity; consumers/access not qualified.',
                         'No floating-point instructions in constructor; strict constructor values match only.']}
    path = Path(a.output)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'cases': n, 'mismatch_count': len(mismatch), 'passed': not mismatch}))
    return bool(mismatch)


if __name__ == '__main__':
    sys.exit(main())
