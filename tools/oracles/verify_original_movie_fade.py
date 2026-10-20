"""Compare all six movie fade states against the original integer instructions."""
import argparse, ctypes, hashlib, json, pathlib, random
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from verify_original_movie import Source
from pe_image import PEImage
from verify_original_math import STACK, STOP

class FadeSource(Source):

    def hook(self, m, address, size, user):
        if address not in [0x512d30, 0x10000200]:
            return
        sp = m.reg_read(UC_X86_REG_ESP)
        if address == 0x512d30:
            self.draw.append(self.read(sp + 4) & 255)
        m.reg_write(UC_X86_REG_EAX, self.now if address == 0x10000200 else 0)
        m.reg_write(UC_X86_REG_EIP, self.read(sp))
        m.reg_write(UC_X86_REG_ESP, sp + 4)

    def run(self, v):
        self.draw = []
        self.now = v[10]
        for a, x in zip([0x3c60000, 0x3c5fff0, 0x3c6001c, 0x10db8b4, 0x10db8b8, 0x3c60008, 0x3c6002c, 0x3c5fff4], v[:8]):
            self.put(a, x)
        self.put(0x3c5ffec, 0)
        self.put(0x3c5fff8, v[9])
        self.put(0x7db270, 0x10000200)
        self.put(STACK, STOP)
        self.put(STACK + 4, v[8])
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.emu_start(0x512df0, STOP, count=1000)
        return [self.read(a) for a in [0x3c60000, 0x3c60008, 0x3c6002c, 0x3c6001c, 0x3c5ffe8]] + [self.m.reg_read(UC_X86_REG_EAX), len(self.draw), self.draw[-1] if self.draw else 0]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe', type=pathlib.Path)
    p.add_argument('native_library', type=pathlib.Path)
    p.add_argument('--output', type=pathlib.Path, required=True)
    a = p.parse_args()
    image = PEImage(a.exe)
    s = FadeSource(image)
    n = ctypes.CDLL(str(a.native_library.resolve()))
    bad = []
    rng = random.Random(20261020)
    cases = [[phase, frame, 100, mode, 130, 77, 88, req, movie, 100, 200] for phase in range(6) for frame in [-2147483648, -100, 0, 90, 99, 100, 114, 115, 129, 130, 131, 160, 2147483647] for mode in [0, 1, 2] for req in [0, 1] for movie in [0, 7]]
    cases += [[rng.randrange(6), rng.getrandbits(32), rng.getrandbits(32), rng.randrange(3), rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32), rng.randrange(2), rng.randrange(9), rng.getrandbits(32), rng.getrandbits(32)] for _ in range(2000)]
    for v in cases:
        o = (ctypes.c_uint32 * 8)()
        n.sadx_native_movie_fade((ctypes.c_uint32 * 11)(*v), o)
        e = s.run(v)
        if list(o) != e:
            bad.append({'input': v, 'native': list(o), 'original': e})
    d = {'cases': len(cases), 'mismatches': len(bad), 'first_mismatches': bad[:5]}
    d.update(passed=not bad, source_sha256=image.sha256, native_library_sha256=hashlib.sha256(a.native_library.read_bytes()).hexdigest(), source_ranges={'0x512df0-0x5130a0': hashlib.sha256(image.read(0x512df0, 688)).hexdigest()})
    root = pathlib.Path(__file__).resolve().parents[2]
    d['native_sources'] = {f: hashlib.sha256((root / f).read_bytes()).hexdigest() for f in ['src/game/movie.hpp', 'src/game/movie.cpp', 'tests/probes/native_movie_probe.cpp']}
    a.output.write_text(json.dumps(d, indent=2))
    print({k: d[k] for k in ['cases', 'mismatches']})
    return bool(bad)
if __name__ == '__main__':
    raise SystemExit(main())
