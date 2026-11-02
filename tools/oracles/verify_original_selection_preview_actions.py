"""Exact preview actions/selection comparison; EV_SetAction remains an ordered lower receipt."""
import argparse
import ctypes
import hashlib
import json
import random
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP

WORK, PREVIEW = DATA + 0x100, DATA + 0x200
TASKS = [DATA + 0x1000 + 128 * n for n in range(7)]
PRIMARY = [DATA + 0x2000 + 128 * n for n in range(7)]
EVENTS = [DATA + 0x3000 + 128 * n for n in range(6)]
NODES = [[DATA + 0x4000 + 256 * n + 32 * k for k in range(4)] for n in range(6)]
SEQUENCES = [DATA + 0x6000 + 64 * n for n in range(6)]
LINKS = [DATA + 0x7000 + 64 * n for n in range(6)]
IDS = {node: n * 8 + k + 1 for n, nodes in enumerate(NODES) for k, node in enumerate(nodes)}

class Source:
    def __init__(self, image):
        self.m = Original(image, 127).machine
        for page in [0x3c5f000, 0x3b2a000, 0x3b42000]:
            self.m.mem_map(page, 4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE, self.hook)
        self.m.hook_add(unicorn.UC_HOOK_MEM_INVALID, self.invalid)
        self.initial = self.m.context_save()

    def get(self, address, size=4):
        return int.from_bytes(self.m.mem_read(address, size), 'little')

    def put(self, address, value, size=4):
        self.m.mem_write(address, (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'))

    def invalid(self, m, access, address, size, value, user):
        pc = m.reg_read(UC_X86_REG_EIP)
        self.failure = {0x512c45: 7, 0x512c4c: 8, 0x51295c: 6,
                        0x512a2a: 2, 0x512a4e: 3, 0x512a36: 4}.get(pc, 99)
        self.fault = {'pc': hex(pc), 'address': hex(address)}
        return False

    def hook(self, m, address, size, user):
        if address not in (0x441260, 0x42fe00):
            return
        sp = m.reg_read(UC_X86_REG_ESP)
        args = [self.get(sp + 4 + 4 * n) for n in range(6)]
        if address == 0x441260:
            slot, mode = args[:2]
            self.trace.extend([1, slot, mode, 0])
        else:
            task, action, texture, speed, loop, link = args
            slot = TASKS.index(task)
            assert texture == slot + 100
            self.trace.extend([2, slot, action, link, 2 if speed == 0x3f000000 else 1, loop])
        self.calls += 1
        if self.calls == self.v[13]:
            self.failure = 5
            m.emu_stop()
            return
        if address == 0x441260:
            # Execute actual SetInputP, including its null-registration guard.
            return
        if self.v[9] & 1:
            self.put(WORK + 52, self.v[10], 1)
        if self.v[9] & 2:
            self.put(0x3b2a2fd, self.v[11], 1)
        if self.v[9] & 4:
            self.put(WORK + 45 + slot, self.v[12], 1)
        if self.v[9] & 8 and slot < 6:
            self.put(WORK + 4 * (slot + 1), 0)
        if self.v[9] & 16:
            self.put(task + 32, 0)
        m.reg_write(UC_X86_REG_EAX, 0)
        m.reg_write(UC_X86_REG_EIP, self.get(sp))
        m.reg_write(UC_X86_REG_ESP, sp + 4)

    def run(self, v):
        self.v = v
        self.trace, self.calls, self.failure, self.fault = [], 0, 0, None
        m = self.m
        m.context_restore(self.initial)
        m.mem_write(DATA, bytes(0x9000))
        self.put(0x3c5ff80, 0 if v[5] else PREVIEW)
        self.put(PREVIEW + 44, 0 if v[4] else WORK + 128)
        self.put(WORK + 52, v[2], 1)
        self.put(0x3b2a2fd, v[1], 1)
        for n in range(7):
            self.put(WORK + 4 * n, 0 if v[3] & (1 << n) else TASKS[n])
            self.put(WORK + 45 + n, v[16 + n], 1)
            self.put(TASKS[n] + 32, 0 if v[6] & (1 << n) else PRIMARY[n])
            self.put(0x3b42e10 + 4 * n, 0 if v[14] and n == 1 else PRIMARY[n])
            if n == 1:
                self.put(PRIMARY[n] + 12, v[15])
            if n >= 6:
                continue
            self.put(PRIMARY[n] + 60, 0 if v[8] & (1 << n) else EVENTS[n])
            self.put(EVENTS[n] + 64, 0 if v[7] & (1 << n) else NODES[n][0])
            self.put(0x3c5ff68 + 4 * n, 0)
            for k in range(4):
                self.put(NODES[n][k] + 16, NODES[n][k+1] if k+1 < v[32+n] else 0)
            self.put(0x7f0788 + 16 * n + 4, n + 100)
            self.put(0x7f0788 + 16 * n + 8, SEQUENCES[n])
            self.put(0x7f0788 + 16 * n + 12, LINKS[n])
            for k in range(8):
                self.put(SEQUENCES[n] + 4 * k, n * 16 + k + 1 if k < v[24+n] else 0)
                self.put(LINKS[n] + 4 * k, n * 10 + k + 3)
        m.mem_write(STACK, struct.pack('<II', STOP, v[48] if v[0] else WORK))
        m.reg_write(UC_X86_REG_ESP, STACK)
        try:
            m.emu_start(0x512c40 if v[0] else 0x512910, STOP, count=10000)
        except unicorn.UcError:
            if not self.failure:
                raise
        if not self.failure and m.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError('source did not return')
        out = [0] * 1024
        out[:5] = [self.failure, self.get(0x3b2a2fd, 1), self.get(WORK+52, 1),
                   self.get(PRIMARY[1]+12), self.get(WORK+128+52, 1)]
        for n in range(7):
            out[8+n] = self.get(WORK+45+n, 1)
            out[16+n] = self.get(PRIMARY[n]+1, 1)
            out[24+n] = self.get(PRIMARY[n]+5, 1)
            if n < 6:
                out[32+n] = IDS.get(self.get(0x3c5ff68+4*n), 0)
        out[40] = len(self.trace)
        out[64:64+len(self.trace)] = self.trace
        return out


def cases(seed, count):
    rng = random.Random(seed)
    def base():
        v = [0] * 64
        v[1:3] = [0, 1]
        v[15] = 0xffffffff
        v[24:30] = [3, 3, 3, 3, 4, 3]
        v[32:38] = [1, 2, 3, 4, 1, 2]
        return v
    rows = []
    for slot in range(7):
        for motion in [0, 1, 2, 127, 128, 255]:
            for counter in [0, 1, 2, 127, 128, 255]:
                v = base(); v[1] = slot; v[2] = motion; v[16:23] = [counter] * 7
                rows.append(v)
    for length in range(8):
        for counter in [0, 1]:
            v = base(); v[24:30] = [length]*6; v[16:23] = [counter]*7; rows.append(v)
    for mask_index in [3, 6, 7, 8]:
        for mask in range(128):
            v = base(); v[mask_index] = mask; rows.append(v)
    for failure in range(1, 19):
        v = base(); v[13] = failure; rows.append(v)
        v = base(); v[13] = failure; v[16:23] = [1]*7; rows.append(v)
    for arg in [0, 1, 255, 256, 257, 0xffffffff, 0x80000000]:
        for task, work in [(0, 0), (1, 0), (0, 1)]:
            v = base(); v[0] = 1; v[48] = arg; v[5] = task; v[4] = work; rows.append(v)
    for _ in range(count):
        v = base(); v[1] = rng.randrange(9); v[2] = rng.randrange(256)
        v[3] = rng.randrange(128); v[15] = rng.getrandbits(32)
        v[16:23] = [rng.choice([0, 1, 1, 2, 127, 128, 255]) for _ in range(7)]
        v[24:30] = [rng.randrange(8) for _ in range(6)]
        v[32:38] = [rng.randrange(1, 5) for _ in range(6)]
        v[9] = rng.randrange(32); v[10] = rng.randrange(256); v[11] = rng.randrange(9); v[12] = 7
        if rng.randrange(4) == 0: v[rng.choice([6,7,8])] = rng.randrange(128)
        v[14] = rng.randrange(8) == 0
        rows.append(v)
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe'); parser.add_argument('library'); parser.add_argument('--output', required=True)
    parser.add_argument('--seed', type=int, default=512910); parser.add_argument('--cases', type=int, default=2000)
    args = parser.parse_args()
    image = PEImage(args.exe)
    assert image.sha256 == SOURCE_SHA256
    source = Source(image)
    probe = ctypes.CDLL(args.library).sadx_native_selection_preview_actions_probe
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32)] * 2
    rows = cases(args.seed, args.cases)
    failures, outcomes = [], {}
    input_digest, observation_digest = hashlib.sha256(), hashlib.sha256()
    for n, v in enumerate(rows):
        expected = source.run(v)
        input_digest.update(struct.pack("<64I", *v))
        observation_digest.update(struct.pack("<1024I", *expected))
        out = (ctypes.c_uint32 * 1024)()
        probe((ctypes.c_uint32 * 64)(*v), out)
        diff = [[i, x, y] for i, (x, y) in enumerate(zip(expected, out)) if x != y]
        outcomes[str(expected[0])] = outcomes.get(str(expected[0]), 0) + 1
        if diff:
            failures.append({'case': n, 'input': v, 'diff': diff, 'source_fault': source.fault})
    report = {'passed': not failures, 'cases': len(rows), 'seed': args.seed, 'outcomes': outcomes,
              'failures': failures, 'source_sha256': image.sha256,
              'native_library_sha256': hashlib.sha256(Path(args.library).read_bytes()).hexdigest(),
              'input_stream_sha256': input_digest.hexdigest(),
              'complete_observation_sha256': observation_digest.hexdigest(),
              'native_sources': {p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in
                  ['src/game/selection_preview.hpp', 'src/game/selection_preview_actions.cpp',
                   'tests/probes/native_selection_preview_actions_probe.cpp']},
              'source_ranges': {f'0x{a:x}-0x{b:x}': hashlib.sha256(image.read(a,b-a)).hexdigest()
                                for a,b in [(0x512910,0x512a69),(0x512c40,0x512c50)]},
              'limits': ['EV_SetAction is an ordered receipt, not a qualified native event engine.',
                         'SetInputP runs original instructions; native receipt mirrors its writes.',
                         'All fields and call arguments compare exactly, including failure prefixes.',
                         'No floating-point arithmetic or FP status mutation in the recovered bodies.']}
    Path(args.output).write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k != 'failures'}))
    if failures: print(json.dumps(failures[:2]))
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
