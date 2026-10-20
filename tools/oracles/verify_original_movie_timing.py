"""Execute the real movie loop timing and MSVC conversions with sampled microsecond fixtures.

Only timer sampling replaces QPC; subtraction and identity conversion are exact
native services. This compares stores and low-six FP flags separately.
Unicorn x87 flags differ from the software arithmetic context; exact hardware
exception/status and deferred delivery parity are not established.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW
from verify_original_movie import Source
from pe_image import PEImage


class TimedSource(Source):
    def hook(self, m, address, size, user):
        if address == 0x644340:
            return  # Execute the actual MSVC conversion.
        if address not in [0x7800e0, 0x780120, 0x780130]:
            return super().hook(m, address, size, user)
        stack = m.reg_read(UC_X86_REG_ESP)
        if address == 0x7800e0:
            self.timer_now = (self.timer_now + self.timer_step) & 0xffffffff
            result = self.timer_now
        elif address == 0x780120:
            result = (self.read(stack + 8) - self.read(stack + 4)) & 0xffffffff
        else:
            result = self.read(stack + 4)
        m.reg_write(UC_X86_REG_EAX, result)
        m.reg_write(UC_X86_REG_EIP, self.read(stack))
        m.reg_write(UC_X86_REG_ESP, stack + 4)

    def timing(self, values):
        self.timer_now, self.timer_step = values[1:]
        self.m.reg_write(UC_X86_REG_FPCW, values[0])
        self.m.reg_write(UC_X86_REG_FPSW, 0)
        self.run([0, 0, 0, 0, 5, 0, 0, -7400])
        return [self.read(a) for a in [0x3c60014, 0x3c6000c, 0x3c60004, 0x3c60018, 0x3c60020]] + [self.m.reg_read(UC_X86_REG_FPSW) & 63]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('native_library', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    original = TimedSource(image)
    native = ctypes.CDLL(str(args.native_library.resolve()))
    cases = [[cw, timer, step] for cw in [127, 1151, 2175, 3199, 639, 1663, 2687, 3711, 895, 1919, 2943, 3967]
             for timer in [0, 1234, 4294967290] for step in [0, 1, 16666, 16667, 1000000, 2147483647, 2147483648, 4294967295]]
    rng = random.Random(20261020)
    cases += [[127, rng.getrandbits(32), rng.getrandbits(32)] for _ in range(400)]
    bad, flags = [], 0
    for values in cases:
        output = (ctypes.c_uint32 * 6)()
        native.sadx_native_movie_timing((ctypes.c_uint32 * 3)(*values), output)
        expected = original.timing(values)
        if list(output)[:5] != expected[:5]:
            bad.append({'input': values, 'native': list(output), 'original': expected})
        flags += int(output[5] != expected[5])
    root = pathlib.Path(__file__).resolve().parents[2]
    report = {'cases': len(cases), 'mismatches': len(bad), 'first_mismatches': bad[:3],
              'values_match': not bad, 'fp_flag_mismatches': flags, 'exception_flags_match': flags == 0,
              'passed': not bad and flags == 0, 'source_sha256': image.sha256,
              'native_library_sha256': hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
              'source_ranges': {'0x5130a0-0x5133b7': hashlib.sha256(image.read(0x5130a0, 791)).hexdigest()},
              'native_sources': {f: hashlib.sha256((root/f).read_bytes()).hexdigest()
                                 for f in ['src/game/movie.hpp', 'src/game/movie.cpp', 'tests/probes/native_movie_probe.cpp']},
              'limits': __doc__}
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ['cases', 'mismatches', 'fp_flag_mismatches']}))
    return int(bool(bad))


if __name__ == '__main__':
    raise SystemExit(main())
