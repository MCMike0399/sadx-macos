"""Original normal/UV quad expansion and triangle-list buffer state, with unchanged COM receipts.

This adds an original-entry cohort. It imports the existing buffer receipt adapter,
then selects the quad source entry without patching instruction bytes or any fixture.
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
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP
from pe_image import PEImage
import verify_original_normal_uv_strip as strip


class Source(strip.Source):
    def __init__(self, image):
        super().__init__(image)
        self.m.hook_add(unicorn.UC_HOOK_CODE, self.entry, begin=0x7835f0, end=0x7835f0)

    def entry(self, machine, address, size, user):
        machine.reg_write(UC_X86_REG_EIP, 0x783830 if self.v[26] else 0x783920)


def fixtures(count, seed):
    rng = random.Random(seed)
    for index in range(count):
        case = strip.base()
        quads = (index // 2) % 8
        triangles = index % 2
        corners = 3 if triangles else 4
        output_vertices = quads * (3 if triangles else 6)
        case[1] = quads
        case[2] = 36
        case[3] = rng.choice([8192, 4096, 2048])
        case[4] = rng.choice([case[3], output_vertices * 36, output_vertices * 36 + 1, 1])
        case[5] = 0
        case[6] = 338
        case[7] = rng.choice([1, 2, 3])
        case[8] = rng.getrandbits(32)
        case[9] = rng.choice([0x7f, 0x27f, 0x37f, 0x47f, 0x87f, 0xc7f])
        case[10] = 1 if index % 17 == 0 else (2 if index % 23 == 0 else 0)
        case[11] = 4096
        case[12] = corners * quads
        case[13] = 2 * corners * quads
        case[26] = triangles
        case[14] = case[25] = 0
        case[15] = case[16] = 192
        case[17] = 512
        case[20] = rng.randrange(1, 9) if index % 9 == 0 else 0
        case[21] = 0
        case[23] = case[24] = 0
        for cursor in range(case[12]):
            case[strip.MESH_INPUT + cursor] = rng.randrange(64)
        for cursor in range(case[13]):
            case[strip.UV_INPUT + cursor] = rng.getrandbits(16)
        for offset in (strip.POINTS_INPUT, strip.NORMALS_INPUT):
            case[offset:offset + 192] = [rng.getrandbits(32) for _ in range(192)]
        yield case


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=1024)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x783920)
    args = parser.parse_args()
    source = Source(PEImage(args.exe))
    library = ctypes.CDLL(str(args.library.resolve()))
    entry = library.sadx_native_normal_uv_quad_probe
    entry.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    entry.restype = ctypes.c_uint
    failures, partials = [], 0
    digest = hashlib.sha256()
    for index, case in enumerate(fixtures(args.cases, args.seed)):
        output = (ctypes.c_uint32 * strip.OUTPUT_WORDS)()
        size = entry((ctypes.c_uint32 * len(case))(*case), output)
        actual = list(output[:size])
        expected = source.run(case)
        if actual[:9] + actual[10:] != expected[:9] + expected[10:]:
            failures.append({'case': index, 'lengths': [len(actual), len(expected)],
                'first_word': next((i for i,(a,b) in enumerate(zip(actual, expected))
                                    if a != b and i != 9), None)})
        elif actual[9] != expected[9]:
            partials += 1
        digest.update(json.dumps(expected, separators=(',',':')).encode())
    report = dict(cases=args.cases, seed=args.seed, value_failures=len(failures),
                  fp_partials=partials, flag_mismatches=partials, values_match=not failures,
                  passed=not failures and not partials,
                  status="fail" if failures else ("partial" if partials else "pass"), failures=failures[:20], observations_sha256=digest.hexdigest(),
                  source_entries=['0x783920', '0x783830'], scope='original quad and triangle-list buffers; COM receipts')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return 1 if failures else (2 if partials else 0)


if __name__ == '__main__':
    raise SystemExit(main())
