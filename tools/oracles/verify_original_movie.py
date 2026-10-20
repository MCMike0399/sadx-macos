"""Compare movie wrapper/control flow with explicit media/input service fixtures.

The decoder readiness fixture is false: this judges playlist, held-skip, reset,
quit, wrapper order and return values. Fade/render have separate judgments.
"""
import argparse
import hashlib
import ctypes
import json
import pathlib
import struct
import sys
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
SPEEDRUN_SHA256 = "f3fc68a3058fb726142547018da860b170f137248e54b82712b52527e7b74802"
DEFAULT_SPEEDRUN_DLL = pathlib.Path(__file__).resolve().parents[2] / "reference/updated-pc-reference/game/mods/sadx-speedrun-mod/sadx-speedrun-mod.dll"
from verify_original_math import Original, STACK, STOP


class Source:
    def __init__(self, image, speedrun_dll=DEFAULT_SPEEDRUN_DLL):
        if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
            raise RuntimeError('Pinned source/Unicorn identity mismatch')
        self.m = Original(image, 127).machine
        for section in image.sections:
            if section['name'] in ['.text', '.rdata']:
                continue
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            address = image.base + section['rva']
            self.m.mem_map(address, size)
            self.m.mem_write(address, image.slice(section['file_offset'], min(size, section['raw_size'])))
        dll = PEImage(speedrun_dll)
        if dll.sha256 != SPEEDRUN_SHA256:
            raise RuntimeError('Pinned Speedrun 2.0 DLL mismatch')
        hook = 0x10007a40
        self.hook_bytes = dll.read(hook, 15)
        if self.hook_bytes != bytes.fromhex('b8a8e3b0038b00a80cff254c000110'):
            raise RuntimeError('Compiled Skipma routine changed')
        self.hook_return = dll.read(0x1001004c, 4)
        if self.hook_return != struct.pack('<I', 0x5132ba):
            raise RuntimeError('Compiled Skipma return pointer changed')
        self.speedrun_dll_sha256 = dll.sha256
        for section in dll.sections:
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            address = dll.base + section['rva']
            self.m.mem_map(address, size)
            self.m.mem_write(address, dll.slice(section['file_offset'], min(size, section['raw_size'])))
        self.m.mem_write(0x5132b1, b'\xe9' + struct.pack('<i', hook - 0x5132b6))
        self.m.mem_write(0x5132b6, b'\x90' * 4)
        self.compiled_hook_visits = 0
        self.m.hook_add(unicorn.UC_HOOK_CODE, self.hook)

    def put(self, address, value):
        self.m.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def read(self, address):
        return struct.unpack('<I', self.m.mem_read(address, 4))[0]

    def hook(self, m, address, size, user):
        if address == 0x10007a40:
            self.compiled_hook_visits += 1
        calls = {0x780be0: 1, 0x406f00: 2, 0x403180: 3, 0x781250: 4,
                 0x77ed40: 5, 0x40ef40: 6, 0x40fdc0: 7, 0x513850: 8,
                 0x513c50: 9, 0x513ed0: 10, 0x513d30: 11, 0x40f460: 12,
                 0x40efe0: 13, 0x40ef30: 14, 0x513d50: 15, 0x513990: 16,
                 0x512d30: 17, 0x40bf30: 18, 0x40bf40: 19, 0x5136e0: 20,
                 0x513840: 21}
        ignored = [0x7800e0, 0x780120, 0x780130, 0x10000200]
        if address not in calls and address not in ignored:
            return
        stack = m.reg_read(UC_X86_REG_ESP)
        if address in calls:
            self.calls.append(calls[address])
        result = 0
        if address == 0x513ed0:
            self.calls.append(self.values[7] & 0xffffffff)
            result = int(self.values[5] == 0)
        elif address == 0x40f460:
            self.pumps += 1
            self.put(0x3b0e3a8, self.values[1] if self.pumps >= self.values[2] else 0)
            if self.values[6] and self.pumps >= self.values[6]:
                self.put(0x3abdf7c, 1)
        elif address == 0x40efe0:
            result = int(self.values[3] != 0 and self.pumps >= self.values[3])
        elif address == 0x513d50:
            result = int(self.pumps < self.values[4])
        elif address == 0x512d30:
            self.calls.append(self.read(stack + 4) & 255)
        elif address == 0x10000200:
            result = 1234
        m.reg_write(UC_X86_REG_EAX, result)
        m.reg_write(UC_X86_REG_EIP, self.read(stack))
        m.reg_write(UC_X86_REG_ESP, stack + 4)

    def run(self, values):
        self.values, self.calls, self.pumps = values, [], 0
        self.put(0x3abdf7c, 0)
        self.put(0x3b2c45c, 77)
        self.put(0x3abdf58, 41)
        self.put(0x3c60000, 0)
        self.put(0x3c5fff4, 0)
        self.put(0x3c5fff0, 0)
        self.put(0x3b0e3a8, 0)
        self.put(0x7db270, 0x10000200)
        self.put(STACK, STOP)
        self.put(STACK + 4, values[0])
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.emu_start(0x42f020, STOP, count=30000)
        result = [self.m.reg_read(UC_X86_REG_EAX), self.read(0x3abdf7c),
                  self.read(0x3b2c45c), self.read(0x3abdf58), self.read(0x3c60000),
                  self.read(0x3c5fff4), self.read(0x3c5fff0), len(self.calls)] + self.calls
        return result + [0] * (1024 - len(result))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('native_library', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--speedrun-dll', type=pathlib.Path, default=DEFAULT_SPEEDRUN_DLL)
    args = parser.parse_args()
    image = PEImage(args.exe)
    source = Source(image, args.speedrun_dll)
    native = ctypes.CDLL(str(args.native_library.resolve()))
    bad, count = [], 0
    for movie_id in range(9):
        for held in [0, 1, 4, 8, 12, 16, 255]:
            for skip_at in [0, 1, 3]:
                for reset_at, quit_at, open_fail in [(0, 0, 0), (2, 0, 0), (0, 2, 0), (0, 0, 1)]:
                    values = [movie_id, held, skip_at, reset_at, 5, open_fail, quit_at, -7400]
                    count += 1
                    output = (ctypes.c_uint32 * 1024)()
                    native.sadx_native_movie((ctypes.c_uint32 * 8)(*values), output)
                    expected = source.run(values)
                    if list(output) != expected:
                        bad.append({'input': values, 'native': list(output)[:60], 'original': expected[:60]})
    report = {'cases': count, 'mismatches': len(bad), 'first_mismatches': bad[:3], 'limits': __doc__}
    root = pathlib.Path(__file__).resolve().parents[2]
    files = ['src/game/movie.hpp', 'src/game/movie.cpp', 'tests/probes/native_movie_probe.cpp']
    report['passed'] = not bad
    report['source_sha256'] = image.sha256
    report['native_library_sha256'] = hashlib.sha256(args.native_library.read_bytes()).hexdigest()
    report['native_sources'] = {f: hashlib.sha256((root / f).read_bytes()).hexdigest() for f in files}
    report['source_ranges'] = {
        f'{hex(start)}-{hex(end)}': hashlib.sha256(image.read(start, end - start)).hexdigest()
        for start, end in [(0x42f020, 0x42f072), (0x5130a0, 0x5133b7)]}
    report['profile'] = 'Executes pinned compiled Skipma replacement; decoder readiness false.'
    report['speedrun_dll_sha256'] = source.speedrun_dll_sha256
    report['compiled_skipma'] = {'hook': '0x10007a40', 'bytes': source.hook_bytes.hex(),
                                 'return_pointer': '0x1001004c', 'return_bytes': source.hook_return.hex(),
                                 'executed': source.compiled_hook_visits}
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': count, 'mismatches': len(bad)}))
    return bool(bad)


if __name__ == '__main__':
    raise SystemExit(main())
