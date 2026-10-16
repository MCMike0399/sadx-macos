#!/usr/bin/env python3
"""Seeded frame-control holdout using the unchanged 19-input/16-output probe ABI."""
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
from extract import SOURCE_SHA256
from verify_original_game_frame_control import source_run

SEED = 0x202610160C
COUNT = 2048


def cases():
    rng = random.Random(SEED)
    for index in range(COUNT):
        row = [rng.getrandbits(32) for _ in range(19)]
        row[0] = index % 4
        row[1] = rng.choice([0x3fff, rng.getrandbits(14),
                             0x3fff ^ (1 << rng.randrange(14))])
        row[2] = rng.choice([0, 1, 2, 3, 0xffffffff, 0x80000000, row[2]])
        row[7] = rng.choice([0, 1, 2, 3, row[7]])
        row[10] = rng.choice([0, 0, 1, 2, 4, 255, row[10] & 255])
        row[11] = rng.choice([0, 4, 64, 68, 255, row[11] & 255])
        row[12] = rng.choice([0, 1, 0xffffffff, row[12]])
        row[13] = rng.choice([0, 1, 255, row[13] & 255])
        row[17] &= 1
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
    library = ctypes.CDLL(str(args.probe.resolve()))
    probe = library.sadx_native_game_frame_control
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
                      ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    probe.restype = ctypes.c_int
    failed, blob = [], bytearray()
    for index, row in enumerate(cases()):
        blob.extend(struct.pack('<19I', *row))
        expected = source_run(image, row)
        actual = (ctypes.c_uint32 * 16)()
        if probe((ctypes.c_uint32 * 19)(*row), 19, actual, 16) != 1:
            raise ValueError('probe rejected holdout input')
        if list(actual) != expected:
            failed.append(dict(index=index, input=row, expected=expected, actual=list(actual)))
    result = dict(seed=hex(SEED), cases=COUNT, case_sha256=hashlib.sha256(blob).hexdigest(),
                  mismatches=len(failed), first_mismatches=failed[:8],
                  source_sha256=image.sha256,
                  probe_sha256=hashlib.sha256(args.probe.read_bytes()).hexdigest(),
                  limits='Exact stores, availability, callback identity and failure boundaries; '
                         'no limiter or real-device timing claim.')
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: result[key] for key in ['seed', 'cases', 'mismatches']}))
    return int(bool(failed))


if __name__ == '__main__':
    raise SystemExit(main())
