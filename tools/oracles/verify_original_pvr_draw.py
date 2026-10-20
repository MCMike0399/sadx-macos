"""Differential source PVR quad and immediate box producers under the pinned GVR patch.

Actual texture metadata and scratch allocation are fixture inputs. Submission stops
at the downstream Ninja draw boundaries; this does not certify device pixels.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP, bits


class Source:
    def __init__(self, image):
        self.m = Original(image, 127).machine
        for p in [0x3b2c000, 0x3b11000, 0x3d0f000, 0x3d12000, 0x909000, 0x3abd000]:
            self.m.mem_map(p, 4096)
        self.m.mem_write(0x433c56, b'\x00')
        self.defaults_active = False
        self.m.hook_add(unicorn.UC_HOOK_CODE, self.hook)

    def word(self, a):
        return struct.unpack('<I', self.m.mem_read(a, 4))[0]

    def put(self, a, v):
        self.m.mem_write(a, struct.pack('<I', v & 0xffffffff))

    def ret(self):
        s = self.m.reg_read(UC_X86_REG_ESP)
        self.m.reg_write(UC_X86_REG_EIP, self.word(s))
        self.m.reg_write(UC_X86_REG_ESP, s + 4)

    def hook(self, m, a, size, user):
        s = m.reg_read(UC_X86_REG_ESP)
        if self.defaults_active and a in [0x10000300, 0x10000320]:
            stage = a == 0x10000320
            if stage and self.word(s + 8) != 0:
                raise RuntimeError('Unexpected default texture stage')
            offset = 12 if stage else 8
            (self.stages if stage else self.render)[self.word(s + offset)] = self.word(s + offset + 4)
            m.reg_write(UC_X86_REG_EIP, self.word(s))
            m.reg_write(UC_X86_REG_ESP, s + (20 if stage else 16))
            return
        if a == 0x403d30:
            self.heap += 128
            m.reg_write(UC_X86_REG_EAX, self.heap)
            self.ret()
        elif a == 0x4073b0:
            self.out[12] += 1
            self.ret()
        elif a == 0x4010d0:
            block = self.word(s + 4)
            points = self.word(block)
            self.out[:8] = struct.unpack('<8I', m.mem_read(points, 32))
            self.out[8] = self.word(self.word(block + 4))
            self.out[9] = self.word(s + 12)
            self.out[10] = self.word(s + 16)
            self.out[11] = 1
            self.ret()
        elif a == 0x403130:
            if self.word(s + 8) != 4:
                raise RuntimeError('Expected four source vertices')
            self.out[24:27] = [1, self.word(s + 12), self.word(s + 16)]
            self.ret()

    def run(self, v, box=False):
        self.out, self.heap = [0] * 32, DATA + 0x4000
        self.put(STACK, STOP)
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.reg_write(UC_X86_REG_FPCW, v[9])
        self.m.reg_write(UC_X86_REG_FPSW, 0)
        self.m.mem_write(0x3b2c5f0, bytes(96))
        self.m.emu_start(0x432ea0, STOP, timeout=1000000)
        if box:
            self.put(0x3b1117c, v[6])
            self.m.mem_write(0x909eb4, bytes([v[7]]))
            self.m.mem_write(0x3abdcfe, bytes([v[8]]))
            self.put(0x3abd9c0, 0)
            args, entry = v[:6], 0x4071c0
        else:
            self.put(0x3b1117c, v[8])
            self.put(0x3d0fa24, DATA + 0x1000)
            self.put(DATA + 0x1000, DATA + 0x2000)
            self.put(DATA + 0x2008 + (v[0] & 0x30ffffff) * 12, DATA + 0x3000)
            for off, value in [(0, v[6]), (32, v[7]), (36, v[4]), (40, v[5])]:
                self.put(DATA + 0x3000 + off, value)
            for i in range(4):
                self.put(0x3b2c604 + i * 24, v[10 + i])
            args, entry = v[:4], 0x4338d0
            if not v[8]:
                self.out[28] = v[0] & 0x30ffffff
        self.put(STACK, STOP)
        for i, value in enumerate(args):
            self.put(STACK + 4 + i * 4, value)
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.reg_write(UC_X86_REG_FPSW, 0)
        self.m.emu_start(entry, STOP, timeout=1000000)
        if box:
            self.out[13] = self.m.mem_read(0x909eb4, 1)[0]
            self.out[14] = self.word(0x3abd9c0)
        else:
            self.out[:24] = struct.unpack('<24I', self.m.mem_read(0x3b2c5f0, 96))
        self.out[30] = self.m.reg_read(UC_X86_REG_FPCW)
        self.out[31] = self.m.reg_read(UC_X86_REG_FPSW) & 63
        return self.out

    def colors(self, v):
        self.put(0x3b2c658, v[2])
        for entry, value in [(0x432f40, v[0]), (0x433010, v[1])]:
            self.put(STACK, STOP)
            self.put(STACK + 4, value)
            self.m.reg_write(UC_X86_REG_ESP, STACK)
            self.m.emu_start(entry, STOP, timeout=1000000)
        return [self.word(0x3b2c604 + i * 24) for i in range(4)] + [
            self.word(0x3b2c658), self.word(0x3b2c650)]

    def defaults(self):
        self.defaults_active = True
        self.render, self.stages = {}, {}
        self.put(0x3d128b0, DATA + 0x9000)
        self.put(DATA + 0x9000, DATA + 0xa000)
        self.put(DATA + 0xa0c8, 0x10000300)
        self.put(DATA + 0xa0fc, 0x10000320)
        for entry in [0x791860, 0x7943f0]:
            self.put(STACK, STOP)
            self.m.reg_write(UC_X86_REG_ESP, STACK)
            self.m.emu_start(entry, STOP, timeout=1000000)
        self.defaults_active = False
        return [word for state in [self.render, self.stages] for pair in sorted(state.items()) for word in pair]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe', type=pathlib.Path)
    p.add_argument('native_library', type=pathlib.Path)
    p.add_argument('--output', type=pathlib.Path, required=True)
    p.add_argument('--seed', type=int, default=20261020)
    p.add_argument('--cases', type=int, default=100)
    a = p.parse_args()
    if hashlib.sha256(a.exe.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise RuntimeError('Unexpected source executable')
    image = PEImage(a.exe)
    source = Source(image)
    lib = ctypes.CDLL(str(a.native_library.resolve()))
    rng = random.Random(a.seed)
    cases = []
    for cw in [127, 639, 895, 1151, 1663, 1919, 2175, 2687, 2943, 3199, 3711, 3967]:
        for anchor in range(9):
            for flip in range(4):
                cases.append((False, [(anchor << 24) | (flip << 30), bits(-64.25), bits(12.5), bits(1.1), 257, 255, 123, 0, 0, cw] + [rng.getrandbits(32) for _ in range(4)]))
            for width, height in [(0x80000001, 255), (257, 0x80000001),
                                  (0x7fffffff, 0xffffffff), (0xffffffff, 0x80000081)]:
                cases.append((False, [anchor << 24, bits(128), bits(128), bits(1.1),
                                      width, height, 123, 0, 0, cw] +
                              [rng.getrandbits(32) for _ in range(4)]))
        for _ in range(a.cases):
            cases.append((False, [(rng.randrange(9) << 24) | (rng.randrange(4) << 30), bits(rng.uniform(-10000, 10000)), bits(rng.uniform(-10000, 10000)), bits(rng.uniform(-100, 100)), rng.randrange(1, 2049), rng.randrange(1, 2049), rng.getrandbits(32), rng.getrandbits(32), rng.randrange(8) == 0, cw] + [rng.getrandbits(32) for _ in range(4)]))
        for fog in range(4):
            for disabled in [0, 1]:
                for enabled in [0, 1]:
                    for priority in [-11., -1., 0., 2.]:
                        cases.append((True, [bits(-1), bits(2), bits(640), bits(480), bits(priority), rng.getrandbits(32), disabled, fog, enabled, cw]))
    bad, flags = [], 0
    for index, (box, values) in enumerate(cases):
        out = (ctypes.c_uint32 * 32)()
        native = lib.sadx_native_pvr_box if box else lib.sadx_native_pvr_draw
        native((ctypes.c_uint32 * len(values))(*values), out)
        expected = source.run(values, box)
        flags += out[31] != expected[31]
        if list(out)[:31] != expected[:31]:
            bad.append({'case': index, 'box': box, 'input': values, 'native': list(out), 'original': expected})
    color_cases = 1024 + a.cases
    for index in range(color_cases):
        values = [rng.getrandbits(32) for _ in range(3)]
        if index < 4:
            values = [[0, 0, 0], [0xffffffff, 0xffffffff, 0xffffffff],
                      [0xff804020, 0x80808080, 0xffffffff], [0xffffffff, 0x7fffffff, 0xffffffff]][index]
        out = (ctypes.c_uint32 * 6)()
        lib.sadx_native_pvr_colors((ctypes.c_uint32 * 3)(*values), out)
        expected = source.colors(values)
        if list(out) != expected:
            bad.append({'color_case': index, 'input': values, 'native': list(out), 'original': expected})
    defaults = (ctypes.c_uint32 * 40)()
    lib.sadx_native_pvr_defaults(defaults)
    expected_defaults = source.defaults()
    if list(defaults) != expected_defaults:
        bad.append({'device_defaults': True, 'native': list(defaults), 'original': expected_defaults})
    root = pathlib.Path(__file__).resolve().parents[2]
    files = ['src/engine/pvr_draw.hpp', 'src/engine/pvr_draw.cpp', 'src/engine/pvr_device.hpp', 'src/engine/pvr_device.cpp', 'tests/probes/native_pvr_draw_probe.cpp']
    report = {'cases': len(cases) + color_cases + 1, 'seed': a.seed, 'mismatches': len(bad), 'first_mismatches': bad[:3], 'flag_mismatches': flags, 'values_match': not bad, 'passed': not bad and not flags,
              'native_sources': {f: hashlib.sha256((root / f).read_bytes()).hexdigest() for f in files},
              'native_library_sha256': hashlib.sha256(a.native_library.read_bytes()).hexdigest(),
              'source_ranges': {f'{hex(start)}-{hex(start + size)}': hashlib.sha256(image.read(start, size)).hexdigest() for start, size in [(0x4338d0, 0x3bb), (0x4071c0, 0x125), (0x432ea0, 0xa0), (0x432df0, 0xac), (0x432f40, 0x43), (0x433010, 0x4d), (0x791860, 0xd3), (0x7943f0, 0xb5)]},
              'limits': ['Pinned loader GVR alpha-mask patch applied at 0x433c56.', 'Valid anchors only; invalid anchors read uninitialized source stack.', 'Texture metadata and scratch allocation are supplied fixtures; downstream Ninja draw calls are observed, device pixels are not certified.', 'Low-six FP exception flags compared separately.']}
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ['cases', 'mismatches', 'flag_mismatches']}))
    return int(bool(bad))


if __name__ == '__main__':
    raise SystemExit(main())
