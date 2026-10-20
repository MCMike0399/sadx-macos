"""RunSegaSonicTeamLogos against original x86 instructions.

Only the update owner is recovered. Objects, display (including the active loader
scaling wrapper), and texture release are explicit call fixtures, not native
implementations. Compare every state word, ordered call and callback-entry state;
also exercise callback changes and failure prefixes. No display/N1 claim.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct

import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP

GLOBALS = (0x3b29d6c, 0x3b29d68, 0x3abdf70, 0x3b2c47c)
CALLS = {0x40b170: 1, 0x42cb20: 2, 0x403290: 3}


class Source:
    def __init__(self, image):
        self.machine = Original(image, 0x37f).machine
        for page in sorted({address & ~4095 for address in GLOBALS}):
            self.machine.mem_map(page, 4096)
        self.machine.hook_add(unicorn.UC_HOOK_CODE, self.call)

    def word(self, address):
        return struct.unpack('<I', self.machine.mem_read(address, 4))[0]

    def write(self, address, value):
        self.machine.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def call(self, machine, address, size, user):
        kind = CALLS.get(address)
        if kind is None:
            return
        stack = machine.reg_read(UC_X86_REG_ESP)
        argument = self.word(stack + 4) if kind == 2 else 0
        self.trace += [kind, argument] + [self.word(a) for a in GLOBALS]
        self.count += 1
        if self.input[4] == self.count:
            self.stopped = True
            machine.emu_stop()
            return
        if kind == 1 and self.input[5]:
            self.write(GLOBALS[0], self.input[6])
            self.write(GLOBALS[1], self.input[7])
        if kind == 3 and self.input[8]:
            self.write(GLOBALS[3], self.input[9])
        machine.reg_write(UC_X86_REG_EIP, self.word(stack))
        machine.reg_write(UC_X86_REG_ESP, stack + 4)

    def run(self, values):
        self.input = values
        self.count, self.trace, self.stopped = 0, [], False
        for address, value in zip(GLOBALS, values):
            self.write(address, value)
        self.write(STACK, STOP)
        self.machine.reg_write(UC_X86_REG_ESP, STACK)
        self.machine.emu_start(0x42cc70, STOP, timeout=1000000)
        if not self.stopped and self.machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('Original logo update did not return with its cdecl ABI')
        result = [self.word(a) for a in GLOBALS]
        result += [99 if self.stopped else self.machine.reg_read(UC_X86_REG_EAX), self.count]
        result += self.trace
        return result + [0] * (24 - len(result))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('native_library', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--seed', type=int, default=20261019)
    parser.add_argument('--cases', type=int, default=4000)
    args = parser.parse_args()
    if hashlib.sha256(args.exe.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise RuntimeError('Unexpected source executable')
    source = Source(PEImage(args.exe))
    library = ctypes.CDLL(str(args.native_library.resolve()))
    native = library.sadx_native_logo
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    native.restype = None
    rng = random.Random(args.seed)
    modes = [0, 1, 2, 3, 14, 15, 16, 17, 18, 0xffffffff]
    frames = [0, 1, 39, 40, 149, 150, 151, 152, 0x7fffffff, 0x80000000, 0xffffffff]
    rows = [[mode, frame, 7, demo, failure, 0, 0, 0, 0, 0]
            for mode in modes for frame in frames for demo in [0, 1, 6, 0xffffffff]
            for failure in [0, 1, 2]]
    rows += [[rng.choice(modes), rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32),
              rng.randrange(3), rng.randrange(2), rng.choice(modes), rng.getrandbits(32),
              rng.randrange(2), rng.choice([0, 1, 6, 0xffffffff])]
             for _ in range(args.cases)]
    mismatches = []
    for index, values in enumerate(rows):
        output = (ctypes.c_uint32 * 24)()
        native((ctypes.c_uint32 * 10)(*values), output)
        expected = source.run(values)
        if list(output) != expected:
            mismatches.append({'case': index, 'input': values,
                               'native': list(output), 'original': expected})
    root = pathlib.Path(__file__).resolve().parents[2]
    report = {
        'source_function': 'SONIC 0x42cc70 RunSegaSonicTeamLogos',
        'source_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
        'source_ranges': {'0x42cc70-0x42cda7': hashlib.sha256(
            PEImage(args.exe).read(0x42cc70, 0x137)).hexdigest()},
        'native_library_sha256': hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
        'native_sources': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in [root / 'src/game/logo.cpp', root / 'src/game/logo.hpp',
                                     root / 'tests/probes/native_logo_probe.cpp']},
        'seed': args.seed, 'cases': len(rows), 'mismatches': len(mismatches),
        'first_mismatches': mismatches[:5], 'passed': not mismatches,
        'limits': ['Object runner, profile-scaled display and texture release are fixtures.',
                   'Initialization, rendering and native N1 journey are not verified.'],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': len(rows), 'mismatches': len(mismatches)}))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
