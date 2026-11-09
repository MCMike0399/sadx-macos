"""GammaUpdateTargetEffects against the pinned sonic.exe.

Unicorn runs 0x4c4da0. njSqrt and _ftol stay on the image. The two FPATAN
sites are replaced with the probe's Bochs P5/P6 result because Unicorn's
FPATAN is host-double math. Create, allocate, destroy, sound, the lock
table and the release call are hooked and returned from the fixture.
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

ENTRY, END = 0x4c4da0, 0x4c506f
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
WK, PLAYER, TASKWK = 0x22200000, 0x22201000, 0x22202000
SEARCH, BLOCK = 0x22100000, 0x22101000
AIM_TASK, AIM_TWK = 0x22102000, 0x22103000
LOCK_BASE = 0x22104000
STACK, STOP = 0x22301000, 0x22400000
PAD, MASK, EVENT = 0x3b0e7f0, 0x909f94, 0x3c53b18
SEARCH_CALL, ALLOC_CALL, FAIL_DESTROY = 0x4c4e01, 0x4c4e14, 0x4c4e3f
PLAY_CALL, AIM_CALL = 0x4c4e5b, 0x4c4e71
TABLE_CALL, RELEASE_CALL = 0x4c4f38, 0x4c4f7d
CLEAR_DESTROY, STOP_CALL, SHARED_TABLE = 0x4c4f98, 0x4c4fac, 0x4c4fd4
FPATAN = (0x4c4ed4, 0x4c4ef0)
MODE, GUN, LOCK, LOCK_COMBO, SHOT, LASER, LOCK_TIMER = range(7)
POS, AIM, DISTANCE, SCOPE, SEARCH_BIT = 7, 10, 13, 14, 15
TASK_MODE, SCOPE_SLOT, LOCK_SLOT, BUTTON = 16, 17, 18, 19
HELD, PRESSED, EVENT_BIT = 20, 28, 36
CREATE, ALLOCATE, CREATE_AIM, TABLE, CONTROL = 37, 38, 39, 40, 41
SUBJECT, WORDS = 42, 46
OUT_WORDS = 42
NAMES = (
    'mode', 'gun', 'lock', 'lock_combo', 'shot', 'laser', 'lock_timer',
    'position_x', 'position_y', 'position_z', 'aim_x', 'aim_y', 'aim_z',
    'distance', 'scope', 'search', 'calls')


def packed(words):
    return struct.pack('<' + 'I' * len(words), *[word & 0xffffffff for word in words])


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


class Source:
    def __init__(self, image, fpatan):
        assert image.sha256 == SOURCE_SHA and uc.__version__ == '2.1.4'
        self.fpatan = fpatan
        machine = self.machine = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        for section in image.sections:
            if section['name'] not in ('.text', '.rdata', '.data'):
                continue
            address = image.base + section['rva']
            size = (max(section['raw_size'], section['virtual_size']) + 4095) & ~4095
            machine.mem_map(address, size, uc.UC_PROT_ALL)
            machine.mem_write(address, image.slice(section['file_offset'], section['raw_size']))
        for page, size in ((0x22100000, 0x8000), (0x22200000, 0x3000),
                           (0x22300000, 0x3000), (0x22400000, 0x1000)):
            machine.mem_map(page, size, uc.UC_PROT_ALL)
        machine.hook_add(uc.UC_HOOK_CODE, self.instruction)
        machine.hook_add(uc.UC_HOOK_MEM_INVALID, self.invalid)
        self.image_bytes = image.read(ENTRY, END - ENTRY)

    def invalid(self, machine, access, address, size, value, user):
        self.fault = address
        return False

    def skip(self, machine, address, result):
        machine.reg_write(UC_X86_REG_EAX, result & 0xffffffff)
        machine.reg_write(UC_X86_REG_EIP, address)

    def substitute_fpatan(self, machine, address):
        status = machine.reg_read(UC_X86_REG_FPSW)
        top = (status >> 11) & 7
        x_sig, x_exp = machine.reg_read(UC_X86_REG_FP0 + top)
        y_sig, y_exp = machine.reg_read(UC_X86_REG_FP0 + ((top + 1) & 7))
        out_sig, out_exp = ctypes.c_uint64(), ctypes.c_uint16()
        ok = self.fpatan(y_sig, y_exp, x_sig, x_exp, ctypes.byref(out_sig), ctypes.byref(out_exp))
        if not ok:
            self.fault = address
            machine.emu_stop()
            return
        machine.reg_write(UC_X86_REG_FP0 + ((top + 1) & 7), (out_sig.value, out_exp.value))
        machine.reg_write(UC_X86_REG_FPSW, (status & ~(7 << 11)) | (((top + 1) & 7) << 11))
        machine.reg_write(UC_X86_REG_EIP, address + 2)

    def instruction(self, machine, address, size, user):
        case = self.case
        if address in FPATAN:
            self.substitute_fpatan(machine, address)
            return
        if address == SEARCH_CALL:
            self.calls.append(1)
            self.skip(machine, 0x4c4e06, SEARCH if case[CREATE] else 0)
        elif address == ALLOC_CALL:
            self.calls.append(2)
            self.skip(machine, 0x4c4e19, BLOCK if case[ALLOCATE] else 0)
        elif address == FAIL_DESTROY:
            self.calls.append(4)
            self.skip(machine, 0x4c4e44, 0)
        elif address == PLAY_CALL:
            self.calls.append(5)
            self.skip(machine, 0x4c4e60, 0)
        elif address == AIM_CALL:
            self.calls.append(7)
            self.skip(machine, 0x4c4e76, AIM_TASK if case[CREATE_AIM] else 0)
        elif address in (TABLE_CALL, SHARED_TABLE):
            self.calls.append(9)
            self.skip(machine, address + 5, case[TABLE])
        elif address == RELEASE_CALL:
            self.calls.append(10)
            self.releases.append(struct.unpack('<I', machine.mem_read(machine.reg_read(UC_X86_REG_ESP), 4))[0])
            self.skip(machine, 0x4c4f82, 0)
        elif address == CLEAR_DESTROY:
            self.calls.append(4)
            self.skip(machine, 0x4c4f9d, 0)
        elif address == STOP_CALL:
            self.calls.append(6)
            self.skip(machine, 0x4c4fb1, 0)

    def run(self, case):
        self.case, self.calls, self.releases, self.fault = case, [], [], 0
        machine = self.machine
        work = bytearray(b'\xa5' * 0x120)
        def put(offset, value, width):
            work[offset:offset + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little')
        put(0, case[MODE], 1)
        put(1, case[GUN], 1)
        put(4, case[LOCK], 1)
        put(0x0a, case[LOCK_COMBO], 2)
        put(0x0c, case[SHOT], 2)
        put(0x10, case[LASER], 2)
        put(0x12, case[LOCK_TIMER], 2)
        for index, word in enumerate(case[POS:POS + 3]):
            put(0x34 + index * 4, word, 4)
        for index, word in enumerate(case[AIM:AIM + 3]):
            put(0x40 + index * 4, word, 4)
        put(0x4c, case[DISTANCE], 4)
        put(0x6c, SEARCH if case[SEARCH_BIT] else 0, 4)
        put(0x84, case[SCOPE], 4)
        for index, subject in enumerate(case[SUBJECT:SUBJECT + 4]):
            task = LOCK_BASE + index * 0x100
            put(0xf4 + index * 4, task, 4)
            machine.mem_write(task, b'\x00' * 0x40)
            if subject:
                machine.mem_write(task + 0x2c, packed([LOCK_BASE + 0x80 + index * 0x10]))
                machine.mem_write(LOCK_BASE + 0x80 + index * 0x10, packed([0, 0, subject]))
            else:
                machine.mem_write(task + 0x2c, packed([0]))
        machine.mem_write(WK, bytes(work))
        machine.mem_write(BLOCK, b'\x00' * 16)
        machine.mem_write(SEARCH, b'\x00' * 0x40)
        player = bytearray(b'\x00' * 0x20)
        player[8] = case[LOCK_SLOT] & 0xff
        machine.mem_write(PLAYER, bytes(player))
        taskwk = bytearray(b'\x00' * 0x20)
        taskwk[8] = case[SCOPE_SLOT] & 0xff
        machine.mem_write(TASKWK, bytes(taskwk))
        machine.mem_write(AIM_TASK, packed([0] * 16))
        machine.mem_write(AIM_TASK + 0x20, packed([AIM_TWK]))
        machine.mem_write(AIM_TWK, b'\x00' * 0x40)
        machine.mem_write(STACK, packed([STOP, TASKWK, PLAYER, WK]))
        for slot in range(8):
            machine.mem_write(PAD + slot * 0x34 + 8, packed([case[HELD + slot]]))
            machine.mem_write(PAD + slot * 0x34 + 0x10, packed([case[PRESSED + slot]]))
        machine.mem_write(MASK, packed([case[BUTTON]]))
        machine.mem_write(EVENT, bytes([1 if case[EVENT_BIT] else 0]))
        for name in ('EAX', 'EBX', 'ECX', 'EDX', 'ESI', 'EDI', 'EBP'):
            machine.reg_write(getattr(uc.x86_const, 'UC_X86_REG_' + name), 0)
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.reg_write(UC_X86_REG_EIP, ENTRY)
        machine.reg_write(UC_X86_REG_FPCW, case[CONTROL] & 0xffff)
        machine.reg_write(UC_X86_REG_FPSW, 0)
        machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
        try:
            machine.emu_start(ENTRY, STOP, count=8000)
        except uc.UcError:
            if not self.fault:
                self.fault = machine.reg_read(UC_X86_REG_EIP)
        image = machine.mem_read(WK, 0x120)
        aim = machine.mem_read(AIM_TWK, 0x30)
        block = machine.mem_read(BLOCK, 0x10)
        output = [0] * OUT_WORDS
        output[0] = image[0]
        output[1] = image[1]
        output[2] = image[4]
        output[3] = int.from_bytes(image[0x0a:0x0c], 'little')
        output[4] = int.from_bytes(image[0x0c:0x0e], 'little')
        output[5] = int.from_bytes(image[0x10:0x12], 'little')
        output[6] = int.from_bytes(image[0x12:0x14], 'little')
        for index in range(3):
            output[7 + index] = int.from_bytes(image[0x34 + index * 4:0x38 + index * 4], 'little')
            output[10 + index] = int.from_bytes(image[0x40 + index * 4:0x44 + index * 4], 'little')
        output[13] = int.from_bytes(image[0x4c:0x50], 'little')
        output[14] = int.from_bytes(image[0x84:0x88], 'little')
        search = int.from_bytes(image[0x6c:0x70], 'little')
        output[15] = 0 if search == 0 else 1 if search == SEARCH else search
        output[16] = len(self.calls)
        for index, call in enumerate(self.calls[:12]):
            output[17 + index] = call
        output[29] = int(int.from_bytes(block[4:8], 'little') == TASKWK)
        output[30] = int(int.from_bytes(block[8:12], 'little') == WK)
        output[31] = int.from_bytes(aim[0x20:0x24], 'little')
        output[32] = int.from_bytes(aim[0x24:0x28], 'little')
        output[33] = int.from_bytes(aim[0x28:0x2c], 'little')
        output[34] = int.from_bytes(aim[0x18:0x1c], 'little')
        output[35] = int.from_bytes(aim[0x1c:0x20], 'little')
        output[36] = int(int.from_bytes(aim[0x10:0x14], 'little') == WK)
        output[37] = len(self.releases)
        for index, subject in enumerate(self.releases[:4]):
            output[38 + index] = subject
        return output, self.fault, machine.reg_read(UC_X86_REG_EIP) == STOP


def blank(updates=None):
    case = [0] * WORDS
    case[CONTROL] = 0x007f
    case[SCOPE] = fbits(1.0)
    case[CREATE] = 1
    case[ALLOCATE] = 1
    case[CREATE_AIM] = 1
    for key, value in (updates or {}).items():
        case[key] = value & 0xffffffff
    return case


def fixtures(count, seed):
    cases = [blank()]  # mode 0, scope released back to 0
    cases.append(blank({MODE: 1, HELD: 1, SCOPE: fbits(18.0)}))
    cases.append(blank({MODE: 1, HELD: 1, SCOPE: fbits(19.0)}))
    cases.append(blank({MODE: 1, HELD: 1, BUTTON: 1, SCOPE: fbits(17.5)}))
    cases.append(blank({MODE: 7, HELD + 1: 1, SCOPE_SLOT: 1, SCOPE: fbits(2.0)}))
    cases.append(blank({MODE: 0xffffffff, HELD + 1: 1, SCOPE_SLOT: 1, SCOPE: fbits(3.0)}))
    cases.append(blank({MODE: 1, PRESSED: 1, TABLE: 0, TASK_MODE: 4}))
    cases.append(blank({MODE: 1, PRESSED + 2: 1, LOCK_SLOT: 2, TABLE: 1, TASK_MODE: 4,
                        HELD + 1: 1, SCOPE_SLOT: 1, SCOPE: fbits(4.0)}))
    cases.append(blank({MODE: 2, EVENT_BIT: 1}))
    cases.append(blank({MODE: 2, EVENT_BIT: 1, ALLOCATE: 0}))
    cases.append(blank({MODE: 2, CREATE: 0, EVENT_BIT: 1}))
    cases.append(blank({MODE: 4, GUN: 9}))
    cases.append(blank({MODE: 4, GUN: 9, LOCK: 2, SUBJECT + 1: 5}))
    cases.append(blank({MODE: 5, SEARCH_BIT: 1, TASK_MODE: 4}))
    cases.append(blank({MODE: 6, SEARCH_BIT: 1, PRESSED: 1, TABLE: 7, TASK_MODE: 4}))
    cases.append(blank({MODE: 3, LASER: 1, POS: fbits(8.0), AIM: fbits(3.0), AIM + 1: fbits(4.0)}))
    cases.append(blank({MODE: 3, LASER: 1, AIM + 2: fbits(3.0)}))
    cases.append(blank({MODE: 3, LASER: 2, TABLE: 1, TASK_MODE: 6, HELD: 1}))
    cases.append(blank({MODE: 3, LASER: 2, TABLE: 0, CREATE_AIM: 0}))
    rng = random.Random(seed)
    modes = [0, 1, 2, 3, 4, 5, 6, 7, 0xffffffff]
    for _ in range(count):
        mode = rng.choice(modes)
        scope_slot, lock_slot = rng.randrange(8), rng.randrange(8)
        held = [0] * 8
        pressed = [0] * 8
        held[scope_slot] = rng.choice([0, 1, 0x909])
        pressed[lock_slot] = rng.choice([0, 1, 0x42])
        subjects = [0, 0, 0, rng.choice([0, 5, 9])]
        aim = [fbits(rng.uniform(-4, 4)) for _ in range(3)]
        case = blank({
            MODE: mode, GUN: rng.randrange(4), LOCK: rng.choice([0, 1, 2, 0xff]),
            LOCK_COMBO: rng.randrange(4), SHOT: rng.randrange(4),
            LASER: rng.choice([0, 1, 2, 180]), LOCK_TIMER: rng.randrange(3),
            SCOPE: fbits(rng.choice([0.0, 1.0, 18.0, 19.0, 4.5])),
            SEARCH_BIT: rng.choice([0, 1]), TASK_MODE: rng.randrange(8),
            SCOPE_SLOT: scope_slot, LOCK_SLOT: lock_slot, BUTTON: 1,
            EVENT_BIT: rng.choice([0, 1]), CREATE: rng.choice([0, 1]),
            ALLOCATE: rng.choice([0, 1]), CREATE_AIM: rng.choice([0, 1]),
            TABLE: rng.choice([0, 1, 7]), DISTANCE: fbits(rng.uniform(0, 3)),
        })
        for slot in range(8):
            case[HELD + slot] = held[slot]
            case[PRESSED + slot] = pressed[slot]
        for index, word in enumerate(aim):
            case[AIM + index] = word
        case[POS] = fbits(rng.uniform(-2, 2))
        for index, subject in enumerate(subjects):
            case[SUBJECT + index] = subject
        cases.append(case)
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('probe')
    parser.add_argument('--output', default='build/oracles/gamma_target_effects.json')
    parser.add_argument('--cases', type=int, default=64)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x4c4da0)
    args = parser.parse_args()
    image = PEImage(args.exe)
    library = ctypes.CDLL(str(Path(args.probe).resolve()))
    library.sadx_native_gamma_target_effects.argtypes = [
        ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    library.sadx_native_gamma_target_effects.restype = ctypes.c_int
    library.sadx_gamma_target_fpatan.argtypes = [
        ctypes.c_uint64, ctypes.c_uint16, ctypes.c_uint64, ctypes.c_uint16,
        ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(ctypes.c_uint16)]
    library.sadx_gamma_target_fpatan.restype = ctypes.c_int
    source = Source(image, library.sadx_gamma_target_fpatan)
    failures, faults, stopped = [], [], 0
    for index, case in enumerate(fixtures(args.cases, args.seed)):
        native = (ctypes.c_uint32 * OUT_WORDS)()
        if library.sadx_native_gamma_target_effects((ctypes.c_uint32 * WORDS)(*case), native) != 1:
            faults.append({'case_id': index, 'where': 'native'})
            continue
        try:
            original, fault, completed = source.run(case)
        except Exception as error:
            faults.append({'case_id': index, 'where': str(error)})
            continue
        stopped += completed
        if fault:
            faults.append({'case_id': index, 'where': hex(fault), 'mode': case[MODE] & 0xff})
            continue
        differences = [word for word in range(OUT_WORDS) if original[word] != native[word]]
        if differences:
            failures.append({
                'case_id': index, 'mode': case[MODE] & 0xff,
                'words': differences,
                'names': [NAMES[word] if word < len(NAMES) else str(word) for word in differences],
                'original': [original[word] for word in differences],
                'native': [native[word] for word in differences]})
    report = {
        'source_sha256': image.sha256,
        'source_ranges': {f'{ENTRY:#x}-{END:#x}': hashlib.sha256(source.image_bytes).hexdigest()},
        'cases': len(fixtures(args.cases, args.seed)),
        'completed': stopped,
        'values_match': not failures and not faults,
        'passed': not failures and not faults,
        'mismatch_count': len(failures),
        'fault_count': len(faults),
        'mismatches': failures[:8],
        'faults': faults[:8],
        'limits': [
            'Unicorn 2.1.4 FPATAN is replaced at 0x4c4ed4 and 0x4c4ef0 with Bochs P5/P6.',
            'x87 status flags are not compared.',
            'Ghidra retained no procedure at 0x4c4da0; the body was checked from the listing and the two callee dossiers.',
        ],
    }
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('cases', 'completed', 'values_match', 'passed',
                                                    'mismatch_count', 'fault_count')}))
    if failures or faults:
        print(json.dumps({'first_failure': failures[:1], 'first_fault': faults[:1]}))
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
