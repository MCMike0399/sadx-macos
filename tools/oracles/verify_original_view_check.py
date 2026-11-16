"""dsCheckViewV against the pinned sonic.exe.

Unicorn runs 0x403330. njCalcPoint (0x781670) is replaced with the fixture
view-space point (its matrix math is owned by the ninja matrix oracle); the
captured point argument is compared. Development and holdout cohorts use
separate seeds; the holdout seed is run once for the final report.
"""
import argparse
import ctypes
import hashlib
import json
import random
import struct
import sys
from pathlib import Path

import unicorn as uc
from unicorn.x86_const import *

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage

ENTRY, END = 0x403330, 0x403463
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
POINT, STACK, STOP = 0x22200000, 0x22301000, 0x22400000
PENDING, DRAW = 0x3b1117c, 0x90082c
SCREEN, RATIO_X, RATIO_Y = 0x3d0f9e0, 0x8928c0, 0x8928c4
MULTI_X, MULTI_Y = 0x892960, 0x892964
CALC_POINT = 0x781670

(PX, PY, PZ, RADIUS, LX, LY, LZ, PENDING_I, DMIN, DMAX, DIST, CX, CY,
 RX, RY, MX, MY, CONTROL) = range(18)
WORDS, OUT_WORDS = 18, 5
NAN = 0x7fc00000
INF = 0x7f800000


def packed(words):
    return struct.pack('<' + 'I' * len(words), *[word & 0xffffffff for word in words])


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


class Source:
    def __init__(self, image):
        assert image.sha256 == SOURCE_SHA and uc.__version__ == '2.1.4'
        machine = self.machine = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        for section in image.sections:
            if section['name'] not in ('.text', '.rdata', '.data'):
                continue
            address = image.base + section['rva']
            size = (max(section['raw_size'], section['virtual_size']) + 4095) & ~4095
            try:
                machine.mem_map(address, size, uc.UC_PROT_ALL)
            except uc.UcError:
                continue
            machine.mem_write(address, image.slice(section['file_offset'], section['raw_size']))
        for page, size in ((0x22200000, 0x1000), (0x22300000, 0x3000), (0x22400000, 0x1000),
                           (0x900000, 0x1000), (0x3b11000, 0x1000), (0x3d0f000, 0x2000),
                           (0x892000, 0x1000)):
            try:
                machine.mem_map(page, size, uc.UC_PROT_ALL)
            except uc.UcError:
                pass
        machine.hook_add(uc.UC_HOOK_CODE, self.instruction)
        machine.hook_add(uc.UC_HOOK_MEM_INVALID, self.invalid)
        self.image_bytes = image.read(ENTRY, END - ENTRY)

    def invalid(self, machine, access, address, size, value, user):
        self.fault = address
        return False

    def instruction(self, machine, address, size, user):
        if address != CALC_POINT:
            return
        case = self.case
        esp = machine.reg_read(UC_X86_REG_ESP)
        point = machine.reg_read(UC_X86_REG_EDX)
        self.captured = list(struct.unpack('<III', machine.mem_read(point, 12)))
        self.called = 1
        out = struct.unpack('<I', machine.mem_read(esp + 4, 4))[0]
        machine.mem_write(out, packed([case[LX], case[LY], case[LZ]]))
        ret = struct.unpack('<I', machine.mem_read(esp, 4))[0]
        machine.reg_write(UC_X86_REG_ESP, (esp + 8) & 0xffffffff)
        machine.reg_write(UC_X86_REG_EIP, ret)

    def run(self, case):
        self.case, self.fault = case, 0
        self.captured, self.called = [0, 0, 0], 0
        machine = self.machine
        machine.mem_write(POINT, packed([case[PX], case[PY], case[PZ]]))
        machine.mem_write(PENDING, packed([case[PENDING_I]]))
        machine.mem_write(DRAW, packed([case[DMIN], case[DMAX]]))
        machine.mem_write(SCREEN, packed([case[DIST], 0, 0, case[CX], case[CY]]))
        machine.mem_write(RATIO_X, packed([case[RX]]))
        machine.mem_write(RATIO_Y, packed([case[RY]]))
        machine.mem_write(MULTI_X, packed([case[MX]]))
        machine.mem_write(MULTI_Y, packed([case[MY]]))
        machine.mem_write(STACK, packed([STOP, POINT, case[RADIUS]]))
        for name in ('EAX', 'EBX', 'ECX', 'EDX', 'ESI', 'EDI', 'EBP'):
            machine.reg_write(getattr(uc.x86_const, 'UC_X86_REG_' + name), 0)
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.reg_write(UC_X86_REG_EIP, ENTRY)
        machine.reg_write(UC_X86_REG_FPCW, case[CONTROL] & 0xffff)
        machine.reg_write(UC_X86_REG_FPSW, 0)
        machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
        try:
            machine.emu_start(ENTRY, STOP, count=2000)
        except uc.UcError:
            if not self.fault:
                self.fault = machine.reg_read(UC_X86_REG_EIP)
        done = machine.reg_read(UC_X86_REG_EIP) == STOP
        visible = machine.reg_read(UC_X86_REG_EAX) & 0xffffffff if done else 0
        output = [visible, self.called, *self.captured]
        return output, self.fault, done


def blank(updates=None):
    case = [0] * WORDS
    case[CONTROL] = 0x007f
    case[PX], case[PY], case[PZ] = fbits(400.0), fbits(0.0), fbits(-20.0)
    case[RADIUS] = fbits(25.0)
    case[LX], case[LY], case[LZ] = fbits(0.0), fbits(0.0), fbits(-100.0)
    case[DMIN], case[DMAX] = fbits(-1.0), fbits(-2049.0)
    case[DIST], case[CX], case[CY] = fbits(320.0), fbits(320.0), fbits(240.0)
    case[RX] = case[RY] = case[MX] = fbits(1.0)
    case[MY] = fbits(0.85)
    for key, value in (updates or {}).items():
        case[key] = value & 0xffffffff
    return case


def fixtures(count, seed, curated=True):
    cases = [
        blank(),
        blank({PENDING_I: 1}),
        blank({LZ: fbits(-3000.0)}),
        blank({LZ: fbits(3000.0)}),
        blank({LZ: fbits(-24.0)}),
        blank({LX: fbits(400.0)}),
        blank({LX: fbits(-400.0)}),
        blank({LY: fbits(400.0)}),
        blank({LY: fbits(-400.0)}),
        blank({LZ: NAN}),
        blank({RADIUS: NAN}),
        blank({LX: NAN}),
        blank({DMIN: fbits(0.0), DMAX: fbits(0.0)}),
        blank({DIST: fbits(0.0)}),
        blank({CONTROL: 0x023f, LZ: fbits(-33.3), LX: fbits(12.5)}),
        blank({RADIUS: fbits(0.0), LZ: fbits(0.0)}),
        blank({LZ: INF}),
        blank({LZ: 0xff800000}),
        blank({LX: fbits(-152.0), LZ: fbits(100.0)}),
        blank({LX: fbits(152.0), LZ: fbits(100.0)}),
    ] if curated else []
    rng = random.Random(seed)
    floats = [0.0, -0.0, 1.0, -1.0, 25.0, 100.0, -100.0, 319.0, 320.0, 321.0, 640.0,
              -2049.0, 2049.0, 0.77, 1e30, -1e30]
    for _ in range(count):
        cases.append(blank({
            PX: fbits(rng.uniform(-400, 400)), PY: fbits(rng.uniform(-100, 100)),
            PZ: fbits(rng.uniform(-400, 400)),
            RADIUS: rng.choice([fbits(v) for v in (0.0, 1.0, 25.0, 100.0)] + [NAN, INF]),
            LX: rng.choice([fbits(v) for v in floats] + [NAN]),
            LY: rng.choice([fbits(v) for v in floats] + [NAN]),
            LZ: rng.choice([fbits(v) for v in floats] + [NAN, INF, 0xff800000]),
            PENDING_I: rng.choice([0, 0, 1]),
            DMIN: fbits(rng.choice([-1.0, 0.0, -100.0])),
            DMAX: fbits(rng.choice([-2049.0, 0.0, -100.0])),
            DIST: fbits(rng.choice([0.0, 320.0, 100.0, -50.0])),
            CX: fbits(rng.choice([320.0, 0.0, 100.0])),
            CY: fbits(rng.choice([240.0, 0.0, 100.0])),
            RX: fbits(rng.choice([1.0, 0.5, 2.0])),
            RY: fbits(rng.choice([1.0, 0.5, 2.0])),
            MX: fbits(rng.choice([1.0, 0.5, -1.0])),
            MY: fbits(rng.choice([0.85, 1.0, -0.85])),
            CONTROL: rng.choice([0x007f, 0x007f, 0x023f]),
        }))
    return cases


def run_cohort(source, library, cases):
    rows, mismatches, faults = [], 0, 0
    for index, case in enumerate(cases):
        native = (ctypes.c_uint32 * OUT_WORDS)()
        ok = library.sadx_native_view_check((ctypes.c_uint32 * WORDS)(*case), native)
        original, fault, completed = source.run(case)
        match = ok == 1 and completed and list(native) == original
        mismatches += not match
        faults += bool(fault) or not completed
        if not match and len(rows) < 8:
            rows.append({'index': index, 'case': case, 'native': list(native),
                         'original': original, 'fault': fault, 'completed': completed, 'ok': ok})
    return mismatches, faults, rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('probe')
    parser.add_argument('--output', default='build/oracles/view_check.json')
    parser.add_argument('--cases', type=int, default=96)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x403330)
    parser.add_argument('--holdout-cases', type=int, default=64)
    parser.add_argument('--holdout-seed', type=lambda value: int(value, 0), default=0x781670)
    args = parser.parse_args()
    image = PEImage(args.exe)
    library = ctypes.CDLL(str(Path(args.probe).resolve()))
    library.sadx_native_view_check.argtypes = [
        ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    library.sadx_native_view_check.restype = ctypes.c_int
    source = Source(image)
    dev = fixtures(args.cases, args.seed)
    mismatches, faults, rows = run_cohort(source, library, dev)
    holdout = fixtures(args.holdout_cases, args.holdout_seed, curated=False) if args.holdout_cases else []
    h_mismatches, h_faults, h_rows = run_cohort(source, library, holdout) if holdout else (0, 0, [])
    report = {
        'passed': mismatches == 0 and faults == 0 and h_mismatches == 0 and h_faults == 0,
        'values_match': mismatches == 0 and h_mismatches == 0,
        'cases': len(dev),
        'completed': len(dev) - faults,
        'mismatches': mismatches,
        'faults': faults,
        'holdout': {'cases': len(holdout), 'mismatches': h_mismatches, 'faults': h_faults,
                    'seed': hex(args.holdout_seed)},
        'source_range_sha256': hashlib.sha256(source.image_bytes).hexdigest(),
        'source': SOURCE_SHA,
        'entry': hex(ENTRY),
        'samples': rows + h_rows,
        'limits': [
            'njCalcPoint is replaced by the fixture view-space point; its matrix math is owned by the ninja matrix oracle.',
            'x87 status flags are not compared; the boolean result and captured point are.',
        ],
    }
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'cases', 'mismatches', 'faults')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
