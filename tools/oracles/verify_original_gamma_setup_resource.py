"""Complete Gamma setup wrapper plus real constructor vs compiled native subsystem.

CAlloc is intercepted under the inherited constructor contract. All wrapper and
constructor instructions execute. Definition payload reads are rejected by a memory hook:
setup publishes its identity without reading any payload. Import checks qualify
payload separately. No floating-point comparison or exclusion is involved.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys
import unicorn as uc
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/lib'))
from pe_image import PEImage

WRAPPER, CONSTRUCTOR, ALLOC = 0x4c5420, 0x456c50, 0x40b2d0
POINTER, RESULT, HEAD, COUNT = 0x982148, 0x982154, 0x3c4858c, 0x3c48590
BUFFER, STACK, STOP = 0x11000000, 0x12000800, 0x13000000
DEFINITION, OLD, MARKER = 0x9820f8, 0x20000000, 0x22000000
OTHER_DEFINITION = 0x23000000
MASK = 0xffffffff


class Source:
    def __init__(self, image):
        if image.sha256 != '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74':
            raise ValueError('original source identity mismatch')
        self.wrapper = image.read(WRAPPER, 22)
        self.constructor = image.read(CONSTRUCTOR, 81)
        if hashlib.sha256(self.constructor).hexdigest() != '0fa30c3ff99ef4a7d0f07319edd21692228239ed62574cfb08cee6a3c936526f':
            raise ValueError('constructor source changed')
        if self.wrapper.hex() != 'a1482198006a0050e82318f9ff83c408a354219800c3':
            raise ValueError('Gamma wrapper source changed')
        self.m = m = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        for address in (WRAPPER, CONSTRUCTOR, ALLOC, POINTER, HEAD, BUFFER, STACK, STOP, OTHER_DEFINITION):
            m.mem_map(address & ~4095, 4096)
        m.mem_write(WRAPPER, self.wrapper)
        m.mem_write(CONSTRUCTOR, self.constructor)
        m.hook_add(uc.UC_HOOK_CODE, self.instruction)
        m.hook_add(uc.UC_HOOK_MEM_WRITE, self.write)
        m.hook_add(uc.UC_HOOK_MEM_READ, self.read)

    def word(self, address):
        return struct.unpack('<I', self.m.mem_read(address, 4))[0]

    def put(self, address, value):
        self.m.mem_write(address, struct.pack('<I', value & MASK))

    def read(self, m, access, address, size, value, user):
        for definition in (DEFINITION, OTHER_DEFINITION):
            if address < definition + 80 and definition < address + size:
                raise AssertionError("setup read definition payload")

    def write(self, m, access, address, size, value, user):
        if address == RESULT:
            self.publications.append(value & MASK)

    def instruction(self, m, address, size, user):
        if address != ALLOC:
            return
        self.calls += 1
        sp = m.reg_read(UC_X86_REG_ESP)
        self.observed_result = self.word(RESULT)
        self.observed_definition = self.word(POINTER)
        self.args = [self.word(sp + 4), self.word(sp + 8)]
        if self.case[3]:
            self.put(COUNT, self.case[4])
            self.put(HEAD, MARKER if self.case[6] else 0)
            self.put(RESULT, MARKER)
        if self.case[8] == 1:
            self.put(POINTER, 0)
        if self.case[8] == 2:
            self.put(POINTER, OTHER_DEFINITION)
        if self.case[8] == 3:
            self.put(POINTER, DEFINITION)
        result = 0 if self.case[1] else BUFFER + 256 * self.allocations
        if result:
            self.allocations += 1
            m.mem_write(result, bytes(28))
        m.reg_write(UC_X86_REG_EAX, result)
        m.reg_write(UC_X86_REG_ESP, sp + 4)
        m.reg_write(uc.x86_const.UC_X86_REG_EIP, self.word(sp))

    def run(self, case):
        self.case = case
        self.calls = self.allocations = self.observed_result = self.observed_definition = 0
        self.args = [0, 0]
        self.publications = []
        definition = OTHER_DEFINITION if case[0] == 2 else DEFINITION
        self.put(POINTER, definition if case[0] else 0)
        self.put(RESULT, MARKER)
        self.put(HEAD, OLD if case[5] else 0)
        self.put(COUNT, case[2])
        rows = []
        for _ in range(case[7]):
            self.put(STACK, STOP)
            self.m.reg_write(UC_X86_REG_ESP, STACK)
            before = len(self.publications)
            self.m.emu_start(WRAPPER, STOP, count=100)
            node = self.word(RESULT)
            # Exactly one wrapper publication per invocation, including NULL.
            assert self.publications[before:] == [node]
            rows.extend([self.calls, self.word(COUNT), self.word(HEAD), node,
                         self.word(node) if node else 0,
                         self.word(node + 16) if node else 0,
                         self.word(node + 24) if node else 0,
                         28 if node else 0, self.observed_result,
                         self.observed_definition, self.word(POINTER), *self.args, 0])
            if node:
                raw = bytes(self.m.mem_read(node, 28))
                assert raw[4:16] == bytes(12) and raw[20:28] == bytes(8)
        return rows


def cases(seed, count, holdout):
    if not holdout:
        import itertools
        for defined, fail, mutate, head, after, repeat, change in itertools.product(
                range(2), range(2), range(2), range(2), range(2), (1, 2, 3), range(2)):
            yield [defined, fail, MASK, mutate, MASK, head, after, repeat, change]
    rng = random.Random(seed)
    for _ in range(count):
        yield [rng.randrange(2), rng.randrange(2), rng.getrandbits(32), rng.randrange(2),
               rng.getrandbits(32), rng.randrange(2), rng.randrange(2), rng.randrange(1, 4),
               rng.randrange(2)]
    # Preserve the original cohort, then exercise two distinct non-null identities.
    import itertools
    for defined, fail, mutate, head, repeat, change in itertools.product(
            (1, 2), range(2), range(2), range(2), (1, 2, 3), (0, 2, 3)):
        yield [defined, fail, MASK, mutate, MASK - 1, head, 1, repeat, change]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--probe', default=str(ROOT / 'build/native-gamma-setup-resource-probe.dylib'))
    parser.add_argument('--output', default=str(ROOT / 'build/oracles/gamma_setup_resource.json'))
    parser.add_argument('--seed', type=int, default=5420)
    parser.add_argument('--count', type=int, default=512)
    parser.add_argument('--holdout', action='store_true')
    args = parser.parse_args()
    source = Source(PEImage(args.exe))
    probe = ctypes.CDLL(args.probe).gamma_setup_case
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    probe.restype = ctypes.c_int
    failures = []
    streams = hashlib.sha256()
    for index, case in enumerate(cases(args.seed, args.count, args.holdout)):
        expected = source.run(case)
        actual = (ctypes.c_uint32 * len(expected))()
        assert probe((ctypes.c_uint32 * 9)(*case), actual) == 1, 'probe rejected an unexpected identity'
        streams.update(struct.pack('<' + 'I' * len(expected), *expected))
        if list(actual) != expected:
            failures.append({'case_id': index, 'input': case, 'original': expected, 'native': list(actual)})
    report = {'oracle': 'gamma_setup_resource', 'passed': not failures,
              'cases': index + 1, 'seed': args.seed, 'holdout': args.holdout,
              'definition_identities': [DEFINITION, OTHER_DEFINITION],
              'native_probe_sha256': hashlib.sha256(Path(args.probe).read_bytes()).hexdigest(),
              'mismatches': failures, 'stream_sha256': streams.hexdigest(),
              'source_ranges': {f'{WRAPPER:#x}-{WRAPPER+22:#x}': hashlib.sha256(source.wrapper).hexdigest(),
                                f'{CONSTRUCTOR:#x}-{CONSTRUCTOR+81:#x}': hashlib.sha256(source.constructor).hexdigest()},
              'limits': ['CAlloc intercepted; lower allocator behavior not independently qualified.',
                         'Host bad_alloc and arbitrary memory faults not qualified.',
                         'Definition payload qualified by separate inverse-byte import test.',
                         'No native app or actor display claim.']}
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': index + 1, 'mismatches': len(failures), 'passed': not failures}))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
