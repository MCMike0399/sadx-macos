"""Shadow against the pinned sonic.exe.

Unicorn runs 0x49ee30. The land query is replaced with the fixture height and
normals: an empty table is the image's -1000000 result, and a loaded walk is
outside this corpus. Matrix and draw calls are hooked and compared as arguments.
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

ENTRY, END = 0x49ee30, 0x49ef24
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
TASK, STACK, STOP = 0x22200000, 0x22301000, 0x22400000
X, Y, Z, YAW, SCALE, HEIGHT, NORMAL_Z, NORMAL_X, MISSED, CONTROL = range(10)
WORDS, OUT_WORDS = 10, 15
HEIGHT_CALL, PATCH = 0x49ee52, 0x49ee63
PREPARE, PUSH, TRANSLATE = 0x420f90, 0x7850f0, 0x784be0
ROTATE_Z, ROTATE_X, ROTATE_Y = 0x784fb0, 0x784ee0, 0x784e20
SCALE_CALL, DRAW, POP = 0x784c70, 0x408690, 0x785140


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
        for page, size in ((0x22200000, 0x1000), (0x22300000, 0x3000), (0x22400000, 0x1000)):
            machine.mem_map(page, size, uc.UC_PROT_ALL)
        machine.hook_add(uc.UC_HOOK_CODE, self.instruction)
        machine.hook_add(uc.UC_HOOK_MEM_INVALID, self.invalid)
        self.image_bytes = image.read(ENTRY, END - ENTRY)

    def invalid(self, machine, access, address, size, value, user):
        self.fault = address
        return False

    def skip_entered(self, machine, nxt, extra=0):
        machine.reg_write(UC_X86_REG_ESP, (machine.reg_read(UC_X86_REG_ESP) + 4 + extra) & 0xffffffff)
        machine.reg_write(UC_X86_REG_EIP, nxt)

    def instruction(self, machine, address, size, user):
        case, esp = self.case, machine.reg_read(UC_X86_REG_ESP)
        if address == HEIGHT_CALL:
            raw = machine.mem_read(esp, 12)
            self.query = list(struct.unpack('<III', raw))
            sig, exp = ext80(case[HEIGHT])
            machine.reg_write(UC_X86_REG_FP0, (sig, exp))
            machine.reg_write(UC_X86_REG_FPSW, 0)
            machine.reg_write(UC_X86_REG_FPTAG, 0xfffc)
            machine.reg_write(UC_X86_REG_EIP, 0x49ee57)
            return
        if address == PATCH:
            machine.mem_write(esp + 4, packed([case[NORMAL_X]]))
            machine.mem_write(esp + 0xc, packed([case[NORMAL_Z]]))
            machine.mem_write(esp + 0x14, packed([case[HEIGHT]]))
            return
        if address == PREPARE:
            self.flags |= 1
            self.skip_entered(machine, 0x49ee89)
        elif address == PUSH:
            self.flags |= 2
            self.skip_entered(machine, 0x49ee90)
        elif address == TRANSLATE:
            self.translate = list(struct.unpack('<III', machine.mem_read(esp + 4, 12)))
            self.skip_entered(machine, 0x49eead, 12)
        elif address == ROTATE_Z:
            self.rot_z = 0x10000 | (machine.reg_read(UC_X86_REG_EDX) & 0xffff)
            self.skip_entered(machine, 0x49eebf)
        elif address == ROTATE_X:
            self.rot_x = 0x10000 | (machine.reg_read(UC_X86_REG_EDX) & 0xffff)
            self.skip_entered(machine, 0x49eed1)
        elif address == ROTATE_Y:
            self.rot_y = 0x10000 | (machine.reg_read(UC_X86_REG_EDX) & 0xffff)
            self.skip_entered(machine, 0x49eee9)
        elif address == SCALE_CALL:
            self.scale = list(struct.unpack('<III', machine.mem_read(esp + 4, 12)))
            # Stack at the call is x, z, y. The probe reports x, y, z.
            self.scale = [self.scale[0], self.scale[2], self.scale[1]]
            self.skip_entered(machine, 0x49ef04, 12)
        elif address == DRAW:
            obj, clip = struct.unpack('<II', machine.mem_read(esp + 4, 8))
            self.clip = clip
            self.flags |= 8
            self.object = obj
            self.skip_entered(machine, 0x49ef0f)
        elif address == POP:
            self.flags |= 4
            self.skip_entered(machine, 0x49ef1c)

    def run(self, case):
        self.case, self.fault = case, 0
        self.query, self.translate, self.scale = [0, 0, 0], [0, 0, 0], [0, 0, 0]
        self.rot_z = self.rot_x = self.rot_y = self.flags = self.clip = self.object = 0
        machine = self.machine
        work = bytearray(b'\x00' * 0x40)
        def put(offset, value, width):
            work[offset:offset + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little')
        put(0x20, case[X], 4)
        put(0x24, case[Y], 4)
        put(0x28, case[Z], 4)
        put(0x18, case[YAW], 4)
        machine.mem_write(TASK, bytes(work))
        machine.mem_write(0x3b1117c, packed([case[MISSED]]))
        machine.mem_write(STACK, packed([STOP, TASK, case[SCALE]]))
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
        output = [case[HEIGHT], *self.query, *self.translate, *self.scale, self.clip,
                  self.rot_z, self.rot_x, self.rot_y, self.flags]
        done = machine.reg_read(UC_X86_REG_EIP) == STOP and self.object in (0, 0x8bf3a0)
        return output, self.fault, done


def blank(updates=None):
    case = [0] * WORDS
    case[CONTROL] = 0x007f
    case[X], case[Y], case[Z] = fbits(400.0), fbits(0.0), fbits(-20.0)
    case[SCALE] = fbits(1.5)
    case[HEIGHT] = 0xc9742400
    for key, value in (updates or {}).items():
        case[key] = value & 0xffffffff
    return case


def fixtures(count, seed):
    cases = [
        blank(),
        blank({YAW: 16384}),
        blank({MISSED: 1, YAW: 16384}),
        blank({Y: fbits(0.0), HEIGHT: fbits(0.0)}),
        blank({Y: fbits(-1000000.0), HEIGHT: 0xc9742400, YAW: 0}),
        blank({NORMAL_Z: 7, NORMAL_X: 9}),
        blank({YAW: 0x10000}),
        blank({Y: fbits(10.0), HEIGHT: fbits(-4.0), SCALE: fbits(2.0)}),
    ]
    rng = random.Random(seed)
    for _ in range(count):
        cases.append(blank({
            X: fbits(rng.uniform(-30, 30)),
            Y: fbits(rng.choice([0.0, 1.0, 10.0, -2.5])),
            Z: fbits(rng.uniform(-30, 30)),
            YAW: rng.choice([0, 1, 16384, 0x10000, 0xffffffff]),
            SCALE: fbits(rng.choice([1.0, 1.5, 2.0])),
            HEIGHT: rng.choice([0xc9742400, fbits(0.0), fbits(-4.0), fbits(3.0)]),
            NORMAL_Z: rng.choice([0, 1, 7, 0x10001]),
            NORMAL_X: rng.choice([0, 9, 0x8001]),
            MISSED: rng.choice([0, 0, 1, 6]),
        }))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('probe')
    parser.add_argument('--output', default='build/oracles/gamma_shadow.json')
    parser.add_argument('--cases', type=int, default=64)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x49ee30)
    args = parser.parse_args()
    image = PEImage(args.exe)
    library = ctypes.CDLL(str(Path(args.probe).resolve()))
    library.sadx_native_gamma_shadow.argtypes = [
        ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    library.sadx_native_gamma_shadow.restype = ctypes.c_int
    source = Source(image)
    cases = fixtures(args.cases, args.seed)
    rows, mismatches, faults = [], 0, 0
    for index, case in enumerate(cases):
        native = (ctypes.c_uint32 * OUT_WORDS)()
        ok = library.sadx_native_gamma_shadow((ctypes.c_uint32 * WORDS)(*case), native)
        original, fault, completed = source.run(case)
        match = ok == 1 and completed and list(native) == original
        mismatches += not match
        faults += bool(fault) or not completed
        if not match and len(rows) < 8:
            rows.append({'index': index, 'case': case, 'native': list(native),
                         'original': original, 'fault': fault, 'completed': completed, 'ok': ok})
    report = {
        'passed': mismatches == 0 and faults == 0,
        'values_match': mismatches == 0,
        'cases': len(cases),
        'completed': len(cases) - faults,
        'mismatches': mismatches,
        'faults': faults,
        'holdout_index': len(cases) // 2,
        'source_range_sha256': hashlib.sha256(source.image_bytes).hexdigest(),
        'source': SOURCE_SHA,
        'entry': hex(ENTRY),
        'samples': rows,
        'limits': [
            'The land walk at 0x49e920 is replaced by the fixture height and normals.',
            'Matrix and late-draw calls are compared as arguments, not executed.',
            'x87 status flags are not compared; the stored binary32 values are.',
        ],
    }
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'cases', 'mismatches', 'faults')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
