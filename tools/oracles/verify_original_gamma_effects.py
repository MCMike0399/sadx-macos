"""Execute complete Gamma effects/binding/pool source bodies against native code.

Only CreateElementalTask, MAlloc, DestroyTask and NeonuLoadTexture are intercepted.
Complete frame tables, allocation arguments/prefixes, task callbacks, typed work,
all observed uninitialized buffer bytes and exact pointer identities are compared.
"""
import argparse
import ctypes
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import sys
import unicorn as uc
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/lib'))
from pe_image import PEImage

ENTRY, BURST_INIT, BIND, POOL = 0x49fb10, 0x49f9f0, 0x4cc330, 0x4cc210
CREATE, ALLOC, DESTROY, LOAD = 0x40b860, 0x40b220, 0x40b570, 0x4228e0
OWNER, LIST, HIT, BURST = 0x3c56df0, 0x91d15c, 0x91d168, 0x91d458
TASK, OLD, MARKER, CUSTOM = 0x21000000, 0x22000000, 0x23000000, 0x24000000
BUFFER, STACK, STOP, PARAMS = 0x11000000, 0x12000800, 0x13000000, 0x25000000
ENTRIES, MEMORIES = 0x26000000, 0x27000000
RANGES = [(ENTRY, 25), (BURST_INIT, 76), (BIND, 173), (POOL, 199)]
MASK = 0xffffffff


def words(data):
    return list(struct.unpack('<' + 'I' * (len(data) // 4), data))


def packed(values):
    return struct.pack('<' + 'I' * len(values), *(x & MASK for x in values))


class Source:
    def __init__(self, image):
        if image.sha256 != '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74':
            raise ValueError('original source mismatch')
        self.ranges = {f'{a:#x}-{a+n:#x}': hashlib.sha256(image.read(a, n)).hexdigest()
                       for a, n in RANGES}
        self.m = m = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        pages = {a & ~4095 for a in [ENTRY, BURST_INIT, BIND, POOL, CREATE, ALLOC, DESTROY,
                 LOAD, OWNER, LIST, TASK, BUFFER, STACK, STOP, PARAMS, ENTRIES, MEMORIES,
                 0x7dcb7c, 0x7dcc98]}
        pages.update(BUFFER + i * 4096 for i in range(8))
        for address in pages:
            m.mem_map(address, 4096)
        for address, size in RANGES:
            m.mem_write(address, image.read(address, size))
        for address in [0x7dcb7c, 0x7dcc98]:
            m.mem_write(address, image.read(address, 4))
        m.hook_add(uc.UC_HOOK_CODE, self.instruction)

    def word(self, address):
        return words(bytes(self.m.mem_read(address, 4)))[0]

    def put(self, address, value):
        self.m.mem_write(address, packed([value]))

    def event(self, kind, first, second, third):
        awp = self.word(TASK + 44) if self.created else 0
        capacity = self.word(awp) if awp else 0
        free = self.word(awp + 4) if awp and self.allocations >= 3 else 0
        active = self.word(awp + 8) if awp and self.allocations >= 4 else 0
        self.events.extend([kind, first, second, third, self.word(OWNER), awp, capacity, free, active])

    def reenter(self):
        if self.case[10] & 1:
            self.put(OWNER, MARKER)
        if self.case[10] & 2 and self.case[0] == 2:
            self.m.mem_write(PARAMS, packed(self.case[14:16]))

    def instruction(self, m, address, size, user):
        if address not in [CREATE, ALLOC, DESTROY, LOAD]:
            return
        sp = m.reg_read(UC_X86_REG_ESP)
        result = 0
        if address == CREATE:
            self.creates += 1
            self.event(1, self.word(sp + 4), self.word(sp + 8), self.word(sp + 12))
            self.reenter()
            if not self.case[4]:
                self.created = True
                m.mem_write(TASK, bytes(52))
                self.put(TASK + 16, self.word(sp + 12))
                result = TASK
        elif address == ALLOC:
            self.allocations += 1
            self.event(2, self.word(sp + 4), self.allocations, 0)
            self.reenter()
            requested = self.word(sp + 4)
            self.sizes.append(requested)
            if self.case[5] != self.allocations and 0 < requested <= 8192:
                result = BUFFER + (self.allocations - 1) * 4096
                m.mem_write(result, bytes([0xa0 + self.allocations]) * requested)
            self.buffers.append(result)
        elif address == DESTROY:
            self.destroyed += 1
            self.event(3, self.word(sp + 4), 0, 0)
            self.reenter()
        else:
            self.event(4, self.word(sp + 4), 0, 0)
            self.reenter()
            if self.case[13]:
                first, second = self.word(ENTRIES + 8), self.word(ENTRIES + 20)
                self.put(ENTRIES + 8, second)
                self.put(ENTRIES + 20, first)
        m.reg_write(UC_X86_REG_EAX, result)
        m.reg_write(UC_X86_REG_ESP, sp + 4)
        m.reg_write(UC_X86_REG_EIP, self.word(sp))

    def run(self, case):
        self.case = case
        self.created = False
        self.events, self.buffers, self.sizes = [], [], []
        self.allocations = self.creates = self.destroyed = 0
        self.put(OWNER, OLD if case[3] else 0)
        self.put(LIST, ENTRIES)
        self.put(LIST + 4, 17)
        self.m.mem_write(HIT, packed(case[16:16 + 17 * 11]))
        self.m.mem_write(BURST, packed(case[16 + 17 * 11:16 + 22 * 11]))
        for i in range(17):
            self.put(ENTRIES + i * 12 + 8, 0 if case[12] & (1 << i) else MEMORIES + i * 64)
            at = 16 + 22 * 11 + i * 3
            self.put(MEMORIES + i * 64, case[at])
            self.put(MEMORIES + i * 64 + 36, case[at + 1])
            self.put(MEMORIES + i * 64 + 40, case[at + 2])
        self.m.mem_write(PARAMS, packed(case[8:10]))
        self.m.reg_write(UC_X86_REG_FPCW, case[1])
        self.m.reg_write(UC_X86_REG_FPSW, 0)
        result, boundary = 0, 0
        for _ in range(case[11]):
            mode = case[0]
            arguments = [] if mode == 0 else ([LIST, HIT, case[2]] if mode == 1 else
                         [case[6], PARAMS, BURST, CUSTOM if case[7] else 0])
            self.m.mem_write(STACK, packed([STOP, *arguments]))
            self.m.reg_write(UC_X86_REG_ESP, STACK)
            try:
                self.m.emu_start([ENTRY, BIND, POOL][mode], STOP, count=2000000)
            except uc.UcError:
                pc = self.m.reg_read(UC_X86_REG_EIP)
                if pc != 0x4cc2ab:
                    raise
                boundary = pc
                break
            if mode == 2:
                result = self.m.reg_read(UC_X86_REG_EAX)
        output = [boundary, self.word(OWNER), result, self.allocations, self.creates,
                  self.destroyed, len(self.events), *self.events]
        output.extend([self.word(TASK + at) if self.created else 0 for at in (16, 20, 24, 44)])
        awp = self.word(TASK + 44) if self.created else 0
        output.extend(words(bytes(self.m.mem_read(awp, 32))) if awp else [0] * 8)
        for i in range(1, 4):
            address = self.buffers[i] if i < len(self.buffers) else 0
            size = self.sizes[i] if address else 0
            output.append(size)
            output.extend(bytes(self.m.mem_read(address, size)) if size else [])
        output.extend(words(bytes(self.m.mem_read(HIT, 17 * 44))))
        output.extend(words(bytes(self.m.mem_read(BURST, 5 * 44))))
        output.append(self.m.reg_read(UC_X86_REG_FPSW) & 63)
        return output


def base_case(image, rng):
    frames = words(image.read(HIT, 17 * 44) + image.read(BURST, 5 * 44))
    memories = []
    for i in range(17):
        memories.extend([0x34567000 + i * 13, 31 + i, 59 + 2 * i])
    return [0, 0x7f, 0, 0, 0, 0, 40, 0, 0x3e800000, 0xbf000000, 0, 1, 0, 0,
            0x3f400000, 0x3f200000, *frames, *memories]


def cases(image, seed, count, holdout=False):
    rng = random.Random(seed)
    if not holdout:
        for mode, fail, reentry, custom in itertools.product((0, 2), range(6), range(4), range(2)):
            case = base_case(image, rng)
            case[0], case[10], case[7] = mode, reentry, custom
            case[4] = int(fail == 5)
            case[5] = fail if fail < 5 else 0
            yield case
        for owner, null, repeat in itertools.product(range(2), (0, 1, MASK), (1, 2, 3)):
            case = base_case(image, rng)
            case[3], case[12], case[11] = owner, null, repeat
            yield case
        for capacity in (0, -1, -2, -2147483648, 1, 7, 65536):
            case = base_case(image, rng)
            case[0], case[6] = 2, capacity & MASK
            yield case
    for _ in range(count):
        case = base_case(image, rng)
        case[0] = rng.randrange(3)
        case[1] = rng.choice((0x7f, 0x47f, 0x87f, 0xc7f, 0x27f, 0x37f))
        case[2] = rng.getrandbits(32)
        case[3] = rng.randrange(2)
        case[4] = int(rng.randrange(9) == 0)
        case[5] = rng.randrange(5)
        case[6] = rng.choice((1, 2, 7, 40, 67, 0, -1, -2, 65536)) & MASK
        case[7] = rng.randrange(2)
        case[8:10] = [rng.getrandbits(32), rng.getrandbits(32)]
        case[10] = rng.randrange(4)
        case[12] = rng.getrandbits(17)
        case[13] = rng.randrange(2)
        case[14:16] = [rng.getrandbits(32), rng.getrandbits(32)]
        # Complete distinct frame payloads, signed sentinels and per-record flags.
        for table, length in ((0, 17), (17, 5)):
            sentinel = rng.randrange(length)
            for i in range(length):
                at = 16 + (table + i) * 11
                case[at:at + 11] = [rng.getrandbits(32) for _ in range(11)]
                index = rng.randrange(17) if i < sentinel else 0xffff
                case[at] = (case[at] & 0xffff0000) | index
        for i in range(17):
            at = 16 + 22 * 11 + i * 3
            case[at:at + 3] = [rng.getrandbits(32) for _ in range(3)]
        yield case


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--probe', default='build/native-gamma-effects-probe.dylib')
    parser.add_argument('--output', default='build/oracles/gamma_effects.json')
    parser.add_argument('--seed', type=int, default=499910)
    parser.add_argument('--count', type=int, default=1024)
    parser.add_argument('--holdout', action='store_true')
    args = parser.parse_args()
    image = PEImage(args.exe)
    source = Source(image)
    probe = ctypes.CDLL(str(Path(args.probe).resolve())).gamma_effects_case
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    probe.restype = ctypes.c_int
    failures, partials = [], []
    stream = hashlib.sha256()
    for index, case in enumerate(cases(image, args.seed, args.count, args.holdout)):
        original = source.run(case)
        output = (ctypes.c_uint32 * 4096)()
        size = probe((ctypes.c_uint32 * len(case))(*case), output)
        native = list(output)[:max(0, size)]
        stream.update(packed(original))
        if size != len(original) or original[:-1] != native[:-1]:
            failures.append({'case_id': index, 'input': case, 'original': original,
                             'native': native, 'native_size': size})
        elif original[-1] != native[-1]:
            partials.append({'case_id': index, 'original_flags': original[-1],
                             'native_flags': native[-1]})
    report = {'oracle': 'gamma_effects', 'cases': index + 1,
              'passed': not failures and not partials, 'value_failures': len(failures),
              'fp_partials': partials, 'mismatches': failures,
              'seed': args.seed, 'holdout': args.holdout,
              'stream_sha256': stream.hexdigest(), 'source_ranges': source.ranges,
              'probe_sha256': hashlib.sha256(Path(args.probe).read_bytes()).hexdigest(),
              'limits': ['Four lower APIs intercepted; actual arena/scheduler/caller tested separately.',
                         'Complete frame/work/allocated-buffer data compared; no particle callbacks run.',
                         'Host allocation exceptions/arbitrary source memory aliases not qualified.']}
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('cases', 'passed', 'value_failures')}),
          'FP partials', len(partials))
    return 1 if failures else 2 if partials else 0


if __name__ == '__main__':
    raise SystemExit(main())
