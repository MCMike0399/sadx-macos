"""Compare both logo initializers, callback mutations and failure prefixes to x86."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct

import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP

GLOBALS = (0x3b29d6c, 0x3b29d68, 0x3abdf70, 0x3b2c47c,
           0x3b2a2e4, 0x3b2a2e8, 0x3b2919c)
CALLS = {0x40b1a0: (1, 0), 0x40b460: (2, 0), 0x42e470: (3, 1),
         0x402f10: (4, 3), 0x421180: (5, 2), 0x411e30: (6, 2),
         0x77ed40: (7, 1), 0x42c4e0: (8, 1), 0x432ea0: (9, 0),
         0x42d3b0: (10, 0)}


class Source:
    def __init__(self, image):
        self.machine = Original(image, 0x7f).machine
        for page in sorted({address & ~4095 for address in GLOBALS}):
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
        args = [self.word(stack + 4 + 4 * i) for i in range(argc)]
        if kind == 5:
            if args != [0x7df3d4, 0x913a48]:
                raise RuntimeError('unexpected source logo resource')
            if bytes(machine.mem_read(args[0], 11)) != b'SEGALOGO_E\0':
                raise RuntimeError('unexpected source logo name')
            args = []
        if kind == 8:
            args = [self.word(args[0])]
        self.trace += [kind] + args + [0] * (3 - len(args))
        self.trace += [self.word(a) for a in GLOBALS]
        self.count += 1
        if self.values[8] == self.count:
            self.stopped = True
            machine.emu_stop()
            return
        if self.values[9] == self.count:
            for a, value in zip(GLOBALS, self.values[10:]):
                self.write(a, value)
        machine.reg_write(UC_X86_REG_EIP, self.word(stack))
        machine.reg_write(UC_X86_REG_ESP, stack + 4)

    def run(self, values):
        self.values = values
        self.count, self.trace, self.stopped = 0, [], False
        for address, value in zip(GLOBALS, values):
            self.write(address, value)
        self.write(STACK, STOP)
        self.machine.reg_write(UC_X86_REG_ESP, STACK)
        self.machine.emu_start(0x42c7b0 if values[7] else 0x42c730, STOP, timeout=1000000)
        if not self.stopped and self.machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('source initializer did not return with cdecl ABI')
        result = [self.word(a) for a in GLOBALS] + [int(not self.stopped), self.count]
        result += self.trace
        return result + [0] * (130 - len(result))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seed', type=int, default=20261020)
    parser.add_argument('--cases', type=int, default=1000)
    args = parser.parse_args()
    if hashlib.sha256(args.exe.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise RuntimeError('unexpected source executable')
    image = PEImage(args.exe)
    source = Source(image)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    native = library.sadx_native_logo_startup
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    native.restype = None
    rng = random.Random(args.seed)
    rows = []
    for entry in (0, 1):
        for stop in range(13):
            for mutation in range(13):
                rows.append([rng.getrandbits(32) for _ in range(7)] +
                            [entry, stop, mutation] +
                            [rng.getrandbits(32) for _ in range(7)])
    rows += [[rng.getrandbits(32) for _ in range(7)] +
             [rng.randrange(2), rng.randrange(13), rng.randrange(13)] +
             [rng.getrandbits(32) for _ in range(7)] for _ in range(args.cases)]
    mismatches = []
    for index, values in enumerate(rows):
        output = (ctypes.c_uint32 * 130)()
        native((ctypes.c_uint32 * 17)(*values), output)
        expected = source.run(values)
        if list(output) != expected:
            mismatches.append({'case': index, 'input': values,
                               'native': list(output), 'original': expected})
    root = Path(__file__).resolve().parents[2]
    paths = ['src/game/logo_startup.hpp', 'src/game/logo_startup.cpp',
             'tests/probes/native_logo_startup_probe.cpp']
    report = {
        'source_sha256': SOURCE_SHA256,
        'source_ranges': {hex(start): hashlib.sha256(image.read(start, size)).hexdigest()
                          for start, size in [(0x42c730, 0x77), (0x42c7b0, 0x20)]},
        'native_sources': {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in paths},
        'native_library_sha256': hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
        'seed': args.seed, 'cases': len(rows), 'mismatches': len(mismatches),
        'first_mismatches': mismatches[:3], 'passed': not mismatches,
        'limits': ['Callees are fixtures; verifies order, arguments, stores and failure prefixes.',
                   'Full save initialization, rendering and native boot need integration checks.'],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': len(rows), 'mismatches': len(mismatches)}))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
