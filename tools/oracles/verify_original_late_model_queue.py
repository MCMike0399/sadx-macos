"""lateDrawModel against the pinned sonic.exe.

Unicorn runs 0x4048e0. The allocator and the sorter execute. njGetMatrix is
replaced with the fixture matrix, and the immediate draw calls are counted
rather than entered. Type 6 is a different, larger record and is not this node.
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
from verify_original_math import STACK, STOP
from verify_original_ninja_sprite_queue import QueueOriginal
from verify_original_task_work import call

ENTRY = 0x4048e0
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
ARENA = 0x20000000
BUCKETS = 0x21000000
MATRIX_CALL = 0x404907
IMMEDIATE_CALLS = {0x404928, 0x40492e, 0x404933, 0x40493a, 0x404962, 0x40496c, 0x404971, 0x404978}
JUMPS = {0x404948, 0x404986}
WORDS, OUT_WORDS = 22, 24


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def matrix_words(translation_z):
    words = [0] * 16
    words[0] = words[5] = words[10] = words[15] = fbits(1.0)
    words[14] = fbits(translation_z)
    return words


def blank(overrides):
    row = [0] * WORDS
    row[21] = 0x007f
    for key, value in overrides.items():
        row[key] = value & 0xffffffff
    return row


def fixtures(count, seed):
    cases = [
        blank({0: 0, 1: 6, 2: 0x8bf374, 3: fbits(-3000.0), 4: fbits(-27952.0), **dict(enumerate(matrix_words(10.0), 5))}),
        blank({0: 0, 1: 1, 2: 0x8bf374, 3: fbits(0.0), 4: fbits(0.0), **dict(enumerate(matrix_words(10.0), 5))}),
        blank({0: 1, 1: 6, 2: 0x1000, 3: fbits(-3000.0), 4: fbits(0.0), **dict(enumerate(matrix_words(-4.0), 5))}),
        blank({0: 0, 1: 7, 2: 0x8bf374, 3: fbits(-1.0), 4: fbits(0.0), **dict(enumerate(matrix_words(-10000.0), 5))}),
        blank({0: 0, 1: 8, 2: 0x8bf374, 3: fbits(0.0), 4: fbits(0.0), **dict(enumerate(matrix_words(1.0), 5))}),
    ]
    rng = random.Random(seed)
    while len(cases) < count:
        translation = rng.choice([0.0, 1.0, -10.0, 40.0, -27952.0, 8000.0])
        cases.append(blank({
            0: rng.choice([0, 0, 0, 1]),
            1: rng.choice([0, 1, 6, 7, 8, 15]),
            2: rng.choice([0, 0x1000, 0x8bf374, 0x200000]),
            3: fbits(rng.choice([-3000.0, 0.0, -1.0, 50.0])),
            4: fbits(rng.choice([0.0, -27952.0, 5.5])),
            **dict(enumerate(matrix_words(translation), 5)),
        }))
    return cases


class Source:
    def __init__(self, image):
        self.original = QueueOriginal(image)
        self.machine = self.original.machine
        self.machine.hook_add(uc.UC_HOOK_CODE, self.instruction)
        self.matrix = b'\x00' * 64
        self.image_bytes = image.read(ENTRY, 0x40498b - ENTRY)

    def instruction(self, machine, address, size, user):
        if address == MATRIX_CALL:
            machine.mem_write(machine.reg_read(UC_X86_REG_ECX), self.matrix)
            machine.reg_write(UC_X86_REG_EIP, address + 5)
            return
        if address in IMMEDIATE_CALLS:
            machine.reg_write(UC_X86_REG_EIP, address + 5)
            return
        if address in JUMPS:
            machine.reg_write(UC_X86_REG_ESP, (machine.reg_read(UC_X86_REG_ESP) + 4) & 0xffffffff)
            machine.reg_write(UC_X86_REG_EIP, STOP)

    def prepare(self, case):
        machine = self.machine
        machine.mem_write(ARENA, bytes([0xcd]) * 65536)
        machine.mem_write(BUCKETS, b''.join(struct.pack('<2I', BUCKETS + index * 8, BUCKETS + index * 8)
                                            for index in range(2048)))
        for address, value in ((0x3abd954, ARENA), (0x3ab9874, ARENA + 65536), (0x3abd9c8, 1),
                               (0x3ab98ac, case[0]), (0x3abd950, 0x12345678), (0x3d0f9c8, 11),
                               (0x3d0f840, 22), (0x3d0f9c4, 33), (0x3b12208, 2), (0x3abd9c0, case[4]),
                               (0x3ab9898, case[3])):
            machine.mem_write(address, struct.pack('<I', value & 0xffffffff))
        machine.mem_write(0x3abdcfe, b'\x03')
        machine.mem_write(0x3ab9890, b'\xa8')
        machine.mem_write(0x3abd9a8, struct.pack('<4f', 1, 2, 3, 4))
        machine.mem_write(0x3ab9864, struct.pack('<4f', 5, 6, 7, 8))
        machine.reg_write(UC_X86_REG_FPCW, case[21])
        machine.reg_write(UC_X86_REG_FPSW, 0)
        machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.matrix = struct.pack('<' + 'I' * 16, *case[5:21])

    def bucket_of(self, node):
        if not node:
            return 0xffffffff
        link = struct.unpack('<I', self.machine.mem_read(node, 4))[0]
        if link == 0xcdcdcdcd:
            return 0xffffffff
        if BUCKETS <= link < BUCKETS + 16384 and (link - BUCKETS) % 8 == 0:
            return (link - BUCKETS) // 8
        return 0xffffffff

    def run(self, case):
        self.prepare(case)
        before = struct.unpack('<I', self.machine.mem_read(0x3abd954, 4))[0]
        try:
            call(self.machine, ENTRY, [case[2], case[1]])
        except uc.UcError:
            return None
        after = struct.unpack('<I', self.machine.mem_read(0x3abd954, 4))[0]
        output = [0] * OUT_WORDS
        if after == before:
            output[0] = 1
            output[2] = 0xffffffff
            return output
        node = before
        raw = bytes(self.machine.mem_read(node, 0x70))
        depth = struct.unpack_from('<I', raw, 4)[0]
        header = raw[8]
        attribute = struct.unpack_from('<H', raw, 10)[0]
        color = struct.unpack_from('<I', raw, 16)[0]
        model = struct.unpack_from('<I', raw, 0x2c)[0]
        matrix = list(struct.unpack_from('<16I', raw, 0x30))
        bucket = self.bucket_of(node)
        queued = bucket != 0xffffffff
        output[0] = 0 if queued else 1
        output[1] = 1
        output[2] = bucket
        output[3] = depth if queued else 0
        output[4] = model
        output[5] = header
        output[6] = attribute
        output[7] = color
        output[8:24] = matrix
        return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('probe')
    parser.add_argument('--output', default='build/oracles/late_model_queue.json')
    parser.add_argument('--cases', type=int, default=48)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x4048e0)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA or uc.__version__ != '2.1.4':
        raise SystemExit('source or unicorn mismatch')
    library = ctypes.CDLL(str(Path(args.probe).resolve()))
    library.sadx_native_late_model_queue.argtypes = [
        ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    library.sadx_native_late_model_queue.restype = ctypes.c_int
    source = Source(image)
    cases = fixtures(args.cases, args.seed)
    rows, mismatches, faults = [], 0, 0
    for index, case in enumerate(cases):
        native = (ctypes.c_uint32 * OUT_WORDS)()
        ok = library.sadx_native_late_model_queue((ctypes.c_uint32 * WORDS)(*case), native)
        original = source.run(case)
        match = ok == 0 and original is not None and list(native) == original
        mismatches += not match
        faults += original is None
        if not match and len(rows) < 8:
            rows.append({'index': index, 'case': case, 'native': list(native),
                         'original': original, 'ok': ok})
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
            'njGetMatrix is replaced by the fixture matrix.',
            'The immediate draw calls are skipped and only the queued-versus-immediate route is compared.',
            'The node type is 1. Type 6 reads past this 0x70 node and is not the shadow blob.',
            'Guest link pointers are compared as bucket indexes, not as addresses.',
        ],
    }
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'cases', 'mismatches', 'faults')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
