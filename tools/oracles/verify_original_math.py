"""Compare native math to selected original instructions under Unicorn 2.1.4.

This executes only the selected pure functions, not PE startup, Win32 imports,
or the full game. FPU control words are explicit experimental inputs. Output
checks binary32 stores, not every use of the original extended register result.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct
import sys
import time
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_PROT_READ, UC_PROT_WRITE, UC_PROT_EXEC
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_FPCW
from pe_image import PEImage
from extract import SOURCE_SHA256, TABLE_ADDRESS

CODE = 0x10000000
STOP = CODE + 0x100
DATA = 0x11000000
FIRST = DATA + 0x100
SECOND = DATA + 0x200
STACK = DATA + 0xf000


class Vector(ctypes.Structure):
    _fields_ = [('x', ctypes.c_float), ('y', ctypes.c_float), ('z', ctypes.c_float)]


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


class Original:
    def __init__(self, image, control_word):
        self.machine = Uc(UC_ARCH_X86, UC_MODE_32)
        for section in image.sections:
            if section['name'] not in ('.text', '.rdata'):
                continue
            size = (section['raw_size'] + 4095) & ~4095
            address = image.base + section['rva']
            self.machine.mem_map(address, size, UC_PROT_READ | UC_PROT_WRITE)
            self.machine.mem_write(address, image.slice(section['file_offset'], section['raw_size']))
            self.machine.mem_protect(address, size,
                                     UC_PROT_READ | (UC_PROT_EXEC if section['name'] == '.text' else 0))
        self.machine.mem_map(CODE, 0x1000, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)
        self.machine.mem_map(DATA, 0x10000, UC_PROT_READ | UC_PROT_WRITE)
        self.machine.reg_write(UC_X86_REG_FPCW, control_word)
        self.wrappers = {}

    def call_float(self, address, ecx=0, edx=0, stack_argument=None):
        wrapper_address = self.wrappers.get(address)
        if stack_argument is not None:
            self.machine.mem_write(FIRST, struct.pack('<I', stack_argument))
        # Keep each executable wrapper immutable after its first execution;
        # this also avoids depending on JIT invalidation for self-modifying code.
        prefix = b'' if stack_argument is None else b'\xff\x35' + struct.pack('<I', FIRST)
        if wrapper_address is None:
            wrapper_address = CODE + len(self.wrappers) * 32
        relative = address - (wrapper_address + len(prefix) + 5)
        # call original; fstp binary32 to DATA; ret to explicit STOP.
        wrapper = prefix + b'\xe8' + struct.pack('<i', relative) + b'\xd9\x1d' + struct.pack('<I', DATA) + b'\xc3'
        if address not in self.wrappers:
            self.machine.mem_write(wrapper_address, wrapper)
            self.wrappers[address] = wrapper_address
        self.machine.mem_write(STACK, struct.pack('<I', STOP))
        self.machine.reg_write(UC_X86_REG_ESP, STACK)
        self.machine.reg_write(UC_X86_REG_ECX, ecx & 0xffffffff)
        self.machine.reg_write(UC_X86_REG_EDX, edx & 0xffffffff)
        self.machine.emu_start(wrapper_address, STOP, count=1000)
        if self.machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('Original function did not return with the expected ABI')
        return struct.unpack('<I', self.machine.mem_read(DATA, 4))[0]

    def vectors(self, first, second):
        self.machine.mem_write(FIRST, struct.pack('<3f', *first))
        self.machine.mem_write(SECOND, struct.pack('<3f', *second))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('native_library', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    if unicorn.__version__ != '2.1.4':
        raise RuntimeError('This verification profile requires Unicorn 2.1.4')
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise RuntimeError('The original-function address map does not match this executable')
    library = ctypes.CDLL(str(args.native_library.resolve()))
    library.sadx_native_trig.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_size_t,
                                        ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t, ctypes.c_uint16]
    library.sadx_native_trig.restype = ctypes.c_int
    library.sadx_native_dot.argtypes = [ctypes.POINTER(Vector), ctypes.POINTER(Vector), ctypes.c_uint16]
    library.sadx_native_dot.restype = ctypes.c_uint32
    library.sadx_native_scalar_squared.argtypes = [ctypes.POINTER(Vector), ctypes.c_uint16]
    library.sadx_native_scalar_squared.restype = ctypes.c_uint32
    library.sadx_native_signed_sqrt.argtypes = [ctypes.c_float, ctypes.c_uint16]
    library.sadx_native_signed_sqrt.restype = ctypes.c_uint32
    table_bytes = image.read(TABLE_ADDRESS, 4100)
    table_buffer = (ctypes.c_ubyte * 4100).from_buffer_copy(table_bytes)
    results = (ctypes.c_uint32 * (65536*2))()
    report = dict(profile='original-i386-instructions/unicorn-2.1.4/native-arm64-binary32-store',
                  source_path=str(image.path), source_sha256=image.sha256,
                  native_library_sha256=hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
                  table_sha256=hashlib.sha256(table_bytes).hexdigest(), control_words=[],
                  limits=['Full Windows game startup and runtime are not executed',
                          'Experimentally selected FPU control words; actual game control state remains unverified',
                          'Only selected pure functions and their binary32 stored results are compared',
                          'An emulator is the instruction oracle; original x86 hardware equivalence is unverified'])
    started = time.monotonic()
    # The original game control word has not been established. Check both the
    # 24/53/64-bit significand modes and every rounding direction. Exceptions
    # are masked. These are tests, not assumptions about the running PC game.
    generator = random.Random(0x53414458)
    vector_pairs = [((1., 2., 3.), (4., 5., 6.)),
                    ((1., -(2.**60), 2.**60), (1., 1., 1.)),
                    ((-(2.**60), 1., 2.**60), (1., 1., 1.)),
                    ((-(2.**59), 1., 2.**59), (1., 1., 1.)),
                    ((-(2.**60), -(2.**-10), 2.**60), (1., 1., 1.)),
                    ((0., -0., 0.), (0., 0., -0.)),
                    ((float('inf'), 1., 0.), (0., 1., 1.)),
                    ((float('nan'), 1., 0.), (1., 1., 1.)),
                    ((2.**-149, -2.**-149, 2.**-126), (1., 1., 1.))]
    for _ in range(1000):
        vector_pairs.append((tuple(struct.unpack('<f', struct.pack('<f', generator.uniform(-100,100)))[0] for _ in range(3)),
                             tuple(struct.unpack('<f', struct.pack('<f', generator.uniform(-100,100)))[0] for _ in range(3))))
    failed = False
    for control_word in (precision | rounding | 0x7f
                         for precision in (0, 0x200, 0x300)
                         for rounding in (0, 0x400, 0x800, 0xc00)):
        original = Original(image, control_word)
        if library.sadx_native_trig(table_buffer, len(table_buffer), results, len(results), control_word):
            raise RuntimeError('Native table probe rejected its inputs')
        item = dict(control_word=hex(control_word), trig_cases=65536*2,
                    trig_mismatches=0, dot_cases=len(vector_pairs), dot_mismatches=0,
                    squared_cases=len(vector_pairs), squared_mismatches=0,
                    sqrt_cases=0, sqrt_mismatches=0, examples=[])
        print('Comparing original instructions at FPCW=' + hex(control_word), flush=True)
        for angle in range(65536):
            for component, address in enumerate((0x788290, 0x7883c0)):
                actual = original.call_float(address, ecx=angle)
                expected = results[angle*2+component]
                if actual != expected:
                    item['trig_mismatches'] += 1
                    if len(item['examples']) < 10:
                        item['examples'].append(dict(function=hex(address), angle=angle,
                                                     original=hex(actual), native=hex(expected)))
        for first, second in vector_pairs:
            original.vectors(first, second)
            a, b = Vector(*first), Vector(*second)
            actual = original.call_float(0x787cf0, ecx=FIRST, edx=SECOND)
            expected = library.sadx_native_dot(ctypes.byref(a), ctypes.byref(b), control_word)
            if actual != expected:
                item['dot_mismatches'] += 1
                if len(item['examples']) < 10:
                    item['examples'].append(dict(function='dot', first_bits=[hex(bits(v)) for v in first],
                                                 second_bits=[hex(bits(v)) for v in second],
                                                 original=hex(actual), native=hex(expected)))
            actual = original.call_float(0x787e10, ecx=FIRST)
            expected = library.sadx_native_scalar_squared(ctypes.byref(a), control_word)
            if actual != expected:
                item['squared_mismatches'] += 1
        for value in (0., -0., 1., -1., 9., -9., 0.1, -0.1, 1e-30, -1e-30, 1e30, -1e30):
            raw = bits(value)
            value = struct.unpack('<f', struct.pack('<I', raw))[0]
            actual = original.call_float(0x7871f0, stack_argument=raw)
            expected = library.sadx_native_signed_sqrt(value, control_word)
            item['sqrt_cases'] += 1
            if actual != expected:
                item['sqrt_mismatches'] += 1
                if len(item['examples']) < 10:
                    item['examples'].append(dict(function='signed_sqrt', input=hex(raw),
                                                 original=hex(actual), native=hex(expected)))
        failed |= any(item[k] for k in ('trig_mismatches','dot_mismatches','squared_mismatches','sqrt_mismatches'))
        report['control_words'].append(item)
        print(json.dumps(item), flush=True)
    report['duration_seconds'] = time.monotonic() - started
    report['tested_profile_matches'] = not failed
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    return int(failed)


if __name__ == '__main__':
    sys.exit(main())
