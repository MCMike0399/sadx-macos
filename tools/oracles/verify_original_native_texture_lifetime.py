"""Texture pool/list release and native lifetime ownership against original instructions.

Valid allocated texture objects only. D3D SetTexture/COM Release and CRT free are
receipts; unbind failure stops before the external call returns. No surface/data
bytes or ignored HRESULTs are compared. Native in-flight retention is checked by
the unit test, not by substituting guessed source image bytes. Genuine source
uninitialized acquisition fields TSP/texture parameters/palette version are
reported undefined. Acquire executes original PVR parse/memory descriptor and
conversion selection; native D3D image creation is an explicit upload receipt.
Registration currently supports valid empty slots and cached records; orphaned
zero-count allocations from reference-count wrap are explicitly unsupported.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_ECX
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original, STACK, STOP

BASE = 0x22000000
POOL = BASE + 0x1000
LIST = BASE + 0x2000
NAMES = BASE + 0x2100
DEVICE = BASE + 0x3000
VTABLE = BASE + 0x3100
UNBIND = BASE + 0x9000
RELEASE = BASE + 0x9100

class Source:
    def __init__(self, image):
        self.m = m = Original(image, 127).machine
        for section in image.sections:
            if section['name'] in ('.text', '.rdata'):
                continue
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            m.mem_map(image.base + section['rva'], size)
            m.mem_write(image.base + section['rva'], image.slice(section['file_offset'], section['raw_size']))
        m.mem_map(BASE, 0x10000)
        # The successful Windows SEH wrappers read FS:[0]; no null-dereference
        # fixture is accepted, so this TEB fixture cannot conceal one.
        m.mem_map(0, 4096)
        self.put(DEVICE, VTABLE)
        self.put(VTABLE + 0xf4, UNBIND)
        self.put(VTABLE + 8, RELEASE)
        m.mem_write(UNBIND, b'\xc2\x0c\x00')
        m.mem_write(RELEASE, b'\xc2\x04\x00')
        self.put(0x3d128b0, DEVICE)
        self.clean = m.context_save()
        for at in (UNBIND, RELEASE, 0x6443e2, 0x78c880, 0x78ca00):
            m.hook_add(unicorn.UC_HOOK_CODE, self.hook, begin=at, end=at)

    def get(self, at, size=4):
        return int.from_bytes(self.m.mem_read(at, size), 'little')

    def put(self, at, value, size=4):
        self.m.mem_write(at, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))

    def hook(self, m, at, size, unused):
        esp = m.reg_read(UC_X86_REG_ESP)
        cached = self.get(0x3d0fdf4)
        if at in (0x78c880, 0x78ca00):
            self.uploaded = True
            m.reg_write(UC_X86_REG_EAX, BASE + 0x4000)
            m.reg_write(UC_X86_REG_EIP, self.get(esp))
            m.reg_write(UC_X86_REG_ESP, esp + 4)
            return
        if at == UNBIND:
            assert self.get(esp + 8) == 0 and self.get(esp + 12) == 0
            self.trace += [1, 0, cached]
            if self.values[5]:
                self.failed = True
                m.emu_stop()
        elif at == RELEASE:
            self.trace += [2, (self.get(esp + 4) - BASE - 0x4000) // 32, cached]
        else:
            self.trace += [3, (self.get(esp + 4) - BASE - 0x5000) // 32, cached]
            m.reg_write(UC_X86_REG_EIP, self.get(esp))
            m.reg_write(UC_X86_REG_ESP, esp + 4)

    def run(self, values):
        self.values = values
        op, count = values[:2]
        m = self.m
        m.context_restore(self.clean)
        self.trace = []
        self.failed = False
        self.put(0, 0)
        self.put(0x3d0f80c, count)
        self.put(0x3d0f844, POOL)
        self.put(0x3d0fdf4, values[2])
        self.put(0x3d129a0, POOL + values[3] * 68 if values[3] < count else 0)
        self.put(0x3d0fa24, LIST if values[4] else 0)
        self.put(0x3ab98ac, values[152])
        self.put(0x3b0e334, values[153])
        m.mem_write(0x3d0f7c0, struct.pack('<8I', *values[8:16]))
        self.put(LIST, NAMES)
        self.put(LIST + 4, count)
        for index in range(count):
            words = values[16 + index * 17:33 + index * 17].copy()
            surface = BASE + 0x4000 + index * 32
            physical = BASE + 0x5000 + index * 32
            self.put(surface, VTABLE)
            words[13] = surface if words[13] else 0
            words[15] = physical if words[15] else 0
            m.mem_write(POOL + index * 68, struct.pack('<17I', *words))
            self.put(NAMES + index * 12, 0xffffffff if values[7] and index == 0 else 123)
            self.put(NAMES + index * 12 + 8, 0 if values[6] & (1 << index) else POOL + index * 68)
        self.put(STACK, STOP)
        self.put(STACK + 4, values[154])
        m.reg_write(UC_X86_REG_ESP, STACK)
        if op == 3:
            m.reg_write(UC_X86_REG_ECX, POOL + values[154] * 68 if values[154] < count else 0)
        if op in (5, 6):
            self.put(STACK + 4, LIST)
        entry = [0x77fb30, 0x77fc50, 0x406f00, 0x78cd50, 0x77f5b0, 0x77fa70, 0x403290][op]
        m.emu_start(entry, STOP, count=50000)
        if not self.failed and m.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError('Original did not return')
        out = [0] * 512
        if op == 4:
            found = m.reg_read(UC_X86_REG_EAX)
            out[0] = (found - POOL) // 68 + 1 if found else 0
        if op in (5, 6) and not self.failed:
            out[0] = m.reg_read(UC_X86_REG_EAX)
        active = self.get(0x3d129a0)
        out[1:8] = [self.get(0x3d0fdf4), (active - POOL) // 68 + 1 if active else 0,
                    int(self.get(0x3d0fa24) != 0), self.get(0x3ab98ac), self.get(0x3b0e334),
                    int(self.failed), len(self.trace) // 3]
        out[8:16] = struct.unpack('<8I', m.mem_read(0x3d0f7c0, 32))
        for index in range(count):
            words = list(struct.unpack('<17I', m.mem_read(POOL + index * 68, 68)))
            original = values[16 + index * 17:33 + index * 17]
            words[13] = original[13] if words[13] else 0
            words[15] = original[15] if words[15] else 0
            out[16 + index * 17:33 + index * 17] = words
        out[200:200 + len(self.trace)] = self.trace
        return out

    def prepare(self, values):
        self.values = values
        self.uploaded = False
        m = self.m
        m.context_restore(self.clean)
        count = values[7]
        self.put(0x3d0f80c, count)
        self.put(0x3d0f844, POOL)
        self.put(0, 0)
        for index in range(count):
            initial = [0xffffffff, 0xffffffff] + [0] * 14 + [0xffff0000]
            if index == 2 and values[8]:
                initial[0] = values[1]
                initial[16] |= values[5] & 65535
                initial[9] = values[6]
            m.mem_write(POOL + index * 68, struct.pack('<17I', *initial))
        descriptor = BASE + 0x6000
        image = BASE + 0x7000
        if values[0] == 0:
            width, height = values[4] & 65535, values[9] & 65535
            header = b'PVRT' + struct.pack('<IIHH', 72, values[2] | (values[3] << 8), width, height)
            m.mem_write(image, header + bytes(64))
            argument = image
            entry = 0x77fbd0
        else:
            info = [image, 151388160, 0, 0, values[4], values[9], 0, 0, 0, 0, 0]
            m.mem_write(descriptor, struct.pack('<11I', *info))
            argument = descriptor
            entry = 0x77f7f0
        self.put(STACK, STOP)
        self.put(STACK + 4, argument)
        self.put(STACK + 8, values[1])
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.emu_start(entry, STOP, count=100000)
        if m.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError('Original acquire did not return')
        address = m.reg_read(UC_X86_REG_EAX)
        if not address:
            return [0] * 32
        words = list(struct.unpack('<17I', m.mem_read(address, 68)))
        # The source stack has no stores for TSP, texture parameters or palette
        # version. Report their definedness instead of inventing an expected value.
        defined = not self.uploaded
        values = [words[0], words[1], int(defined), int(defined), words[4], int(words[5] != 0),
                  words[6], words[7], words[8], words[9], words[10], words[11], words[12],
                  int(words[13] != 0), int(words[14] != 0), int(words[15] != 0),
                  words[16] & 65535, int(defined)]
        return [(address - POOL) // 68 + 1] + values + [0] * 13


def compare_preparation(image, library, seed=0x77fbd0, repetitions=512):
    rng = random.Random(seed)
    source = Source(image)
    fn = library.sadx_native_texture_prepare
    bad = []
    for index in range(repetitions):
        values = [index % 2, rng.getrandbits(32), index % 3, rng.choice([1, 2, 3, 9]),
                  rng.randrange(1, 2048), rng.choice([0, 1, 2, 65535]), rng.getrandbits(32),
                  index % 5, index % 2, rng.randrange(1, 2048)]
        out = (ctypes.c_uint32 * 32)()
        fn((ctypes.c_uint32 * 10)(*values), out)
        expected = source.prepare(values)
        if list(out) != expected:
            bad.append({'case': index, 'input': values,
                        'differences': [[i, a, b] for i, (a, b) in enumerate(zip(out, expected)) if a != b]})
    return repetitions, bad


def compare_initial_records(image, library):
    source = Source(image)
    m = source.m
    source.put(STACK, STOP)
    source.put(STACK + 4, POOL)
    source.put(STACK + 8, 8)
    m.reg_write(UC_X86_REG_ESP, STACK)
    m.emu_start(0x77f610, STOP, count=10000)
    if m.reg_read(UC_X86_REG_EIP) != STOP:
        raise RuntimeError('Original pool initializer did not return')
    out = (ctypes.c_uint32 * 17)()
    library.sadx_native_texture_empty_record(out)
    bad = []
    for index in range(8):
        expected = list(struct.unpack('<17I', m.mem_read(POOL + index * 68, 68)))
        if list(out) != expected:
            bad.append({'kind': 'empty_record', 'index': index,
                        'differences': [[i, a, b] for i, (a, b) in enumerate(zip(out, expected)) if a != b]})
    return 8, bad


def compare(image, library, seed=0x77fb30, repetitions=128):
    rng = random.Random(seed)
    source = Source(image)
    fn = library.sadx_native_texture_lifetime
    bad = []
    cases = []
    for op in range(7):
        for n in range(repetitions):
            values = [rng.getrandbits(32) for _ in range(160)]
            count = n % 9
            values[:8] = [op, count, rng.choice([0xffffffff, n % 5]), rng.randrange(10),
                          n % 2, n % 3 == 0, 0 if op == 5 else rng.getrandbits(8), n % 11 == 0]
            values[154] = rng.randrange(10) if op == 3 else rng.choice([0xffffffff, rng.getrandbits(32), n % 5])
            for index in range(count):
                at = 16 + index * 17
                values[at] = rng.choice([0xffffffff, n % 5, rng.getrandbits(32)])
                values[at + 13] = rng.randrange(2)
                values[at + 15] = rng.randrange(2)
                values[at + 16] = (rng.getrandbits(16) << 16) | rng.choice([0, 1, 2, 3, 65535])
            cases.append(values)
    for index, values in enumerate(cases):
        out = (ctypes.c_uint32 * 512)()
        fn((ctypes.c_uint32 * 160)(*values), out)
        expected = source.run(values)
        if list(out) != expected:
            bad.append({'case': index, 'op': values[0], 'input': values,
                        'differences': [[i, a, b] for i, (a, b) in enumerate(zip(out, expected)) if a != b][:16]})
    return len(cases), bad


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('Pinned source/Unicorn mismatch')
    library = ctypes.CDLL(str(args.native_library.resolve()))
    count, bad = compare(image, library)
    prepared_count, prepared_bad = compare_preparation(image, library)
    count += prepared_count
    bad += prepared_bad
    initial_count, initial_bad = compare_initial_records(image, library)
    count += initial_count
    bad += initial_bad
    sha = lambda data: hashlib.sha256(data).hexdigest()
    root = Path(__file__).resolve().parents[2]
    paths = ['src/engine/native_texture_lifetime.hpp', 'src/engine/native_texture_lifetime.cpp',
             'tests/probes/native_native_texture_lifetime_probe.cpp']
    ranges = [(0x77fb30, 0x77fbc1), (0x77fc50, 0x77fc80), (0x406f00, 0x406f64),
              (0x40d3b0, 0x40d3bb), (0x78cd50, 0x78cdb8), (0x77f5b0, 0x77f607),
              (0x77f610, 0x77f6d5), (0x77f7f0, 0x77f90c), (0x78cbd0, 0x78cd42), (0x77f910, 0x77f9c2), (0x77fbd0, 0x77fc4b), (0x77f2f0, 0x77f3d0),
              (0x77fa70, 0x77fb21), (0x403290, 0x403328), (0x78bba0, 0x78bbd0)]
    report = dict(source_sha256=image.sha256, cases=count, mismatch_count=len(bad),
                  mismatches=bad[:12], values_order_and_failure_prefix_match=not bad,
                  native_library_sha256=sha(args.native_library.read_bytes()),
                  native_sources={path: sha((root / path).read_bytes()) for path in paths},
                  source_ranges={f'{hex(a)}-{hex(b)}': sha(image.read(a, b - a)) for a, b in ranges},
                  limits=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': count, 'mismatches': len(bad)}))
    return int(bool(bad))

if __name__ == '__main__':
    raise SystemExit(main())
