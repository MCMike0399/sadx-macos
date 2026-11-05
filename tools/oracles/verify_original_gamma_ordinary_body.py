"""Original ordinary display action selection and postbody prefix vs game tier.

Executes source 4800b4 through 480135, stopping before the arm-frame/asset read.
The first body renderer and numeric matrix operations are common trace leaves;
DrawAction/traversal and matrix arithmetic have separate source comparisons.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
from verify_original_gamma_object_callback import Source as CallbackSource, BASE, STACK, STOP, packed, bits
from unicorn.x86_const import *

PLAYER, EXTRA, TABLE, EXPORTS = BASE + 0x100, BASE + 0x300, BASE + 0x600, BASE + 0xe00


def fixtures(count, seed):
    rng = random.Random(seed)
    specials = [0, 0x80000000, 1, 0x80000001, 0x7f800000, 0xff800000,
                0x7fc12345, 0x7f812345, 0x7f7fffff, 0xff7fffff]
    angles = [0, 1, 65535, 65536, 0xffff0000, 0x80000000, 0xffffffff]
    for i in range(count):
        yield [rng.randrange(91), [0, 1, 2, 3, 0xffff, 0x8000][i % 6],
               specials[(i // 6) % len(specials)] if i < 180 else bits(rng.uniform(0, 100)),
               BASE + 0x10000, BASE + 0x20000,
               specials[(i // 6) % len(specials)] if i < 180 else bits(rng.uniform(-1e20, 1e20)),
               angles[(i // 6) % len(angles)] if i < 180 else rng.getrandbits(32)]


class Source(CallbackSource):
    def event(self, tag, a=0, b=0, c=0):
        self.trace.extend([tag, a, b, c, 0])

    def instruction(self, m, address, size, user):
        if address == 0x4800b4:
            self.event(10, self.case[2])
        elif address == 0x4800ba:
            self.event(11, self.case[1])
        elif address == 0x407020:
            stack = m.reg_read(UC_X86_REG_ESP)
            action, frame = self.read(stack + 4, 2)
            self.event(13 if self.case[1] == 2 else 12, action, frame)
            self.ret(0)
        elif address == 0x7802d0:
            assert m.reg_read(UC_X86_REG_ECX) == 0
            self.event(14)
            self.ret(0)
        elif address == 0x7850f0:
            assert m.reg_read(UC_X86_REG_ECX) == 0
            self.event(15)
            self.ret(0)
        elif address == 0x788ad0:
            assert m.reg_read(UC_X86_REG_ECX) == 0
            assert m.reg_read(UC_X86_REG_EDX) == 0x3c53ad8
            self.event(16, 1)
            self.ret(0)
        elif address == 0x480121:
            self.event(17, self.case[6])
        elif address == 0x480135:
            self.event(18)
            m.emu_stop()
        else:
            super().instruction(m, address, size, user)

    def run(self, case):
        self.case = case
        m = self.m
        self.trace = []
        m.mem_write(BASE, bytes(4096))
        for address, words in ((0x3c53b70, [EXTRA]), (0x3ab916c, [EXPORTS]),
                               (PLAYER + 0x120, [case[1]]),
                               (PLAYER + 0x130, [case[2]]),
                               (PLAYER + 0x140, [TABLE]),
                               (PLAYER + 0x148, [case[4]]),
                               (EXTRA + 140, [case[6], case[5]]),
                               (STACK, [STOP])):
            m.mem_write(address, packed(words))
        for i in range(91):
            m.mem_write(TABLE + i * 16, packed([case[3] + i * 64]))
        m.reg_write(UC_X86_REG_EBX, PLAYER)
        m.reg_write(UC_X86_REG_ESI, EXTRA)
        m.reg_write(UC_X86_REG_EDI, case[0])
        m.reg_write(UC_X86_REG_ECX, 0x47f990)
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.reg_write(UC_X86_REG_FPCW, 127)
        m.reg_write(UC_X86_REG_FPSW, 0)
        m.emu_start(0x4800b4, STOP, count=1000)
        assert m.reg_read(UC_X86_REG_EIP) == 0x480135
        assert m.reg_read(UC_X86_REG_ESP) == STACK
        return [m.reg_read(UC_X86_REG_FPSW) & 0x3f, len(self.trace), *self.trace]


def main():
    from pe_image import PEImage
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--cases', type=int, default=1024)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x4800b4)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    source = Source(image)
    library = ctypes.CDLL(str(args.library.resolve()))
    native = library.gamma_ordinary_body_probe
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    native.restype = ctypes.c_uint32
    failures, partials = [], []
    digest = hashlib.sha256()
    for index, case in enumerate(fixtures(args.cases, args.seed)):
        expected = source.run(case)
        output = (ctypes.c_uint32 * 128)()
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
                  source_ranges={'0x4800b4-0x480135': hashlib.sha256(image.read(0x4800b4, 129)).hexdigest()},
                  scope='ordinary source action selection through arm-frame boundary; body draw and numeric matrix leaves common receipts',
                  status='fail' if failures else 'partial' if partials else 'pass')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return 1 if failures else 2 if partials else 0


if __name__ == '__main__':
    raise SystemExit(main())
