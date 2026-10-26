"""The unmodified five-instruction ECX wrapper and original texture cache.

Uses the established concrete texture fixture schema. Only supplied Memory words
belong to that schema; wrapped reads of other NJS_TEXNAME fields stop at the read
boundary rather than interpreting invented filename/attribute pointer words.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn

from unicorn.x86_const import UC_X86_REG_EIP
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_ninja_texture_selection import ARRAY, MEM
from verify_original_texture_selection_core import QueuedSource, base

ROOT = Path(__file__).resolve().parents[2]
ENTRY = 0x78d140


class NumberSource(QueuedSource):
    def code(self, machine, address, size, user):
        if self.w is None:
            return
        if address == 0x77f440:
            # Fixture setup is reused; execute the real ECX number entry instead.
            for index, memory in enumerate(MEM):
                pointer = 0 if self.w[44] & (1 << index) else memory
                machine.mem_write(ARRAY + index * 12 + 8, struct.pack('<I', pointer))
            machine.reg_write(UC_X86_REG_EIP, ENTRY)
            return
        if address == 0x78d149 and self.w[2] == 0:
            return self.stop(address)
        if address == 0x78d14b:
            offset = self.w[1] * 12 & 0xffffffff
            if offset % 12 or offset // 12 >= self.w[41]:
                return self.stop(address)
        super().code(machine, address, size, user)


def cases(seed, count):
    for present in (0, 1):
        for index in (0, 1, 2, 3, 0x40000000, 0x40000001, 0x80000002,
                      0xc0000000, 0xffffffff, 0x15555556):
            for size in (0, 1, 3):
                for requested in range(4):
                    words = base()
                    words[0], words[1], words[2], words[41], words[46] = 2, index, present, size, requested
                    yield words
    for fmt in (0, 0x08000000, 0x10000000, 0x18000000, 0x28000000, 0xffffffff):
        for receipt in range(3):
            for surface in (0, base()[19]):
                for requested in range(4):
                    words = base()
                    words[0], words[18], words[9], words[19], words[46] = 2, fmt, receipt, surface, requested
                    yield words
    for missing in range(8):
        for index in range(3):
            words = base()
            words[0], words[1], words[44] = 2, index, missing
            yield words
    rng = random.Random(seed)
    for _ in range(count):
        words = base()
        words[0] = 2
        words[1] = rng.choice((0, 1, 2, 3, 0x40000000, 0x40000001, rng.getrandbits(32)))
        words[2], words[41], words[46], words[47] = (rng.randrange(2), rng.randrange(4),
                                                    rng.randrange(4), rng.randrange(4))
        words[5], words[9], words[44] = rng.choice((7, 8, 9, rng.getrandbits(32))), rng.randrange(3), rng.randrange(8)
        words[48], words[49] = rng.randrange(2), rng.getrandbits(32)
        words[35], words[36], words[37] = rng.choice((2, 5000)), rng.randrange(8), rng.choice((1, 2))
        for index in range(3):
            start = 16 + index * 6
            words[start:start + 5] = [rng.choice((7, 8, 9, rng.getrandbits(32))),
                                      rng.choice((0xffffffff, 2, 5000)),
                                      rng.choice((0, 0x08000000, 0x10000000, 0x28000000)),
                                      rng.choice((0, base()[19])), rng.randrange(8)]
        yield words


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seed', type=int, default=781140)
    parser.add_argument('--random-cases', type=int, default=3000)
    args = parser.parse_args()
    image = PEImage(args.exe)
    assert image.sha256 == SOURCE_SHA256
    assert unicorn.__version__ == '2.1.4'
    source = NumberSource(image)
    probe = ctypes.CDLL(str(args.library.resolve())).sadx_native_texture_number
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32)] * 2
    probe.restype = ctypes.c_int
    rows = list(cases(args.seed, args.random_cases))
    failures = []
    for index, words in enumerate(rows):
        expected = source.run(words)
        output = (ctypes.c_uint32 * 128)()
        assert probe((ctypes.c_uint32 * 50)(*words), output) == 0
        actual = dict(boundary=output[1], completed=bool(output[2]), outcome=output[3],
                      current=bool(output[4]), cache=output[5], requested=output[6], active=output[7],
                      versions=list(output[8:11]),
                      calls=[tuple(output[12 + i * 3:15 + i * 3]) for i in range(output[11])],
                      queued=output[22], queued_index=output[23], callback_state=list(output[24:30]))
        if not expected['completed']:
            del expected['outcome']
            del actual['outcome']
        if expected != actual:
            failures.append(dict(case=index, input=words, expected=expected, actual=actual))
    sha = lambda value: hashlib.sha256(value).hexdigest()
    files = ['src/engine/texture_selection_native.cpp', 'src/engine/texture_selection_native.hpp',
             'src/engine/texture_selection_internal.hpp', 'tests/probes/native_texture_number_probe.cpp',
             'tools/oracles/verify_original_texture_number.py']
    report = dict(passed=not failures, cases=len(rows), seed=args.seed, mismatches=len(failures),
                  first=failures[:8], source_sha256=image.sha256,
                  source_range_sha256=sha(image.read(ENTRY, 20)),
                  library_sha256=sha(args.library.read_bytes()),
                  source_files_sha256={f: sha((ROOT / f).read_bytes()) for f in files},
                  cases_sha256=sha(b''.join(struct.pack('<50I', *row) for row in rows)),
                  limits=['Original78d140 and cache bodies execute without source-byte patches.',
                          'Unprovided array words and non-Memory wrapped fields stop at the original read.',
                          'Palette lock and unavailable device receipts retain established explicit boundaries.',
                          'No physical GPU or completed CharacterSelect proof.'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'cases', 'mismatches', 'seed', 'first')}))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
