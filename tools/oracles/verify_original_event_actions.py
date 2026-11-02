"""Exact event queue/admission comparison, using real CAlloc and native arena allocations."""

import argparse
import ctypes
import hashlib
import json
import random
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "tools/lib"), str(ROOT / "tools/oracles")]
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP

PRIMARY, REPLACEMENT = DATA + 0x100, DATA + 0x200
EVENT, OTHER = DATA + 0x300, DATA + 0x400
TASK, ACTION, REGISTERED, ALTERNATE = DATA + 0x500, DATA + 0x600, DATA + 0x700, DATA + 0x800
NODES = {n: DATA + 0x1000 + 64 * n for n in range(1, 33)}
NODES[32] = DATA + 0x4000
PTRS = {40: PRIMARY, 41: REPLACEMENT, 42: EVENT, 43: OTHER, **NODES}
IDS = {address: n for n, address in PTRS.items()}


class Source:
    def __init__(self, image):
        self.m = Original(image, 127).machine
        self.m.mem_map(0x3B42000, 8192)
        self.m.hook_add(unicorn.UC_HOOK_CODE, self.hook)
        self.m.hook_add(unicorn.UC_HOOK_MEM_INVALID, self.invalid)
        self.m.hook_add(unicorn.UC_HOOK_MEM_WRITE, self.write)
        self.initial = self.m.context_save()

    def get(self, a, n=4):
        return int.from_bytes(self.m.mem_read(a, n), "little")

    def put(self, a, v, n=4):
        self.m.mem_write(a, (v & ((1 << (8 * n)) - 1)).to_bytes(n, "little"))

    def pid(self, a):
        return IDS.get(a, 0 if a == 0 else 0xFFFFFFFF)

    def ret(self, value=0):
        sp = self.m.reg_read(UC_X86_REG_ESP)
        self.m.reg_write(UC_X86_REG_EAX, value)
        self.m.reg_write(UC_X86_REG_EIP, self.get(sp))
        self.m.reg_write(UC_X86_REG_ESP, sp + 4)

    def write(self, m, access, address, size, value, user):
        if address == REGISTERED + 5 and self.v[0] == 0:
            if self.v[14] == 1:
                self.put(0x3B42E10 + 4 * (self.v[8] & 255), ALTERNATE)
            elif self.v[14] == 2:
                self.put(0x3B42E10 + 4 * (self.v[8] & 255), 0)

    def invalid(self, m, access, address, size, value, user):
        pc = m.reg_read(UC_X86_REG_EIP)
        self.failure = {
            0x417E85: 1,
            0x417EF5: 1,
            0x417EA6: 1,
            0x417EF8: 2,
            0x417EA9: 2,
            0x417F21: 3,
            0x417F2A: 4,
            0x417F2F: 5,
            0x417E9B: 6,
        }.get(pc, 99)
        self.fault = {"pc": hex(pc), "address": hex(address)}
        return False

    def hook(self, m, address, size, user):
        if address == 0x40B220:
            sp = m.reg_read(UC_X86_REG_ESP)
            size = self.get(sp + 4)
            result = 0 if self.v[11] else NODES[32]
            self.trace.extend([1, size, 32 if result else 0])
            if result:
                self.allocated[32] = size
            callback = self.v[14]
            if callback == 1:
                self.put(TASK + 32, REPLACEMENT)
            elif callback == 2:
                self.put(PRIMARY + 60, OTHER)
            elif callback == 3:
                self.put(EVENT + 64, 0)
            elif callback == 4 and self.v[1]:
                self.put(NODES[self.v[1]] + 16, NODES[31])
            elif callback == 5:
                self.put(ACTION, self.get(ACTION) ^ 0xFFFFFFFF)
                self.put(ACTION + 4, self.get(ACTION + 4) ^ 0x12345678)
            self.ret(result)
        elif address == 0x40B310:
            sp = m.reg_read(UC_X86_REG_ESP)
            p = self.get(sp + 4)
            identity = self.pid(p)
            self.trace.extend([2, identity, 0])
            if identity in self.freed:
                self.failure = 7
                self.fault = {"pc": hex(address), "address": hex(p)}
                m.emu_stop()
                return
            callback = self.v[14]
            if callback == 6:
                self.put(EVENT + 64, NODES[31])
            elif callback == 7:
                self.put(p + 16, NODES[31])
            elif callback == 8:
                self.put(EVENT + 68, NODES[31])
            elif callback == 9:
                self.put(EVENT + 60, 0x3F800000)
            self.freed.add(identity)
            self.ret()

    def run(self, v):
        self.v = v
        self.failure = 0
        self.fault = None
        self.trace = []
        self.allocated = {}
        self.freed = set()
        self.m.context_restore(self.initial)
        self.m.mem_write(DATA, bytes(0x10000))
        if v[15]:
            for primary, salt in ((PRIMARY, 41), (REPLACEMENT, 73)):
                self.m.mem_write(
                    primary,
                    bytes(1 + ((v[15] % 255 + salt + 37 * offset) % 255) for offset in range(64)),
                )
        self.m.mem_write(0x3B42000, bytes(8192))
        self.put(TASK + 32, 0 if v[2] & 2 else PRIMARY)
        self.put(PRIMARY + 60, 0 if v[2] & 4 else EVENT)
        self.put(REPLACEMENT + 60, OTHER)
        for event in [EVENT, OTHER]:
            self.m.mem_write(event, bytes((17 + 37 * n) & 255 for n in range(124)))
            self.put(event + 64, 0)
            self.put(event + 68, 0)
        self.put(EVENT, 129, 1)
        self.put(EVENT + 60, 0x7FC01234)
        for n in range(1, v[1] + 1):
            p = NODES[n]
            self.allocated[n] = 24
            self.put(p, n, 1)
            self.put(p + 1, n + 20, 1)
            self.put(p + 4, 100 + n)
            self.put(p + 8, 200 + n)
            self.put(p + 12, 300 + n)
            self.put(p + 20, 0x3F000000 + n)
            self.put(EVENT + 64 if n == 1 else NODES[n - 1] + 16, p)
        if (v[14] == 4 or v[14] >= 6) and v[12] != 1:
            self.allocated[31] = 24
        if v[12] == 1:
            self.allocated[31] = 24
            self.put(EVENT + 68, NODES[31])
        elif v[12] == 2 and v[1]:
            self.put(EVENT + 68, NODES[1])
        self.put(ACTION, v[6])
        self.put(ACTION + 4, v[7])
        action = ACTION
        if v[13]:
            action = NODES[32] + 4 * (v[13] - 1)
        if v[2] & 16:
            action = DATA + 0xFFFC
            self.put(action, v[6])
        if v[2] & 8:
            action = 0
        self.put(REGISTERED + 4, v[6], 2)
        self.put(REGISTERED + 1, v[7], 1)
        self.put(0x3B42E10 + 4 * (v[8] & 255), 0 if v[2] & 32 else REGISTERED)
        operation = v[0]
        primary = 0 if v[2] & 2 else PRIMARY
        task = 0 if v[2] & 1 else TASK
        entries = [0x441260, 0x417E80, 0x42FE00, 0x417EF0, 0x417EA0, 0x42FE40]
        texture = [900, 0, 901][v[10]]
        args = [
            [v[8], v[9]],
            [primary],
            [task, action, texture, v[5], v[3], v[4]],
            [primary, action, texture, v[5], v[3], v[4]],
            [primary],
            [task],
        ][operation]
        self.m.mem_write(STACK, struct.pack("<" + "I" * (len(args) + 1), STOP, *args))
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        try:
            self.m.emu_start(entries[operation], STOP, count=10000)
        except unicorn.UcError:
            if not self.failure:
                raise
        if not self.failure and self.m.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError("source did not return")
        out = [0] * 512
        out[:10] = [
            self.failure,
            self.pid(self.get(PRIMARY + 60)),
            self.pid(self.get(EVENT + 64)),
            self.pid(self.get(EVENT + 68)),
            self.get(EVENT, 1),
            self.get(EVENT + 60),
            self.get(REGISTERED + 4, 2),
            self.get(REGISTERED + 1, 1),
            self.pid(self.get(TASK + 32)),
            len(self.trace) // 3,
        ]
        reg = self.get(0x3B42E10 + 4 * (v[8] & 255))
        out[10:13] = [
            0 if not reg else 1 if reg == REGISTERED else 2,
            self.get(ALTERNATE + 4, 2),
            self.get(ALTERNATE + 1, 1),
        ]
        out[13] = int(self.fault["pc"], 16) if self.fault else 0
        out[15] = int(self.m.reg_read(UC_X86_REG_EIP) == STOP)
        out[16 : 16 + len(self.trace)] = self.trace
        for n, size in self.allocated.items():
            p = NODES[n]
            dst = 64 + (n - 1) * 10
            out[dst : dst + 2] = [1, int(n in self.freed)]
            if n == 32 and size == 124:
                out[dst + 2] = self.get(p, 1)
            else:
                # Old distinct storage is zeroed and has no typed action payload.
                out[dst + 2 : dst + 9] = [
                    self.get(p, 1),
                    self.get(p + 1, 1),
                    self.get(p + 4),
                    self.get(p + 8),
                    self.get(p + 12),
                    0 if n in self.freed else self.pid(self.get(p + 16)),
                    self.get(p + 20),
                ]
                if n not in self.freed:
                    out[dst + 9] = self.get(p + 2, 2)

        def snapshot(address, offset, size, event):
            for at in range(0, size, 4):
                value = self.get(address + at)
                if (event and at in (64, 68)) or (not event and at == 60):
                    value = self.pid(value)
                out[offset + at // 4] = value

        snapshot(PRIMARY, 384, 64, False)
        snapshot(EVENT, 400, 124, True)
        snapshot(OTHER, 432, 124, True)
        if self.allocated.get(32) == 124:
            snapshot(NODES[32], 464, 124, True)
        snapshot(REPLACEMENT, 495, 64, False)
        return out


def cases(seed, count):
    rng = random.Random(seed)
    base = [2, 0, 0, 1, 0, 0x3E800000, 0x12345678, 0x9ABCDEF0, 1, 49, 0, 0, 0, 0, 0, 0]
    result = []
    for operation in range(6):
        for length in [0, 1, 2, 11]:
            for flags in [0, 1, 2, 4, 8, 16, 32]:
                v = base.copy()
                v[:3] = [operation, length, flags]
                result.append(v)
    for texture in [0, 1, 2]:
        v = base.copy()
        v[10] = texture
        result.append(v)
    for mode in [0, 1, 127, 128, 255, 256, 257, 0xFFFFFFFF, 0x80000000]:
        for link in [0, 1, 127, 128, 255, 256, 257, 0xFFFFFFFF, 0x80000000]:
            v = base.copy()
            v[3:5] = [mode, link]
            result.append(v)
    for operation in [1, 2, 3, 4, 5]:
        for length in [0, 1, 11]:
            for callback in range(6):
                for fail in [0, 1]:
                    v = base.copy()
                    v[0] = operation
                    v[1] = length
                    v[11] = fail
                    v[14] = callback
                    v[12] = 1
                    if operation == 1:
                        v[2] = 4
                    result.append(v)
    for operation in [4, 5]:
        for length in [0, 1, 2, 11]:
            for old in [0, 1, 2]:
                for callback in range(6, 10):
                    v = base.copy()
                    v[0] = operation
                    v[1] = length
                    v[12] = old
                    v[14] = callback
                    result.append(v)
    for length in [0, 1, 11]:
        for alias in range(1, 6):
            v = base.copy()
            v[1] = length
            v[13] = alias
            v[3] = 258
            v[4] = 259
            result.append(v)
    for length in [1, 2, 11]:
        v = base.copy()
        v[0] = 4
        v[1] = length
        v[12] = 2
        result.append(v)
    for player in [0, 1, 7, 256, 257, 263, 512]:
        for mode in [0, 255, 256, 0xFFFFFFFF]:
            for callback in [0, 1, 2]:
                v = base.copy()
                v[0] = 0
                v[8] = player
                v[9] = mode
                v[14] = callback
                result.append(v)
    fixed_count = len(result)
    for _ in range(count):
        v = base.copy()
        v[0] = rng.randrange(6)
        v[1] = rng.randrange(12)
        v[2] = rng.choice([0, 0, 0, 1, 2, 4, 8, 16, 32])
        v[3] = rng.getrandbits(32)
        v[4] = rng.getrandbits(32)
        v[5] = rng.getrandbits(32)
        v[6] = rng.getrandbits(32)
        v[7] = rng.getrandbits(32)
        v[8] = rng.choice([0, 1, 7, 256, 263])
        v[9] = rng.getrandbits(32)
        v[10] = rng.randrange(3)
        v[11] = int(rng.randrange(8) == 0)
        v[12] = rng.randrange(3)
        v[14] = rng.randrange(10)
        if not v[2] & 24 and rng.randrange(4) == 0:
            v[13] = rng.randrange(1, 6)
        result.append(v)
    # Retain every original input, then repeat fixed paths with distinct nonzero primaries.
    for index in range(fixed_count):
        sentinel_case = result[index].copy()
        sentinel_case[15] = 1 + index % 251
        result.append(sentinel_case)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe")
    parser.add_argument("probe")
    parser.add_argument("--cases", type=int, default=1500)
    parser.add_argument("--seed", type=int, default=417239)
    parser.add_argument("--output", default="build/oracles/event_actions.json")
    args = parser.parse_args()
    image = PEImage(args.exe)
    if hashlib.sha256(Path(args.exe).read_bytes()).hexdigest() != SOURCE_SHA256:
        raise RuntimeError("wrong source image")
    source = Source(image)
    library = ctypes.CDLL(args.probe)
    fn = library.sadx_event_actions_probe
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    fn.restype = ctypes.c_uint
    inputs = cases(args.seed, args.cases)
    failures = []
    observations = hashlib.sha256()
    outcomes = {}
    for index, v in enumerate(inputs):
        expected = source.run(v)
        actual = (ctypes.c_uint32 * 512)()
        assert fn((ctypes.c_uint32 * 16)(*v), actual) == 1
        actual = list(actual)
        outcomes[str(expected[0])] = outcomes.get(str(expected[0]), 0) + 1
        observations.update(struct.pack("<512I", *expected))
        if expected != actual:
            failures.append(
                {
                    "index": index,
                    "input": v,
                    "differences": [
                        (n, a, b) for n, (a, b) in enumerate(zip(expected, actual)) if a != b
                    ][:20],
                    "fault": source.fault,
                }
            )
            if len(failures) >= 8:
                break
    report = {
        "passed": not failures,
        "cases": len(inputs),
        "executed": index + 1,
        "seed": args.seed,
        "outcomes": outcomes,
        "failures": failures,
        "input_stream_sha256": hashlib.sha256(
            json.dumps(inputs, separators=(",", ":")).encode()
        ).hexdigest(),
        "complete_observation_sha256": observations.hexdigest(),
        "source_sha256": SOURCE_SHA256,
        "native_library_sha256": hashlib.sha256(Path(args.probe).read_bytes()).hexdigest(),
        "source_ranges": {
            f"0x{start:x}-0x{end:x}": hashlib.sha256(image.read(start, end - start)).hexdigest()
            for start, end in [
                (0x417E80, 0x417EA0),
                (0x417EA0, 0x417EE2),
                (0x417EF0, 0x417F48),
                (0x42FE00, 0x42FE15),
                (0x42FE40, 0x42FE55),
                (0x441260, 0x44127C),
                (0x40B2D0, 0x40B304),
            ]
        },
        "native_sources": {
            p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest()
            for p in [
                "src/player/event_actions.hpp",
                "src/player/event_actions.cpp",
                "src/engine/task_work.hpp",
                "tests/probes/native_event_actions_probe.cpp",
            ]
        },
        "limits": [
            "Queue/admission and cleanup only; no animation stepping, controller or render qualification.",
            "Void ABI: return/stop and all modeled state/call/allocation/failure observations; incidental caller-saved registers are not API results.",
            "Source CAlloc executes; MAlloc/FreeMemory are recorded allocation boundaries. Native adapter delegates to the real TaskWorkArena.",
            "Duplicate free is a stopped allocator contract violation on both adapters, not a claim about original heap-corruption behavior.",
            "SetInputP registry slots 0..7, including byte-truncated input words; arbitrary beyond-registry memory is unavailable natively.",
        ],
    }
    path = Path(args.output)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n")
    print(
        json.dumps(
            {
                "passed": report["passed"],
                "cases": len(inputs),
                "executed": index + 1,
                "failures": failures[:2],
            }
        )
    )
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
