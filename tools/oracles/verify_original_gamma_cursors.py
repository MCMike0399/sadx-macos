#!/usr/bin/env python3
"""Complete original Gamma cursor initializer/expander versus the native core.

MAlloc alone is an ordered receipt with the actual arena's rounding, exhaustion
and returned allocation identities. This judges CPU template/indexed copies and
publication/failure prefixes; it does not independently re-prove MAlloc or GPUs.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP

SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
EXPAND, INITIALIZE, ALLOCATE = 0x4c2d40, 0x4c2e10, 0x40b220
RANGES = ((EXPAND, 0x4c2de4), (INITIALIZE, 0x4c2e48))
BASE, HEAP = 0x10000000, 0x20000000
FIRST, SECOND = BASE + 0x100, BASE + 0x140
POINTS, ALTPOINTS = BASE + 0x1000, BASE + 0x2000
COLORS, ALTCOLORS, INDICES = BASE + 0x3000, BASE + 0x4000, BASE + 0x5000
STACK, STOP = BASE + 0xf000, BASE + 0xff00
TEMPLATE1, TEMPLATE2 = 0x981ea0, 0x981ec0
ARENA_BYTES = 0x1000000


class Source:
    def __init__(self, image):
        assert image.sha256 == SOURCE_SHA
        assert unicorn.__version__ == '2.1.4'
        self.m = m = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
        for page in (0x4c2000, 0x40b000, 0x981000):
            m.mem_map(page, 4096)
        m.mem_map(BASE, 0x10000)
        m.mem_map(HEAP, ARENA_BYTES)
        for start, end in RANGES:
            m.mem_write(start, image.read(start, end - start))
        m.hook_add(unicorn.UC_HOOK_CODE, self.hook)

    def put(self, at, value, size=4):
        self.m.mem_write(at, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))

    def get(self, at, size=4):
        return int.from_bytes(self.m.mem_read(at, size), 'little')

    def hook(self, m, at, size, user):
        if at == 0x4c2da0:
            self.vertex += 1
        if at == EXPAND:
            self.vertex = -1
        if at != ALLOCATE:
            return
        sp = m.reg_read(UC_X86_REG_ESP)
        requested = self.get(sp + 4)
        total = (requested + 63) & ~31
        pointer = 0
        if requested and requested <= 0xffffffff - 63 and self.next + total <= ARENA_BYTES:
            pointer = HEAP + self.next + 32
            self.next += total
            self.allocations[pointer] = requested
            m.mem_write(pointer, b'\xff' * requested)
        self.receipts.append([requested, pointer - HEAP if pointer else 0])
        if pointer:
            self.successful_allocations += 1
            if self.successful_allocations in (2, 4):
                model = FIRST if self.successful_allocations == 2 else SECOND
                previous = self.get(model + 16)
                self.observations[self.successful_allocations // 2 + 2] = previous - HEAP if previous else 0
        if pointer and not self.mutated:
            self.mutated = True
            v = self.values
            if v[4] & 1:
                self.put(FIRST, ALTPOINTS)
                self.put(FIRST + 4, ALTCOLORS)
                self.put(FIRST + 8, 0)
                self.put(FIRST + 12, 0, 2)
            if v[4] & 2:
                self.put(TEMPLATE2 + 12, v[6], 2)
            if v[4] & 4:
                self.put(TEMPLATE2, ALTPOINTS + v[5] * 12)
                self.put(TEMPLATE2 + 4, COLORS + v[5] * 4)
        m.reg_write(unicorn.x86_const.UC_X86_REG_EAX, pointer)
        m.reg_write(UC_X86_REG_EIP, self.get(sp))
        m.reg_write(UC_X86_REG_ESP, sp + 4)

    def snapshot(self, address, descriptor, payload, out):
        point, color, index = (self.get(address + i * 4) for i in range(3))
        for pointer, bases, slot, stride in ((point, (POINTS, ALTPOINTS), descriptor, 4),
                                           (color, (COLORS, ALTCOLORS), descriptor + 2, 4)):
            for identity, base in enumerate(bases, 1):
                if base <= pointer < base + 512:
                    out[slot] = identity
                    out[slot + 1] = (pointer - base) // stride
                    break
        out[descriptor + 4] = int(index == INDICES)
        out[descriptor + 5] = self.get(address + 12, 2)
        out[descriptor + 6] = self.get(address + 14, 2)
        for field, slot, output, limit in ((16, descriptor + 7, payload, 384),
                                            (20, descriptor + 8, payload + 96, 128)):
            pointer = self.get(address + field)
            out[slot] = pointer - HEAP if pointer else 0
            if pointer:
                count = min(limit, self.allocations[pointer]) // 4
                out[output:output + count] = struct.unpack('<' + 'I' * count,
                                                          self.m.mem_read(pointer, count * 4))
        out[descriptor + 9] = int(self.get(address + 24) != 0)
        out[descriptor + 10] = self.get(address + 28)

    def run(self, values):
        self.values = v = values
        self.next = ARENA_BYTES - v[3] if v[3] else 0
        self.allocations, self.receipts, self.observations = {}, [], {}
        self.successful_allocations = 0
        self.mutated, self.vertex = False, -1
        self.m.mem_write(BASE, bytes(0x10000))
        for at, words in ((POINTS, v[32:80]), (COLORS, v[80:96]),
                          (ALTCOLORS, v[128:144]), (ALTPOINTS, v[144:192])):
            self.m.mem_write(at, struct.pack('<' + 'I' * len(words), *words))
        self.m.mem_write(INDICES, struct.pack('<32H', *[x & 65535 for x in v[96:128]]))
        first = struct.pack('<IIIHHIIII', POINTS + v[5] * 12, COLORS + v[5] * 4,
                            INDICES, v[1] & 65535, v[7], 0, 0, 0, 0)
        second = struct.pack('<IIIHHIIII', POINTS + v[5] * 12, ALTCOLORS + v[5] * 4,
                             INDICES, v[2] & 65535, v[8], 0, 0, 0, 0)
        self.m.mem_write(FIRST, first)
        self.m.mem_write(TEMPLATE1, first)
        self.m.mem_write(TEMPLATE2, second)
        self.put(STACK, STOP)
        self.put(STACK + 4, FIRST)
        self.put(STACK + 8, SECOND)
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        out = [0] * 512
        try:
            self.m.emu_start(INITIALIZE if v[0] else EXPAND, STOP, count=10000)
            assert self.m.reg_read(UC_X86_REG_EIP) == STOP
        except unicorn.UcError:
            pc = self.m.reg_read(UC_X86_REG_EIP)
            faults = {0x4c2da0: (1, 0), 0x4c2dad: (2, 0), 0x4c2db3: (2, 1),
                      0x4c2db9: (2, 2), 0x4c2db1: (3, 0), 0x4c2db6: (3, 1),
                      0x4c2dbc: (3, 2), 0x4c2dc3: (4, 0), 0x4c2dc6: (5, 0)}
            if pc not in faults:
                raise
            out[0], out[2] = faults[pc]
            out[1] = self.vertex
        for slot, value in self.observations.items():
            out[slot] = value
        self.snapshot(FIRST, 16, 128, out)
        self.snapshot(SECOND, 32, 256, out)
        return out


def cases(seed, count):
    rng = random.Random(seed)
    special = [0, 0x80000000, 0x7f800001, 0x7fc12345, 0x3f800001, 0xffffffff]
    counts = [0, 1, 2, 8, 16, -1, -32768, 32767]
    for case in range(count):
        v = [0] * 192
        v[0] = case & 1
        v[1], v[2] = (rng.choice(counts) & 65535 for _ in range(2))
        # Large positive counts are exercised only with exhaustion, so no unknown
        # source indices beyond the explicitly supplied 32-index view are read.
        v[3] = rng.choice([32, 64, 96, 128, 256, 512, 0])
        if 32767 in (v[1], v[2]):
            v[3] = 32
        v[4] = rng.randrange(8)
        v[5] = rng.randrange(16)
        v[6] = rng.choice(counts[:-1]) & 65535
        v[7], v[8] = rng.randrange(65536), rng.randrange(65536)
        for index in list(range(32, 96)) + list(range(128, 192)):
            v[index] = rng.choice(special) if rng.randrange(3) == 0 else rng.getrandbits(32)
        for index in range(96, 128):
            v[index] = (rng.randrange(16) - v[5]) & 65535
        yield v


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', required=True)
    parser.add_argument('--probe', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--holdout', action='store_true')
    parser.add_argument('--cases', type=int, default=768)
    args = parser.parse_args()
    image = PEImage(Path(args.exe))
    source = Source(image)
    probe = ctypes.CDLL(str(Path(args.probe).resolve())).sadx_native_gamma_cursors
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    mismatch, receipts, boundary = [], 0, 0
    for number, values in enumerate(cases(0x742a39 if args.holdout else 0x192db0, args.cases)):
        original = source.run(values)
        result = (ctypes.c_uint32 * 512)()
        assert probe((ctypes.c_uint32 * 192)(*values), result) == 1
        native = list(result)
        boundary += int(original[0] != 0)
        receipts += len(source.receipts)
        if native != original:
            mismatch.append({'case': number, 'input': values,
                             'differences': [[i, a, b] for i, (a, b) in
                                             enumerate(zip(original, native)) if a != b][:16]})
    report = {'slug': 'gamma_cursors', 'passed': not mismatch, 'values_match': not mismatch,
              'cases': args.cases, 'holdout': args.holdout, 'mismatch_count': len(mismatch),
              'mismatches': mismatch[:8], 'allocator_receipts': receipts,
              'failure_prefix_cases': boundary,
              'original_ranges_sha256': {
                  f'{start:x}-{end:x}': hashlib.sha256(image.read(start, end - start)).hexdigest()
                  for start, end in RANGES},
              'limits': ['MAlloc is an ordered allocation receipt, not an independent allocator proof.',
                         'Bounded source arrays and real native arena; no GPU or PJoinVertexes claim.',
                         'Large positive counts only with allocation exhaustion; no unknown array reads.']}
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in
                      ('passed', 'cases', 'mismatch_count', 'allocator_receipts', 'failure_prefix_cases')}))
    return int(bool(mismatch))


if __name__ == '__main__':
    raise SystemExit(main())
