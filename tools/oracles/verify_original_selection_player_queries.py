"""Execute both complete original predicate leaves against the native bindings.

No callees intercepted. Task identity is never dereferenced. Full EAX and ordered
source reads are checked; native API consumes already borrowed bootstrap bytes.
"""
import argparse
import ctypes
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import sys

import unicorn as uc
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/lib'))
from pe_image import PEImage

RANGES = [(0x42fb00, 0x42fb0e, '411020a5153aadf9b70e6c1d0d56a45e59a74ee7a0cff4e6f5763be5dc1a13a9'),
          (0x426690, 0x4266b4, '9fec0fb19cb439ba74a8ae988f8ecc426620202c1592c572551e01cad534636c')]
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
TASK, FRAMES, SECONDS, MINUTES = 0x3b2c578, 0x3b0ef35, 0x3b0f128, 0x3b0ef48
STACK, STOP = 0x11000800, 0x12000000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('probe', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--holdout', action='store_true')
    args = parser.parse_args()
    if hashlib.sha256(args.exe.read_bytes()).hexdigest() != SOURCE_SHA:
        raise ValueError('Source identity mismatch')
    image = PEImage(args.exe)
    machine = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
    pages = {a & ~4095 for a, _, _ in RANGES} | {a & ~4095 for a in (TASK, FRAMES, SECONDS, MINUTES, STACK, STOP)}
    for page in pages:
        machine.mem_map(page, 4096)
    for start, end, digest in RANGES:
        code = image.read(start, end - start)
        if hashlib.sha256(code).hexdigest() != digest:
            raise ValueError('Range identity mismatch')
        machine.mem_write(start, code)
    reads, writes = [], []
    globals_ = {TASK, FRAMES, SECONDS, MINUTES}
    machine.hook_add(uc.UC_HOOK_MEM_READ, lambda m, access, address, size, value, user:
                     reads.append((address, size)) if address in globals_ else None)
    machine.hook_add(uc.UC_HOOK_MEM_WRITE, lambda m, access, address, size, value, user:
                     writes.append((address, size, value)))
    library = ctypes.CDLL(str(args.probe.resolve()))
    native = library.native_selection_player_queries
    native.argtypes = [ctypes.c_uint32, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8]
    native.restype = ctypes.c_uint32
    rng = random.Random(0x4259 if args.holdout else 0x4266)
    cases = [(0, 0, 0, 0), (1, 0, 0, 0), (0xffffffff, 0, 0, 0)]
    for axis, value in itertools.product(range(3), range(256)):
        timer = [0, 0, 0]
        timer[axis] = value
        cases.append((rng.getrandbits(32), *timer))
    cases += [(rng.getrandbits(32), *(rng.randrange(256) for _ in range(3)))
              for _ in range(1024 if args.holdout else 4096)]
    failures = []
    for case in cases:
        task, frames, seconds, minutes = case
        machine.mem_write(TASK, struct.pack('<I', task))
        for address, value in zip((FRAMES, SECONDS, MINUTES), (frames, seconds, minutes)):
            machine.mem_write(address, bytes([value]))
        results = []
        for start, _, _ in RANGES:
            machine.mem_write(STACK, struct.pack('<I', STOP))
            machine.reg_write(UC_X86_REG_ESP, STACK)
            machine.reg_write(UC_X86_REG_EAX, 0xa5a5a5a5)
            reads.clear()
            writes.clear()
            machine.emu_start(start, STOP, count=32)
            expected_reads = [(TASK, 4)]
            if start == RANGES[1][0]:
                expected_reads = [(FRAMES, 1)]
                if frames == 0:
                    expected_reads.append((SECONDS, 1))
                    if seconds == 0:
                        expected_reads.append((MINUTES, 1))
            if (reads != expected_reads or writes or machine.reg_read(UC_X86_REG_EIP) != STOP
                    or machine.reg_read(UC_X86_REG_ESP) != STACK + 4):
                raise AssertionError(('Original boundary changed', case, reads, writes))
            results.append(machine.reg_read(UC_X86_REG_EAX))
        expected = results[0] | (results[1] << 1)
        actual = native(*case)
        if actual != expected:
            failures.append({'case': case, 'expected': expected, 'actual': actual})
    report = {'passed': not failures, 'cases': len(cases), 'mismatch_count': len(failures),
              'values_match': not failures, 'holdout': args.holdout,
              'original_ranges_sha256': {f'{start:#x}-{end:#x}': sha for start, end, sha in RANGES},
              'mismatches': failures[:10],
              'limits': ['Pure predicates only; bootstrap owner mapping and caller timing require runtime review.',
                         'No FP instructions; no rendering, controller, GPU or N3b qualification.']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
