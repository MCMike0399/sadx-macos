"""Execute original basic-model wrappers/parser with explicit lower-callee receipts.

No original instruction bytes are patched. D3DX, texture/shading, environment,
material-offset and strip callees are receipt boundaries, not recovered algorithms.
The alternate renderer, other primitives and buffered paths stop explicitly.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles')]
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP,
                              UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW)
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_ninja_texture import Original, BASE, STACK, STOP

DEVICE, VTABLE = BASE + 0x1000, BASE + 0x2000
MATERIAL, MODEL, MESH = BASE + 0x3000, BASE + 0x4000, BASE + 0x5000
POINTS, NORMALS, CURRENT = BASE + 0x6000, BASE + 0x7000, BASE + 0x8000
FLAGS, COLOR, VERTEX, LAND, CULL = 0x3d08498, 0x3d08488, 0x3d0848c, 0x3d08494, 0x3d08490
MATRICES = (0x3d12e00, 0x3d0fd40, 0x3d12900, 0x3d128c0)
CALLBACK = STOP + 0x800
SECOND_CALLBACK = CALLBACK + 16
MASK = 0xffffffff
RANGES = ((0x77ef70, 0x77ef87), (0x77eda0, 0x77ef4e), (0x781cc0, 0x781dea),
          (0x781df0, 0x781e14), (0x781ec0, 0x78202c),
          (0x7944b0, 0x7944f1), (0x794500, 0x79453e), (0x784850, 0x784adf), (0x791a50, 0x791a7c),
          (0x791a80, 0x791aa4), (0x791990, 0x791a4a), (0x791ab0, 0x791ac9), (0x791ad0, 0x791ae9))

def words(data):
    return list(struct.unpack('<' + 'I' * (len(data) // 4), data))

def binary(values):
    return struct.pack('<' + 'I' * len(values), *values)

def float_bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]

def material_words(w, index=0):
    return [w[13] ^ index, w[14] ^ index, w[15], w[16], w[17]]

def fixture_pack(w):
    """Independent valid immutable pack for dispatch/callback/material transitions."""
    normals, colors, uv = bool(w[25] & 4), bool(w[25] & 2), bool(w[25] & 1)
    count = w[24]
    primitive = w[28]
    # Each primitive has one three-corner triangle/strip, or a four-corner quad.
    corners = 4 if primitive == 1 else 3
    indices = [0, 1, 2, 0] if primitive == 1 else [0, 1, 2]
    if primitive >= 2:
        indices.insert(0, corners)
    index_bytes = struct.pack('<' + 'h' * len(indices), *indices)
    mesh_bytes = bytearray(binary([count]))
    for i in range(count):
        mesh_bytes.extend(struct.pack('<HH6I', (primitive << 14) | ((w[27] >> i) & 1),
                                      1, 10, 0, 0, 11 if colors else 0, 12 if uv else 0, 0))
    records = [
        (1, binary([2, 4])),
        (2, binary([23, 3]) + binary([0] * 6 + [float_bits(1)] * 3 + [0, 0])),
        (3, struct.pack('<5I2H5I', 6, 7 if normals else 0, 3, 8, 9, count, 2, 0, 0, 0, 0, 0)),
        (11, struct.pack('<IIHHI', 5, 1, 3, 2, 1)),
        (12, binary([1, 0, 0, 0, 0])),
        (6, binary([3] + [float_bits(i + 1) for i in range(9)])),
        (6, binary([3] + [float_bits(i + 11) for i in range(9)])),
        (4, bytes(mesh_bytes)),
        (5, binary([2] + material_words(w) + material_words(w, 1))),
        (9, binary([len(indices)]) + index_bytes),
        (7, binary([corners] + [0xff112233] * corners)),
        (8, binary([corners]) + bytes(corners * 4)),
    ]
    return b'SDXACT01' + binary([len(records), 2, 100, 1, 101, 1]) + b''.join(
        binary([kind, len(payload)]) + payload for kind, payload in records)

class Source(Original):
    def __init__(self, image):
        super().__init__(image)
        self.clean = self.m.context_save()
        self.m.hook_add(unicorn.UC_HOOK_CODE, self.code)
        self.api = {STOP + 0x100 + i * 16: (slot, size) for i, (slot, size) in enumerate(
            [(0xa8, 8), (0xc8, 12), (0xfc, 16), (0x94, 12), (0x14c, 16), (0x154, 12)])}
        self.primitive_targets = {}
        for layout in range(8):
            table = words(image.read(0x389d690 + layout * 16, 16))
            for primitive, target in enumerate(table):
                if target != 0x78c5a0:
                    self.primitive_targets[target] = (layout, primitive)
        # Repeated polygon stub has no layout identity: decode current mesh instead.
        self.primitive_targets[0x78c5a0] = None

    def read(self, address, count=1):
        return words(bytes(self.m.mem_read(address, count * 4)))

    def write(self, address, values):
        self.m.mem_write(address, binary(values))

    def event(self, tag, *args):
        self.events.extend([tag, len(args), *args])

    def ret(self, cleanup=0, value=None):
        esp = self.m.reg_read(UC_X86_REG_ESP)
        target = self.read(esp)[0]
        self.m.reg_write(UC_X86_REG_ESP, esp + 4 + cleanup)
        self.m.reg_write(UC_X86_REG_EIP, target)
        if value is not None:
            self.m.reg_write(UC_X86_REG_EAX, value & MASK)

    def code(self, machine, address, size, user):
        w = self.w
        esp = machine.reg_read(UC_X86_REG_ESP)
        if address in self.api:
            slot, cleanup = self.api[address]
            args = self.read(esp + 4, cleanup // 4)
            if slot == 0xa8:
                self.event(3, *self.read(args[1], 17))
            elif slot == 0xc8:
                self.event(1, *args[1:])
            elif slot == 0xfc:
                assert args[1] == 0
                self.event(2, *args[2:])
                if args[2] == 24:
                    self.write(FLAGS, [self.read(FLAGS)[0] ^ w[20]])
            elif slot == 0x94:
                assert args[1] in (16, 256)
                self.event(4 if args[1] == 256 else 21, *self.read(args[2], 16))
            elif slot == 0x14c:
                assert args[1:] == [0, 0, 0]
                self.event(19)
            elif slot == 0x154:
                assert args[1:] == [0, 0]
                self.event(20)
            self.ret(cleanup, w[19])  # original ignores every synthetic HRESULT
        elif address == 0x770804:
            out, first, second = self.read(esp + 4, 3)
            a, b = self.read(first, 16), self.read(second, 16)
            self.event(5, *a, *b)
            self.write(out, [x ^ y for x, y in zip(a, b)])
            self.ret(12, out)
        elif address == 0x770d10:
            out, determinant, source = self.read(esp + 4, 3)
            assert determinant == 0
            a = self.read(source, 16)
            self.event(6, *a)
            if not w[23]:
                self.write(out, list(reversed(a)))
            self.ret(12, 0 if w[23] else out)
        elif address == 0x78d140:
            self.event(8, machine.reg_read(UC_X86_REG_ECX))
            self.ret(value=w[19])
        elif address == 0x78bba0:
            self.event(7)
            self.ret()
        elif address == 0x784ae0:
            self.event(13)
            self.boundary = 4
            machine.emu_stop()
        elif address in self.primitive_targets:
            mesh, points, normals = self.read(esp + 4, 3)
            ordinal = (mesh - MESH) // 28
            assert points == POINTS
            if address == 0x7835f0:
                self.event(14, ordinal, 6, 7)
                self.ret()
            else:
                layout = w[25] & (~2 if self.entry_flags & 16 else 7)
                self.event(15, layout, w[28], ordinal, 6, 7 if normals else 0)
                self.boundary = 5
                machine.emu_stop()
        elif address == CALLBACK:
            self.event(17)
            self.write(FLAGS, [self.read(FLAGS)[0] ^ w[22]])
            self.write(0x3d08420, [SECOND_CALLBACK])
            self.ret()
        elif address == SECOND_CALLBACK:
            self.event(18)
            self.ret()

    def run(self, w):
        self.w, self.events, self.boundary = w, [], 0
        self.entry_flags = w[1]
        machine = self.m
        machine.context_restore(self.clean)
        machine.reg_write(UC_X86_REG_FPCW, w[18])
        machine.reg_write(UC_X86_REG_FPSW, 0)
        self.write(DEVICE, [VTABLE])
        for address, (slot, _) in self.api.items():
            self.write(VTABLE + slot, [address])
        for address, value in [(FLAGS, w[1]), (0x3d0f9c8, w[2]), (0x3d0f840, w[3]),
                               (0x3d0f9c4, w[4]), (COLOR, w[5]), (VERTEX, w[6]),
                               (LAND, w[7]), (CULL, w[8]), (0x3d128b0, DEVICE),
                               (0x3d0f818, CURRENT),
                               (0x3d08420, CALLBACK if w[26] else 0)]:
            self.write(address, [value])
        self.write(0x3d0f7f0, w[9:13])
        for address, values in [(CURRENT, w[32:48]), (0x389d358, w[48:64]),
                                (0x389d318, w[64:80]), (MATRICES[0], w[96:112]),
                                (MATRICES[1], w[80:96]), (MATRICES[2], w[112:128]),
                                (MATRICES[3], w[128:144])]:
            self.write(address, values)
        self.write(MATERIAL, material_words(w) + material_words(w, 1))
        self.write(MODEL, [POINTS, NORMALS if w[25] & 4 else 0, 3, MESH, MATERIAL,
                           w[24] | (2 << 16), 0, 0, 0, 0, 0])
        for i in range(w[24]):
            raw = struct.pack('<HH6I', (w[28] << 14) | ((w[27] >> i) & 1), 1, 0, 0, 0,
                              BASE + 0x9000 if w[25] & 2 else 0,
                              BASE + 0xa000 if w[25] & 1 else 0, 0)
            machine.mem_write(MESH + i * 28, raw)
        entries = [0x781df0, 0x784850, 0x781cc0, 0x791a80, 0x791a50,
                   0x791ab0, 0x791ad0, 0x77ef70, 0x77eda0, 0x791990]
        machine.reg_write(UC_X86_REG_ECX, w[29] if w[0] == 9 else MATERIAL)
        self.write(STACK, [STOP, CURRENT if w[0] == 3 else MODEL])
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.emu_start(entries[w[0]], STOP, count=20000)
        if not self.boundary:
            assert machine.reg_read(UC_X86_REG_EIP) == STOP, 'source instruction budget exhausted'
            assert machine.reg_read(UC_X86_REG_ESP) == STACK + 4, 'source cleanup mismatch'
        state = [self.read(address)[0] for address in (FLAGS, COLOR, VERTEX, LAND, CULL)]
        return state + [machine.reg_read(UC_X86_REG_FPSW) & 63, self.boundary, len(self.events)] + \
            sum((self.read(address, 16) for address in MATRICES), []) + self.events

def base():
    w = [0] * 144
    w[3], w[5:9], w[9:13] = MASK, [0x17654321, 0xabcdef01, 0x12345678, 3], [float_bits(x) for x in (1, .2, .4, .6)]
    w[13:19] = [0xa1223344, 0xb1556677, float_bits(12), 0x12340005, 0x20200000, 0x7f]
    w[19], w[21], w[24:29] = MASK, 0xddeeff11, [3, 5, 1, 2, 3]
    for i in range(32, 144):
        w[i] = float_bits(i - 31)
    return w

def fixtures(seed, count):
    rng = random.Random(seed)
    # Each material flag field independently exercises both maintained lookup tables.
    for operation in range(10):
        for flags in (0, 1, 2, 4, 6, 7, 16, 17, 0x80000018):
            for control in (0, 4, 16, 32, 48):
                w = base(); w[0:3] = operation, flags, control
                yield w
    for u in range(4):
        for v in range(4):
            for source in range(8):
                for destination in range(8):
                    w = base(); w[0] = 1
                    w[17] = (1 << 21) | ((u & 1) << 16) | ((u & 2) << 17) | \
                        ((v & 1) << 15) | ((v & 2) << 16) | (source << 29) | (destination << 26)
                    yield w
    for mode in (0, 1, 2, 3, 4, MASK):
        w = base(); w[0] = 9; w[29] = mode; yield w
    # Source-backed Question material supplied by Root's immutable pack read.
    for operation in (1, 7, 8):
        w = base(); w[0] = operation; w[24:29] = [1, 5, 1, 0, 3]
        w[13:18] = [0xffffff00, 0xffffffff, float_bits(11), 7, 0x9461a400]
        yield w
    for component in (0, 0x80000000, 1, 0x80000001, 0x7fc12345, 0x7f812345,
                      float_bits(1), float_bits(-1), 0x7f800000, 0xff800000):
        for control in (16, 32):
            for index in range(4):
                w = base(); w[0] = 2; w[2] = control; w[9:13] = [0] * 4; w[9 + index] = component
                yield w
    for layout in range(8):
        for primitive in range(4):
            for flag in (0, 16):
                w = base(); w[0] = 8; w[1] = flag; w[25] = layout; w[28] = primitive
                yield w
    for _ in range(count):
        w = base()
        w[0] = rng.randrange(10)
        w[1] = rng.getrandbits(32)
        w[2] = rng.randrange(64)
        w[3:9] = [rng.getrandbits(32) for _ in range(6)]
        w[9:18] = [rng.getrandbits(32) for _ in range(9)]
        w[18] = rng.choice([0x7f, 0x27f, 0x37f, 0x47f, 0x87f, 0xc7f])
        w[19:24] = [rng.getrandbits(32) for _ in range(5)]
        w[23] &= 1
        w[29] = rng.choice([0, 1, 2, 3, 4, MASK])
        w[24:29] = [rng.randrange(5), rng.randrange(8), rng.randrange(2), rng.randrange(16), rng.randrange(4)]
        for i in range(32, 144):
            w[i] = rng.getrandbits(32)
        yield w

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seed', type=int, default=7819850)
    parser.add_argument('--cases', type=int, default=800)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('source/oracle identity mismatch')
    library = ctypes.CDLL(str(args.library.resolve()))
    function = library.sadx_native_basic_model
    function.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32),
                         ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
    function.restype = ctypes.c_int
    source = Source(image)
    cases = list(fixtures(args.seed, args.cases))
    blob = b''.join(binary(w) for w in cases)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.with_suffix('.cases.bin').write_bytes(blob)
    mismatch_count, partial_count, mismatches = 0, 0, []
    receipts_path = args.output.with_suffix('.receipts.jsonl')
    receipts = receipts_path.open('w')
    for index, w in enumerate(cases):
        expected = source.run(w)
        pack = fixture_pack(w) if w[0] in (7, 8) else b''
        data = (ctypes.c_uint8 * len(pack)).from_buffer_copy(pack)
        output = (ctypes.c_uint32 * 8192)()
        status = function((ctypes.c_uint32 * len(w))(*w), output, data, len(pack))
        if status:
            raise RuntimeError(f'native probe error {status}, case {index}, input {w[:32]}')
        actual = list(output[:72 + output[7]])
        receipts.write(json.dumps(dict(case=index, source=expected, native=actual), separators=(',', ':')) + '\n')
        # FP status is separately reported, never counted as an exact pass.
        values_match = actual[:5] + actual[6:] == expected[:5] + expected[6:]
        if actual[5] != expected[5]:
            partial_count += 1
        if not values_match:
            mismatch_count += 1
            if len(mismatches) < 12:
                mismatches.append(dict(case=index, input=w, source=expected, native=actual))
    receipts.close()
    sha = lambda data: hashlib.sha256(data).hexdigest()
    report = dict(source_sha256=image.sha256, cases=len(cases), mismatch_count=mismatch_count,
                  fp_status_mismatches=partial_count, values_match=mismatch_count == 0,
                  passed=mismatch_count == 0 and partial_count == 0,
                  exit_status=1 if mismatch_count else (2 if partial_count else 0),
                  case_sha256=sha(blob), library_sha256=sha(args.library.read_bytes()),
                  raw_receipts=dict(path=str(receipts_path), sha256=sha(receipts_path.read_bytes())),
                  source_files_sha256={str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in
                      [ROOT / 'src/engine/basic_model.cpp', ROOT / 'src/engine/basic_model.hpp',
                       ROOT / 'tests/probes/native_basic_model_probe.cpp',
                       ROOT / 'tests/probes/basic_model_backend.hpp', Path(__file__).resolve()]},
                  original_ranges_sha256={f'{a:#x}-{b:#x}': sha(image.read(a, b-a)) for a, b in RANGES},
                  mismatches=mismatches,
                  limits=['Unmodified complete wrapper/parser/transform instruction bodies',
                          'D3DX calls use operand/store receipts, not numerical matrix proof',
                          'Texture and strip calls are synthetic lower receipts; all three material helpers execute original bodies',
                          'Alternate/other primitive/buffer paths remain explicit stops',
                          'Actual source _ftol instructions; FP-status differences remain partial',
                          'No runtime, GPU, Windows-device or Question-entry FP-state claim'])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('cases', 'mismatch_count', 'fp_status_mismatches', 'values_match', 'passed')}))
    return report['exit_status']

if __name__ == '__main__':
    raise SystemExit(main())
