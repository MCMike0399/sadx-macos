"""Compare DrawFadePoly's full point/color strip and ordered blend/draw calls.

No downstream renderer is replaced by guessed geometry. The boundary is the
actual __njDrawPolygon2D call; low-six x87 flags are reported independently.
Unicorn x87 flags differ from the software arithmetic context; exact hardware
exception/status and deferred delivery parity are not established.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import struct
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW
from verify_original_movie import Source
from pe_image import PEImage
from verify_original_math import STACK, STOP


class Geometry(Source):
    def hook(self, m, address, size, user):
        if address not in [0x4030d0, 0x403c60]:
            return
        stack = m.reg_read(UC_X86_REG_ESP)
        if address == 0x4030d0:
            self.blend += [self.read(stack + 4), self.read(stack + 8)]
        else:
            record = self.read(stack + 4)
            points, colors = self.read(record), self.read(record + 4)
            self.output = list(struct.unpack('<8I', m.mem_read(points, 32)))
            self.output += list(struct.unpack('<4I', m.mem_read(colors, 16))) + self.blend
            self.output += [self.read(stack + 12), self.read(stack + 8), self.read(stack + 16)]
        m.reg_write(UC_X86_REG_EIP, self.read(stack))
        m.reg_write(UC_X86_REG_ESP, stack + 4)

    def run(self, values):
        self.blend, self.output = [], []
        self.put(0x8928c0, values[1])
        self.put(0x8928c4, values[2])
        self.put(STACK, STOP)
        self.put(STACK + 4, values[0])
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.reg_write(UC_X86_REG_FPCW, values[3])
        self.m.reg_write(UC_X86_REG_FPSW, 0)
        self.m.emu_start(0x512d30, STOP, count=1000)
        return self.output + [self.m.reg_read(UC_X86_REG_FPSW) & 63]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('native_library', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    native = ctypes.CDLL(str(args.native_library.resolve()))
    source = Geometry(image)
    bad, flags, count = [], 0, 0
    bits = lambda x: struct.unpack('<I', struct.pack('<f', x))[0]
    for cw in [127, 1151, 2175, 3199, 639, 1663, 2687, 3711, 895, 1919, 2943, 3967]:
        for alpha in [0, 1, 63, 127, 191, 192, 254, 255]:
            for sx, sy in [(0, 0), (.5, 1), (1, 1), (2, 2), (4/3, 1), (-1, 1), (1, -1), (1e-30, 1e30)]:
                values = [alpha, bits(sx), bits(sy), cw]
                output = (ctypes.c_uint32 * 20)()
                native.sadx_native_movie_fade_polygon((ctypes.c_uint32 * 4)(*values), output)
                expected = source.run(values)
                count += 1
                if list(output)[:19] != expected[:19]:
                    bad.append({'input': values, 'native': list(output), 'original': expected})
                flags += int(output[19] != expected[19])
    root = pathlib.Path(__file__).resolve().parents[2]
    report = {'cases': count, 'mismatches': len(bad), 'first_mismatches': bad[:3],
              'values_match': not bad, 'fp_flag_mismatches': flags, 'exception_flags_match': flags == 0,
              'passed': not bad and flags == 0, 'source_sha256': image.sha256,
              'native_library_sha256': hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
              'source_ranges': {'0x512d30-0x512dea': hashlib.sha256(image.read(0x512d30, 186)).hexdigest()},
              'native_sources': {f: hashlib.sha256((root/f).read_bytes()).hexdigest()
                                 for f in ['src/game/movie.hpp', 'src/game/movie.cpp', 'tests/probes/native_movie_probe.cpp']},
              'limits': __doc__}
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ['cases', 'mismatches', 'fp_flag_mismatches']}))
    return int(bool(bad))


if __name__ == '__main__':
    raise SystemExit(main())
