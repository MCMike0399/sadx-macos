#!/usr/bin/env python3
"""Independent seeded ground holdout through the unchanged 63-word probe ABI."""
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
from pe_image import PEImage
from extract import SOURCE_SHA256, TABLE_ADDRESS
from verify_original_ground_acceleration import GroundOriginal, CONSTANTS, UNIT, bits

SEED = 0x2026101606
COUNT = 320


def cases(parameters):
    rng = random.Random(SEED)
    for index in range(COUNT):
        row = [0] * 63
        row[0] = rng.choice([0, 0x2000])
        row[1:5] = [rng.getrandbits(32) for _ in range(4)]
        row[5:8] = [bits(rng.choice([0.0, 0.25, 0.65, 0.95, 1.0, -0.5])),
                    bits(rng.uniform(-0.8, 1.1)), rng.choice([0, 4, 0x800000])]
        row[8:12] = [rng.getrandbits(32), bits(123.0), bits(456.0), bits(rng.uniform(-2, 2))]
        row[12:21] = [bits(rng.uniform(-15, 15)) for _ in range(9)]
        row[21:24] = [bits(111.0), bits(222.0), bits(333.0)]
        row[24:30] = [bits(rng.uniform(-1, 1)) for _ in range(6)]
        row[30:63] = parameters
        if index % 3 == 0:
            for parameter in [4, 9, 10, 11, 13, 15, 17, 21, 22, 23, 24, 25, 26, 30]:
                row[30 + parameter] = bits(rng.uniform(-2, 12))
        if index % 11 == 0:
            row[rng.choice([5, 6, 11, *range(12, 21), *range(24, 30), 51, 54, 56, 60])] = rng.choice([
                0, 0x80000000, 1, 0x80000001, 0x7f800000, 0xff800000, 0x7fc12345, 0xff812345])
        yield row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('probe', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('source identity mismatch')
    table, unit = image.read(TABLE_ADDRESS, 4100), image.read(UNIT, 64)
    parameters = list(struct.unpack('<33I', image.read(0x9154e8, 132)))
    rows = list(cases(parameters))
    library = ctypes.CDLL(str(args.probe.resolve()))
    probe = library.sadx_native_ground_acceleration
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t, ctypes.c_uint16,
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    probe.restype = ctypes.c_int
    tb = (ctypes.c_uint8 * 4100).from_buffer_copy(table)
    ub = (ctypes.c_uint8 * 64).from_buffer_copy(unit)
    constants = (ctypes.c_uint32 * 24)(*[image.unpack('<I', address)[0] for address in CONSTANTS])
    failures, controls = [], []
    for precision in [0, 0x200, 0x300]:
        for rounding in [0, 0x400, 0x800, 0xc00]:
            cw = precision | rounding | 0x7f
            source = GroundOriginal(image, cw, unit)
            counts = dict(cw=hex(cw), cases=len(rows), value_mismatches=0,
                          call_mismatches=0, cw_mismatches=0, flag_mismatches=0)
            for index, row in enumerate(rows):
                values, calls, flags, restored = source.run(row, cw)
                out = (ctypes.c_uint32 * 43)()
                status = probe(tb, 4100, ub, 64, constants, 24,
                               (ctypes.c_uint32 * 63)(*row), 63, cw, out, 43)
                if status or out[12]:
                    raise ValueError(f'probe rejected holdout {cw:#x}/{index}: {status}/{out[12]}')
                actual = list(out[:12]) + list(out[40:43])
                bad = []
                for label, expected, observed in [('value', values, actual),
                        ('call', calls, list(out[18:18 + out[17]])), ('cw', restored, out[16])]:
                    if expected != observed:
                        counts[label + '_mismatches'] += 1
                        bad.append(label)
                if flags != out[15]:
                    counts['flag_mismatches'] += 1
                if bad:
                    failures.append(dict(cw=hex(cw), case=index, fields=bad, input=row))
            controls.append(counts)
    result = dict(seed=hex(SEED), cases=len(rows) * len(controls),
        case_sha256=hashlib.sha256(b''.join(struct.pack('<63I', *row) for row in rows)).hexdigest(),
        source_sha256=image.sha256, probe_sha256=hashlib.sha256(args.probe.read_bytes()).hexdigest(),
        control_words=controls, mismatch_count=len(failures), first_mismatches=failures[:8],
        limits='Existing real input/coordinate/cross/fast-yaw owners; five controlled no-mutation callees. '
               'Flags reported separately, not a hardware or full-gameplay claim.')
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(cases=result['cases'], mismatches=len(failures))))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
