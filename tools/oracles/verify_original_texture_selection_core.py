"""Queued selection against the original wrapper and unmodified reached callees.
The independent legacy judge retains its entire missing-fact corpus. This judge
accepts concrete native records and compares queue stores plus callback-time state.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_ninja_texture_selection import Source, LIST, MEM, STACK, SURFACE

QUEUE = 0x3abd950
QUEUE_INDEX = 0x3ab9934

class QueuedSource(Source):
    def code(self, machine, address, size, user):
        if self.w is None:
            return
        if self.enter_wrapper and address == 0x77f3d0:
            self.enter_wrapper = False
            machine.mem_write(QUEUE, struct.pack('<I', LIST if self.w[48] else 0))
            machine.mem_write(QUEUE_INDEX, struct.pack('<I', self.w[49]))
            machine.mem_write(STACK + 4, struct.pack('<I', machine.reg_read(UC_X86_REG_ECX)))
            machine.reg_write(UC_X86_REG_EIP, 0x403070)
            return
        if address in (0x78cf4e, 0x78ce1b) and self.w[9] != 1:
            words = [struct.unpack('<I', machine.mem_read(p, 4))[0]
                     for p in (0x3d0fdf4, 0x3d129a0, 0x3d0fa24, 0x3ce7128, QUEUE, QUEUE_INDEX)]
            self.callback_state = [words[0], 0 if not words[1] else MEM.index(words[1]) + 1,
                                   int(bool(words[2])), 0 if not words[3] else MEM.index(words[3]) + 1,
                                   int(bool(words[4])), words[5]]
        super().code(machine, address, size, user)

    def run(self, words):
        self.enter_wrapper = words[0] == 1
        self.callback_state = [0] * 6
        self.m.mem_write(QUEUE, struct.pack('<I', LIST if words[48] else 0))
        self.m.mem_write(QUEUE_INDEX, struct.pack('<I', words[49]))
        expected = super().run(words)
        expected['queued'] = int(bool(struct.unpack('<I', self.m.mem_read(QUEUE, 4))[0]))
        expected['queued_index'] = struct.unpack('<I', self.m.mem_read(QUEUE_INDEX, 4))[0]
        expected['callback_state'] = self.callback_state
        return expected

def fixtures(seed, count):
    rng = random.Random(seed)
    # Directed Cartesian coverage includes null lists, null surfaces, palette
    # misses/hits/revision changes, aliasing, failed callbacks and wrapped offsets.
    for operation in range(3):
        for fmt in (0, 0x08000000, 0x10000000, 0x18000000, 0x28000000, 0xffffffff):
            for surface in (0, SURFACE):
                for bank in (0xffffffff, 2, 5000):
                    for cache in (7, 99):
                        for receipt in (0, 1, 2):
                            w = base()
                            w[0], w[18], w[19], w[17], w[5], w[9] = operation, fmt, surface, bank, cache, receipt
                            yield w
    for index in (0, 1, 2, 3, 0x40000000, 0x80000000, 0xc0000000, 0xffffffff, 0x15555556):
        w = base(); w[0] = 2; w[1] = index; yield w
    for kind in (0, 1):
        for size in (0, 1, 3):
            w = base(); w[0] = 1; w[2] = kind; w[41] = size; yield w
    for revision in (0, 5, 6):
        for surface in (0, SURFACE):
            w = base(); w[18] = 0x28000000; w[17] = 2; w[20] = revision; w[19] = surface; yield w
    for _ in range(count):
        w = base()
        w[0] = rng.randrange(3); w[1] = rng.choice([0, 1, 2, 3, 0x40000000, rng.getrandbits(32)])
        w[2] = rng.randrange(2); w[3] = rng.randrange(3); w[5] = rng.choice([7, 8, 9, rng.getrandbits(32)])
        w[9] = rng.randrange(3); w[11] = rng.getrandbits(32)
        w[35] = rng.choice([2, 5000]); w[36] = rng.randrange(8)
        w[37] = rng.choice([1, 2]); w[41] = rng.randrange(4)
        w[46] = rng.randrange(4); w[47] = rng.randrange(4)
        w[48] = rng.randrange(2); w[49] = rng.getrandbits(32)
        for i in range(3):
            w[16 + i * 6] = rng.choice([7, 8, 9, rng.getrandbits(32)])
            w[17 + i * 6] = rng.choice([0xffffffff, 2, 5000])
            w[18 + i * 6] = rng.choice([0, 0x08000000, 0x10000000, 0x18000000, 0x28000000, rng.getrandbits(32)])
            w[19 + i * 6] = rng.choice([0, SURFACE])
            w[20 + i * 6] = rng.randrange(8)
        yield w

def base():
    w = [0] * 50
    w[2] = 1; w[4] = 3; w[5] = 99; w[8] = 1; w[11] = 0xffffffff
    w[12] = 1; w[34] = 1000; w[35] = 2; w[36] = 5; w[37] = 1
    w[39] = 1; w[40] = 1; w[41] = 3; w[42] = 3
    w[46] = 2; w[47] = 3; w[48] = 1; w[49] = 123
    for i in range(3):
        w[16 + i * 6:22 + i * 6] = [7 + i, 0xffffffff, 0, SURFACE, 0, 31]
    return w

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=pathlib.Path); parser.add_argument('library', type=pathlib.Path)
    parser.add_argument('--output', required=True, type=pathlib.Path)
    parser.add_argument('--seed', type=int, default=314159); parser.add_argument('--random-cases', type=int, default=3000)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('Source/oracle identity mismatch')
    library = ctypes.CDLL(str(args.library.resolve()))
    probe = library.sadx_native_texture_selection_core
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32)] * 2; probe.restype = ctypes.c_int
    source = QueuedSource(image); cases = list(fixtures(args.seed, args.random_cases)); failures = []
    for index, words in enumerate(cases):
        expected = source.run(words)
        output = (ctypes.c_uint32 * 40)()
        if probe((ctypes.c_uint32 * 50)(*words), output): raise RuntimeError('Native probe rejected concrete input')
        actual = dict(boundary=output[1], completed=bool(output[2]), outcome=output[3], current=bool(output[4]),
                      cache=output[5], requested=output[6], active=output[7], versions=list(output[8:11]),
                      calls=[tuple(output[12 + i * 3:15 + i * 3]) for i in range(output[11])],
                      queued=output[22], queued_index=output[23], callback_state=list(output[24:30]))
        if not expected['completed']:
            expected.pop('outcome'); actual.pop('outcome')
        if actual != expected: failures.append(dict(case=index, input=words, expected=expected, actual=actual))
    sha = lambda data: hashlib.sha256(data).hexdigest()
    report = dict(passed=not failures, cases=len(cases), seed=args.seed, mismatches=len(failures), first=failures[:8],
                  source_sha256=image.sha256, library_sha256=sha(args.library.read_bytes()),
                  source_ranges={f"{start:#x}-{end:#x}": sha(image.read(start, end-start))
                                 for start, end in [(0x403070, 0x403089), (0x77f3d0, 0x77f3f0),
                                                    (0x77f440, 0x77f467), (0x78cdc0, 0x78cf14),
                                                    (0x78cf20, 0x78cf73)]},
                  source_files_sha256={path: sha((pathlib.Path(__file__).resolve().parents[2] / path).read_bytes())
                                       for path in ['src/engine/texture_selection_native.hpp',
                                                    'src/engine/texture_selection_native.cpp',
                                                    'src/engine/texture_selection_internal.hpp',
                                                    'tests/probes/native_texture_selection_core_probe.cpp',
                                                    'tools/oracles/verify_original_texture_selection_core.py',
                                                    'tools/oracles/verify_original_ninja_texture_selection.py']},
                  cases_sha256=sha(b''.join(struct.pack('<50I', *w) for w in cases)),
                  limits=['Synthetic completed/unavailable device receipts, no physical GPU claim',
                          'Dirty palette paths stop at the original surface lock after revision store',
                          'Concrete native records; unchanged legacy 683-case judge separately covers missing facts'])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ['passed', 'cases', 'mismatches', 'seed', 'first']}))
    return int(bool(failures))

if __name__ == '__main__':
    raise SystemExit(main())
