"""Original E102ObjectCallback vs native row/aim dispatch and recoil arithmetic.

Source instructions execute at 47f990. Matrix rotation and translation are
common numeric seams: ordered arguments, row dispatch and x87 recoil doubling
are compared. Matrix results, mesh submission and pixels are outside this test.
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
TABLE, EXTRA = BASE + 0x100, BASE + 0x200
SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'


def packed(words):
    return struct.pack('<' + 'I' * len(words), *words)


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def fixtures(count, seed):
    rng = random.Random(seed)
    special = [0, 0x80000000, 1, 0x80000001, 0x7f800000, 0xff800000,
               0x7fc12345, 0x7f812345, 0x7f7fffff, 0xff7fffff]
    angles = [0, 1, 65535, 65536, 0xffff0000, 0x80000000, 0xffffffff]
    for i in range(count):
        rows = [BASE + 0x400 + j * 64 for j in range(3)]
        if (i // 5) % 5 == 0:
            rows[2] = rows[1]  # row12 wins when the two tested rows alias.
        elif (i // 5) % 7 == 0:
            rows[0] = rows[1]
        node = (rows + [0, BASE + 0x800])[i % 5]
        angle = angles[(i // 5) % len(angles)] if i < 140 else rng.getrandbits(32)
        recoil = special[(i // 5) % len(special)] if i < 140 else bits(rng.uniform(-1e20, 1e20))
        yield [node, *rows, angle, recoil]


class Source:
    def __init__(self, image):
        assert image.sha256 == SHA and uc.__version__ == '2.1.4'
        self.m = m = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        for section in image.sections:
            if section['name'] not in ('.text', '.rdata', '.data'):
                continue
            address = image.base + section['rva']
            size = (max(section['raw_size'], section['virtual_size']) + 4095) & ~4095
            m.mem_map(address, size)
            m.mem_write(address, image.slice(section['file_offset'], section['raw_size']))
        for page in (BASE, STACK & ~4095, STOP):
            m.mem_map(page, 4096)
        m.hook_add(uc.UC_HOOK_CODE, self.instruction)

    def read(self, address, count=1):
        return list(struct.unpack('<' + 'I' * count, self.m.mem_read(address, count * 4)))

    def ret(self, extra):
        stack = self.m.reg_read(UC_X86_REG_ESP)
        target = self.read(stack)[0]
        self.m.reg_write(UC_X86_REG_ESP, stack + 4 + extra)
        self.m.reg_write(UC_X86_REG_EIP, target)

    def instruction(self, m, address, size, user):
        if address == 0x784e20:
            assert m.reg_read(UC_X86_REG_ECX) == 0
            self.trace.extend([1, m.reg_read(UC_X86_REG_EDX), 0, 0, 0])
            self.ret(0)
        elif address == 0x784be0:
            assert m.reg_read(UC_X86_REG_ECX) == 0
            stack = m.reg_read(UC_X86_REG_ESP)
            self.trace.extend([2, *self.read(stack + 4, 3), 0])
            self.ret(12)

    def run(self, case):
        m = self.m
        self.trace = []
        m.mem_write(BASE, bytes(4096))
        for address, words in ((0x3a8af6c, [TABLE]), (0x3c53b70, [EXTRA]),
                               (TABLE + 11 * 4, case[1:3]),
                               (TABLE + 28 * 4, [case[3]]),
                               (EXTRA + 140, case[4:6]),
                               (STACK, [STOP, case[0]])):
            m.mem_write(address, packed(words))
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.reg_write(UC_X86_REG_FPCW, 127)
        m.reg_write(UC_X86_REG_FPSW, 0)
        m.emu_start(0x47f990, STOP, count=1000)
        assert m.reg_read(UC_X86_REG_ESP) == STACK + 4
        return [m.reg_read(UC_X86_REG_FPSW) & 0x3f, len(self.trace), *self.trace]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--cases', type=int, default=1024)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x47f990)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    source = Source(image)
    library = ctypes.CDLL(str(args.library.resolve()))
    native = library.gamma_object_callback_probe
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    native.restype = ctypes.c_uint32
    failures, partials = [], []
    digest = hashlib.sha256()
    for index, case in enumerate(fixtures(args.cases, args.seed)):
        expected = source.run(case)
        output = (ctypes.c_uint32 * 64)()
        size = native((ctypes.c_uint32 * len(case))(*case), output)
        actual = list(output[:size])
        digest.update(packed(expected))
        if actual[1:] != expected[1:]:
            failures.append(dict(index=index, case=case, expected=expected, actual=actual))
        elif actual[0] != expected[0]:
            partials.append(dict(index=index, expected_flags=expected[0], actual_flags=actual[0]))
    report = dict(cases=args.cases, seed=args.seed, value_failures=len(failures),
                  fp_partials=len(partials), values_match=not failures,
                  flag_mismatches=len(partials), passed=not failures and not partials,
                  failures=failures[:10], partials=partials[:10],
                  observations_sha256=digest.hexdigest(),
                  source_ranges={'0x47f990-0x47f9da': hashlib.sha256(image.read(0x47f990, 74)).hexdigest()},
                  scope='original row/aim dispatch and recoil arithmetic; numeric matrix leaves common argument receipts',
                  status='fail' if failures else 'partial' if partials else 'pass')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return 1 if failures else 2 if partials else 0


if __name__ == '__main__':
    raise SystemExit(main())
