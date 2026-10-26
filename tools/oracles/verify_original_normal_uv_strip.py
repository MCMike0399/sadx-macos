"""Original normal+UV strip / PolyBuff instruction judge, including ordered failure prefixes.

Only COM/CRT allocation boundaries are receipts. All eight selected game bodies execute.
Input ranges model available imported data; a read outside one reports the actual source
instruction/address rather than manufacturing values. No original judge is changed.
"""
import argparse
import ctypes
import hashlib
import json
import random
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles')]
import unicorn
from unicorn.x86_const import *
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP, CODE

INPUT_WORDS, OUTPUT_WORDS = 10784, 262144
MESH_INPUT, UV_INPUT, POINTS_INPUT, NORMALS_INPUT, ARGS_INPUT = 32, 2080, 6176, 8224, 10272
BUFFER, ARGS, MESH, UV = 0x3d0ff20, DATA + 0x3000, DATA + 0x5000, DATA + 0x7000
DEVICE, RESOURCE, VTABLE, RVTABLE = DATA + 0x1000, DATA + 0x2000, DATA + 0x1100, DATA + 0x2100
POINTS, NORMALS, BYTES = 0x12100000, 0x12300000, 0x12500000
ENTRIES = [0x7835f0, 0x794630, 0x794760, 0x794540, 0x7945c0, 0x794800, 0x794600, 0x794750]
RANGES = [(0x7835f0, 0x783822), (0x794540, 0x7945b8), (0x7945c0, 0x7945f2),
          (0x794600, 0x79462e), (0x794630, 0x7946b1), (0x794750, 0x794759),
          (0x794760, 0x7947ac), (0x794800, 0x794835)]

class Boundary(Exception):
    pass

def u32(n):
    return n & 0xffffffff

def s32(n):
    return ctypes.c_int32(n).value

def bits(f):
    return struct.unpack('<I', struct.pack('<f', f))[0]

class Source:
    def __init__(self, image):
        self.m = Original(image, 127).machine
        m = self.m
        for section in image.sections:
            if section['name'] in ('.text', '.rdata'):
                continue
            size = (max(section['raw_size'], section['virtual_size']) + 4095) & ~4095
            m.mem_map(image.base + section['rva'], size)
            if section['raw_size']:
                m.mem_write(image.base + section['rva'], image.slice(section['file_offset'], section['raw_size']))
        for base in (POINTS, NORMALS):
            m.mem_map(base - 0x80000, 0x100000)
        m.mem_map(BYTES, 0x100000)
        slots = {0x130: 1, 0x14c: 2, 0xc8: 5, 0x118: 6, 0x5c: 7}
        self.api_addresses = {}
        for offset, kind in slots.items():
            address = CODE + 0x200 + kind * 16
            self.put(VTABLE + offset, address)
            self.api_addresses[address] = kind
        for offset, kind in ((0x2c, 3), (0x30, 4), (8, 9)):
            address = CODE + 0x200 + kind * 16
            self.put(RVTABLE + offset, address)
            self.api_addresses[address] = kind
        self.api_addresses[0x645002] = 8
        self.api_addresses[0x644e51] = 10
        self.put(DEVICE, VTABLE)
        self.put(RESOURCE, RVTABLE)
        self.put(0x3d128b0, DEVICE)
        for address in self.api_addresses:
            m.hook_add(unicorn.UC_HOOK_CODE, self.api, begin=address, end=address)
        m.hook_add(unicorn.UC_HOOK_MEM_READ | unicorn.UC_HOOK_MEM_WRITE, self.memory)
        m.hook_add(unicorn.UC_HOOK_MEM_INVALID, self.invalid)
        m.hook_add(unicorn.UC_HOOK_INTR, self.interrupt)
        self.initial = m.context_save()
        self.boundaries = set()

    def word(self, address):
        return int.from_bytes(self.m.mem_read(address, 4), 'little')

    def put(self, address, value):
        self.m.mem_write(address, struct.pack('<I', u32(value)))

    def row(self, *values):
        self.trace.append(list(values) + [0] * (6 - len(values)))

    def fault(self, kind, index, address):
        if self.failed:
            raise Boundary()
        self.failed = True
        self.boundaries.add((self.m.reg_read(UC_X86_REG_EIP), address, kind, u32(index)))
        self.row(5, kind, u32(index))
        raise Boundary()

    def interrupt(self, m, vector, user):
        if vector != 0:
            raise RuntimeError(f'unexpected interrupt {vector}')
        self.row(5, 7)
        raise Boundary()

    def invalid(self, m, access, address, size, value, user):
        ip = m.reg_read(UC_X86_REG_EIP)
        if (self.v[10] & (64 | 8)) and (0x794540 <= ip < 0x7945b8 or 0x794630 <= ip < 0x7946b1 or 0x794760 <= ip < 0x7947ac):
            self.fault(4, address // 4, address)
        if access in (unicorn.UC_MEM_WRITE_UNMAPPED, unicorn.UC_MEM_WRITE_PROT):
            self.fault(5, address, address)
        self.fault(6, 0, address)

    def memory(self, m, access, address, size, value, user):
        ip = m.reg_read(UC_X86_REG_EIP)
        argument_operations = (0x79459a, 0x7945a0, 0x794680, 0x794683, 0x794689,
                               0x794770, 0x794784, 0x79478f)
        if ip in argument_operations:
            # A wrapped source row can point before/elsewhere than this allocation.
            # Such bytes are an absent fact, never manufactured mapped zeroes.
            index = u32(address - self.word(BUFFER + 20)) // 4
            available = self.regions[-1][3]
            if index >= available:
                self.fault(4, index, address)
            if access == unicorn.UC_MEM_WRITE:
                self.row(2, index, value)
            return
        if BUFFER <= address < BUFFER + 36 and access == unicorn.UC_MEM_WRITE:
            field = (address - BUFFER) // 4
            normalized = int(value != 0) if field in (0, 5) else value
            self.row(1, field, normalized)
        for base, width, kind, count, first in self.regions:
            # Guard ranges cover the full signed16 indexed-vector domain.
            radius = {0: 4096, 1: 8192, 2: 0x80000, 3: 0x80000, 4: 2048}[kind]
            before = 0x80000 if kind in (2, 3) else 8
            if base - before <= address < base + radius:
                index = (address - base) // width
                if index < first or index + (size + width - 1) // width > first + count:
                    self.fault(kind, index, address)
                if access == unicorn.UC_MEM_WRITE and kind == 4:
                    self.row(2, index, value)
                return
        if BYTES <= address < BYTES + 0x100000 and access == unicorn.UC_MEM_WRITE:
            offset = address - BYTES
            if offset // 4 >= self.v[11]:
                self.fault(5, offset, address)
            self.row(3, offset, value)

    def api(self, m, address, size, user):
        kind = self.api_addresses[address]
        sp = m.reg_read(UC_X86_REG_ESP)
        a = [self.word(sp + 4 + 4 * n) for n in range(6)]
        pop = {1: 12, 2: 20, 3: 24, 4: 8, 5: 16, 6: 20, 7: 28, 8: 4, 9: 8, 10: 4}[kind]
        if kind == 1:
            assert a[0] == DEVICE
            row = [4, kind, a[1]]
        elif kind == 2:
            assert a[0] == DEVICE
            row = [4, kind, int(a[2] != 0), a[1], a[3]]
        elif kind == 3:
            assert a[0] == RESOURCE
            row = [4, kind, a[1], a[2], a[4], 1]
        elif kind in (4, 9):
            assert a[0] == RESOURCE
            row = [4, kind, 1]
        elif kind in (5, 6):
            assert a[0] == DEVICE
            row = [4, kind, *a[1:3 if kind == 5 else 4]]
        elif kind == 7:
            assert a[0] == DEVICE and a[5] == BUFFER
            row = [4, kind, *a[1:5]]
        elif kind == 8:
            row = [4, kind, a[0]]
        else:
            assert a[0] == ARGS
            row = [4, kind, 1]
        self.row(*row)
        self.api_count += 1
        if self.api_count == self.v[20]:
            raise Boundary()
        result = 0x80004005 if self.v[10] & 2 else 0
        if kind == 3 and not self.v[10] & 1:
            self.put(a[3], BYTES + self.v[21])
        if kind == 7 and not self.v[10] & 4:
            self.put(BUFFER, RESOURCE)
            self.row(1, 0, 1)
        if kind == 8:
            result = 0 if self.v[10] & 8 else ARGS
            self.regions[-1] = (ARGS, 4, 4, 0 if self.v[10] & 8 else self.v[17], 0)
        m.reg_write(UC_X86_REG_EAX, result)
        m.reg_write(UC_X86_REG_EIP, self.word(sp))
        m.reg_write(UC_X86_REG_ESP, sp + pop)

    def run(self, v):
        self.v, self.trace, self.api_count, self.failed = v, [], 0, False
        m = self.m
        m.context_restore(self.initial)
        m.reg_write(UC_X86_REG_FPCW, v[9])
        self.regions = [(MESH, 2, 0, v[12], s32(v[23])), (UV, 2, 1, v[13], s32(v[24])),
                        (POINTS, 4, 2, v[15], s32(v[14])), (NORMALS, 4, 3, v[16], s32(v[25])),
                        (ARGS, 4, 4, 0 if v[10] & 64 else v[17], 0)]
        raw = [0 if v[10] & 16 else RESOURCE, v[3], v[4], v[2], v[6],
               0 if v[10] & 64 else ARGS, v[5], v[19], v[22]]
        m.mem_write(BUFFER, struct.pack('<9I', *raw))
        for base, start, count, width, first in (
            (MESH, MESH_INPUT, v[12], 2, s32(v[23])), (UV, UV_INPUT, v[13], 2, s32(v[24])),
            (POINTS, POINTS_INPUT, v[15], 4, s32(v[14])),
            (NORMALS, NORMALS_INPUT, v[16], 4, s32(v[25])), (ARGS, ARGS_INPUT, 512, 4, 0)):
            fmt = 'H' if width == 2 else 'I'
            m.mem_write(base + first * width, struct.pack('<' + fmt * count,
                        *[x & ((1 << (width * 8)) - 1) for x in v[start:start + count]]))
        m.mem_write(BYTES, b"\xcd" * (v[11] * 4))
        self.put(0x3d0848c, v[8])
        self.put(0x3d08490, v[7])
        mesh_record = DATA + 0x9000
        m.mem_write(mesh_record, struct.pack('<HH5I', 0, v[1] & 0xffff, MESH, 0, 0, 0, UV))
        sp = STACK - 128
        stack_args = {0: [mesh_record, POINTS, NORMALS], 1: [v[7]],
                      3: [v[2], v[6], v[19]]}.get(v[0], [])
        m.mem_write(sp, struct.pack('<' + 'I' * (1 + len(stack_args)), STOP, *stack_args))
        m.reg_write(UC_X86_REG_ESP, sp)
        m.reg_write(UC_X86_REG_ECX, 0 if v[10] & 32 and v[0] in (3, 4) else BUFFER)
        m.reg_write(UC_X86_REG_EDX, v[18])
        try:
            m.emu_start(ENTRIES[v[0]], STOP, count=2000000)
            assert m.reg_read(UC_X86_REG_EIP) == STOP, 'instruction budget exhausted'
            cleanup = 16 if v[0] == 3 else 8 if v[0] == 1 else 4
            assert m.reg_read(UC_X86_REG_ESP) == sp + cleanup
            if v[0] == 1:
                pointer = m.reg_read(UC_X86_REG_EAX)
                self.row(6, int(BYTES <= pointer < BYTES + 0x100000), pointer - BYTES if BYTES <= pointer < BYTES + 0x100000 else pointer)
            if v[0] == 7:
                self.row(6, m.reg_read(UC_X86_REG_EAX))
        except (Boundary, unicorn.UcError):
            if not self.trace or self.trace[-1][0] != 5:
                if self.api_count != self.v[20] or not self.v[20]:
                    raise
        raw = [self.word(BUFFER + n * 4) for n in range(9)]
        for n in (0, 5):
            raw[n] = int(raw[n] != 0)
        offsets = sorted({row[1] for row in self.trace if row[0] == 3})
        stored = [len(offsets)] + [word for offset in offsets for word in (offset, self.word(BYTES + offset))]
        argument_count = self.regions[-1][3] if self.word(BUFFER + 20) else 0
        arguments = [argument_count] + [self.word(ARGS + word * 4) for word in range(argument_count)]
        return raw + [m.reg_read(UC_X86_REG_FPSW) & 63, m.reg_read(UC_X86_REG_FPCW), len(self.trace)] + [x for row in self.trace for x in row] + stored + arguments

def base(mode=0):
    v = [0] * INPUT_WORDS
    v[:26] = [mode, 1, 36, 8192, 8192, 0, 0x152, 2, 0xa153728e, 0x7f, 0,
              16384, 4, 6, -12 & 0xffffffff, 48, 48, 512, 4, 77, 0, 0, 0x9bad, 0, 0, -12 & 0xffffffff]
    v[MESH_INPUT:MESH_INPUT+4] = [3, 0, 1, 2]
    v[UV_INPUT:UV_INPUT+6] = [1, -1 & 0xffff, 255, 256, -32768 & 0xffff, 32767]
    for n in range(48):
        v[POINTS_INPUT+n] = bits(n / 7 - 4)
        v[NORMALS_INPUT+n] = bits(9 - n / 9)
    for n in range(512):
        v[ARGS_INPUT+n] = u32(0xa5b60000 + n)
    return v

def vectors(seed, count):
    rows = []
    for cw in (0x7f, 0x27f, 0x37f, 0x47f, 0x87f, 0xc7f):
        for high in (0, 0x4000, 0x8000, 0xc000):
            for length in (0, 1, 2, 3, 7):
                v = base(); v[9] = cw; v[12] = length + 2; v[13] = max(2, length * 2)
                v[MESH_INPUT:MESH_INPUT+length+2] = [high | length] + [n % 7 - 3 for n in range(length + 1)]
                v = [u32(x) for x in v]; rows.append(v)
    # Several strips, both winding bits, distinct signed coordinates and degenerate endpoints.
    v = base(); v[1] = 3; mesh = [0x8003, -2, 1, 0, 0x4002, 3, -1, 0xc001, 2]
    v[12] = len(mesh); v[13] = 12; v[MESH_INPUT:MESH_INPUT+len(mesh)] = [u32(x) for x in mesh]; rows.append(v)
    for mode in range(8):
        for failures in (0, 1, 2, 3, 4, 8, 16, 32, 64):
            v = base(mode); v[10] = failures; rows.append(v)
        for stride in (0, 1, 12, 36, 0x80000000, 0xffffffff):
            for remaining in (0, 143, 144, 145, 8192):
                v = base(mode); v[2] = stride; v[4] = remaining
                if mode == 2: v[5] = 2
                rows.append(v)
        for api_stop in range(1, 8):
            v = base(mode); v[20] = api_stop; rows.append(v)
    for mode in (0, 1, 2, 3):
        for args in (0, 1, 2, 3, 4, 5, 6):
            v = base(mode); v[17] = args; v[5] = 1 if mode == 2 else 0; rows.append(v)
    for words in range(46):
        v = base(); v[11] = words; rows.append(v)
    for kind in (12, 13, 15, 16):
        for size in range(8):
            v = base(); v[kind] = size; rows.append(v)
    for requested in (0, 1, 2, 3, 1000, 0xffffffff, 0x80000000):
        v = base(1); v[18] = requested; rows.append(v)
    for index in (-32768, -1, 0, 32767):
        v = base(); v[MESH_INPUT + 1] = index & 0xffff; rows.append(v)
    for strip_count in (0, 2, 65535):
        v = base(); v[1] = strip_count; rows.append(v)
    for lock_count in (0x10000000, 0x40000000, 0xffffffff):
        v = base(1); v[5] = lock_count; rows.append(v)
    # A genuinely available zero-count strip still emits its two source vertices.
    v = base(); v[12] = 2; v[MESH_INPUT:MESH_INPUT+2] = [0, 1]
    v[24] = u32(-2); v[13] = 4; rows.append(v)
    # A large count is not narrowed to valid geometry; fail at the first absent header/index.
    v = base(); v[MESH_INPUT] = 0xffff; rows.append(v)
    rng = random.Random(seed)
    for _ in range(count):
        mode = rng.randrange(8); v = base(mode)
        v[9] = rng.choice((0x7f, 0x27f, 0x37f, 0x47f, 0x87f, 0xc7f))
        v[3] = rng.getrandbits(32); v[4] = rng.getrandbits(32)
        v[2] = rng.choice((0, 1, 12, 36, 256, 0xffffffff))
        v[5] = rng.randrange(5) if mode in (1, 2) else 0
        v[18] = rng.randrange(8); v[7] = rng.getrandbits(32); v[8] = rng.getrandbits(32)
        v[10] = rng.choice((0, 0, 0, 1, 2, 3, 4, 8, 16, 32, 64))
        v[21] = rng.choice((0, 4, 16, 36)); v[20] = rng.choice((0, 0, 0, 1, 2, 3, 4))
        if mode == 0:
            v[1] = rng.randrange(4); mesh = []; uv = []
            for strip in range(v[1]):
                length = rng.randrange(1, 9)
                mesh += [length | rng.choice((0, 0x4000, 0x8000, 0xc000))]
                mesh += [rng.randrange(-4, 12) & 0xffff for _ in range(length)]
                uv += [rng.randrange(65536) for _ in range(length*2)]
            v[12] = len(mesh); v[13] = len(uv)
            v[MESH_INPUT:MESH_INPUT+len(mesh)] = mesh; v[UV_INPUT:UV_INPUT+len(uv)] = uv
        rows.append(v)
    return rows

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe'); parser.add_argument('library'); parser.add_argument('--output', required=True)
    parser.add_argument('--seed', type=int, default=783590); parser.add_argument('--cases', type=int, default=400)
    args = parser.parse_args()
    image = PEImage(args.exe); assert image.sha256 == SOURCE_SHA256
    assert unicorn.__version__ == '2.1.4'
    source = Source(image)
    probe = ctypes.CDLL(str(Path(args.library).resolve())).sadx_native_normal_uv_strip_probe
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32)] * 2; probe.restype = ctypes.c_uint
    bad = []; flag_bad = 0; total = 0; traces = 0; writes = 0
    reference_receipts = hashlib.sha256(); native_receipts = hashlib.sha256()
    input_digest = hashlib.sha256(); flag_patterns = {}
    for case, v in enumerate(vectors(args.seed, args.cases)):
        expected = source.run(v)
        out = (ctypes.c_uint32 * OUTPUT_WORDS)()
        size = probe((ctypes.c_uint32 * INPUT_WORDS)(*v), out)
        actual = list(out[:size]); total += 1
        if actual[9] != expected[9]:
            flag_bad += 1
            pattern = f'cw={v[9]:#x}:source={expected[9]:#x},native={actual[9]:#x}'
            flag_patterns[pattern] = flag_patterns.get(pattern, 0) + 1
        input_digest.update(struct.pack('<' + 'I' * len(v), *v))
        for digest, result in ((reference_receipts, expected), (native_receipts, actual)):
            # Low-six FP flags are compared separately, never hidden by these receipt hashes.
            payload = result[:9] + result[10:]
            digest.update(struct.pack('<' + 'I' * len(payload), *payload))
        diff = [[n, a, b] for n, (a, b) in enumerate(zip(expected, actual)) if a != b and n != 9]
        if len(expected) != len(actual) or diff:
            bad.append({'case': case, 'mode': v[0], 'metadata': v[:32], 'lengths': [len(expected), len(actual)], 'diff': diff[:15]})
        traces += expected[11]; writes += sum(expected[n] == 3 for n in range(12, 12 + expected[11] * 6, 6))
    report = {'passed': not bad and not flag_bad, 'values_match': not bad,
              'cases': total, 'value_mismatches': len(bad), 'flag_mismatches': flag_bad,
              'ordered_receipts': traces, 'vertex_word_stores': writes,
              'reference_state_api_vertex_sha256': reference_receipts.hexdigest(),
              'native_state_api_vertex_sha256': native_receipts.hexdigest(),
              'input_sha256': input_digest.hexdigest(), 'flag_patterns': flag_patterns,
              'receipt_encoding': 'uint32 little-endian: state9, FPCW, trace count, ordered six-word receipts, final successful vertex byte snapshots, final owned render-argument words; low-six FP flags compared separately',
              'source_sha256': image.sha256,
              'source_ranges': {f'{a:#x}-{b:#x}': hashlib.sha256(image.read(a, b-a)).hexdigest() for a,b in RANGES},
              'boundary_facts': [{'instruction': hex(a), 'address': hex(b), 'kind': k, 'index': i} for a,b,k,i in sorted(source.boundaries)],
              'seed': args.seed, 'mismatches': bad[:10],
              'limits': 'COM/CRT receipts; no GPU, model composition or N3b claim. Compared low-six x87 exception flags differ: see flag_patterns (native inexact versus source clear in observed rows). Strict pass is false. Full FPSW/tag/stack and unmasked/deferred exceptions unverified.'}
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'values_match', 'cases', 'value_mismatches', 'flag_mismatches', 'ordered_receipts', 'vertex_word_stores')}))
    return 1 if bad else 2 if flag_bad else 0

if __name__ == '__main__':
    sys.exit(main())
