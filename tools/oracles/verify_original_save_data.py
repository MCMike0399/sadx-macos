"""Check complete initial SAVE_DATA bytes and CRC against original instructions."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct

import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_ESI,
                               UC_X86_REG_EIP, UC_X86_REG_ESP)
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP, DATA

SAVE = 0x3b2b3a8
RINGS = 0x3b0f0e4
SIZE = 1392


class Source:
    def __init__(self, image, input_mod):
        self.machine = Original(image, 127).machine
        for section in image.sections:
            if section['name'] in ('.text', '.rdata'):
                continue
            address = image.base + section['rva']
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            if size:
                self.machine.mem_map(address, size)
                self.machine.mem_write(address, image.slice(section['file_offset'],
                                                           section['raw_size']))
        # Execute the actual pinned DLL's exported default-rumble jump. Its original
        # image base does not overlap the reference sentinel's single header page.
        for section in input_mod.sections:
            address = input_mod.base + section['rva']
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            if size:
                self.machine.mem_map(address, size)
                self.machine.mem_write(address, input_mod.slice(section['file_offset'],
                                                               section['raw_size']))
        patch, target = struct.unpack('<II', input_mod.read(input_mod.base + 0xe0a8, 8))
        if (patch, target) != (0x42d52d, 0x10009240):
            raise RuntimeError('pinned Input default-rumble jump changed')
        self.machine.mem_write(patch, b'\xe9' + struct.pack('<i', target - patch - 5))
        self.machine.hook_add(unicorn.UC_HOOK_CODE, self.check_cookie,
                              begin=0x643ea7, end=0x643ea7)

    @staticmethod
    def check_cookie(machine, address, size, user):
        # Compiler stack-cookie epilogue; no save or gameplay behavior is replaced.
        stack = machine.reg_read(UC_X86_REG_ESP)
        target = struct.unpack('<I', machine.mem_read(stack, 4))[0]
        machine.reg_write(UC_X86_REG_ESP, stack + 4)
        machine.reg_write(UC_X86_REG_EIP, target)

    def run(self, address):
        self.machine.mem_write(STACK, struct.pack('<I', STOP))
        self.machine.reg_write(UC_X86_REG_ESP, STACK)
        self.machine.emu_start(address, STOP, timeout=1000000)
        if self.machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('source save function did not return')

    def initialize(self, rings, pattern):
        self.machine.mem_write(SAVE, bytes([pattern]) * (SIZE + 16))
        self.machine.mem_write(RINGS, struct.pack('<h', rings))
        self.run(0x42d3b0)
        if bytes(self.machine.mem_read(SAVE + SIZE, 16)) != bytes([pattern]) * 16:
            raise RuntimeError('source save initializer changed adjacent record')
        return (bytes(self.machine.mem_read(SAVE, SIZE)),
                struct.unpack('<h', self.machine.mem_read(RINGS, 2))[0])

    def checksum(self, values, scratch):
        self.machine.mem_write(DATA, values)
        self.machine.reg_write(UC_X86_REG_ESI, DATA)
        self.machine.reg_write(UC_X86_REG_EDI, scratch)
        self.run(0x42cf90)
        return self.machine.reg_read(UC_X86_REG_EAX) & 65535


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seed', type=int, default=20261020)
    parser.add_argument('--cases', type=int, default=1000)
    parser.add_argument('--input-dll', type=Path, default=Path(
        'reference/updated-pc-reference/game/mods/sadx-input-mod/sadx-input-mod.dll'))
    args = parser.parse_args()
    if hashlib.sha256(args.exe.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise RuntimeError('unexpected source executable')
    input_sha = hashlib.sha256(args.input_dll.read_bytes()).hexdigest()
    if input_sha != 'c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5':
        raise RuntimeError('unexpected pinned Input mod')
    image = PEImage(args.exe)
    input_mod = PEImage(args.input_dll)
    source = Source(image, input_mod)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    crc = library.sadx_native_save_checksum
    crc.argtypes = [ctypes.POINTER(ctypes.c_uint8)]
    crc.restype = ctypes.c_uint16
    initialize = library.sadx_native_save_initialize
    initialize.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_int16]
    initialize.restype = ctypes.c_int16
    mismatches = []
    init_count = 0
    initial_hash = None
    for rings in (0, 1, 999, 32767, -32768, -1):
        for pattern in (0, 85, 170, 255):
            expected, expected_rings = source.initialize(rings, pattern)
            native = (ctypes.c_uint8 * SIZE)()
            result = initialize(native, rings)
            init_count += 1
            initial_hash = hashlib.sha256(expected).hexdigest()
            if result != expected_rings or bytes(native) != expected:
                mismatches.append({'kind': 'initialize', 'rings': rings,
                                   'differences': [[i, a, b] for i, (a, b) in
                                                   enumerate(zip(native, expected)) if a != b][:20]})
    rng = random.Random(args.seed)
    rows = [bytes([value]) * SIZE for value in (0, 1, 85, 170, 255)]
    rows += [rng.randbytes(SIZE) for _ in range(args.cases)]
    for index, values in enumerate(rows):
        expected = source.checksum(values, rng.getrandbits(32))
        result = crc((ctypes.c_uint8 * SIZE).from_buffer_copy(values))
        if result != expected:
            mismatches.append({'kind': 'checksum', 'case': index,
                               'original': expected, 'native': result})
    root = Path(__file__).resolve().parents[2]
    paths = ['src/game/save_data.hpp', 'src/game/save_data.cpp',
             'tests/probes/native_save_data_probe.cpp']
    report = {
        'source_sha256': SOURCE_SHA256,
        'input_mod_sha256': input_sha,
        'profile_patch': {'site': '0x42d52d', 'target': '0x10009240',
                          'replacement_bytes': input_mod.read(0x10009240, 14).hex()},
        'source_ranges': {'0x42d3b0-0x42d622': hashlib.sha256(
            image.read(0x42d3b0, 0x272)).hexdigest()},
        'native_sources': {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in paths},
        'native_library_sha256': hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
        'seed': args.seed, 'cases': init_count + len(rows), 'initialization_cases': init_count,
        'checksum_cases': len(rows), 'initial_save_sha256': initial_hash,
        'mismatches': len(mismatches), 'first_mismatches': mismatches[:5],
        'passed': not mismatches,
        'limits': ['Tests initial save record and CRC; persistence and story progression need journeys.',
                   'Compiler security-cookie epilogue is bypassed; all save callees execute.',
                   'Pinned Input default-rumble replacement executes from the actual DLL.'],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': report['cases'], 'mismatches': len(mismatches),
                      'first_mismatch': mismatches[:1]}))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
