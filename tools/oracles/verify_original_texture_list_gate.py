"""Compare the native typed List gate with every original isTextureNG instruction.

Concrete imports only: entries.size() is the full validated source count and
array, and first_filename_invalid represents exactly the first -1 filename.
The legacy queue judge retains responsibility for missing guest/fixture facts.
No callee interception, source substitutions, game execution or graphics claims.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/lib'))
from extract import SOURCE_SHA256
from pe_image import PEImage

START, END = 0x403250, 0x40328d
RANGE_SHA256 = '6df189d84533a30623a99b4e6c68bf5a4b5053abcfb8acf19899035bac7d7eb8'
LIST, ENTRIES, MEMORY, STACK, STOP = 0x11000000, 0x11001000, 0x11002000, 0x12000800, 0x13000000
PRESERVED_ESI = 0x13579bdf


class Source:
    def __init__(self, image):
        code = image.read(START, END - START)
        if hashlib.sha256(code).hexdigest() != RANGE_SHA256:
            raise RuntimeError('Original gate range identity mismatch')
        self.machine = m = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
        m.mem_map(START & ~4095, 4096)
        m.mem_write(START, code)
        m.mem_protect(START & ~4095, 4096, unicorn.UC_PROT_READ | unicorn.UC_PROT_EXEC)
        m.mem_map(LIST, 0x3000)
        m.mem_map(STACK & ~4095, 4096)
        m.mem_map(STOP, 4096, unicorn.UC_PROT_EXEC)
        m.hook_add(unicorn.UC_HOOK_MEM_READ, self.read)
        m.hook_add(unicorn.UC_HOOK_MEM_WRITE, self.write)

    def read(self, machine, access, address, size, value, user):
        if LIST <= address < LIST + 0x3000:
            self.reads.append((address, size))

    def write(self, machine, access, address, size, value, user):
        self.writes.append((address, size, value))

    def run(self, words):
        present, count, sentinel, *bindings = words
        m = self.machine
        # Irrelevant filename/attribute fields are deliberately distinct. Only
        # the first filename controls the result, after every binding is checked.
        records = [(0xffffffff if sentinel else 0x2468ace0, 0xabcdef01, identity)
                   if i == 0 else (0xffffffff, i + 17, identity)
                   for i, identity in enumerate(bindings)]
        m.mem_write(LIST, struct.pack('<2I', ENTRIES, count))
        m.mem_write(ENTRIES, b''.join(struct.pack('<3I', name, attr,
                                                 MEMORY + 16 * binding if binding else 0)
                                      for name, attr, binding in records))
        m.mem_write(STACK, struct.pack('<I', STOP))
        m.reg_write(UC_X86_REG_EAX, LIST if present else 0)
        m.reg_write(UC_X86_REG_ESI, PRESERVED_ESI)
        m.reg_write(UC_X86_REG_ESP, STACK)
        self.reads, self.writes = [], []
        m.emu_start(START, STOP, count=4096)
        if m.reg_read(UC_X86_REG_EIP) != STOP or m.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('Original gate did not return with its source ABI')
        if m.reg_read(UC_X86_REG_ESI) != PRESERVED_ESI:
            raise RuntimeError('Original gate changed callee-saved ESI')
        expected_reads = []
        if present:
            # Even the zero-count path loads the array pointer, but never reads it.
            expected_reads = [(LIST + 4, 4), (LIST, 4)]
            for i, binding in enumerate(bindings):
                expected_reads.append((ENTRIES + 12 * i + 8, 4))
                if not binding:
                    break
            else:
                if count:
                    expected_reads.append((ENTRIES, 4))
        if self.reads != expected_reads:
            raise RuntimeError(f'Original input read order changed: {self.reads}')
        expected_writes = [(STACK - 4, 4, PRESERVED_ESI)] if present else []
        if self.writes != expected_writes:
            raise RuntimeError(f'Unexpected source writes: {self.writes}')
        return m.reg_read(UC_X86_REG_EAX)


def directed():
    for sentinel in (0, 1):
        yield [0, 0, sentinel]
        yield [0, 3, sentinel, 1, 0, 3]
        yield [1, 0, sentinel]
        for count in [*range(1, 17), 31, 32, 63, 64, 127, 128]:
            bindings = [1 + i % 3 for i in range(count)]
            yield [1, count, sentinel, *bindings]
            for missing in range(count):
                case = bindings.copy()
                case[missing] = 0
                yield [1, count, sentinel, *case]
            yield [1, count, sentinel, *([0] * count)]


def fixtures(seed, count, random_only):
    if not random_only:
        yield from directed()
    rng = random.Random(seed)
    for _ in range(count):
        length = rng.randrange(129)
        bindings = [rng.randrange(1, 4) for _ in range(length)]
        if length and rng.randrange(3):
            for _ in range(rng.randrange(1, length + 1)):
                bindings[rng.randrange(length)] = 0
        yield [int(rng.randrange(8) != 0), length, rng.randrange(2), *bindings]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--seed', type=int, default=403250)
    parser.add_argument('--random-cases', type=int, default=3000)
    parser.add_argument('--random-only', action='store_true')
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('Source/oracle identity mismatch')
    source = Source(image)
    probe = ctypes.CDLL(str(args.library.resolve())).sadx_native_texture_list_gate
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
                      ctypes.POINTER(ctypes.c_uint32)]
    probe.restype = ctypes.c_int
    cases = list(fixtures(args.seed, args.random_cases, args.random_only))
    mismatches, outcomes = [], {}
    for index, words in enumerate(cases):
        expected = source.run(words)
        outcomes[expected] = outcomes.get(expected, 0) + 1
        out = (ctypes.c_uint32 * 2)()
        if probe((ctypes.c_uint32 * len(words))(*words), len(words), out) != 1:
            raise RuntimeError('Native probe rejected a concrete validated fixture')
        if out[0] != expected or out[1] != 1:
            mismatches.append(dict(case=index, input=words, expected=[expected, 1], actual=list(out)))
    sha = lambda data: hashlib.sha256(data).hexdigest()
    files = ['src/engine/texture_list_gate.hpp', 'src/engine/texture_list_gate.cpp',
             'src/engine/native_texture_lifetime.hpp', 'tests/probes/native_texture_list_gate_probe.cpp',
             'tools/oracles/verify_original_texture_list_gate.py']
    report = dict(passed=not mismatches, values_match=not mismatches, cases=len(cases),
                  directed_cases=0 if args.random_only else len(list(directed())),
                  random_cases=args.random_cases, seed=args.seed, outcomes=outcomes,
                  mismatch_count=len(mismatches), mismatches=mismatches[:8],
                  source_sha256=image.sha256, source_ranges={f'{START:#x}-{END:#x}': RANGE_SHA256},
                  source_files_sha256={p: sha((ROOT / p).read_bytes()) for p in files},
                  library_sha256=sha(args.library.read_bytes()),
                  cases_sha256=sha(b''.join(struct.pack(f'<{len(w)}I', *w) for w in cases)),
                  limits=[__doc__, 'Native inputs require validated complete imports; no raw guest pointers',
                          'Integer BOOL result and native List nonmutation compared; native CPU flags not compared',
                          'Original ABI, exact input read order and stack-only write trace checked for every case'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ['passed', 'cases', 'mismatch_count', 'outcomes', 'seed']}))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
