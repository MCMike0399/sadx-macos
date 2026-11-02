"""Original model-matrix producer/getter and numeric primitives versus native code.

Fixtures own explicit nonzero stack, count/limit, matrix, hierarchy and key state.
No startup or actor-frame qualification is inferred from these isolated calls.
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
import unicorn as uc
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                              UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW,
                              UC_X86_REG_FPSW)
from pe_image import PEImage
from extract import SOURCE_SHA256, TABLE_ADDRESS
from verify_original_math import Original, DATA, STACK, STOP

NODES, SLOTS = 8, 8
MATRICES, STACK_WORDS, BASE_WORDS, DEST_WORDS = 16, 2064, 2192, 2208
OBJECT_WORDS, PRESENT_WORDS, KEY_WORDS, WORDS = 2224, 2328, 2352, 2736
UNIT = 0x389d650
COUNT, BUFFER, LIMIT = 0x3c58270, 0x3c58278, 0x3c5a278
FRAMES, BASE, DEST = DATA + 0x1000, DATA + 0x100, DATA + 0x200
OBJECTS, NDATA, KEYS, ACTION, MOTION = (DATA + n for n in (0x2000, 0x3000, 0x4000, 0x800, 0x900))

def pack(words):
    return struct.pack('<' + 'I' * len(words), *words)

def word(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]

def sha(data):
    return hashlib.sha256(data).hexdigest()

class Source(Original):
    def __init__(self, image):
        super().__init__(image, 127)
        m = self.machine
        m.mem_map(0x389d000, 4096)
        m.mem_write(UNIT, image.read(UNIT, 64))
        m.mem_map(0x3d0f000, 4096)
        m.mem_map(0x3c58000, 0x3000)
        self.failure = False
        m.hook_add(uc.UC_HOOK_CODE, self.hook)

    def hook(self, m, address, size, user):
        if address == 0x4b82a9 and not BUFFER <= m.reg_read(UC_X86_REG_EDX) < BUFFER + 8192:
            self.failure = True
            m.emu_stop()
        # A fixture declares four keys per channel. Preserve an explicit missing
        # key boundary rather than silently borrowing the next fixture's bytes.
        if address in (0x4b7df3, 0x4b802e):
            self.key_end = m.reg_read(uc.x86_const.UC_X86_REG_ESI) + 64
        if address in (0x4b7e00, 0x4b8037) and m.reg_read(UC_X86_REG_EDX) >= self.key_end:
            self.failure = True
            m.emu_stop()
        if address == 0x4b7d60:  # original unsupported-layout infinite loop
            self.failure = True
            m.emu_stop()

    def run(self, c):
        m = self.machine
        self.failure = False
        m.mem_write(BUFFER, pack(c[MATRICES:STACK_WORDS]))
        m.mem_write(COUNT, pack([c[11]]))
        m.mem_write(LIMIT, pack([c[12]]))
        m.mem_write(FRAMES, pack(c[STACK_WORDS:BASE_WORDS]))
        m.mem_write(BASE, pack(c[BASE_WORDS:DEST_WORDS]))
        m.mem_write(DEST, pack(c[DEST_WORDS:OBJECT_WORDS]))
        for n in range(NODES):
            o = c[OBJECT_WORDS + n * 13:OBJECT_WORDS + (n + 1) * 13].copy()
            for slot in (11, 12):
                o[slot] = OBJECTS + (o[slot] - 1) * 52 if o[slot] else 0
            m.mem_write(OBJECTS + n * 52, pack(o))
        channels = 2 if c[4] < 4 else 3
        for n in range(NODES):
            pointers = []
            for ch in range(channels):
                addr = KEYS + (n * 3 + ch) * 64
                offset = KEY_WORDS + (n * 3 + ch) * 16
                m.mem_write(addr, pack(c[offset:offset + 16]))
                pointers.append(addr if c[PRESENT_WORDS + n * 3 + ch] else 0)
            m.mem_write(NDATA + n * channels * 8, pack(pointers + [3] * channels))
        m.mem_write(ACTION, pack([OBJECTS + (c[6] - 1) * 52 if c[6] else 0, MOTION]))
        m.mem_write(MOTION, struct.pack('<IIHH', NDATA, c[5], c[4], channels))
        for at, val in [(0x3d0f818, FRAMES + (c[9] - 1) * 64),
                        (0x3d0fa28, c[10]), (0x3d0fa2c, c[9])]:
            m.mem_write(at, pack([val]))
        mode = c[0]
        if mode == 0:
            entry, args = 0x4b82d0, [STOP, c[2], DEST]
        elif mode == 1:
            entry, args = 0x4b81f0, [STOP, BASE if c[7] else 0, ACTION, c[3], c[2], DEST if c[8] else 0]
        elif mode == 2:
            entry, args = 0x788ad0, [STOP]
            m.reg_write(UC_X86_REG_ECX, DEST)
            m.reg_write(UC_X86_REG_EDX, DEST if c[8] else BASE)
        else:
            entry, args = 0x781770, [STOP, c[14], c[15]]
            m.reg_write(UC_X86_REG_ECX, DEST)
            m.reg_write(UC_X86_REG_EDX, c[13])
        m.mem_write(STACK, pack(args))
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.reg_write(UC_X86_REG_FPCW, c[1])
        m.reg_write(UC_X86_REG_FPSW, 0)
        try:
            m.emu_start(entry, STOP, count=100000)
        except uc.UcError:
            self.failure = True
        if not self.failure:
            assert m.reg_read(UC_X86_REG_EIP) == STOP, hex(m.reg_read(UC_X86_REG_EIP))
            assert m.reg_read(UC_X86_REG_ESP) == STACK + (12 if mode == 3 else 4)
        def read(at, count):
            return list(struct.unpack('<' + 'I' * count, m.mem_read(at, count * 4)))
        result = 0 if self.failure else m.reg_read(UC_X86_REG_EAX) if mode < 2 else 1
        return [result, *read(COUNT, 1), *read(LIMIT, 1), *read(0x3d0fa2c, 1),
                m.reg_read(UC_X86_REG_FPSW) & 63, int(self.failure),
                *read(BUFFER, 2048), *read(FRAMES, 128), *read(BASE, 16), *read(DEST, 16)]

def fixtures(count, seed):
    rng = random.Random(seed)
    def make(mode=1):
        c = [0] * WORDS
        c[:16] = [mode, 127, 7, word(1.25), 3, 4, 1, 1, 1, 1, 8, 5, 6, 123, 456, 789]
        for i in range(MATRICES, OBJECT_WORDS):
            c[i] = word(rng.uniform(-8, 8))
        for n in range(NODES):
            offset = OBJECT_WORDS + n * 13
            c[offset:offset + 13] = [0, 0, word(n + 0.25), word(-n - 2), word(n * 2 + 3),
                rng.randrange(65536), rng.randrange(65536), rng.randrange(65536),
                word(0.75 + n / 8), word(-1.5), word(2.25), n + 2 if n < 7 else 0, 0]
            for ch in range(3):
                c[PRESENT_WORDS + n * 3 + ch] = 1
                for k, frame in enumerate([0, 2, 3, 4]):
                    start = KEY_WORDS + (n * 3 + ch) * 16 + k * 4
                    vals = [rng.randrange(65536) for _ in range(3)] if ch == 1 else [word(rng.uniform(-3, 3)) for _ in range(3)]
                    c[start:start + 4] = [frame, *vals]
        return c
    cases = []
    for total in (0, 1, 2, 127, 128):
        for index in (0, 1, 54, 126, 127, 128, 0x80000000, 0xffffffff):
            c = make(0); c[11], c[2] = total, index; cases.append(c)
    for index in range(128):
        c = make(0); c[11], c[2] = 128, index; cases.append(c)
    for axis in range(3):
        for angle in (0, 1, 15, 16, 16384, 32768, 49152, 65535, 65536,
                      0xffffffff, 0xffff0000, 0x80000000, 0x7fffffff):
            c = make(3); c[13:16] = [0, 0, 0]; c[13 + axis] = angle; cases.append(c)
    for mode in (2, 3):
        for _ in range(80):
            c = make(mode); c[8] = rng.randrange(2); cases.append(c)
        for raw in (0, 0x80000000, 1, 0x80000001, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345):
            for axis in range(16):
                c = make(mode); c[DEST_WORDS + axis] = raw; cases.append(c)
    for layout in (3, 7):
        for absent in range(8):
            for flags in (0, 1, 2, 4, 7, 32):
                c = make(); c[4] = layout
                for n in range(NODES):
                    c[OBJECT_WORDS + n * 13] = flags
                    for ch in range(3): c[PRESENT_WORDS + n * 3 + ch] = int(not (absent & (1 << ch)))
                cases.append(c)
        for requested in (0, 1, 6, 7, 8, 127, 128, 0xffffffff):
            for depth, capacity in ((1, 8), (2, 8), (7, 8), (8, 8), (1, 1)):
                c = make(); c[4], c[2], c[9], c[10] = layout, requested, depth, capacity
                if requested >= 128: c[8] = 0
                cases.append(c)
        for requested in (128, 0x80000000, 0xffffffff):
            c = make(); c[4], c[2] = layout, requested; cases.append(c)
        for frame in (-5.25, -1, -0.0, 0, 0.5, 1, 2, 2.5, 3, 3.5, 4, 5.75, 16777216):
            c = make(); c[4], c[3] = layout, word(frame); cases.append(c)
        for failure in ('zero-period', 'null-root', 'unsupported'):
            c = make(); c[4] = layout
            c[{'zero-period': 5, 'null-root': 6, 'unsupported': 4}[failure]] = 0
            cases.append(c)
    # Root sibling must remain unvisited; child siblings must be visited.
    for root_sibling in (0, 8):
        c = make(); c[OBJECT_WORDS + 12] = root_sibling
        c[OBJECT_WORDS + 13 + 11] = 0
        c[OBJECT_WORDS + 13 + 12] = 3
        cases.append(c)
    for _ in range(count):
        c = make(rng.choice([0, 1, 1, 2, 3])); c[1] = rng.choice([127, 639, 895, 3199])
        c[4], c[3] = rng.choice([3, 7]), word(rng.uniform(-16, 16))
        c[2], c[7], c[8] = rng.randrange(8), rng.randrange(2), rng.randrange(2)
        for n in range(NODES):
            c[OBJECT_WORDS + n * 13] = rng.randrange(64)
            for ch in range(3): c[PRESENT_WORDS + n * 3 + ch] = rng.randrange(2)
        cases.append(c)
    return cases

def run(exe, library, count, seed, output):
    image = PEImage(exe)
    assert image.sha256 == SOURCE_SHA256 and uc.__version__ == '2.1.4'
    lib = ctypes.CDLL(str(Path(library).resolve()))
    assert lib.sadx_model_matrix_case_words() == WORDS, lib.sadx_model_matrix_case_words()
    size = lib.sadx_model_matrix_output_words()
    table = ctypes.create_string_buffer(image.read(TABLE_ADDRESS, 4100))
    unit = ctypes.create_string_buffer(image.read(UNIT, 64))
    lib.sadx_native_model_matrices.argtypes = [ctypes.c_void_p] * 4
    source = Source(image)
    cases = fixtures(count, seed)
    mismatches, partials, observations, failures = [], [], [], 0
    for index, case in enumerate(cases):
        expected = source.run(case)
        out = (ctypes.c_uint32 * size)()
        raw = (ctypes.c_uint32 * WORDS)(*case)
        assert lib.sadx_native_model_matrices(raw, table, unit, out) == 0
        actual = list(out)
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        failures += expected[5]
        row = {'case': index, 'mode': case[0], 'control_word': case[1],
               'source_flags': expected[4], 'native_flags': actual[4]}
        if any(i != 4 for i in differences):
            mismatches.append({**row, 'differences': [[i, hex(expected[i]), hex(actual[i])] for i in differences[:20]]})
        elif differences:
            partials.append(row)
        observations.append(pack(expected) + pack(actual))
    report = {'source_sha256': image.sha256, 'cases': len(cases), 'failure_prefixes': failures,
              'value_mismatches': len(mismatches), 'mismatches': mismatches,
              'individual_fp_partials': partials, 'fp_partials': len(partials),
              'flag_mismatches': len(partials), 'passed': not mismatches and not partials,
              'values_match': not mismatches, 'fixture_sha256': sha(b''.join(pack(c) for c in cases)),
              'observations_sha256': sha(b''.join(observations)), 'seed': seed,
              'limits': ['Isolated source instructions; runtime initialization/order remains separate.',
                         'Unicorn x87 status omits precision flags raised by maintained native arithmetic; individual partials retained.',
                         'No hardware FP parity, active weld/attachment or actor-frame claim.',
                         'Failure prefixes compare count/limit/stack/storage, not exact hardware fault PCs.',
                         'Fixtures declare four keys/channel; source is stopped at the first undeclared key-frame read.',
                         'Fixtures use independent typed resources; arbitrary resource/output aliases unqualified.']}
    output = Path(output); output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    output.with_suffix('.observations.bin').write_bytes(b''.join(observations))
    print(f'{len(cases)} cases, {len(mismatches)} value mismatches, {len(partials)} FP partials, {failures} failure prefixes')
    return 1 if mismatches else 2 if partials else 0

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe'); p.add_argument('library'); p.add_argument('--output', required=True)
    p.add_argument('--cases', type=int, default=384); p.add_argument('--seed', type=lambda x: int(x, 0), default=0x4b81f0)
    a = p.parse_args()
    return run(a.exe, a.library, a.cases, a.seed, a.output)

if __name__ == '__main__':
    raise SystemExit(main())
