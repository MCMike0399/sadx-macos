"""Compare the unscaled DisplayLogoScreen call plan with original x86 code.

Texture selection, material/color setup and the draw callees are explicit fixtures.
The active loader's background-scaling wrapper is not executed; this verifies the
original body, not profile-scaled pixels or native N1. Compare low-six x87
exception flags; full condition/tag/stack flags are outside the native interface.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct

import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP

CALLS = {0x403070: (1, 1), 0x432f40: (2, 1), 0x433170: (3, 0),
         0x433010: (4, 1), 0x4338d0: (5, 4), 0x4071c0: (6, 6)}


def bits(number):
    return struct.unpack('<I', struct.pack('<f', number))[0]


class Source:
    def __init__(self, image):
        self.machine = Original(image, 0x7f).machine
        for page in (0x3b29000, 0x3b2c000, 0x892000):
            self.machine.mem_map(page, 4096)
        self.machine.hook_add(unicorn.UC_HOOK_CODE, self.call)

    def word(self, address):
        return struct.unpack('<I', self.machine.mem_read(address, 4))[0]

    def write(self, address, value):
        self.machine.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def call(self, machine, address, size, user):
        if address not in CALLS:
            return
        kind, argc = CALLS[address]
        stack = machine.reg_read(UC_X86_REG_ESP)
        values = [self.word(stack + 4 + 4 * i) for i in range(argc)]
        if kind == 1:
            if values != [0x913a48]:
                raise RuntimeError('Unexpected source logo texture list')
            values = []
        self.trace += [kind] + values + [0] * (7 - len(values))
        self.count += 1
        if self.input[4] == self.count:
            self.stopped = True
            machine.emu_stop()
            return
        machine.reg_write(UC_X86_REG_EIP, self.word(stack))
        machine.reg_write(UC_X86_REG_ESP, stack + 4)

    def run(self, values):
        self.input = values
        self.count, self.trace, self.stopped = 0, [], False
        for address, value in zip((0x3b29d68, 0x8928c0, 0x8928c4, 0x3b2c604), values):
            self.write(address, value)
        self.write(STACK, STOP)
        self.write(STACK + 4, values[6])
        self.machine.reg_write(UC_X86_REG_ESP, STACK)
        self.machine.reg_write(UC_X86_REG_FPCW, values[5])
        self.machine.reg_write(UC_X86_REG_FPSW, 0)
        self.machine.emu_start(0x42cb20, STOP, timeout=1000000)
        if not self.stopped and self.machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('Source display did not return with cdecl ABI')
        output = [int(not self.stopped), self.count,
                  self.machine.reg_read(UC_X86_REG_FPCW),
                  self.machine.reg_read(UC_X86_REG_FPSW) & 63] + self.trace
        return output + [0] * (96 - len(output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('native_library', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--seed', type=int, default=20261019)
    parser.add_argument('--cases', type=int, default=128)
    args = parser.parse_args()
    if hashlib.sha256(args.exe.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise RuntimeError('Unexpected source executable')
    image = PEImage(args.exe)
    source = Source(image)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    native = library.sadx_native_logo_display
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    native.restype = None
    rng = random.Random(args.seed)
    rows = []
    for precision in [0, 512, 768]:
        for rounding in [0, 1024, 2048, 3072]:
            control = precision | rounding | 127
            for frame in [0, 1, 31, 32, 33, 40, 150, 0x7fffffff, 0x80000000, 0xffffffff]:
                for scale in [0.0, 1.0, 1.2, -1.0]:
                    for stop in [0, 1, 4, 8, 11]:
                        rows.append([frame, bits(scale), bits(scale), 0xa1b2c3d4,
                                     stop, control, 12])
            for _ in range(args.cases):
                rows.append([rng.getrandbits(32), bits(rng.uniform(-4, 4)),
                             bits(rng.uniform(-4, 4)), rng.getrandbits(32),
                             rng.randrange(12), control, rng.randrange(256)])
    mismatches = []
    flag_mismatches = 0
    for index, values in enumerate(rows):
        output = (ctypes.c_uint32 * 96)()
        native((ctypes.c_uint32 * 7)(*values), output)
        expected = source.run(values)
        flag_mismatches += output[3] != expected[3]
        if list(output)[:3] + list(output)[4:] != expected[:3] + expected[4:]:
            mismatches.append({'case': index, 'input': values,
                               'native': list(output), 'original': expected})
    root = pathlib.Path(__file__).resolve().parents[2]
    sources = ['src/game/logo.hpp', 'src/game/logo_display.cpp',
               'tests/probes/native_logo_display_probe.cpp']
    report = {
        'source_ranges': {'0x42cb20-0x42cc6a': hashlib.sha256(
            image.read(0x42cb20, 0x14a)).hexdigest()},
        'native_sources': {s: hashlib.sha256((root / s).read_bytes()).hexdigest() for s in sources},
        'native_library_sha256': hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
        'seed': args.seed, 'cases': len(rows), 'mismatches': len(mismatches),
        'first_mismatches': mismatches[:3], 'values_match': not mismatches,
        'flag_mismatches': flag_mismatches,
        'passed': not mismatches and not flag_mismatches,
        'limits': ['Unpatched original display body with setup/draw call fixtures.',
                   'Active profile scaling wrapper, downstream pixels and N1 unverified.',
                   'Only low-six x87 exception bits compared; full status/tag/stack outside native API.',
                   'Soft x87 and Unicorn can differ on exception flags; these remain explicit partials.'],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': len(rows), 'mismatches': len(mismatches),
                      'flag_mismatches': flag_mismatches}))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
