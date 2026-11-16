"""CharacterShadow against the pinned sonic.exe.

Unicorn runs 0x49f1a0. ListGroundForCollision is replaced by the fixture land
table (the real global addresses are written), and the unrecovered effect,
polygon, scalar and draw callees are scripted from the case words. Every
scripted call's arguments and order are captured as events and compared
alongside the final shadowwk words. The development cohort includes the curated
cases; the holdout cohort is generated from its own seed without them, and is
meant to be run once for the final report (use --holdout-cases 0 while
iterating).
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

ENTRY, END = 0x49f1a0, 0x49f44c
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
TASK, SWP, STACK, STOP = 0x22200000, 0x22200400, 0x22301000, 0x22400000
OBJECT, MODEL, TASK_BLOCK, FORCE = 0x22201000, 0x22202000, 0x22203000, 0x22204000
LAND, LAND_COUNT = 0x3b2f720, 0x3b32724
SPLASH, RIPPLE, LAND_CALL = 0x440890, 0x4407c0, 0x43acd0
WATER, POLYGON, SCALAR, DRAW = 0x456510, 0x452b30, 0x787cc0, 0x49f0b0

(POSX, POSY, POSZ, YAW, PLAYER, ANGX, ANGZ, ATTR_TOP, ATTR_BOTTOM, TOP_Y, BOTTOM_Y,
 PRIOR_TOP, PRIOR_BOTTOM, SCALE, COUNT, CONTROL) = range(16)
BASE = 16
(S_ATTR, S_OBJX, S_OBJZ, S_SX, S_SY, S_SZ, S_R, S_HASTASK, S_HASFORCE, S_OFFSET,
 S_DISTANCE, S_ZX_FOUND, S_ZX_UFOUND, S_ZX_UPOS, S_POLY_FOUND, S_POLY_LFOUND, S_POLY_UFOUND,
 S_POLY_LPOS, S_POLY_UPOS, S_POLY_LAX, S_POLY_LAZ) = range(21)
CANDIDATES, STRIDE = 8, 21
WORDS = BASE + CANDIDATES * STRIDE
OUT_WORDS = 10 + 24 * 8

SPLASH_EVENT, RIPPLE_EVENT, WATER_EVENT, DISTANCE_EVENT = 1, 2, 3, 4
POLYGON_EVENT, DRAW_EVENT, LAND_EVENT = 5, 6, 7


def packed(words):
    return struct.pack('<' + 'I' * len(words), *[word & 0xffffffff for word in words])


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def ext80(bits):
    sign = (bits >> 31) & 1
    exponent = (bits >> 23) & 0xff
    mantissa = bits & 0x7fffff
    if exponent == 0:
        return 0, 0
    return ((1 << 63) | (mantissa << 40), (sign << 15) | (exponent - 127 + 16383))


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
        for page, size in ((0x22200000, 0x5000), (0x22300000, 0x3000), (0x22400000, 0x1000)):
            machine.mem_map(page, size, uc.UC_PROT_ALL)
        machine.hook_add(uc.UC_HOOK_CODE, self.instruction)
        machine.hook_add(uc.UC_HOOK_MEM_INVALID, self.invalid)
        self.image_bytes = image.read(ENTRY, END - ENTRY)

    def invalid(self, machine, access, address, size, value, user):
        self.fault = address
        return False

    def u32(self, address):
        return struct.unpack('<I', self.machine.mem_read(address, 4))[0]

    def skip(self, machine):
        esp = machine.reg_read(UC_X86_REG_ESP)
        ret = struct.unpack('<I', machine.mem_read(esp, 4))[0]
        machine.reg_write(UC_X86_REG_ESP, (esp + 4) & 0xffffffff)
        machine.reg_write(UC_X86_REG_EIP, ret)

    def script(self, obj):
        index = (obj - OBJECT) // 0x40
        return self.case[BASE + index * STRIDE:BASE + (index + 1) * STRIDE]

    def event(self, *words):
        self.events.append(list(words) + [0] * (8 - len(words)))

    def instruction(self, machine, address, size, user):
        esp = machine.reg_read(UC_X86_REG_ESP)
        if address == SPLASH:
            self.event(SPLASH_EVENT)
            self.skip(machine)
        elif address == RIPPLE:
            pno = self.u32(esp + 4)
            position = self.u32(esp + 8)
            angles = self.u32(esp + 12)
            self.event(RIPPLE_EVENT, pno, self.u32(position), self.u32(position + 4),
                       self.u32(position + 8), self.u32(angles), self.u32(angles + 4),
                       self.u32(angles + 8))
            self.skip(machine)
        elif address == LAND_CALL:
            self.event(LAND_EVENT, self.u32(esp + 4), self.u32(esp + 8), self.u32(esp + 12),
                       self.u32(esp + 16))
            count = self.case[COUNT] & 0xff
            count = min(count, CANDIDATES)
            for index in range(CANDIDATES):
                script = self.case[BASE + index * STRIDE:BASE + (index + 1) * STRIDE]
                attributes, obj, task = 0, 0, 0
                if index < count:
                    attributes = script[S_ATTR]
                    obj = OBJECT + index * 0x40
                    if script[S_HASTASK]:
                        task = TASK_BLOCK + index * 0x40
                machine.mem_write(LAND + index * 0xc, packed([attributes, obj, task]))
            machine.mem_write(LAND_COUNT, struct.pack('<h', count))
            self.skip(machine)
        elif address == WATER:
            carry = self.u32(esp + 4)
            obj = self.u32(esp + 8)
            script = self.script(obj)
            self.event(WATER_EVENT, obj, self.u32(carry), self.u32(carry + 4),
                       self.u32(carry + 8))
            found = script[S_ZX_FOUND]
            machine.reg_write(UC_X86_REG_EAX, found)
            if found:
                machine.mem_write(carry + 0x2c, packed([script[S_ZX_UFOUND]]))
                machine.mem_write(carry + 0x3c, packed([script[S_ZX_UPOS]]))
            self.skip(machine)
        elif address == POLYGON:
            carry = self.u32(esp + 4)
            obj = self.u32(esp + 8)
            attributes = self.u32(esp + 12)
            script = self.script(obj)
            self.event(POLYGON_EVENT, obj, attributes)
            found = script[S_POLY_FOUND]
            machine.reg_write(UC_X86_REG_EAX, found)
            if found:
                machine.mem_write(carry + 0xc, packed([script[S_POLY_LFOUND]]))
                machine.mem_write(carry + 0x14, packed([script[S_POLY_LAX]]))
                machine.mem_write(carry + 0x18, packed([script[S_POLY_LAZ]]))
                machine.mem_write(carry + 0x1c, packed([script[S_POLY_LPOS]]))
                machine.mem_write(carry + 0x2c, packed([script[S_POLY_UFOUND]]))
                machine.mem_write(carry + 0x3c, packed([script[S_POLY_UPOS]]))
            self.skip(machine)
        elif address == SCALAR:
            vector = machine.reg_read(UC_X86_REG_ECX)
            index = 0
            if self.scalar_calls < len(self.scalar_order):
                index = self.scalar_order[self.scalar_calls]
                self.scalar_calls += 1
            script = self.script(OBJECT + index * 0x40)
            self.event(DISTANCE_EVENT, self.u32(vector), self.u32(vector + 4),
                       self.u32(vector + 8))
            signature, exponent = ext80(script[S_DISTANCE])
            machine.reg_write(UC_X86_REG_FP0, (signature, exponent))
            machine.reg_write(UC_X86_REG_FPSW, 0)
            machine.reg_write(UC_X86_REG_FPTAG, 0xfffc)
            self.skip(machine)
        elif address == DRAW:
            self.event(DRAW_EVENT)
            self.skip(machine)

    def run(self, case):
        self.case, self.fault = case, 0
        self.events, self.scalar_calls = [], 0
        machine = self.machine
        count = min(case[COUNT] & 0xff, CANDIDATES)
        self.scalar_order = [index for index in range(count)
                             if (case[BASE + index * STRIDE + S_ATTR] & 0x400002) == 0
                             and case[BASE + index * STRIDE + S_HASTASK]]

        task = bytearray(b'\x00' * 0x40)
        task[0x8] = case[PLAYER] & 0xff
        task[0x18:0x1c] = packed([case[YAW]])
        task[0x20:0x24] = packed([case[POSX]])
        task[0x24:0x28] = packed([case[POSY]])
        task[0x28:0x2c] = packed([case[POSZ]])
        machine.mem_write(TASK, bytes(task))
        machine.mem_write(SWP, packed(case[ANGX:SCALE + 1]))
        for index in range(CANDIDATES):
            script = case[BASE + index * STRIDE:BASE + (index + 1) * STRIDE]
            obj = bytearray(b'\x00' * 0x40)
            obj[4:8] = packed([MODEL + index * 0x40])
            obj[8:12] = packed([script[S_OBJX]])
            obj[0x10:0x14] = packed([script[S_OBJZ]])
            obj[0x20:0x24] = packed([script[S_SX]])
            obj[0x24:0x28] = packed([script[S_SY]])
            obj[0x28:0x2c] = packed([script[S_SZ]])
            machine.mem_write(OBJECT + index * 0x40, bytes(obj))
            model = bytearray(b'\x00' * 0x40)
            model[0x24:0x28] = packed([script[S_R]])
            machine.mem_write(MODEL + index * 0x40, bytes(model))
            task_block = bytearray(b'\x00' * 0x40)
            if script[S_HASFORCE]:
                task_block[0x28:0x2c] = packed([FORCE + index * 0x40])
            machine.mem_write(TASK_BLOCK + index * 0x40, bytes(task_block))
            force = bytearray(b'\x00' * 0x40)
            force[0x14:0x18] = packed([script[S_OFFSET]])
            machine.mem_write(FORCE + index * 0x40, bytes(force))
        machine.mem_write(LAND_COUNT, struct.pack('<h', 0))
        machine.mem_write(STACK, packed([STOP, TASK, SWP]))
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
        output = [self.u32(SWP + index * 4) for index in range(9)]
        output.append(len(self.events))
        for event in self.events:
            output.extend(event)
        output.extend([0] * (OUT_WORDS - len(output)))
        done = machine.reg_read(UC_X86_REG_EIP) == STOP
        return output, self.fault, done


def blank(updates=None):
    case = [0] * WORDS
    case[CONTROL] = 0x007f
    case[POSX], case[POSY], case[POSZ] = fbits(10.0), fbits(20.0), fbits(-20.0)
    case[YAW], case[PLAYER] = 16384, 7
    case[ANGX], case[ANGZ] = 0x11, 0x22
    case[TOP_Y], case[BOTTOM_Y] = fbits(2.0), fbits(-3.0)
    case[SCALE] = fbits(1.5)
    case[COUNT] = 0
    for index in range(CANDIDATES):
        block = BASE + index * STRIDE
        case[block + S_ATTR] = 0
        case[block + S_OBJX] = fbits(100.0)
        case[block + S_OBJZ] = fbits(-50.0)
        case[block + S_SX] = fbits(2.0)
        case[block + S_SY] = fbits(3.0)
        case[block + S_SZ] = fbits(4.0)
        case[block + S_R] = fbits(10.0)
        case[block + S_DISTANCE] = fbits(5.0)
    for key, value in (updates or {}).items():
        case[key] = value & 0xffffffff
    return case


def candidate(case, index, attributes=0, object_x=100.0, object_z=-50.0, scale_x=2.0,
              scale_y=3.0, scale_z=4.0, radius=10.0, has_task=0, has_force=1, offset=0.0,
              distance=5.0,
              zx_found=0, zx_upper_found=0, zx_upper=0.0, poly_found=0, poly_lower_found=0,
              poly_upper_found=0, poly_lower=0.0, poly_upper=0.0, poly_angle_x=0,
              poly_angle_z=0):
    block = BASE + index * STRIDE
    case[block + S_ATTR] = attributes
    case[block + S_OBJX] = fbits(object_x)
    case[block + S_OBJZ] = fbits(object_z)
    case[block + S_SX] = fbits(scale_x)
    case[block + S_SY] = fbits(scale_y)
    case[block + S_SZ] = fbits(scale_z)
    case[block + S_R] = fbits(radius)
    case[block + S_HASTASK] = has_task
    case[block + S_HASFORCE] = has_force
    case[block + S_OFFSET] = fbits(offset)
    case[block + S_DISTANCE] = fbits(distance)
    case[block + S_ZX_FOUND] = zx_found
    case[block + S_ZX_UFOUND] = zx_upper_found
    case[block + S_ZX_UPOS] = fbits(zx_upper)
    case[block + S_POLY_FOUND] = poly_found
    case[block + S_POLY_LFOUND] = poly_lower_found
    case[block + S_POLY_UFOUND] = poly_upper_found
    case[block + S_POLY_LPOS] = fbits(poly_lower)
    case[block + S_POLY_UPOS] = fbits(poly_upper)
    case[block + S_POLY_LAX] = poly_angle_x
    case[block + S_POLY_LAZ] = poly_angle_z
    return case


WATER_BITS = 0x400002
NAN = 0x7fc00000


def fixtures(count, seed, curated=True):
    cases = []
    if curated:
        # No candidates, dry top.
        cases.append(blank())
        # No candidates, splash edge and ripple from the previous top height.
        cases.append(blank({ATTR_TOP: WATER_BITS}))
        # Prior already water: ripple without splash.
        cases.append(blank({ATTR_TOP: WATER_BITS, PRIOR_TOP: WATER_BITS}))
        # Dry current top after a water frame: no effect call.
        cases.append(blank({ATTR_TOP: 0x1, PRIOR_TOP: WATER_BITS}))
        # One non-water candidate raises the top.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x10, poly_found=1,
                               poly_upper_found=1, poly_upper=205.0))
        # One lower hit sets the bottom and the shadow angles.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x20, poly_found=1,
                               poly_lower_found=1, poly_lower=5.0, poly_angle_x=0x33,
                               poly_angle_z=0x44))
        # A lower hit exactly at the point is not above it and is accepted.
        cases.append(candidate(blank({COUNT: 1, POSY: fbits(4.0)}), 0, attributes=0x20,
                               poly_found=1, poly_lower_found=1, poly_lower=5.0,
                               poly_angle_x=0x33, poly_angle_z=0x44))
        # The complex-object cull skips the polygon call.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x10, has_task=1,
                               distance=100.0, poly_found=1, poly_upper_found=1,
                               poly_upper=205.0))
        # A task without force work still culls but does not shift the hits.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x10, has_task=1,
                               has_force=0, distance=1.0, poly_found=1,
                               poly_upper_found=1, poly_upper=205.0))
        # At the distance limit the candidate is kept.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x10, has_task=1,
                               distance=40.0, poly_found=1, poly_upper_found=1,
                               poly_upper=205.0))
        # NaN point height never enters a contact.
        cases.append(candidate(blank({COUNT: 1, POSY: NAN}), 0, attributes=0x10,
                               poly_found=1, poly_lower_found=1, poly_upper_found=1,
                               poly_lower=5.0, poly_upper=205.0))
        # NaN hit positions are unordered.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x10, poly_found=1,
                               poly_lower_found=1, poly_upper_found=1))
        cases[-1][BASE + S_POLY_LPOS] = NAN
        cases[-1][BASE + S_POLY_UPOS] = NAN
        # A task force-work offset shifts both hits before the comparisons.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x10, has_task=1,
                               offset=5.0, distance=1.0, poly_found=1,
                               poly_lower_found=1, poly_upper_found=1, poly_lower=6.0,
                               poly_upper=100.0))
        # A water candidate runs the water query before the shared polygon test.
        cases.append(candidate(blank({COUNT: 1, ATTR_TOP: WATER_BITS}), 0,
                               attributes=WATER_BITS, zx_found=1, zx_upper_found=1,
                               zx_upper=50.0, poly_found=1, poly_upper_found=1,
                               poly_upper=40.0))
        # A water query miss still reaches the shared test.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=WATER_BITS, zx_found=0,
                               poly_found=1, poly_upper_found=1, poly_upper=40.0))
        # A non-1 upper find flag does not update the top.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=WATER_BITS, zx_found=1,
                               zx_upper_found=2, zx_upper=50.0, poly_found=1,
                               poly_upper_found=1, poly_upper=40.0))
        # Two candidates: the nearest top above and the highest bottom below win.
        cases.append(candidate(candidate(blank({COUNT: 2}), 0, attributes=0x10,
                                         poly_found=1, poly_lower_found=1,
                                         poly_upper_found=1, poly_lower=3.0,
                                         poly_upper=300.0, poly_angle_x=1, poly_angle_z=2),
                               1, attributes=0x20, poly_found=1, poly_lower_found=1,
                               poly_upper_found=1, poly_lower=7.0, poly_upper=90.0,
                               poly_angle_x=3, poly_angle_z=4))
        # A candidate without a task never takes the cull even with object data.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x10, has_task=0,
                               distance=1000.0, poly_found=1, poly_upper_found=1,
                               poly_upper=205.0))
        # A NaN scale makes the cull limit unordered and keeps the candidate.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x10, has_task=1,
                               scale_x=float('nan'), distance=1.0, poly_found=1,
                               poly_upper_found=1, poly_upper=205.0))
        # A found polygon with no lower/upper find flags changes nothing.
        cases.append(candidate(blank({COUNT: 1}), 0, attributes=0x10, poly_found=1))
        # Attributes and both prior words rotate together.
        cases.append(blank({ATTR_TOP: 0x40, ATTR_BOTTOM: 0x80, PRIOR_TOP: 1,
                            PRIOR_BOTTOM: 2}))
        # The top and bottom already hold closer contacts; further candidates lose.
        cases.append(candidate(blank({COUNT: 1, TOP_Y: fbits(30.0),
                                      BOTTOM_Y: fbits(10.0)}), 0, attributes=0x10,
                               poly_found=1, poly_lower_found=1, poly_upper_found=1,
                               poly_lower=5.0, poly_upper=205.0))
    rng = random.Random(seed)
    for _ in range(count):
        case = blank({
            POSX: fbits(rng.choice([-400.0, 0.0, 10.0, 400.0])),
            POSY: rng.choice([fbits(v) for v in (-30.0, 0.0, 4.0, 20.0, 100.0)] + [NAN]),
            POSZ: fbits(rng.uniform(-400.0, 400.0)),
            YAW: rng.choice([0, 16384, 0xffffffff]),
            PLAYER: rng.choice([0, 7, 255]),
            ANGX: rng.choice([0, 1, 0xffffffff]),
            ANGZ: rng.choice([0, 0x8000, 0xffffffff]),
            ATTR_TOP: rng.choice([0, 1, 0x2, 0x10, 0x400000, 0x400002, 0xffffffff]),
            ATTR_BOTTOM: rng.choice([0, 0x40, 0xffffffff]),
            TOP_Y: rng.choice([fbits(v) for v in (-30.0, 0.0, 2.0, 30.0, 1.0e6)] + [NAN]),
            BOTTOM_Y: rng.choice([fbits(v) for v in (-1.0e6, -3.0, 0.0, 5.0, 1.0e6)] + [NAN]),
            PRIOR_TOP: rng.choice([0, 1, 0x400000, 0x400002, 0xffffffff]),
            PRIOR_BOTTOM: rng.choice([0, 2, 0xffffffff]),
            SCALE: fbits(rng.choice([0.0, 1.5, -2.0])),
            COUNT: rng.randrange(0, 5),
            CONTROL: rng.choice([0x007f, 0x007f, 0x023f]),
        })
        for index in range(min(case[COUNT], CANDIDATES)):
            candidate(
                case, index,
                attributes=rng.choice([0, 1, 0x10, 0x400000, 0x400002, 0xffffffff]),
                object_x=rng.choice([-50.0, 100.0, 400.0]),
                object_z=rng.choice([-400.0, -50.0, 0.0]),
                scale_x=rng.choice([0.0, 2.0, -3.0]),
                scale_y=rng.choice([0.0, 3.0, 1.0e6]),
                scale_z=rng.choice([0.0, 4.0, -1.0]),
                radius=rng.choice([0.0, 1.0, 10.0, -2.0]),
                has_task=rng.choice([0, 1]),
                has_force=rng.choice([0, 1]),
                offset=rng.choice([0.0, 5.0, -5.0]),
                distance=rng.choice([0.0, 5.0, 40.0, 100.0]),
                zx_found=rng.choice([0, 1, 2]),
                zx_upper_found=rng.choice([0, 1, 2]),
                zx_upper=rng.choice([-30.0, 0.0, 50.0, 205.0]),
                poly_found=rng.choice([0, 1]),
                poly_lower_found=rng.choice([0, 1, 2]),
                poly_upper_found=rng.choice([0, 1, 2]),
                poly_lower=rng.choice([-30.0, 0.0, 5.0, 21.0]),
                poly_upper=rng.choice([-30.0, 0.0, 40.0, 205.0]),
                poly_angle_x=rng.choice([0, 0x33, -1]),
                poly_angle_z=rng.choice([0, 0x44, -1]),
            )
            if rng.random() < 0.15:
                case[BASE + index * STRIDE + S_POLY_UPOS] = NAN
        cases.append(case)
    return cases


def run_cohort(source, library, cases, cohort):
    rows, mismatches, faults = [], 0, 0
    for index, case in enumerate(cases):
        native = (ctypes.c_uint32 * OUT_WORDS)()
        ok = library.sadx_native_character_shadow((ctypes.c_uint32 * WORDS)(*case), native)
        original, fault, completed = source.run(case)
        match = ok == 1 and completed and list(native) == original
        mismatches += not match
        faults += bool(fault) or not completed
        if not match and len(rows) < 8:
            rows.append({'cohort': cohort, 'index': index, 'case': case, 'native': list(native),
                         'original': original, 'fault': fault, 'completed': completed, 'ok': ok})
    return mismatches, faults, rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('probe')
    parser.add_argument('--output', default='build/oracles/character_shadow.json')
    parser.add_argument('--cases', type=int, default=96)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x49f1a0)
    parser.add_argument('--holdout-cases', type=int, default=64)
    parser.add_argument('--holdout-seed', type=lambda value: int(value, 0), default=0x49f0b0)
    args = parser.parse_args()
    image = PEImage(args.exe)
    library = ctypes.CDLL(str(Path(args.probe).resolve()))
    library.sadx_native_character_shadow.argtypes = [
        ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    library.sadx_native_character_shadow.restype = ctypes.c_int
    source = Source(image)
    dev = fixtures(args.cases, args.seed)
    mismatches, faults, rows = run_cohort(source, library, dev, 'development')
    holdout = fixtures(args.holdout_cases, args.holdout_seed, curated=False) if args.holdout_cases else []
    h_mismatches, h_faults, h_rows = run_cohort(source, library, holdout, 'holdout') if holdout else (0, 0, [])
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
            'ListGroundForCollision is replaced by the fixture land table; the table walk itself is owned by that routine, and the land entries, object positions, scales and radii are fixture words written at the real object addresses.',
            'PSetSplashEffect and PSetRippleEffect are replaced by recorded calls; their arguments and order are checked.',
            'GetZxShadowOnFDPolygon and TestShadowPolygon are replaced by the case script; their arguments, order and returned carry fields are checked.',
            'njScalor is replaced by the case distance word and the x87 return value; its argument vector is checked.',
            'FUN_0049f0b0 is replaced by a recorded draw call; x87 status flags are not compared, only the stored binary32 values and the fcomp branches.',
            'Unicorn 2.1.4 reports an ordered greater result for FCOMPP with a NaN register operand here, so the unordered-distance cull branch is asserted by the unit test instead of the oracle.',
        ],
    }
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'cases', 'mismatches', 'faults')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
