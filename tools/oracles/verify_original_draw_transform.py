"""Numeric XYZ/ZYX and scalar njTranslate comparison for animated draws.

Actual source numeric wrappers and Ninja leaves execute without substitutes.
Control word127 is the measured post-device nearest/single profile; this focused
judge does not claim startup, other control words or hardware exception delivery.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys
sys.path[:0] = [str(Path(__file__).resolve().parents[1] / 'lib')]
from pe_image import PEImage
from extract import SOURCE_SHA256, TABLE_ADDRESS, UNIT_MATRIX_ADDRESS
from verify_original_math import Original, DATA, STACK, STOP
import unicorn as uc
from unicorn.x86_const import *

MATRIX = DATA + 0x4000

def packed(words): return struct.pack('<' + 'I' * len(words), *words)
def bits(value): return struct.unpack('<I', struct.pack('<f', value))[0]


def fixtures(count, seed, unit):
    cases = []
    words = list(struct.unpack('<16I', unit))
    angles = (0, 1, 65536, -65536, 16384, 32768, 49152, 65535,
              -1, -2147483648, 2147483647)
    for mode in range(6):
        for a in angles:
            for axis in range(3):
                rotation = [0, 0, 0]; rotation[axis] = a & 0xffffffff
                cases.append([mode] + rotation + [bits(1), bits(-2), bits(3)] + words)
    rng = random.Random(seed)
    for i in range(count):
        mode = i % 6
        rotation = [rng.getrandbits(32) for _ in range(3)]
        position = [bits(rng.uniform(-1000, 1000)) for _ in range(3)]
        matrix = [bits(rng.uniform(-10, 10)) for _ in range(16)]
        cases.append([mode] + rotation + position + matrix)
    return cases


class Source(Original):
    def __init__(self, image):
        super().__init__(image, 127)
        self.machine.mem_map(0x3d0f000, 4096)
        self.machine.hook_add(uc.UC_HOOK_CODE, self.observe)
    def observe(self, machine, address, size, user):
        if address == 0x788290:
            self.trig_order.append(machine.reg_read(UC_X86_REG_ECX))
    def run(self, case):
        self.trig_order = []
        m = self.machine
        m.mem_write(MATRIX, packed(case[7:23]))
        m.mem_write(0x3d0f818, packed([MATRIX]))
        m.mem_write(0x3d0f7ac, packed([0]))
        address = (0x781770, 0x7819c0, 0x784be0)[case[0] % 3]
        if case[0] % 3 < 2:
            m.reg_write(UC_X86_REG_EDX, case[1])
            m.mem_write(STACK, packed([STOP, case[2], case[3]]))
            expected_stack = STACK + 12
        else:
            m.mem_write(STACK, packed([STOP] + case[4:7]))
            expected_stack = STACK + 16
        m.reg_write(UC_X86_REG_ECX, MATRIX if case[0] < 3 else 0)
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.reg_write(UC_X86_REG_FPCW, 127)
        m.reg_write(UC_X86_REG_FPSW, 0)
        m.reg_write(UC_X86_REG_FPTAG, 0xffff)
        m.emu_start(address, STOP, count=10000)
        assert m.reg_read(UC_X86_REG_ESP) == expected_stack
        return list(struct.unpack('<16I', m.mem_read(MATRIX, 64))) + [m.reg_read(UC_X86_REG_FPSW) & 63]


def main():
    p = argparse.ArgumentParser()
    p.add_argument('exe'); p.add_argument('probe')
    p.add_argument('--cases', type=int, default=1024)
    p.add_argument('--seed', type=lambda x: int(x, 0), default=0x7819c0)
    p.add_argument('--output', default='build/draw-path-2/draw-transform-dev.json')
    a = p.parse_args()
    image = PEImage(a.exe); assert image.sha256 == SOURCE_SHA256
    sine = image.read(TABLE_ADDRESS, 4100); unit = image.read(UNIT_MATRIX_ADDRESS, 64)
    sine_arg = (ctypes.c_uint8 * len(sine)).from_buffer_copy(sine)
    unit_arg = (ctypes.c_uint8 * len(unit)).from_buffer_copy(unit)
    lib = ctypes.CDLL(str(Path(a.probe).resolve())); fn = lib.draw_transform_probe
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8),
                   ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    fn.restype = ctypes.c_int
    original = Source(image)
    failures, partials, examples = 0, 0, []
    observation_case = [1, 0x1234, 0x2345, 0x3456, bits(1), bits(2), bits(3)] + list(struct.unpack('<16I', unit))
    original.run(observation_case)
    rotation_observation = {'source_entry': '0x7819c0',
                            'angles_xyz': observation_case[1:4],
                            'sin_entry': '0x788290',
                            'sin_angle_order': original.trig_order.copy()}
    cases = fixtures(a.cases, a.seed, unit)
    for index, case in enumerate(cases):
        expected = original.run(case); out = (ctypes.c_uint32 * 17)()
        status = fn(sine_arg, unit_arg, (ctypes.c_uint32 * 23)(*case), out)
        actual = list(out)
        mismatch = bool(status) or actual[:16] != expected[:16]
        failures += mismatch
        partial = not mismatch and actual[16] != expected[16]
        partials += partial
        if (mismatch or partial) and len(examples) < 8:
            examples.append({'case': index, 'mode': case[0], 'status': status,
                             'expected': expected, 'actual': actual})
    report = {'cases': len(cases), 'random_cases': a.cases, 'edge_cases': len(cases) - a.cases,
              'seed': a.seed, 'value_failures': failures, 'FP_partials': partials,
              'values_match': failures == 0, 'flag_mismatches': partials,
              'passed': failures == 0 and partials == 0, 'examples': examples,
              'source_ranges': {f'{start:#x}-{end:#x}': hashlib.sha256(image.read(start, end-start)).hexdigest()
                                for start, end in ((0x781770, 0x7819c0), (0x7819c0, 0x781c10),
                                                   (0x784be0, 0x784c70))},
              'original_exe_sha256': image.sha256,
              'probe_sha256': hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),
              'control_word': '0x007f',
              'zyx_source_observation': rotation_observation,
              'limits': ['Focused actual numeric original wrappers and their leaves; no call substitution.',
                         'Only nearest/single post-device control word127 compared.',
                         'Finite input matrix/vector words; actual source sine and unit bytes.',
                         'FP flag differences reported separately; no hardware exception delivery claim.']}
    target = Path(a.output); target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(report, indent=2) + '\n'); print(json.dumps(report, indent=2))
    return 1 if failures else 2 if partials else 0

if __name__ == '__main__': sys.exit(main())
