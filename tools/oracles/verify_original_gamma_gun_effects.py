"""GammaUpdateGunEffects against the pinned sonic.exe.

Unicorn runs 0x4c44a0 through the empty lock list. The lock helper at
0x4c38d0 is the image itself: a non-positive count returns null and clears
the marker, so these cases do not hook it. A positive count, and the volley
that follows a real selection, are outside this corpus.
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

ENTRY, END = 0x4c44a0, 0x4c4793
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
TASK, TASKWK, WK = 0x22200000, 0x22201000, 0x22202000
STACK, STOP = 0x22301000, 0x22400000
MODE, LASER, MARKER, LOCK, ARMED, VARIANT, CHARGE, VALUE, FADE, CONTROL = range(10)
WORDS = 10
OUT_WORDS = 9


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
            machine.mem_map(address, size, uc.UC_PROT_ALL)
            machine.mem_write(address, image.slice(section['file_offset'], section['raw_size']))
        for page, size in ((0x22200000, 0x3000), (0x22300000, 0x3000), (0x22400000, 0x1000)):
            machine.mem_map(page, size, uc.UC_PROT_ALL)
        machine.hook_add(uc.UC_HOOK_MEM_INVALID, self.invalid)
        self.image_bytes = image.read(ENTRY, END - ENTRY)

    def invalid(self, machine, access, address, size, value, user):
        self.fault = address
        return False

    def run(self, case):
        self.fault = 0
        machine = self.machine
        work = bytearray(b'\xa5' * 0x120)
        def put(offset, value, width):
            work[offset:offset + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little')
        put(0, case[LASER], 1)
        put(1, case[MODE], 1)
        put(2, case[MARKER], 1)
        put(4, case[LOCK], 1)
        put(5, case[ARMED], 1)
        put(6, case[VARIANT], 1)
        put(0x70, case[CHARGE], 4)
        put(0x80, case[VALUE], 4)
        put(0x90, case[FADE], 4)
        machine.mem_write(WK, bytes(work))
        machine.mem_write(TASK, b'\x00' * 0x40)
        machine.mem_write(TASK + 0x20, packed([TASKWK]))
        machine.mem_write(TASKWK, b'\x00' * 0x20)
        machine.mem_write(STACK, packed([STOP, TASK, 0, WK]))
        for name in ('EAX', 'EBX', 'ECX', 'EDX', 'ESI', 'EDI', 'EBP'):
            machine.reg_write(getattr(uc.x86_const, 'UC_X86_REG_' + name), 0)
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.reg_write(UC_X86_REG_EIP, ENTRY)
        machine.reg_write(UC_X86_REG_FPCW, case[CONTROL] & 0xffff)
        machine.reg_write(UC_X86_REG_FPSW, 0)
        machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
        try:
            machine.emu_start(ENTRY, STOP, count=4000)
        except uc.UcError:
            if not self.fault:
                self.fault = machine.reg_read(UC_X86_REG_EIP)
        image = machine.mem_read(WK, 0x120)
        output = [0] * OUT_WORDS
        output[0] = image[1]
        output[1] = image[0]
        output[2] = image[2]
        output[3] = image[4]
        output[4] = image[5]
        output[5] = image[6]
        output[6] = int.from_bytes(image[0x70:0x74], 'little')
        output[7] = int.from_bytes(image[0x80:0x84], 'little')
        output[8] = int.from_bytes(image[0x90:0x94], 'little')
        return output, self.fault, machine.reg_read(UC_X86_REG_EIP) == STOP and image[3] == 0xa5


def blank(updates=None):
    case = [0] * WORDS
    case[CONTROL] = 0x007f
    case[LASER] = 1
    case[MARKER] = 0x5a
    case[LOCK] = 0
    case[ARMED] = 9
    case[VARIANT] = 9
    case[CHARGE] = fbits(0.0)
    case[VALUE] = 0x111
    case[FADE] = fbits(4.0)
    for key, value in (updates or {}).items():
        case[key] = value & 0xffffffff
    return case


def fixtures(count, seed):
    cases = [
        blank({MODE: 0}),
        blank({MODE: 1, VARIANT: 9}),
        blank({MODE: 3}),
        blank({MODE: 4, CHARGE: fbits(13.0)}),
        blank({MODE: 4, CHARGE: fbits(14.0)}),
        blank({MODE: 4, CHARGE: fbits(14.5)}),
        blank({MODE: 5, CHARGE: fbits(0.0)}),
        blank({MODE: 5, CHARGE: fbits(28.0)}),
        blank({MODE: 5, CHARGE: fbits(29.0)}),
        blank({MODE: 6, CHARGE: fbits(3.0)}),
        blank({MODE: 0xffffffff, MARKER: 0x5a}),
        blank({MODE: 1, LOCK: 0xff, MARKER: 7}),
    ]
    rng = random.Random(seed)
    modes = [0, 1, 3, 4, 5, 6, 7, 0xffffffff]
    for _ in range(count):
        mode = rng.choice(modes)
        charge = rng.choice([0.0, 1.0, 13.0, 14.0, 14.5, 15.0, 28.0, 29.0, 29.5])
        cases.append(blank({
            MODE: mode,
            LASER: rng.choice([0, 1, 2]),
            MARKER: rng.choice([0, 0x5a, 7]),
            LOCK: rng.choice([0, 0xff]),
            ARMED: rng.randrange(4),
            VARIANT: rng.choice([0, 9, 0xff]),
            CHARGE: fbits(charge),
            VALUE: rng.choice([0, 0x111]),
            FADE: fbits(rng.choice([0.0, 4.0, 1.5])),
        }))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('probe')
    parser.add_argument('--output', default='build/oracles/gamma_gun_effects.json')
    parser.add_argument('--cases', type=int, default=64)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x4c44a0)
    args = parser.parse_args()
    image = PEImage(args.exe)
    library = ctypes.CDLL(str(Path(args.probe).resolve()))
    library.sadx_native_gamma_gun_effects.argtypes = [
        ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    library.sadx_native_gamma_gun_effects.restype = ctypes.c_int
    source = Source(image)
    cases = fixtures(args.cases, args.seed)
    holdout = cases[len(cases) // 2]
    rows, mismatches, faults = [], 0, 0
    for index, case in enumerate(cases):
        native = (ctypes.c_uint32 * OUT_WORDS)()
        ok = library.sadx_native_gamma_gun_effects(
            (ctypes.c_uint32 * WORDS)(*case), native)
        original, fault, completed = source.run(case)
        match = ok == 1 and completed and list(native) == original
        mismatches += not match
        faults += bool(fault) or not completed
        if not match and len(rows) < 8:
            rows.append({
                'index': index, 'case': case, 'native': list(native),
                'original': original, 'fault': fault, 'completed': completed, 'ok': ok})
    report = {
        'passed': mismatches == 0 and faults == 0,
        'values_match': mismatches == 0,
        'cases': len(cases),
        'completed': len(cases) - faults,
        'mismatches': mismatches,
        'faults': faults,
        'holdout_index': len(cases) // 2,
        'holdout_mode': holdout[MODE],
        'source_range_sha256': hashlib.sha256(source.image_bytes).hexdigest(),
        'source': SOURCE_SHA,
        'entry': hex(ENTRY),
        'samples': rows,
        'limits': [
            'Positive lock counts and the volley after a real selection are excluded.',
            'The variant table is the image BSS word at 0x3c5b2e4, which the file leaves zero.',
            'x87 status flags are not compared; the stored binary32 values are.',
        ],
    }
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'cases', 'mismatches', 'faults')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
