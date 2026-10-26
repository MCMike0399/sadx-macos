"""Unmodified CorrectMaterial instructions, concrete device receipts and ordered API calls."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from extract import SOURCE_SHA256
from pe_image import PEImage
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP,
                               UC_X86_REG_FPCW, UC_X86_REG_FPSW)

ROOT = Path(__file__).resolve().parents[2]
ENTRY, END = 0x40a340, 0x40a3f4
DEVICE, VTABLE, GET, SET, STOP, STACK = 0x100000, 0x101000, 0x102000, 0x102010, 0x103000, 0x200000


def pack(words):
    return struct.pack('<' + 'I' * len(words), *words)


class Source:
    def __init__(self, image):
        self.machine = u = Uc(UC_ARCH_X86, UC_MODE_32)
        for base, size in ((0x40a000, 4096), (0x7db000, 4096), (0x3d12000, 4096),
                           (0x3abd000, 4096), (DEVICE, 4 * 4096), (STACK, 65536)):
            u.mem_map(base, size)
        u.mem_write(0x40a000, image.read(0x40a000, 4096))
        u.mem_write(0x7db2c4, image.read(0x7db2c4, 4))
        u.mem_write(0x3d128b0, pack([DEVICE]))
        u.mem_write(DEVICE, pack([VTABLE]))
        u.mem_write(VTABLE + 0xac, pack([GET]))
        u.mem_write(VTABLE + 0xa8, pack([SET]))
        u.hook_add(UC_HOOK_CODE, self.code)

    def code(self, u, address, size, user):
        if address == STOP:
            self.output[0] = 1
            u.emu_stop()
            return
        if address not in (GET, SET):
            return
        call = 1 if address == GET else 2
        self.output[4 + self.output[3]] = call
        self.output[3] += 1
        if self.words[3] & call:
            self.output[1] = call
            u.emu_stop()
            return
        stack = u.reg_read(UC_X86_REG_ESP)
        ret, device, pointer = struct.unpack('<3I', u.mem_read(stack, 12))
        assert device == DEVICE
        if call == 1:
            u.mem_write(pointer, pack(self.words[6:23]))
            u.mem_write(0x3abdb20, pack([self.words[2]]))
        else:
            self.output[6:23] = struct.unpack('<17I', u.mem_read(pointer, 68))
        u.reg_write(UC_X86_REG_EAX, self.words[3 + call])
        u.reg_write(UC_X86_REG_ESP, stack + 12)
        u.reg_write(UC_X86_REG_EIP, ret)

    def run(self, words):
        self.words, self.output = words, [0] * 24
        u = self.machine
        stack = STACK + 60000
        u.mem_write(stack, pack([STOP]))
        u.mem_write(0x3abdb20, pack([words[1]]))
        u.reg_write(UC_X86_REG_ESP, stack)
        u.reg_write(UC_X86_REG_FPCW, words[0])
        u.reg_write(UC_X86_REG_FPSW, 0)
        u.emu_start(ENTRY, STOP + 1, count=200)
        self.output[2] = u.reg_read(UC_X86_REG_FPSW) & 63
        return self.output


def cases(seed, count):
    # Every channel is independently distinguished; alpha/emissive must remain unchanged.
    values = (0, 0x80000000, 1, 0x007fffff, 0x00800000, 0x3f800000,
              0x437f0000, 0x7f7fffff, 0x7f800000, 0xff800000, 0x7fc12345, 0x7fa12345)
    rng = random.Random(seed)
    for control in (0x007f, 0x027f, 0x037f, 0x047f, 0x087f, 0x0c7f):
        for channel in range(17):
            for value in values:
                words = [control, 0x3f800000, 0x40000000, 0, 0x80004005, 0x80004005]
                material = [0x40000000 + index * 0x10000 for index in range(17)]
                material[channel] = value
                yield words + material
    for stop in (1, 2, 3):
        yield [0x007f, 0, 0x42a00000, stop, 0, 0] + [0x437f0000] * 17
    for _ in range(count):
        yield [rng.choice((0x007f, 0x027f, 0x037f, 0x047f, 0x087f, 0x0c7f)),
               rng.getrandbits(32), rng.getrandbits(32), rng.choice((0, 0, 0, 1, 2, 3)),
               rng.getrandbits(32), rng.getrandbits(32)] + [rng.getrandbits(32) for _ in range(17)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seed', type=int, default=403340)
    parser.add_argument('--random-cases', type=int, default=3000)
    args = parser.parse_args()
    image = PEImage(args.exe)
    assert image.sha256 == SOURCE_SHA256
    source = Source(image)
    probe = ctypes.CDLL(str(args.library.resolve())).sadx_native_material_correction
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32)] * 2
    probe.restype = ctypes.c_int
    rows, failures, flag_failures = list(cases(args.seed, args.random_cases)), [], []
    for index, words in enumerate(rows):
        expected = source.run(words)
        output = (ctypes.c_uint32 * 24)()
        assert probe((ctypes.c_uint32 * 23)(*words), output) == 0
        actual = list(output)
        if expected[:2] + expected[3:] != actual[:2] + actual[3:]:
            failures.append(dict(case=index, input=words, expected=expected, actual=actual))
        if expected[2] != actual[2]:
            flag_failures.append(dict(case=index, expected=expected[2], actual=actual[2]))
    sha = lambda data: hashlib.sha256(data).hexdigest()
    files = ('src/engine/material_correction.cpp', 'src/engine/material_correction.hpp',
             'tests/probes/native_material_correction_probe.cpp', 'tools/oracles/verify_original_material_correction.py')
    report = dict(passed=not failures and not flag_failures, values_match=not failures,
                  cases=len(rows), mismatches=len(failures), fp_flag_mismatches=len(flag_failures),
                  flag_examples=flag_failures[:8], seed=args.seed,
                  first=failures[:8], source_sha256=image.sha256,
                  source_ranges={f'{ENTRY:#x}-{END:#x}': sha(image.read(ENTRY, END - ENTRY))},
                  library_sha256=sha(args.library.read_bytes()),
                  source_files_sha256={name: sha((ROOT / name).read_bytes()) for name in files},
                  cases_sha256=sha(b''.join(pack(row) for row in rows)),
                  limits=['Concrete GetMaterial receipts supply all 17 words even for failing HRESULTs.',
                          'Missing device receipts stop before the call; no uninitialized stack values invented.',
                          'Arithmetic exceptions remain masked, across six precision/rounding control words.',
                          'FP status differences are reported as partial, never a strict pass.',
                          'No native runtime binding, GPU pixels or CharacterSelect completion claim.'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'values_match', 'cases', 'mismatches', 'fp_flag_mismatches', 'first')}))
    return int(bool(failures or flag_failures))


if __name__ == '__main__':
    raise SystemExit(main())
