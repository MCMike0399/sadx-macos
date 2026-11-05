"""Original normal-only primitives using the canonical source buffer6.

The existing COM/range receipt adapter seeds a portable buffer view. At the new
source entry that seed is transported to buffer6 and its stores are observed in
that view; no original instruction bytes or existing fixture files are changed.
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
        machine.mem_write(0x3d0fed8, bytes(machine.mem_read(strip.BUFFER, 36)))
        machine.reg_write(UC_X86_REG_EIP, [0x783270, 0x783410, 0x7834c0][self.v[26]])

    def memory(self, machine, access, address, size, value, user):
        if 0x3d0fed8 <= address < 0x3d0fed8 + 36:
            if access == unicorn.UC_MEM_WRITE:
                field = (address - 0x3d0fed8) // 4
                normalized = int(value != 0) if field in (0, 5) else value
                self.row(1, field, normalized)
                machine.mem_write(strip.BUFFER + field * 4, int(value).to_bytes(4, 'little'))
            return
        super().memory(machine, access, address, size, value, user)


def fixtures(count, seed):
    rng = random.Random(seed)
    for index in range(count):
        case = strip.base()
        count = (index // 3) % 6
        primitive = index % 3
        corners = 3 if primitive == 1 else 4
        output_vertices = count * (3 if primitive == 1 else 6)
        case[1] = count
        case[2] = 28
        case[3] = rng.choice([8192, 4096, 2048])
        case[4] = rng.choice([case[3], output_vertices * 28, output_vertices * 28 + 1, 1])
        case[5] = 0
        case[6] = 82
        case[7] = rng.choice([1, 2, 3])
        case[8] = rng.getrandbits(32)
        case[9] = rng.choice([0x7f, 0x27f, 0x37f, 0x47f, 0x87f, 0xc7f])
        case[10] = 1 if index % 17 == 0 else (2 if index % 23 == 0 else 0)
        case[11] = 4096
        case[12] = corners * count
        case[13] = 0
        case[26] = primitive
        case[14] = case[25] = 0
        case[15] = case[16] = 192
        case[17] = 512
        case[20] = rng.randrange(1, 9) if index % 9 == 0 else 0
        case[21] = 0
        case[23] = case[24] = 0
        if primitive == 0:
            cursor = 0
            for item in range(count):
                length = (index + item) % 5
                case[strip.MESH_INPUT + cursor] = length
                cursor += 1
                for _ in range(length):
                    case[strip.MESH_INPUT + cursor] = rng.randrange(64)
                    cursor += 1
            case[12] = cursor + 1
        else:
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
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x783410)
    args = parser.parse_args()
    source = Source(PEImage(args.exe))
    library = ctypes.CDLL(str(args.library.resolve()))
    entry = library.sadx_native_normal_primitives_probe
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
                  source_entries=['0x783270', '0x783410', '0x7834c0'], scope='original normal-only strip/triangle/quad and buffer state; COM receipts')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return 1 if failures else (2 if partials else 0)


if __name__ == '__main__':
    raise SystemExit(main())
