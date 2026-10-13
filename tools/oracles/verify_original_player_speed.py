"""Compare PGetSpeed and real tail instructions with explicit external test seeds.

Both rotation callees are identity test doubles and input checks use a selected
per-call bit pattern. This does not verify original rotation/input providers,
Windows startup, functioning gameplay, or original x86 hardware behavior.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct
import time

import unicorn
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import DATA, STACK, STOP, Original

TASK, MOTION, PLAYER = DATA + 0x100, DATA + 0x500, DATA + 0x1000
PARAMETER_BASE = 0x9154E8
SEED = 0x443F50


def digest(data):
    return hashlib.sha256(data).hexdigest()


def word(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def seed_case(parameter_words):
    return [0, 12, (-123456) & 0xffffffff, 67,
            word(91), word(92), word(93), word(81), word(82), word(83),
            word(0), word(0), word(0), word(0), word(0), word(0), word(1),
            word(0), word(0), word(0), word(0), word(1), word(0), 0, 0] + list(parameter_words)


def cases(parameter_words):
    result = []
    # Systematic sign/crossing/stopping cases. Used parameters are explicit
    # test perturbations; all remaining fields retain supplied source bits.
    for x in (-2, -1, -0.05, -0.0, 0.0, 0.05, 0.87, 1, 2):
        for ax in (-3, -0.1, 0.0, 0.1, 3):
            for z in (-2, -0.5, 0, 0.5, 2):
                for az in (-1, 0, 1):
                    item = seed_case(parameter_words)
                    item[10:16] = [word(x), word(0), word(z), word(ax), word(0), word(az)]
                    for offset, value in ((2, 20), (3, 20), (9, 1), (10, 2), (18, -0.1)):
                        item[25 + offset] = word(value)
                    result.append((item, 0, 'finite-systematic'))
    generator = random.Random(SEED)
    for _ in range(2000):
        item = seed_case(parameter_words)
        item[10:23] = [word(generator.uniform(-10, 10)) for _ in range(13)]
        item[23] = generator.choice((0, 0x4000))
        item[24] = generator.choice((0, 2, 0x400000, 0x400002))
        for offset in (9, 10):
            item[25 + offset] = word(generator.uniform(-3, 3))
        for offset in (2, 3):
            item[25 + offset] = word(generator.uniform(-5, 20))
        item[25 + 18] = word(generator.uniform(-3, 3))
        result.append((item, generator.randrange(4), 'finite-random'))
    # Raw words avoid Python float conversion changing NaN payload/signaling.
    for raw in (0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x807fffff,
                0x7f800000, 0xff800000, 0x7fc12345, 0xffc54321, 0x7f812345):
        for index in (10, 11, 12, 13, 14, 15, 16, 17, 25 + 2, 25 + 9):
            for gate_pattern in range(4):
                item = seed_case(parameter_words)
                item[index] = raw
                result.append((item, gate_pattern, 'raw-zero-subnormal-nonfinite'))
    return result


class OriginalSpeed(Original):
    def __init__(self, image, control_word):
        super().__init__(image, control_word)
        self.gate_pattern = 0
        self.gate_calls = 0
        # Function-local memory overlays, never writes to the user's PE file.
        for address in (0x43bdc0, 0x43ec90, 0x43edd0):
            self.machine.mem_write(address, b'\xc3')
        self.machine.hook_add(UC_HOOK_CODE, self.input_stub, begin=0x43bdc0, end=0x43bdc0)

    def input_stub(self, machine, address, size, user_data):
        if self.gate_calls >= 32:
            raise RuntimeError('Test gate pattern exhausted; unexpected original call count')
        active = (self.gate_pattern >> self.gate_calls) & 1
        self.gate_calls += 1
        machine.reg_write(UC_X86_REG_EAX, active)

    def run(self, words, gate_pattern, control_word):
        self.gate_pattern, self.gate_calls = gate_pattern, 0
        task, motion, player = bytearray(64), bytearray(80), bytearray(0x300)
        struct.pack_into('<H', task, 4, words[0] & 0xffff)
        struct.pack_into('<3I', task, 0x14, *words[1:4])
        struct.pack_into('<6I', motion, 4, *words[4:10])
        for offset, values in ((0x38, words[10:13]), (0x2c, words[13:16]),
                               (0x1c, words[16:17]), (0x20, words[17:20]),
                               (0x50, words[20:23]), (0x5c, words[23:24]),
                               (0x1e4, words[24:25]), (0x9c, words[25:58])):
            struct.pack_into('<' + 'I' * len(values), player, offset, *values)
        self.machine.mem_write(TASK, bytes(task))
        self.machine.mem_write(MOTION, bytes(motion))
        self.machine.mem_write(PLAYER, bytes(player))
        self.machine.mem_write(STACK, struct.pack('<4I', STOP, TASK, MOTION, PLAYER))
        self.machine.reg_write(UC_X86_REG_ESP, STACK)
        self.machine.reg_write(UC_X86_REG_FPCW, control_word)
        # Clear only accrued low-six exceptions between independent cases.
        self.machine.reg_write(UC_X86_REG_FPSW, self.machine.reg_read(UC_X86_REG_FPSW) & ~0x3f)
        self.machine.emu_start(0x443f50, STOP, count=2000)
        if self.machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('Original PGetSpeed failed to return with expected ABI')
        values = struct.unpack('<6I', self.machine.mem_read(PLAYER + 0x38, 12) +
                               self.machine.mem_read(MOTION + 4, 12))
        untouched = struct.unpack('<6I', self.machine.mem_read(MOTION + 0x10, 12) +
                                  self.machine.mem_read(PLAYER + 0x2c, 12))
        return values, untouched, self.gate_calls, self.machine.reg_read(UC_X86_REG_FPSW) & 0x3f


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('native_library', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--cases-output', type=pathlib.Path)
    args = parser.parse_args()
    if unicorn.__version__ != '2.1.4':
        raise RuntimeError('This profile requires Unicorn 2.1.4')
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise RuntimeError('The function address map does not match supplied executable')
    parameter_bytes = image.read(PARAMETER_BASE, 132)
    fixture_cases = cases(struct.unpack('<33I', parameter_bytes))
    blob = b''.join(struct.pack('<59I', *words, pattern) for words, pattern, _ in fixture_cases)
    blob_path = args.cases_output or args.output.with_suffix('.cases.bin')
    blob_path.parent.mkdir(parents=True, exist_ok=True)
    blob_path.write_bytes(blob)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    library.sadx_native_player_speed.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
        ctypes.c_uint16, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    library.sadx_native_player_speed.restype = ctypes.c_int
    report = dict(profile='exact-PGetSpeed-and-tail-i386/native-arm64/explicit-external-test-seeds',
        source_path=str(image.path), source_sha256=image.sha256,
        native_library_path=str(args.native_library.resolve()),
        native_library_sha256=digest(args.native_library.read_bytes()),
        parameter_source=dict(address=hex(PARAMETER_BASE), size=132, row='Sonic', sha256=digest(parameter_bytes)),
        case_blob=dict(path=str(blob_path.resolve()), sha256=digest(blob), bytes=len(blob),
                       record='58 little-endian uint32 source/state words followed by one gate-pattern uint32'),
        seed=hex(SEED), case_groups={group: sum(g == group for _, _, g in fixture_cases)
                                     for group in sorted({g for _, _, g in fixture_cases})},
        function_bytes={hex(a): dict(size=b-a, sha256=digest(image.read(a,b-a)))
                        for a,b in ((0x443f50,0x4441ab),(0x785170,0x785189),
                                    (0x43c380,0x43c4b4),(0x43c4c0,0x43c547))},
        overlays=[dict(address=hex(a), bytes='c3', behavior=behavior)
                  for a,behavior in ((0x43bdc0,'selected per-call input gate, EAX set by hook'),
                                     (0x43ec90,'identity task-to-world test double'),
                                     (0x43edd0,'identity floor-to-world test double'))],
        control_words=[], limits=['Input gate and BOTH transform providers are explicit test doubles',
            'PGetSpeed/vector-add/limiter/cleanup original instructions actually execute',
            'Control words and source parameter perturbations are explicit experimental fixtures',
            'Emulator exception flags are reported separately and do not establish hardware flag equivalence',
            'Original x86 hardware, actual game startup/control state, mods and functioning gameplay are unverified'])
    started = time.monotonic()
    for cw in (p | q | 0x7f for p in (0,0x200,0x300) for q in (0,0x400,0x800,0xc00)):
        original = OriginalSpeed(image, cw)
        item = dict(control_word=hex(cw), cases=len(fixture_cases), value_mismatches=0,
                    untouched_mismatches=0, call_count_mismatches=0, exception_flag_mismatches=0, examples=[])
        for index,(words,pattern,group) in enumerate(fixture_cases):
            actual, untouched, call_count, exceptions = original.run(words,pattern,cw)
            output = (ctypes.c_uint32 * 16)()
            status = library.sadx_native_player_speed((ctypes.c_uint32 * 58)(*words),58,cw,pattern,output,16)
            if status or output[8]:
                raise RuntimeError(f'Native probe rejected case {index}: status={status}, failure={output[8]}')
            numeric = tuple(output[:6]) != actual
            preserved = tuple(output[10:16]) != untouched
            calls_differ = output[6] != call_count
            flags_differ = output[7] != exceptions
            item['value_mismatches'] += numeric
            item['untouched_mismatches'] += preserved
            item['call_count_mismatches'] += calls_differ
            item['exception_flag_mismatches'] += flags_differ
            if (numeric or preserved or calls_differ or flags_differ) and len(item['examples']) < 10:
                item['examples'].append(dict(case=index,group=group,gate_pattern=pattern,
                    original_values=[hex(v) for v in actual],native_values=[hex(v) for v in output[:6]],
                    calls=[call_count,output[6]],exception_flags=[hex(exceptions),hex(output[7])]))
        report['control_words'].append(item)
        print(f"CW={cw:#x}: {len(fixture_cases)} cases, values={item['value_mismatches']}, "
              f"untouched={item['untouched_mismatches']}, calls={item['call_count_mismatches']}, "
              f"flag differences={item['exception_flag_mismatches']}",flush=True)
    report['tested_values_and_calls_match'] = all(
        not any(item[k] for k in ('value_mismatches','untouched_mismatches','call_count_mismatches'))
        for item in report['control_words'])
    report['emulator_exception_flags_match'] = all(not item['exception_flag_mismatches'] for item in report['control_words'])
    report['duration_seconds'] = time.monotonic()-started
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    if not report['tested_values_and_calls_match']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
