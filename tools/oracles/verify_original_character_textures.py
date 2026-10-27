#!/usr/bin/env python3
"""Judge complete ordinary LoadCharTextures using original instructions.

Lower calls are ordered receipts, not proofs of the lower algorithms. The named
receipt is the profile-active LoadPVM_r boundary (not selection/individual load).
Only valid bounded native views are generated; source arrays include terminators.
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
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_ESI

SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
ENTRY, END = 0x421280, 0x4212d9
DIRECTORY, LOADED = 0x90ed54, 0x3b290c4
BASE, TABLE, ALTERNATE, LISTS, NAMES = 0x10000000, 0x10001000, 0x10001100, 0x10002000, 0x10003000
STACK, STOP = BASE + 0xf000, BASE + 0xff00


class Source:
    def __init__(self, image):
        assert image.sha256 == SOURCE_SHA
        assert unicorn.__version__ == '2.1.4'
        self.m = m = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
        for page in (0x420000, 0x421000, 0x4b3000, 0x90e000, 0x3b29000):
            m.mem_map(page, 4096)
        m.mem_map(BASE, 0x10000)
        # Execute the complete original instruction bytes; lower bodies are receipts.
        m.mem_write(ENTRY, image.read(ENTRY, END - ENTRY))
        m.hook_add(unicorn.UC_HOOK_CODE, self.hook)

    def put(self, at, value, size=4):
        self.m.mem_write(at, (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'))

    def get(self, at, size=4):
        return int.from_bytes(self.m.mem_read(at, size), 'little')

    def hook(self, m, at, size, user):
        if at not in (0x420d80, 0x421180, 0x4b3da0):
            return
        v, out = self.values, self.out
        sp = m.reg_read(UC_X86_REG_ESP)
        kind = {0x421180: 1, 0x420d80: 2, 0x4b3da0: 3}[at]
        name = (self.get(sp + 4) - NAMES) // 64 if kind == 1 else 0
        pointer = self.get(sp + (8 if kind == 1 else 4)) if kind != 3 else 0
        list_id = (pointer - LISTS) // 8 + 1 if pointer else 0
        offset = 32 + self.calls * 4
        out[offset:offset + 4] = [kind, name, list_id, self.get(LOADED + v[0], 1)]
        self.calls += 1
        if self.calls == v[4]:
            index = (m.reg_read(UC_X86_REG_ESI) - TABLE) // 8
            if v[5] & 1: self.put(LOADED + v[0], v[6], 1)
            if v[5] & 2 and index + 1 < 8:
                self.put(TABLE + (index + 1) * 8 + 4, LISTS + (v[7] - 1) * 8 if v[7] else 0)
            if v[5] & 4 and index + 1 < 8:
                self.put(TABLE + (index + 1) * 8, NAMES + v[8] * 64 if v[8] else 0)
            if v[5] & 8: self.put(DIRECTORY + v[0] * 4, ALTERNATE)
            if v[5] & 16: self.put(TABLE + index * 8 + 4, 0)
            if v[5] & 32:
                out[0] = 1
                m.emu_stop()
                return
        m.reg_write(UC_X86_REG_EIP, self.get(sp))
        m.reg_write(UC_X86_REG_ESP, sp + 4)

    def run(self, values):
        self.values, self.out, self.calls = values, [0] * 512, 0
        v = values
        self.m.mem_write(BASE, bytes(0x10000))
        for i in range(9):
            self.put(DIRECTORY + i * 4, TABLE)
            self.put(LOADED + i, v[1] if i == v[0] else 37, 1)
        if v[2]: self.put(DIRECTORY + v[0] * 4, 0)
        for i in range(8):
            name, texture = v[16 + i * 2:18 + i * 2]
            self.put(TABLE + i * 8, NAMES + name * 64 if name else 0)
            self.put(TABLE + i * 8 + 4, LISTS + (texture - 1) * 8 if texture else 0)
        # The view count is the location of the original's implicit sentinel.
        # Entries after count are still compared, but never accessed by the source.
        self.put(TABLE + v[9] * 8 + 4, 0)
        for _ in range(v[10]):
            self.put(STACK, STOP)
            self.put(STACK + 4, v[0])
            self.m.reg_write(UC_X86_REG_ESP, STACK)
            self.m.emu_start(ENTRY, STOP, count=1000)
            if self.out[0]: break
            assert self.m.reg_read(UC_X86_REG_ESP) == STACK + 4
            assert self.m.reg_read(UC_X86_REG_EIP) == STOP
        out = self.out
        out[1] = self.calls
        out[8:17] = [self.get(LOADED + i, 1) for i in range(9)]
        out[17] = int(self.get(DIRECTORY + v[0] * 4) == ALTERNATE)
        for i in range(8):
            name, texture = self.get(TABLE + i * 8), self.get(TABLE + i * 8 + 4)
            out[400 + i * 2] = (name - NAMES) // 64 if name else 0
            out[401 + i * 2] = (texture - LISTS) // 8 + 1 if texture else 0
        return out


def case(character=0, loaded=0, null=0, entries=((1, 1), (0, 2), (3, 3)), trigger=0,
         mutation=0, byte=0, next_list=0, next_name=0, repeat=1, capacity=8):
    v = [character, loaded, null, 0, trigger, mutation, byte, next_list, next_name, capacity, repeat] + [0] * 21
    for i, pair in enumerate(entries): v[16 + i * 2:18 + i * 2] = pair
    # Keep implicit count terminators identical in both worlds.
    if capacity < 8: v[17 + capacity * 2] = 0
    return v


def cases(split):
    if split == 'dev':
        for character in range(9):
            for loaded in range(256):
                yield case(character, loaded)
                yield case(character, loaded, null=1)
        for character in range(9):
            yield case(character, entries=())
            yield case(character, entries=(), capacity=0)
            yield case(character, entries=((4, 0), (1, 1)))
            yield case(character, entries=((0, 1),) * 8)
            for trigger in (1, 2, 3, 4):
                for mode in (1, 2, 4, 8, 16, 32, 3, 5, 9, 33, 63):
                    yield case(character, trigger=trigger, mutation=mode, byte=255,
                               next_list=4, next_name=2)
        for byte in range(256):
            yield case(5, trigger=1, mutation=1, byte=byte)
        yield case(5, loaded=255, repeat=3)
    else:
        rng = random.Random(0x42128020261026)
        for _ in range(4096):
            capacity = rng.randrange(9)
            entries = tuple((rng.randrange(5), rng.randrange(9)) for _ in range(capacity))
            yield case(character=rng.randrange(9), loaded=rng.choice([0, 0, rng.randrange(256)]),
                       null=rng.randrange(8) == 0, entries=entries,
                       trigger=rng.randrange(10), mutation=rng.randrange(64),
                       byte=rng.randrange(256), next_list=rng.randrange(9),
                       next_name=rng.randrange(5), repeat=rng.randrange(1, 4), capacity=8)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('probe', type=Path)
    parser.add_argument('--split', choices=['dev', 'holdout'], default='dev')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.executable)
    source = Source(image)
    lib = ctypes.CDLL(str(args.probe.resolve()))
    native = lib.sadx_native_character_textures
    buffer = ctypes.c_uint32 * 512
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    failures, count = [], 0
    corpus_hash = hashlib.sha256()
    for v in cases(args.split):
        count += 1
        corpus_hash.update(struct.pack('<32I', *v))
        original = source.run(v)
        incoming, outgoing = (ctypes.c_uint32 * 32)(*v), buffer()
        assert native(incoming, outgoing) == 1
        actual = list(outgoing)
        if original != actual:
            failures.append({'case': count, 'inputs': v, 'differences': [
                [i, x, y] for i, (x, y) in enumerate(zip(original, actual)) if x != y][:16]})
            if len(failures) >= 8: break
    report = {
        'name': 'character_textures', 'split': args.split, 'cases': count,
        'value_mismatches': len(failures), 'values_match': not failures, 'passed': not failures,
        'strict': not failures, 'fp_partial_cases': 0, 'failures': failures,
        'source_sha256': image.sha256,
        'original_ranges_sha256': {'0x421280-0x4212d9': hashlib.sha256(image.read(ENTRY, END - ENTRY)).hexdigest()},
        'probe_sha256': hashlib.sha256(args.probe.read_bytes()).hexdigest(),
        'corpus_sha256': corpus_hash.hexdigest(),
        'evidence_id': 'ev_49279b700b5fec055393ce16c7aa84e0ef6d683b686745a628bcc0a54435447b',
        'limits': ['Complete wrapper instructions, ordered lower-callee receipts and byte/table state only.',
                   'Lower algorithms and imported resource contents are not re-proved here.',
                   'No floating-point arithmetic; no actor, runtime binding or N3b completion claim.',
                   'Only valid character indices and bounded views; host invalid-index guard is not original behavior.']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('split', 'cases', 'value_mismatches', 'strict', 'fp_partial_cases')}))
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
