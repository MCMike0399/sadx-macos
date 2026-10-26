"""Full InitPolyBuffers plus all nine original PolyBuff_Init bodies.

Only CRT malloc and D3D CreateVertexBuffer are receipts. Ordered stores, helper
entries, API/allocation operands, completion, final records/scratch/argument bytes
are compared without tolerance. Existing judges are not imported or changed except
for the shared Original instruction-machine constructor.
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

RECORDS, SCRATCH = 0x3d0fe00, 0x3d0fae0
DEVICE, VTABLE, API = DATA + 0x1000, DATA + 0x2000, CODE + 0x400
ARGUMENTS = 0x20000000
RANGES = [(0x78e720, 0x78e791), (0x794540, 0x7945ba)]

def pack(values):
    return struct.pack(f'<{len(values)}I', *values)

class Source:
    def __init__(self, image):
        self.m = Original(image, 127).machine
        for s in image.sections:
            if s['name'] in ('.text', '.rdata'): continue
            size = (max(s['raw_size'], s['virtual_size']) + 4095) & ~4095
            self.m.mem_map(image.base + s['rva'], size)
            if s['raw_size']:
                self.m.mem_write(image.base + s['rva'], image.slice(s['file_offset'], s['raw_size']))
        self.m.mem_map(ARGUMENTS, 9 * 0x100000)
        self.put(DEVICE, VTABLE)
        self.put(VTABLE + 92, API)
        self.put(0x3d128b0, DEVICE)
        for address in (0x645002, API, 0x794540):
            self.m.hook_add(unicorn.UC_HOOK_CODE, self.code, begin=address, end=address)
        self.m.hook_add(unicorn.UC_HOOK_MEM_WRITE, self.write)
        self.m.hook_add(unicorn.UC_HOOK_MEM_INVALID, self.invalid)
        self.clean = self.m.context_save()
        self.boundaries = set()

    def word(self, address):
        return int.from_bytes(self.m.mem_read(address, 4), 'little')

    def put(self, address, value):
        self.m.mem_write(address, pack([value & 0xffffffff]))

    def row(self, *values):
        self.trace.append(list(values) + [0] * (8 - len(values)))

    def api(self, kind, bytes, usage=0, fmt=0, pool=0):
        self.row(3, kind, self.current, bytes, usage, fmt, pool)
        self.api_count += 1
        if self.api_count == self.v[0]:
            self.stopped = True
            self.m.emu_stop()
            return False
        return True

    def code(self, m, address, size, user):
        esp = m.reg_read(UC_X86_REG_ESP)
        ret = self.word(esp)
        if address == 0x794540:
            self.current = (m.reg_read(UC_X86_REG_ECX) - RECORDS) // 36
            self.row(6, self.current, m.reg_read(UC_X86_REG_EDX), self.word(esp + 4),
                     self.word(esp + 8), self.word(esp + 12))
            return  # execute the entire original helper
        if address == 0x645002:
            scratch = ret == 0x78e73b
            if not self.api(0 if scratch else 2, self.word(esp + 4)): return
            if scratch:
                result = 0 if self.v[1] else DATA + 0x8000
            else:
                result = 0 if self.v[3] == self.current + 1 else ARGUMENTS + self.current * 0x100000
                self.allocated[self.current] = result != 0
                if result:
                    count = self.word(esp + 4) // 4
                    m.mem_write(result, pack([0xa5a5a5a5] * count))
            m.reg_write(UC_X86_REG_EAX, result)
            m.reg_write(UC_X86_REG_ESP, esp + 4)
            m.reg_write(UC_X86_REG_EIP, ret)
            return
        if address == API:
            args = [self.word(esp + 4 + 4 * n) for n in range(6)]
            if not self.api(1, args[1], args[2], args[3], args[4]): return
            if self.v[2] != self.current + 1:
                # API writes are part of the ordered prefix too.
                self.row(1, self.current, 0, 1)
                self.put(args[5], DATA + 0x3000 + self.current * 16)
            m.reg_write(UC_X86_REG_EAX, 0x80004005 if self.v[2] == self.current + 1 else 0)
            m.reg_write(UC_X86_REG_ESP, esp + 28)
            m.reg_write(UC_X86_REG_EIP, ret)

    def write(self, m, access, address, size, value, user):
        if RECORDS <= address < RECORDS + 9 * 36:
            row, field = divmod((address - RECORDS) // 4, 9)
            self.row(1, row, field, int(value != 0) if field in (0, 5) else value)
        elif SCRATCH <= address < SCRATCH + 512:
            self.row(4, 2, (address - SCRATCH) // 4, value)
        elif address == 0x3d127a0:
            self.row(4, 0, 0, value)
        elif address == 0x3d128ac:
            self.row(4, 1, 0, int(value != 0))
        elif ARGUMENTS <= address < ARGUMENTS + 9 * 0x100000:
            row, offset = divmod(address - ARGUMENTS, 0x100000)
            self.row(2, row, offset // 4, value)

    def invalid(self, m, access, address, size, value, user):
        ip = m.reg_read(UC_X86_REG_EIP)
        if address < 8 and ip in (0x79459a, 0x7945a0):
            self.row(5, 4, self.current, address // 4)
            self.boundaries.add((ip, address))
            self.stopped = True
            m.emu_stop()
            return False
        raise RuntimeError(f'unexpected invalid access {address:#x} at {ip:#x}')

    def run(self, v):
        self.v, self.trace, self.current, self.api_count, self.stopped = v, [], 0, 0, False
        self.allocated = [False] * 9
        m = self.m
        m.context_restore(self.clean)
        self.put(0x3d127a0, v[7])
        self.put(0x3d128ac, 0)
        m.mem_write(SCRATCH, pack([v[8]] * 128))
        records = [0, v[9], v[9], v[9], v[9], 0, v[9], v[9], v[9]] * 9
        m.mem_write(RECORDS, pack(records))
        m.mem_write(0x389d864, pack(v[10:46]))
        m.mem_write(STACK, pack([STOP, v[6], v[4], v[5]]))
        m.reg_write(UC_X86_REG_ESP, STACK)
        try:
            m.emu_start(0x78e720, STOP, count=4000000)
        except unicorn.UcError:
            if not self.stopped: raise
        complete = m.reg_read(UC_X86_REG_EIP) == STOP
        if not complete and not self.stopped: raise RuntimeError('source instruction budget exhausted')
        out = [int(complete), self.word(0x3d127a0), int(self.word(0x3d128ac) != 0)]
        for r in range(9):
            fields = [self.word(RECORDS + 36*r + 4*w) for w in range(9)]
            fields[0], fields[5] = int(fields[0] != 0), int(fields[5] != 0)
            out += fields
        out += [self.word(SCRATCH + 4*w) for w in range(128)]
        out.append(len(self.trace))
        out += [w for row in self.trace for w in row]
        for r in range(9):
            length = v[10+r] * 4 if self.allocated[r] else 0
            out.append(length)
            if length:
                out += list(struct.unpack(f'<{length}I', m.mem_read(ARGUMENTS + r * 0x100000, length*4)))
        return pack(out)

def cases(image, seed, count):
    rng = random.Random(seed)
    counts = [8000, 4000, 8000, 12000, 24000, 4000, 4000, 12000, 32000]
    tables = list(struct.unpack('<27I', image.read(0x389d888, 108)))
    # Every API boundary is a deterministic failure prefix, including each of nine helpers.
    for stop in range(21):
        v = [stop, 0, 0, 0, 0x6000, 0x38fcf08, 32, 0, 0x44444444, 0x77777777]
        yield v + [1, 0, 2, 3, 4, 1, 0, 2, 5] + tables
    for n in range(count):
        v = [rng.choice([0, 0, 0, rng.randrange(1, 21)]), rng.randrange(2),
             rng.randrange(10), rng.randrange(10), rng.getrandbits(32), rng.getrandbits(32),
             rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32)]
        sizes = [rng.randrange(17) for _ in range(9)]
        stride = [rng.choice([0, 1, 36, rng.getrandbits(32)]) for _ in range(9)]
        yield v + sizes + stride + [rng.getrandbits(32) for _ in range(18)]
    # Source configured count, PE pre-configuration count and all-zero counts remain distinct.
    for sizes in (counts, counts[:-1] + [12000], [0]*9):
        yield [0, 0, 0, 0, 0x6000, 0x38fcf08, 32, 0, 0x44444444, 0] + sizes + tables

def main():
    p = argparse.ArgumentParser()
    p.add_argument('exe', type=Path)
    p.add_argument('native_library', type=Path)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--seed', type=int, default=789720)
    p.add_argument('--cases', type=int, default=128)
    a = p.parse_args()
    image = PEImage(a.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('pinned source/Unicorn mismatch')
    source = Source(image)
    lib = ctypes.CDLL(str(a.native_library.resolve()))
    fn = lib.sadx_native_polybuff_initialization
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    fn.restype = ctypes.c_uint
    output = (ctypes.c_uint32 * 4000000)()
    bad, total, receipts = [], 0, 0
    ih, sh, nh = hashlib.sha256(), hashlib.sha256(), hashlib.sha256()
    for v in cases(image, a.seed, a.cases):
        ih.update(pack(v))
        expected = source.run(v)
        length = fn((ctypes.c_uint32 * len(v))(*v), output)
        actual = ctypes.string_at(output, length * 4)
        sh.update(expected); nh.update(actual)
        receipts += len(source.trace)
        if expected != actual:
            first = next((i for i, (x,y) in enumerate(zip(expected, actual)) if x != y),
                         min(len(expected), len(actual)))
            bad.append({'case': total, 'first_byte': first, 'input': v,
                        'expected': expected[max(0,first-16):first+32].hex(),
                        'actual': actual[max(0,first-16):first+32].hex()})
        total += 1
    report = {'passed': not bad, 'values_match': not bad, 'cases': total,
              'value_mismatches': len(bad), 'ordered_receipts': receipts,
              'input_sha256': ih.hexdigest(), 'reference_receipt_sha256': sh.hexdigest(),
              'native_receipt_sha256': nh.hexdigest(), 'source_sha256': image.sha256,
              'source_ranges': {f'0x{x:x}-0x{y:x}': hashlib.sha256(image.read(x,y-x)).hexdigest()
                                for x,y in RANGES},
              'fault_boundaries': [list(x) for x in sorted(source.boundaries)],
              'mismatches': bad[:10], 'limits': ['No runtime or canonical owner migration certified.',
              'D3D and CRT resource operations are receipts; real allocation/device errors unverified.',
              'Integer-only initialization; no FP result or flag certification implied.']}
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k:report[k] for k in ('passed','cases','value_mismatches','ordered_receipts')}))
    return 0 if not bad else 1
if __name__ == '__main__':
    sys.exit(main())
