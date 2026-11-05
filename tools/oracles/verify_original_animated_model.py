"""DrawAction/MDATA2/traversal differential judge.

Original instructions execute through ds_ActionClipNoScale, DrawAction, motion
setup, channel selection and key sampling. Numeric matrix leaves and final model
submission share an ordered argument trace. This does not compare matrix numeric
implementation, raster output, or exception flags.
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
from unicorn.x86_const import *
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage

BASE, STACK, STOP = 0x11000000, 0x12000800, 0x13000000
ACTION, MOTION, TRACKS = BASE + 0x100, BASE + 0x120, BASE + 0x200
OBJECTS = [BASE + 0x400 + i * 64 for i in range(4)]
MODELS = [BASE + 0x600 + i * 64 for i in range(4)]
KEYS = BASE + 0x800
CALLBACK = STOP + 0x100
INPUT_WORDS, OUTPUT_WORDS = 512, 2048
SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'

def packed(words): return struct.pack('<' + 'I' * len(words), *words)
def bits(value): return struct.unpack('<I', struct.pack('<f', value))[0]
def flt(word): return struct.unpack('<f', struct.pack('<I', word))[0]


def fixtures(count, seed):
    rng = random.Random(seed)
    cases = []
    for i in range(count):
        a = [0] * INPUT_WORDS
        # loop, texture status, root presence, motion presence, frame, channels,
        # interpolation, animated callback, callback graph mutation, initial clip,
        # flags, wrapper, simple model enable, palette.
        a[:16] = [0, 0, 1, 1, bits(rng.randrange(0, 512) / 7), (0, 1, 2, 3, 4, 5, 6)[i % 7],
                  2, i % 2, (i // 2) % 4, bits(3.5), rng.getrandbits(32),
                  i % 2, i % 2, (i // 3) % 3, 0, 0]
        # A fixed disjoint tree with independently selected hidden/unit flags.
        for node in range(4):
            at = 16 + node * 13
            flags = rng.randrange(128)
            model = MODELS[node] if rng.randrange(4) else 0
            pose = [bits(rng.randrange(-64, 65) / 4) for _ in range(3)]
            angles = [rng.randrange(-40000, 40001) & 0xffffffff for _ in range(3)]
            scale = [bits(rng.randrange(1, 17) / 8) for _ in range(3)]
            child = OBJECTS[1] if node == 0 else 0
            sibling = OBJECTS[2] if node == 1 else OBJECTS[3] if node == 0 else 0
            a[at:at + 13] = [flags, model] + pose + angles + scale + [child, sibling]
        # Each two-pointer MDATA2 row has independent pointer/count pairs.
        channel_kinds = [kind for kind in range(3) if a[5] & (1 << kind)]
        for node in range(4):
            for channel in range(2):
                at = 68 + (node * 2 + channel) * 17
                a[at] = rng.randrange(1, 5) if rng.randrange(4) else 0
                for k, frame in enumerate((0, 3, 7, 11)):
                    values = [rng.randrange(-60000, 60001) & 0xffffffff for _ in range(3)] if channel < len(channel_kinds) and channel_kinds[channel] == 1 else [bits(rng.randrange(-1000, 1001) / 10) for _ in range(3)]
                    a[at + 1 + k * 4:at + 5 + k * 4] = [frame] + values
        # Guaranteed guard cohorts leave caller's old scratch values intact.
        if i < 16:
            a[0] = int(i == 0)
            a[1] = int(i == 1)
            a[2] = int(i != 2)
            a[3] = int(i != 3)
        cases.append(a)
    # Additional negative frames preserve the original positive/guard cohort.
    # These exercise unsigned CRT frame wrapping and first-key interval selection.
    templates = [case.copy() for case in cases[:16]]
    for frame in (-0.001, -0.5, -0.999, -1.0, -1.5, -2.75, -11.5,
                  -12.0, -12.5, -16.25, -100.25, -1000.0, -65535.75):
        for template in templates:
            case = template.copy()
            case[:4] = [0, 0, 1, 1]
            case[4] = bits(frame)
            cases.append(case)
    # Stop inside a shared numeric leaf, before its caller increments channel.
    for motion_type in range(7):
        template = next(case for case in cases if case[5] == motion_type)
        for leaf in (3, 4, 5):
            case = template.copy()
            case[:4] = [0, 0, 1, 1]
            case[7] = 0
            case[14] = leaf
            case[16] &= ~64
            cases.append(case)
    return cases


class Source:
    def __init__(self, image):
        assert image.sha256 == SHA and uc.__version__ == '2.1.4'
        self.m = m = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        for section in image.sections:
            if section['name'] not in ('.text', '.rdata', '.data'): continue
            address = image.base + section['rva']
            size = (max(section['raw_size'], section['virtual_size']) + 4095) & ~4095
            m.mem_map(address, size)
            m.mem_write(address, image.slice(section['file_offset'], section['raw_size']))
        for page in (BASE, STACK & ~4095, STOP): m.mem_map(page, 4096)
        m.hook_add(uc.UC_HOOK_CODE, self.instruction)
    def read(self, address, count=1): return list(struct.unpack('<' + 'I' * count, self.m.mem_read(address, count * 4)))
    def write(self, address, words): self.m.mem_write(address, packed(words))
    def ret(self, extra=0):
        m = self.m
        sp = m.reg_read(UC_X86_REG_ESP)
        target = self.read(sp)[0]
        m.reg_write(UC_X86_REG_ESP, sp + 4 + extra)
        m.reg_write(UC_X86_REG_EIP, target)
    def event(self, tag, *words):
        self.trace.extend([tag] + list(words) + [0] * (5 - len(words)))
        if self.case[14] and self.case[14] == tag:
            self.failed = True
            self.m.emu_stop()
    def instruction(self, m, address, size, user):
        sp = m.reg_read(UC_X86_REG_ESP)
        if address == STOP:
            m.emu_stop(); return
        if address == 0x403250:
            self.event(1)
            m.reg_write(UC_X86_REG_EAX, self.case[1]); self.ret(); return
        if address in (0x781c80, 0x7816e0):
            self.event(2 if address == 0x781c80 else 8)
            self.ret(); return
        if address in (0x784be0, 0x784c70):
            self.event(3 if address == 0x784be0 else 5, *self.read(sp + 4, 3))
            self.ret(12); return
        if address in (0x781770, 0x7819c0):
            self.event(4, int(address == 0x7819c0), m.reg_read(UC_X86_REG_EDX), *self.read(sp + 4, 2))
            self.ret(8); return
        if address == CALLBACK:
            obj = self.read(sp + 4)[0]
            self.event(6, obj)
            if self.case[8] == 1:
                self.write(obj + 4, [MODELS[3]])
            elif self.case[8] == 2:
                self.write(obj, [self.read(obj)[0] ^ 8])
            elif self.case[8] == 3 and obj == OBJECTS[3]:
                self.write(0x3ab9930, [bits(0.75)])
            self.ret(); return
        # __DrawModel dispatch executes; the two renderer leaves share a trace.
        if address in (0x401430, 0x77ef70):
            self.event(7, self.read(sp + 4)[0], self.read(0x3abd9a4)[0],
                       self.read(0x3ab9930)[0], self.read(0x3abd95c)[0])
            self.ret(); return
    def run(self, case):
        self.case, self.trace, self.failed = case, [], False
        m = self.m
        self.write(BASE, [0] * 1024)
        self.write(ACTION, [OBJECTS[0] if case[2] else 0, MOTION if case[3] else 0])
        self.write(MOTION, [TRACKS, 12, case[5] | (case[6] << 16)])
        for node, obj in enumerate(OBJECTS):
            self.write(obj, case[16 + node * 13:29 + node * 13])
            self.write(MODELS[node], [0] * 10) # zero radius avoids clip-test backend
            row = []
            counts = []
            for channel in range(2):
                at = 68 + (node * 2 + channel) * 17
                key = KEYS + (node * 2 + channel) * 64
                row.append(key if case[at] else 0)
                counts.append(case[at])
                self.write(key, case[at + 1:at + 17])
            self.write(TRACKS + node * 16, row + counts)
        for address, words in ((0x3b1117c, [case[0]]), (0x3ab9908, [CALLBACK if case[7] else 0]),
                               (0x3abd95c, [0]), (0x3abd9a4, [91]),
                               (0x3ab9930, [case[9]]), (0x3ab991c, [bits(-17)]),
                               (0x3ab9910, [TRACKS]), (0x3ab9924, [79]), (0x3ab992c, [83]),
                               (0x3ab9894, [0]), (0x3ab9904, [0]),
                               (0x90a0a0, [case[12]]), (0x3b12208, [case[13]])):
            self.write(address, words)
        m.reg_write(UC_X86_REG_FPCW, 0x007f)
        m.reg_write(UC_X86_REG_FPSW, 0)
        m.reg_write(UC_X86_REG_FPTAG, 0xffff)
        if case[11]:
            self.write(STACK, [STOP, ACTION, case[4]])
            entry = 0x407020
        else:
            self.write(STACK, [STOP, case[4], case[10], 0, 0x403470])
            m.reg_write(UC_X86_REG_EAX, ACTION)
            entry = 0x4053d0
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.emu_start(entry, STOP + 1, count=50000)
        assert self.failed or m.reg_read(UC_X86_REG_EIP) == STOP
        state = [self.read(0x3abd95c)[0], self.read(0x3abd9a4)[0],
                 self.read(0x3ab9930)[0], self.read(0x3ab991c)[0],
                 (self.read(0x3ab9910)[0] - TRACKS) // 16,
                 self.read(0x3ab9924)[0], self.read(0x3ab9894)[0] & 65535,
                 self.read(0x3ab9904)[0] & 65535, self.read(0x3ab992c)[0]]
        objects = sum((self.read(obj, 13) for obj in OBJECTS), [])
        return state + objects + [len(self.trace)] + self.trace


class Native:
    def __init__(self, path):
        self.lib = ctypes.CDLL(str(Path(path).resolve()))
        self.fn = self.lib.animated_model_probe
        self.fn.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
        self.fn.restype = ctypes.c_uint32
    def run(self, case):
        inp, out = (ctypes.c_uint32 * INPUT_WORDS)(*case), (ctypes.c_uint32 * OUTPUT_WORDS)()
        count = self.fn(inp, out)
        if count > OUTPUT_WORDS: return [count] # Explicit native boundary is a mismatch, never a pass.
        return list(out[:count])


def main():
    p = argparse.ArgumentParser()
    p.add_argument('exe'); p.add_argument('probe')
    p.add_argument('--cases', type=int, default=1024)
    p.add_argument('--seed', type=lambda x: int(x, 0), default=0x4053d0)
    p.add_argument('--output', default='build/oracles/animated_model.json')
    a = p.parse_args()
    image = PEImage(a.exe)
    original, native = Source(image), Native(a.probe)
    failures, observations = [], []
    cases = fixtures(a.cases, a.seed)
    for index, case in enumerate(cases):
        expected, actual = original.run(case), native.run(case)
        observations.append(expected)
        if expected != actual:
            first = next((i for i, (x, y) in enumerate(zip(expected, actual)) if x != y), min(len(expected), len(actual)))
            failures.append({'case': index, 'first_word': first,
                             'expected': expected[max(0, first - 4):first + 8],
                             'actual': actual[max(0, first - 4):first + 8],
                             'expected_length': len(expected), 'actual_length': len(actual)})
    report = {'original_exe_sha256': SHA,
              'source_ranges': {f'{start:#x}-{end:#x}': hashlib.sha256(image.read(start, end-start)).hexdigest()
                                for start, end in ((0x407020, 0x40703b), (0x4053d0, 0x40544f),
                                                   (0x404b20, 0x4052e6))},
              'probe_sha256': hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),
              'cases': len(cases), 'positive_cases': a.cases,
              'negative_cases': 13 * min(a.cases, 16), 'leaf_stop_cases': 21, 'seed': a.seed, 'value_failures': len(failures),
              'values_match': not failures, 'first_failures': failures[:12],
              'FP_flags_compared': False, 'FP_partials': None,
              'source_observations_sha256': hashlib.sha256(json.dumps(observations, separators=(',', ':')).encode()).hexdigest(),
              'limits': ['Matrix numeric leaves and model/texture backend are common observational seams.',
                         'Animation angle sampler is original-profile interpolation disabled path.',
                         'No raster, numeric matrix or FP exception flag comparison.']}
    target = Path(a.output); target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return int(bool(failures))

if __name__ == '__main__': sys.exit(main())
