"""Additional E102Display placement cohort; preserves topology observation format.

Ninja numeric leaves and palette/queue-material backends are common seams. The original
body and integer render leaves execute unchanged, including SetMaterial scratch
stores. This is not a matrix, palette-backend, mesh or pixel equivalence claim.
"""
import ctypes
import hashlib
import json
import random
import struct
from unicorn.x86_const import *
from verify_original_gamma_display import Source, BASE, STACK, STOP, TABLE, WELD, packed, fixtures

GLOBALS = (0x3d0f9c8, 0x3d0f840, 0x3d0f9c4, 0x3b2d3c4, 0x3b2d3c8, 0x3b2d3c0,
           0x3b18220, 0x3b18224, 0x3b18228, 0x3b1822c, 0x3d08844)
SIZE = 1185


class BodySource(Source):
    def read(self, address, count=1):
        return list(struct.unpack('<' + 'I' * count, self.m.mem_read(address, count * 4)))

    def trace(self, op, a=0, b=0):
        self.calls.extend((op, a & 0xffffffff, b & 0xffffffff))

    def ret(self, pop=0):
        sp = self.m.reg_read(UC_X86_REG_ESP)
        target = self.read(sp)[0]
        self.m.reg_write(UC_X86_REG_ESP, sp + 4 + pop)
        self.m.reg_write(UC_X86_REG_EIP, target)

    def instruction(self, m, address, size, user):
        if address == 0x47fdbf:
            self.body = True
        if not self.body:
            return super().instruction(m, address, size, user)
        sp = m.reg_read(UC_X86_REG_ESP)
        if address == 0x412420:
            self.trace(1, self.read(sp + 4)[0]); self.ret()
        elif address == 0x403070:
            # E102Display retains this cdecl argument until its shared 32-byte
            # cleanup after OffConstantAttr. Popping here shifts its locals.
            self.trace(2, self.read(sp + 4)[0]); self.ret()
        elif address == 0x4395c0:
            self.trace(3)
        elif address == 0x77ff60:
            self.trace(4, self.read(sp + 4)[0])
        elif address == 0x4128a0:
            args = self.read(sp + 4, 4)
            self.trace(5, args[0], args[1])
            assert args == [0x3f800000] * 4
        elif address == 0x402f40:
            self.ret()
        elif address == 0x439520:
            self.trace(6)
        elif address == 0x439590:
            self.trace(7, *self.read(sp + 4, 2))
        elif address == 0x7850f0:
            self.trace(9 if m.reg_read(UC_X86_REG_ECX) else 8); self.ret()
        elif address in (0x784ee0, 0x784e20, 0x784fb0):
            self.trace({0x784ee0: 10, 0x784e20: 11, 0x784fb0: 12}[address],
                       m.reg_read(UC_X86_REG_EDX)); self.ret()
        elif address == 0x784d60:
            value = self.read(m.reg_read(UC_X86_REG_EDX), 3)
            self.trace(13, value[0], value[1]); self.trace(14, value[2])
            m.mem_write(self.read(sp + 4)[0], packed([self.case[6], value[1], self.case[7]]))
            self.ret(4)
        elif address == 0x785140:
            self.trace(15); self.ret()
        elif address == 0x785070:
            value = self.read(m.reg_read(UC_X86_REG_EDX), 3)
            self.trace(16, value[0], value[1]); self.trace(17, value[2]); self.ret()
        elif address == 0x7802d0:
            self.trace(18, m.reg_read(UC_X86_REG_ECX))
        elif address in (0x47ff80, 0x48004f, 0x4806ee, 0x4800b4):
            if address == 0x4800b4:
                self.trace(22, m.reg_read(UC_X86_REG_EDI))
            else:
                self.trace({0x47ff80: 19, 0x48004f: 20, 0x4806ee: 21}[address])
            m.emu_stop()

    def run(self, case):
        self.body, self.calls = False, []
        for address, value in zip(GLOBALS, case[10:20] + [0]):
            self.m.mem_write(address, packed([value]))
        self.m.reg_write(UC_X86_REG_FPCW, 0x007f)
        # Source.run's 200-instruction bound fits the old prefix only.
        self.case, self.step, self.failure = case, 0, 0
        m = self.m
        m.mem_write(BASE, packed(case[8:]))
        m.mem_write(TABLE, packed([case[8]])); m.mem_write(WELD, packed([case[9]]))
        m.mem_write(STACK, packed([STOP, case[0]])); m.reg_write(UC_X86_REG_ESP, STACK)
        m.emu_start(0x47fd50, STOP, count=2000)
        assert m.reg_read(UC_X86_REG_EIP) in (0x47ff80, 0x48004f, 0x4806ee, 0x4800b4)
        words = self.read(BASE, 1024)
        words[0] = self.read(TABLE)[0]; words[1] = self.read(WELD)[0]
        result = words + [0, 18] + [self.read(a)[0] for a in GLOBALS] + [0] * 3
        result += [len(self.calls) // 3] + self.calls
        return result + [0] * (SIZE - len(result))


def body_fixtures(count, seed):
    rng = random.Random(seed)
    # All topology accesses valid here; failures/alias/reloads stay in the old cohort.
    template = fixtures(0, seed)[0]
    cases = []
    for index in range(count):
        c = template.copy(); c[1:5] = [0] * 4; c[5] = 1
        def put(offset, value): c[8 + offset // 4] = value
        def float_word(value): return struct.unpack('<I', struct.pack('<f', value))[0]
        primary, player, collision, info, event, extra = (1600, 768, 1664, 1728, 1856, 1536)
        put(256 + 32, BASE + primary)
        put(player + 4, 512 if index & 1 else 0)
        put(primary + 4, 2 << 16 if index % 17 == 0 else 0)
        for axis in range(3):
            put(primary + 20 + axis * 4, rng.choice([0, 1, 32768, 65536, 0xffffffff, rng.getrandbits(32)]))
            put(primary + 32 + axis * 4, float_word(rng.uniform(-1000, 1000)))
        put(player + 284, float_word(rng.uniform(-30, 30)))
        mode = 2 if index % 3 == 0 else 0
        put(player + 288, mode)
        request = rng.choice([0, 1, 22, 45, 46, 52, 69, 65535])
        put(player + 292, request | (rng.randrange(65536) << 16))
        put(primary + 56, BASE + collision); put(collision + 12, BASE + info)
        put(primary + 60, BASE + event); put(event + 64, BASE + extra if index % 11 == 0 else 0)
        c[6:8] = [float_word(rng.uniform(-25, 25)) for _ in range(2)]
        cases.append(c)
    return cases


def judge(image, library, count, seed, output):
    source = BodySource(image)
    native = library.sadx_native_gamma_display
    failures, observations = [], []
    cases = body_fixtures(count, seed)
    for index, case in enumerate(cases):
        expected = source.run(case)
        result = (ctypes.c_uint32 * SIZE)()
        assert native((ctypes.c_uint32 * 1032)(*case), result) == 1
        actual = list(result)
        observations.append({'case': index,
            'input_sha256': hashlib.sha256(packed(case)).hexdigest(),
            'original_sha256': hashlib.sha256(packed(expected)).hexdigest(),
            'native_sha256': hashlib.sha256(packed(actual)).hexdigest()})
        if actual != expected:
            failures.append({'case': index, 'words': [i for i, (a, b) in enumerate(zip(expected, actual)) if a != b],
                             'original_calls': expected[1040:], 'native_calls': actual[1040:]})
    rows = output.with_suffix('.body-observations.json')
    rows.write_text(json.dumps(observations) + '\n')
    return {'cases': len(cases), 'seed': seed, 'value_failures': len(failures),
            'status': 'fail' if failures else 'pass', 'failures': failures[:3],
            'observations_sha256': hashlib.sha256(rows.read_bytes()).hexdigest(),
            'scope': 'original display through ordinary-draw boundary at 0x4800b4, before frame/action selection; palette/queue-material backends and Ninja numeric leaves are common seams; actual integer render leaves and material scratch stores execute'}
