#!/usr/bin/env python3
"""Independent seeded holdout of the unchanged game-mode probe ABI against original x86."""
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
from verify_original_game_mode import Source, RETURNING

SEED = 0x20261016
COUNT = 2048


def cases():
    rng = random.Random(SEED)
    results = [0, 1, 2, 3, 4, 5, 10, 0xfffffffe, 0xffffffff,
               100, 101, 102, 103, 104, 107, 0x10001, 0xffff0001, 0x80000000]
    for i in range(COUNT):
        row = [0] * 48
        row[0] = rng.choice(list(range(30)) + [0xffffffff, 0x80000000, 0x7fffffff])
        row[1:5] = [rng.getrandbits(32), rng.choice([0, 1, 0xffffffff]), rng.randrange(256),
                    rng.choice([0, 0x11223344])]
        for k in range(5, 12):
            row[k] = rng.choice([0, 1, 2, rng.randrange(256)])
        row[12:16] = [rng.randrange(8), rng.randrange(2), rng.getrandbits(16), int(i % 13 == 0)]
        row[16:35] = [rng.choice(results) for _ in RETURNING]
        for k in range(2):
            if rng.randrange(3) == 0:
                row[35 + 3*k:38 + 3*k] = [rng.choice([0x4228f0, 0x4221e0, 0x5034a0,
                    0x424c80, 0x42d770, 0x427ee0, 0x780fb0, 0x424320]),
                    rng.randrange(5), rng.choice(results)]
        row[41] = rng.randrange(1, 45) if rng.randrange(7) == 0 else 0
        yield row


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe', type=Path)
    p.add_argument('probe', type=Path)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('source identity mismatch')
    source = Source(image)
    library = ctypes.CDLL(str(args.probe.resolve()))
    fn = library.sadx_native_game_mode
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
                   ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    fn.restype = ctypes.c_int
    failed = []
    blob = bytearray()
    for index, row in enumerate(cases()):
        blob.extend(struct.pack('<48I', *row))
        expected = source.run(row)
        output = (ctypes.c_uint32 * 256)()
        if fn((ctypes.c_uint32 * 48)(*row), 48, output, 256) != 1:
            raise ValueError('probe rejected holdout input')
        if list(output) != expected:
            failed.append(dict(index=index, input=row, fields=[n for n in range(256)
                               if output[n] != expected[n]]))
    result = dict(seed=hex(SEED), cases=COUNT, case_sha256=hashlib.sha256(blob).hexdigest(),
                  mismatches=len(failed), first_mismatches=failed[:8],
                  source_sha256=image.sha256, probe_sha256=hashlib.sha256(args.probe.read_bytes()).hexdigest(),
                  limits='Exact 48-word ABI comparison; existing owner fixtures, no subsystem implementation claim.')
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ['seed', 'cases', 'mismatches']}))
    return int(bool(failed))


if __name__ == '__main__':
    raise SystemExit(main())
