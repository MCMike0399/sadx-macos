"""Stage/character light and palette selection against original instructions.

Integer control and byte copies only: arbitrary binary32 payloads are retained.
Game-owned sequence/hotel queries are explicit receipts for time adjustment;
SeqGetTime itself is separately executed with null/non-null source pointers.
The current title path needs no hotel query. No hotel geometry, light-palette
shading, GPU upload or rendered pixels are certified by this CPU-state judge.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESP
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original, STACK, STOP

SCRATCH = 0x22000000
TABLE_RANGES = [(0x9008b4, 48), (0x9008e4, 48), (0x900e88, 256 * 48), (0x903e88, 256 * 96)]
ACTIVE_RANGES = [(0x3abd9f8, 192), (0x3abdab8, 48), (0x3abdaf0, 96)]
ENTRIES = [0x40a8a0, 0x40a950, 0x40aa30, 0x40aab0, 0x40ab60, 0x40ad60,
           0x40aeb0, 0x40aee0, 0x40a420, 0x40a4a0, 0x412c10]

class Source:
    def __init__(self, image):
        self.m = m = Original(image, 127).machine
        for section in image.sections:
            if section['name'] in ('.text', '.rdata'):
                continue
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            m.mem_map(image.base + section['rva'], size)
            m.mem_write(image.base + section['rva'], image.slice(section['file_offset'], section['raw_size']))
        m.mem_map(SCRATCH, 4096)
        self.clean = m.context_save()
        for address in (0x412c10, 0x62ea30):
            m.hook_add(unicorn.UC_HOOK_CODE, self.query, begin=address, end=address)

    def put(self, address, value, size=4):
        self.m.mem_write(address, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))

    def get(self, address, size=4):
        return int.from_bytes(self.m.mem_read(address, size), 'little')

    def query(self, m, address, size, unused):
        if self.values[0] == 10:
            return
        kind = 1 if address == 0x412c10 else 2
        self.calls[kind] += 1
        if self.values[10] and sum(self.calls) == self.values[10]:
            self.failure = kind
            m.emu_stop()
            return
        result = (self.values[7] + self.calls[1] - 1) & 0xffffffff if kind == 1 else self.values[8]
        m.reg_write(UC_X86_REG_EAX, result)
        esp = m.reg_read(UC_X86_REG_ESP)
        m.reg_write(UC_X86_REG_EIP, self.get(esp))
        m.reg_write(UC_X86_REG_ESP, esp + 4)

    def run(self, values, data):
        self.values = values
        self.calls = [0, 0, 0]
        self.failure = 0
        m = self.m
        m.context_restore(self.clean)
        cursor = 0
        for address, size in TABLE_RANGES:
            m.mem_write(address, data[cursor:cursor + size])
            cursor += size
        for address, size in ACTIVE_RANGES:
            m.mem_write(address, bytes([165]) * size)
        self.put(0x3abdbc1, 165, 1)
        for address in (0x3abdae8, 0x3abd9f0, 0x3abdbbc):
            self.put(address, 2779096485)
        self.put(0x3b22dcc, values[5], 2)
        self.put(0x3b22dec, values[6], 2)
        self.put(0x3b22dc0, 0, 2)
        self.put(SCRATCH, values[2])
        self.put(SCRATCH + 4, values[3])
        self.put(SCRATCH + 8, values[7], 1)
        self.put(0x3b0ef4c, SCRATCH + 8 if values[11] else 0)
        self.put(0x3b2a2e8, values[12])
        self.put(STACK, STOP)
        for index, value in enumerate(values[2:5]):
            self.put(STACK + 4 + index * 4, value)
        if values[0] == 2:
            m.reg_write(UC_X86_REG_EAX, values[4])
        if values[0] == 8:
            m.reg_write(UC_X86_REG_EBX, SCRATCH)
            m.reg_write(UC_X86_REG_EDI, SCRATCH + 4)
        if values[0] == 9:
            self.put(STACK + 4, SCRATCH)
            self.put(STACK + 8, SCRATCH + 4)
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.emu_start(ENTRIES[values[0]], STOP, count=1000000)
        if not self.failure and m.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError('Source did not return')
        out = [0] * 9400
        if values[0] in (4, 10) and not self.failure:
            out[0] = m.reg_read(UC_X86_REG_EAX)
        out[1:7] = [self.get(SCRATCH), self.get(SCRATCH + 4), self.get(0x3b22dcc, 2),
                    self.get(0x3b22dec, 2), self.calls[1], self.calls[2]]
        out[7] = self.failure
        if values[0] == 2:
            pointer = m.reg_read(UC_X86_REG_EAX)
            out[8] = (pointer - 0x903e88) // 96 + 1 if pointer else 0
        out[16:20] = [self.get(0x3abdbc1, 1), self.get(0x3abdae8), self.get(0x3abd9f0), self.get(0x3abdbbc)]
        cursor = 32
        for address, size in TABLE_RANGES + ACTIVE_RANGES:
            words = struct.unpack('<' + str(size // 4) + 'I', m.mem_read(address, size))
            out[cursor:cursor + len(words)] = words
            cursor += len(words)
        return out


def fixtures(seed=0x40aeb0, repetitions=64):
    rng = random.Random(seed)
    stages = [0, 1, 26, 29, 33, 255, 256, 0xffffffff]
    acts = [0, 1, 2, 3, 4, 5, 255, 256, 0xffffffff, 0x80000000]
    selectors = [0, 1, 6, 8, 255, 0xffffffff, 0xfffffffa, 0xfffffff8, 0x80000000]
    for op in range(11):
        for index in range(repetitions):
            stage, act = rng.choice(stages), rng.choice(acts)
            initial_stage = rng.choice([0, 26, 29, 33, rng.getrandbits(16)])
            initial_act = rng.choice(acts) & 65535
            values = [op, rng.getrandbits(32), stage, act, rng.choice(selectors),
                      initial_stage, initial_act, rng.choice([0, 1, 2, 127, 128, 255]),
                      index % 4, 0, index % 5 if index % 3 == 0 else 0, index % 2,
                      rng.choice([0, 18, 19, rng.getrandbits(32)])]
            data = bytearray(rng.randbytes(36960))
            for row in range(256):
                at = 96 + row * 48
                struct.pack_into('<4B', data, at, stage & 255 if row % 4 == 0 else row % 7,
                                 act & 255 if row % 3 else 0, rng.randrange(256), rng.randrange(256))
                at = 96 + 256 * 48 + row * 96
                struct.pack_into('<4B', data, at, stage & 255 if row % 4 == 0 else row % 7,
                                 act & 255 if row % 3 else 0, rng.choice([0, 6, 8, row % 6]), rng.randrange(256))
            data[96 + (index * 17 % 256) * 48] = 255
            data[96 + 256 * 48 + (index * 37 % 256) * 96] = 255
            yield values, bytes(data)


def compare(image, library, seed=0x40aeb0, repetitions=64):
    source = Source(image)
    fn = library.sadx_native_stage_lighting
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint32)]
    fn.restype = ctypes.c_int
    bad = []
    count = 0
    for values, data in fixtures(seed, repetitions):
        out = (ctypes.c_uint32 * 9400)()
        if fn((ctypes.c_uint32 * 13)(*values), (ctypes.c_uint8 * len(data)).from_buffer_copy(data), out) != 1:
            raise RuntimeError('Native rejected')
        expected = source.run(values, data)
        count += 1
        if list(out) != expected:
            bad.append({'input': values, 'differences': [[i, a, b] for i, (a, b) in enumerate(zip(out, expected)) if a != b][:16]})
    return count, bad


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('Pinned source/Unicorn mismatch')
    count, bad = compare(image, ctypes.CDLL(str(args.native_library.resolve())))
    sha = lambda data: hashlib.sha256(data).hexdigest()
    root = Path(__file__).resolve().parents[2]
    names = ['src/game/stage_lighting.hpp', 'src/game/stage_lighting.cpp',
             'tests/probes/native_stage_lighting_probe.cpp']
    report = dict(source_sha256=image.sha256, cases=count, mismatch_count=len(bad), mismatches=bad[:12],
                  selected_bytes_and_query_boundaries_match=not bad,
                  native_library_sha256=sha(args.native_library.read_bytes()),
                  native_sources={name: sha((root / name).read_bytes()) for name in names},
                  source_ranges={f'{hex(a)}-{hex(b)}': sha(image.read(a, b - a)) for a, b in
                      [(0x40a420, 0x40a496), (0x40a4a0, 0x40a50e), (0x40a8a0, 0x40a8f0),
                       (0x40a950, 0x40aa24), (0x40aa30, 0x40aa5d), (0x40aa60, 0x40aaab), (0x40aab0, 0x40ab58),
                       (0x40ab60, 0x40ac08), (0x40ad60, 0x40adb6), (0x40aeb0, 0x40aed4),
                       (0x40aee0, 0x40af05), (0x412c10, 0x412c28)]}, limits=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': count, 'mismatches': len(bad)}))
    return int(bool(bad))

if __name__ == '__main__':
    raise SystemExit(main())
