"""RunUnderwaterStuff against the pinned sonic.exe.

Unicorn runs 0x446f10 with the real pose recorder at 0x43b960 and the real
njAddVector at 0x785170. PConvertVector_P2G (0x43ec90) is replaced with the
fixture-converted vector (its own oracle owns the rotation), and
CreateElementalTask (0x40b860) is replaced with the fixture task result; both
calls' arguments are checked. The task table entry is a distinct block from
the passed task work, so the recorder's source cannot be confused with the
vector-add source. The development cohort includes the curated cases; the
holdout cohort is generated from its own seed without them, and is meant to
be run once for the final report (use --holdout-cases 0 while iterating).
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

ENTRY, END = 0x446f10, 0x447001
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
TASK, FAKE, STACK, STOP = 0x22200000, 0x22201000, 0x22301000, 0x22400000
MWP, PWP, POSE = TASK + 0x400, TASK + 0x800, 0x22201400
PLAYERTWP, UNDERWATER_WORD = 0x3b42e10, 0x3b42e70
POS_INDEX, POS_BUFFER, ROT_BUFFER = 0x3b36d80, 0x3b3ce10, 0x3b36df0
CONVERT, CREATE = 0x43ec90, 0x40b860
BREATH_CALLBACK = 0x446b10
SENTINEL = 0xeeeeeeee

(STATUS, SLOT, POS_X, POS_Y, POS_Z, ROT_X, ROT_Y, ROT_Z,
 MACC_X, MACC_Y, MACC_Z, MACCEL, EFF_X, EFF_Y, EFF_Z, ACC_X, ACC_Y, ACC_Z,
 NOCON, BREATH, ISLAND, EYES, TOP_Y, PRIOR, HIST, CONV_X, CONV_Y, CONV_Z,
 CREATE_RESULT, CONTROL, WORD_INIT,
 POSE_X, POSE_Y, POSE_Z, PROT_X, PROT_Y, PROT_Z) = range(37)
WORDS, OUT_WORDS = 37, 31


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
        for page, size in ((0x22200000, 0x2000), (0x22300000, 0x3000), (0x22400000, 0x1000)):
            machine.mem_map(page, size, uc.UC_PROT_ALL)
        machine.hook_add(uc.UC_HOOK_CODE, self.instruction)
        machine.hook_add(uc.UC_HOOK_MEM_INVALID, self.invalid)
        self.image_bytes = image.read(ENTRY, END - ENTRY)

    def invalid(self, machine, access, address, size, value, user):
        self.fault = address
        return False

    def skip_call(self, machine):
        esp = machine.reg_read(UC_X86_REG_ESP)
        ret = struct.unpack('<I', machine.mem_read(esp, 4))[0]
        machine.reg_write(UC_X86_REG_ESP, (esp + 4) & 0xffffffff)
        machine.reg_write(UC_X86_REG_EIP, ret)

    def instruction(self, machine, address, size, user):
        case, esp = self.case, machine.reg_read(UC_X86_REG_ESP)
        if address == CONVERT:
            self.convert_task = struct.unpack('<I', machine.mem_read(esp + 4, 4))[0]
            vector = struct.unpack('<I', machine.mem_read(esp + 8, 4))[0]
            self.conv_in = list(struct.unpack('<III', machine.mem_read(vector, 12)))
            machine.mem_write(vector, packed([case[CONV_X], case[CONV_Y], case[CONV_Z]]))
            self.flags |= 1
            self.skip_call(machine)
        elif address == CREATE:
            self.create_args = list(struct.unpack('<III', machine.mem_read(esp + 4, 12)))
            machine.reg_write(UC_X86_REG_EAX, FAKE if case[CREATE_RESULT] else 0)
            self.flags |= 2
            self.skip_call(machine)

    def run(self, case):
        self.case, self.fault = case, 0
        self.conv_in, self.create_args, self.flags, self.convert_task = [0, 0, 0], None, 0, None
        machine = self.machine
        slot = case[SLOT]

        def block(size, fields):
            data = bytearray(b'\x00' * size)
            for offset, value, width in fields:
                data[offset:offset + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little')
            return bytes(data)

        machine.mem_write(TASK, block(0x40, [
            (4, case[STATUS], 2), (8, slot, 1),
            (0x14, case[ROT_X], 4), (0x18, case[ROT_Y], 4), (0x1c, case[ROT_Z], 4),
            (0x20, case[POS_X], 4), (0x24, case[POS_Y], 4), (0x28, case[POS_Z], 4)]))
        machine.mem_write(MWP, block(0x40, [
            (0x10, case[MACC_X], 4), (0x14, case[MACC_Y], 4), (0x18, case[MACC_Z], 4),
            (0x38, case[MACCEL], 4)]))
        machine.mem_write(PWP, block(0x200, [
            (0xa, case[NOCON], 2), (0xc, case[BREATH], 2), (0x12, case[ISLAND], 2),
            (0x20, case[EFF_X], 4), (0x24, case[EFF_Y], 4), (0x28, case[EFF_Z], 4),
            (0x2c, case[ACC_X], 4), (0x30, case[ACC_Y], 4), (0x34, case[ACC_Z], 4),
            (0x118, case[EYES], 4), (0x1ec, case[TOP_Y], 4), (0x1f4, case[PRIOR], 4)]))
        machine.mem_write(POSE, block(0x40, [
            (0x14, case[PROT_X], 4), (0x18, case[PROT_Y], 4), (0x1c, case[PROT_Z], 4),
            (0x20, case[POSE_X], 4), (0x24, case[POSE_Y], 4), (0x28, case[POSE_Z], 4)]))
        machine.mem_write(FAKE, block(0x120, [(0x2c, FAKE + 0x100, 4), (0x100, 0xee, 1)]))
        machine.mem_write(PLAYERTWP + slot * 4, packed([POSE]))
        machine.mem_write(UNDERWATER_WORD, packed([case[WORD_INIT]]))
        machine.mem_write(POS_INDEX + slot, bytes([case[HIST] & 0xff]))
        fill = b'\xee' * (2048 * 12)
        machine.mem_write(POS_BUFFER, fill)
        machine.mem_write(ROT_BUFFER, fill)
        machine.mem_write(STACK, packed([STOP, TASK, MWP, PWP]))
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

        def word(address):
            return struct.unpack('<I', machine.mem_read(address, 4))[0]

        def short(address):
            return struct.unpack('<H', machine.mem_read(address, 2))[0]

        index = machine.mem_read(POS_INDEX + slot, 1)[0]
        entry = slot + index * 8
        stored_byte = machine.mem_read(FAKE + 0x100, 1)[0]
        stored = stored_byte != 0xee
        if stored:
            self.flags |= 4
        output = [
            short(TASK + 4), short(PWP + 0xa), short(PWP + 0xc), short(PWP + 0x12), index,
            word(POS_BUFFER + entry * 12), word(POS_BUFFER + entry * 12 + 4),
            word(POS_BUFFER + entry * 12 + 8),
            word(ROT_BUFFER + entry * 12), word(ROT_BUFFER + entry * 12 + 4),
            word(ROT_BUFFER + entry * 12 + 8),
            word(UNDERWATER_WORD), *self.conv_in, self.flags, stored_byte if stored else 0,
            word(MWP + 0x10), word(MWP + 0x14), word(MWP + 0x18), word(MWP + 0x38),
            word(PWP + 0x20), word(PWP + 0x24), word(PWP + 0x28),
            word(PWP + 0x2c), word(PWP + 0x30), word(PWP + 0x34),
            word(POS_BUFFER + (entry - 1) * 12) if entry > 0 else SENTINEL,
            word(POS_BUFFER + (entry + 1) * 12) if entry + 1 < 2048 else SENTINEL,
            word(ROT_BUFFER + (entry - 1) * 12) if entry > 0 else SENTINEL,
            word(ROT_BUFFER + (entry + 1) * 12) if entry + 1 < 2048 else SENTINEL,
        ]
        create_ok = self.create_args is None or self.create_args == [8, 0, BREATH_CALLBACK]
        convert_ok = self.convert_task is None or self.convert_task == TASK
        done = machine.reg_read(UC_X86_REG_EIP) == STOP and create_ok and convert_ok
        return output, self.fault, done


def blank(updates=None):
    case = [0] * WORDS
    case[CONTROL] = 0x007f
    case[STATUS] = 0xffff
    case[SLOT] = 5
    case[POS_X], case[POS_Y], case[POS_Z] = fbits(400.0), fbits(0.0), fbits(-20.0)
    case[ROT_Y] = 16384
    case[MACC_X], case[MACC_Y], case[MACC_Z] = fbits(1.0), fbits(2.0), fbits(3.0)
    case[MACCEL] = fbits(4.0)
    case[EFF_X], case[EFF_Y], case[EFF_Z] = fbits(5.0), fbits(6.0), fbits(7.0)
    case[ACC_X], case[ACC_Y], case[ACC_Z] = fbits(8.0), fbits(9.0), fbits(10.0)
    case[NOCON], case[BREATH], case[ISLAND] = 5, 77, 3
    case[EYES] = fbits(8.0)
    case[CONV_Y] = fbits(8.0)
    case[WORD_INIT] = 0x12345678
    case[POSE_X], case[POSE_Y], case[POSE_Z] = fbits(-7.0), fbits(11.0), fbits(3.0)
    case[PROT_X], case[PROT_Y], case[PROT_Z] = 111, 222, 333
    for key, value in (updates or {}).items():
        case[key] = value & 0xffffffff
    return case


WATER = {PRIOR: 0x400000}
NAN = 0x7fc00000


def fixtures(count, seed, curated=True):
    cases = [
        blank(),
        blank({NOCON: 0}),
        blank({NOCON: 0xfffd}),
        blank({NOCON: 0x8000}),
        blank({NOCON: 1, BREATH: 0xffff}),
        blank({**WATER, TOP_Y: fbits(4.0)}),
        blank({**WATER, TOP_Y: fbits(8.0), BREATH: 41}),
        blank({PRIOR: 0x2, TOP_Y: fbits(12.0), BREATH: 1078}),
        blank({**WATER, TOP_Y: fbits(12.0), BREATH: 1079, CREATE_RESULT: 0}),
        blank({**WATER, TOP_Y: fbits(12.0), BREATH: 1079, CREATE_RESULT: 1}),
        blank({**WATER, TOP_Y: fbits(12.0), BREATH: 1080}),
        blank({**WATER, TOP_Y: NAN}),
        blank({**WATER, TOP_Y: fbits(12.0), CONV_Y: NAN}),
        blank({PRIOR: 0x1}),
        blank({PRIOR: 0x400001, TOP_Y: fbits(12.0)}),
        blank({SLOT: 0, HIST: 255}),
        blank({SLOT: 7, HIST: 254}),
        blank({SLOT: 3, HIST: 100}),
        blank({STATUS: 0x14}),
        blank({**WATER, TOP_Y: fbits(-1.0), POS_Y: fbits(-20.0), CONV_Y: fbits(8.0)}),
        blank({**WATER, TOP_Y: fbits(100.0), CONTROL: 0x023f, POS_Y: fbits(0.1),
               CONV_Y: fbits(0.2)}),
    ] if curated else []
    rng = random.Random(seed)
    for _ in range(count):
        cases.append(blank({
            STATUS: rng.choice([0, 0x14, 0xffff, 0x1038, 0xefc7]),
            SLOT: rng.randrange(8),
            POS_X: fbits(rng.uniform(-400, 400)),
            POS_Y: fbits(rng.choice([0.0, -20.0, 5.5, 100.0])),
            POS_Z: fbits(rng.uniform(-400, 400)),
            ROT_X: rng.choice([0, 1, -1 & 0xffffffff, 0x7fffffff]),
            ROT_Y: rng.choice([0, 16384, 32768]),
            NOCON: rng.choice([0, 1, 30, 0x7fff, 0x8000, 0xfffe]),
            BREATH: rng.choice([0, 1, 540, 1079, 1080, 1081, 0xffff]),
            ISLAND: rng.choice([0, 1, 0xffff]),
            EYES: fbits(rng.choice([0.0, 8.0, 10.5])),
            CONV_Y: fbits(rng.choice([0.0, 8.0, -3.0, 20.0])),
            TOP_Y: rng.choice([fbits(v) for v in (-30.0, 0.0, 4.0, 8.0, 12.0, 90.0)] + [NAN]),
            PRIOR: rng.choice([0, 0x2, 0x400000, 0x400002, 0x1, 0x800000]),
            HIST: rng.choice([0, 1, 127, 254, 255]),
            CREATE_RESULT: rng.choice([0, 1]),
            CONTROL: rng.choice([0x007f, 0x007f, 0x023f]),
            WORD_INIT: rng.choice([0, 1, 0xffffffff]),
            POSE_X: fbits(rng.uniform(-50, 50)),
            POSE_Y: fbits(rng.uniform(-50, 50)),
            POSE_Z: fbits(rng.uniform(-50, 50)),
            PROT_X: rng.choice([0, 7, 0xffffffff]),
            PROT_Y: rng.choice([0, 8192]),
            PROT_Z: rng.choice([0, 3]),
        }))
    return cases


def run_cohort(source, library, cases):
    rows, mismatches, faults = [], 0, 0
    for index, case in enumerate(cases):
        native = (ctypes.c_uint32 * OUT_WORDS)()
        ok = library.sadx_native_player_underwater((ctypes.c_uint32 * WORDS)(*case), native)
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
    parser.add_argument('--output', default='build/oracles/player_underwater.json')
    parser.add_argument('--cases', type=int, default=96)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x446f10)
    parser.add_argument('--holdout-cases', type=int, default=64)
    parser.add_argument('--holdout-seed', type=lambda value: int(value, 0), default=0x43b960)
    args = parser.parse_args()
    image = PEImage(args.exe)
    library = ctypes.CDLL(str(Path(args.probe).resolve()))
    library.sadx_native_player_underwater.argtypes = [
        ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    library.sadx_native_player_underwater.restype = ctypes.c_int
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
            'PConvertVector_P2G is replaced by the fixture-converted vector; the rotation itself is owned by the player_coordinates oracle.',
            'CreateElementalTask is replaced by the fixture result; its arguments (8, 0, 0x446b10) are checked and the slot-byte store is observed.',
            'A null playertwp entry faults in the original after the index store and fails in the native service before it; that fault order is not exercised.',
            'x87 status flags are not compared; the stored binary32 values and the fcomp branch are.',
        ],
    }
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'cases', 'mismatches', 'faults')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
