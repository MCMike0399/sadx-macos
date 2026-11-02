"""Complete 4c3870 instructions versus the compiled buffered-input restoration.

All 1024 memory words are compared, including aliased source/destination storage.
Access denials stop before the selected instruction; these qualify ordered failure
prefixes, not exact hardware exception PCs. No floating-point operations occur.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage
import unicorn as uc
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP

ENTRY, END = 0x4c3870, 0x4c3893
BASE, STACK, STOP = 0x3b0e000, 0x11000800, 0x12000000
STEPS = {0x4c3874: 1, 0x4c387e: 2, 0x4c3885: 3, 0x4c388b: 4}
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'


def packed(words):
    return struct.pack('<' + 'I' * len(words), *words)


class Source:
    def __init__(self, image):
        assert image.sha256 == SOURCE_SHA and uc.__version__ == '2.1.4'
        self.machine = m = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        for page in (ENTRY & ~4095, BASE, STACK & ~4095, STOP):
            m.mem_map(page, 4096)
        m.mem_write(ENTRY, image.read(ENTRY, END - ENTRY))
        m.hook_add(uc.UC_HOOK_CODE, self.instruction)
        m.hook_add(uc.UC_HOOK_MEM_INVALID, self.invalid)

    def instruction(self, m, address, size, user):
        if address not in STEPS:
            return
        self.step = STEPS[address]
        if self.case[3] == self.step and self.case[4] < 1024:
            m.mem_write(BASE + self.case[4] * 4, packed([self.case[5]]))
        if self.case[2] == self.step:
            self.failure = self.step
            m.emu_stop()

    def invalid(self, m, access, address, size, value, user):
        self.failure = self.step
        return False

    def run(self, case):
        self.case, self.step, self.failure = case, 0, 0
        m = self.machine
        m.mem_write(BASE, packed(case[8:]))
        extra = (BASE + case[1] * 4 - 556) & 0xffffffff
        m.mem_write(STACK, packed([STOP, extra, case[0]]))
        m.reg_write(UC_X86_REG_ESP, STACK)
        try:
            m.emu_start(ENTRY, STOP, count=20)
        except uc.UcError:
            if not self.failure:
                raise
        assert self.failure or m.reg_read(UC_X86_REG_EIP) == STOP
        return list(struct.unpack('<1024I', m.mem_read(BASE, 4096))) + [self.failure, self.step]


def fixtures(count, seed):
    rng = random.Random(seed)
    cases = []
    def add(player=5, source=800, deny=0, mutation=0, word=801, value=0x7f812345):
        words = [rng.getrandbits(32) for _ in range(1024)]
        cases.append([player & 0xffffffff, source, deny, mutation, word, value, 0, 0] + words)
    for player in range(8):
        for deny in range(5):
            add(player=player, deny=deny)
        for source in [488 + player * 2 - 1, 488 + player * 2, 488 + player * 2 + 1]:
            add(player=player, source=source)
        for bits in [0, 0x80000000, 0x7f800000, 0xff800000, 0x7f812345, 0xffc12345]:
            add(player=player, mutation=3, value=bits)
    for player in [-1, 8, -244, -245, 267, 268, 0x20000000, 0x7fffffff, 0x80000000]:
        add(player=player)
    for source in [1023, 1024, 0xffffffff]:
        add(source=source)
    for _ in range(count):
        add(player=rng.randrange(8), source=rng.choice([800, 488, 497, 503, 1023]),
            deny=rng.randrange(5), mutation=rng.randrange(5), word=rng.randrange(1024),
            value=rng.getrandbits(32))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('probe')
    parser.add_argument('--output', default='build/oracles/gamma_input_buffer.json')
    parser.add_argument('--cases', type=int, default=512)
    parser.add_argument('--seed', type=lambda x: int(x, 0), default=0x4c3870)
    args = parser.parse_args()
    image = PEImage(args.exe)
    source = Source(image)
    library = ctypes.CDLL(str(Path(args.probe).resolve()))
    native = library.sadx_native_gamma_input_buffer
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    native.restype = ctypes.c_int
    failures, original_stream, native_stream, prefixes = [], bytearray(), bytearray(), 0
    cases = fixtures(args.cases, args.seed)
    for index, case in enumerate(cases):
        original = source.run(case)
        result = (ctypes.c_uint32 * 1026)()
        assert native((ctypes.c_uint32 * len(case))(*case), result) == 1
        actual = list(result)
        prefixes += bool(original[1024])
        original_stream.extend(packed(original))
        native_stream.extend(packed(actual))
        differences = [i for i, pair in enumerate(zip(original, actual)) if pair[0] != pair[1]]
        if differences:
            failures.append({'case_id': index, 'config': case[:8], 'words': differences,
                             'original': [original[i] for i in differences],
                             'native': [actual[i] for i in differences]})
    report = {'source_sha256': image.sha256,
              'source_ranges': {f'{ENTRY:#x}-{END:#x}':
                                hashlib.sha256(image.read(ENTRY, END - ENTRY)).hexdigest()},
              'cases': len(cases), 'failure_prefixes': prefixes, 'mismatches': failures,
              'mismatch_count': len(failures), 'values_match': not failures,
              'seed': args.seed, 'original_sha256': hashlib.sha256(original_stream).hexdigest(),
              'native_sha256': hashlib.sha256(native_stream).hexdigest(),
              'limits': ['Access denial prefixes do not establish exact hardware exception PCs.',
                         'Complete original body, all memory words; no FP arithmetic/partials.',
                         'The adjacent input-history producer is analyzed, not implemented.']}
    destination = Path(args.output)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ['cases', 'failure_prefixes', 'mismatch_count']}))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
