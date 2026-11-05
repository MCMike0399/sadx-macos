"""Original constant/offset-material dispatcher, with existing material parser and trace leaves.

The source entry is njDrawModel_ 0x784ae0. D3DX/device/primitive leaves have the
same explicit receipts as basic_model's independent judge. No instruction bytes
are patched, and no lower raster fidelity is asserted by this cohort.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles')]
from verify_original_basic_model import Source, base, fixture_pack, MESH, POINTS, MASK
from pe_image import PEImage
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP


class DynamicSource(Source):
    def code(self, machine, address, size, user):
        if address == 0x77eda0:
            # Select the tested entry while retaining the common source seeding helper.
            machine.reg_write(UC_X86_REG_EIP, 0x784ae0)
            return
        if address == 0x784ae0:
            return
        if address == 0x78c5a0:
            return  # Execute the original RET instead of a primitive receipt.
        if address in self.primitive_targets:
            mesh, points, normals = self.read(machine.reg_read(UC_X86_REG_ESP) + 4, 3)
            assert points == POINTS
            layout = self.w[25] & (~2 if self.entry_flags & 16 else 7)
            self.event(22, layout, self.w[28], (mesh - MESH) // 28, 6, 7 if normals else 0)
            self.ret()
            return
        super().code(machine, address, size, user)


def fixtures(count, seed):
    rng = random.Random(seed)
    for index in range(count):
        w = base()
        w[0] = 8
        w[1] = rng.choice([0, 2, 4, 6, 16, 18, 20, 22])
        w[2] = rng.randrange(64)
        w[3:9] = [rng.getrandbits(32) for _ in range(6)]
        w[13:18] = [rng.getrandbits(32) for _ in range(5)]
        w[18] = 0x7f
        w[19:24] = [rng.getrandbits(32) for _ in range(5)]
        w[23] &= 1
        w[24:29] = [rng.randrange(1, 8), rng.randrange(8), rng.randrange(2),
                    rng.randrange(256), rng.randrange(4)]
        for slot in range(32, 144):
            w[slot] = rng.getrandbits(32)
        yield w


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=1024)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x784ae0)
    args = parser.parse_args()
    image = PEImage(args.exe)
    source = DynamicSource(image)
    library = ctypes.CDLL(str(args.library.resolve()))
    entry = library.sadx_native_dynamic_model
    entry.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32),
                      ctypes.POINTER(ctypes.c_ubyte), ctypes.c_size_t]
    entry.restype = ctypes.c_int
    failures = []
    partials = 0
    digest = hashlib.sha256()
    for index, case in enumerate(fixtures(args.cases, args.seed)):
        pack = fixture_pack(case)
        backing = (ctypes.c_ubyte * len(pack)).from_buffer_copy(pack)
        native = (ctypes.c_uint32 * 8192)()
        rc = entry((ctypes.c_uint32 * len(case))(*case), native, backing, len(pack))
        expected = source.run(case)
        actual = list(native[:len(expected)])
        # FPU exception status is a separately reported strict partial, never masked as exact.
        if rc or actual[:5] + actual[6:] != expected[:5] + expected[6:]:
            failures.append({'case': index, 'native_return': rc,
                             'first_word': next((i for i,(a,b) in enumerate(zip(actual,expected))
                                                 if a != b and i != 5), None)})
        elif actual[5] != expected[5]:
            partials += 1
        digest.update(json.dumps(expected,separators=(',',':')).encode())
    report = dict(cases=args.cases, seed=args.seed, value_failures=len(failures),
                  fp_partials=partials, flag_mismatches=partials, values_match=not failures,
                  passed=not failures and not partials,
                  status="fail" if failures else ("partial" if partials else "pass"), failures=failures[:20],
                  observations_sha256=digest.hexdigest(),
                  original_entry='0x784ae0',
                  scope='original dynamic-model loop and material parser; primitive and D3DX receipts')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return 1 if failures else (2 if partials else 0)


if __name__ == '__main__':
    raise SystemExit(main())
