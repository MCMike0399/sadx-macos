"""Compare the recovered speed path with its actual input and transform callees.

No instruction overlays, identity transforms or gate replacements are used.
Runtime state, matrix storage and CRT dispatcher bypass are explicit fixtures.
This is a function-composition check, not a game tick or Windows/mod startup.
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
from unicorn import UC_HOOK_CODE, UC_PROT_READ, UC_PROT_WRITE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_FPCW,
                               UC_X86_REG_FPSW, UC_X86_REG_FPTAG)
from extract import SOURCE_SHA256, TABLE_ADDRESS
from pe_image import PEImage
from verify_original_math import Original, DATA, STACK, STOP
from verify_original_player_speed import cases, word, PARAMETER_BASE, TASK, MOTION, PLAYER, seed_case
from verify_original_player_coordinates import UNIT_ADDRESS, GLOBALS, MATRIX
from verify_original_floor_angles import CONSTANTS, CRT_CONTROL

EVENT, CAMERA = DATA + 0x800, DATA + 0xc00
SEED = 0x43ed50443f50


def sha(data):
    return hashlib.sha256(data).hexdigest()


def fixtures(parameter_words, unit):
    result = []
    rng = random.Random(SEED)
    unit_words = list(struct.unpack('<16I', unit))
    for index, (state, pattern, group) in enumerate(cases(parameter_words)):
        # The former per-call pattern is not a real input producer. Seed a
        # single normalized snapshot consumed by every actual GetAnalog call.
        state = list(state)
        state[1:4] = [rng.getrandbits(32) for _ in range(3)]
        if index % 7 == 0:
            state[0] = rng.choice((0x100, 0x2000, 0x4000, 0x6000))
        slot = rng.randrange(8)
        input_state = [slot, rng.randrange(8), rng.randrange(2),
                       rng.choice((1, 2, 4, 9)), rng.randrange(4), rng.randrange(2),
                       rng.getrandbits(32), word(1 if pattern & 1 else 0),
                       rng.choice((0, 0, 1)), rng.getrandbits(32)]
        depth = rng.choice((1, 2, 5, 6))
        normalize = rng.randrange(2)
        frames = unit_words * 6
        result.append((state + input_state + [depth, normalize] + frames, group))
    # Additional finite cases exercise non-pole slopes through actual CRTasin
    # and both negative/positive floor angle conversions, not a supplied angle.
    for index in range(1000):
        state = list(seed_case(parameter_words))
        state[1:4] = [rng.getrandbits(32) for _ in range(3)]
        state[10:20] = [word(rng.uniform(-4, 4)) for _ in range(10)]
        state[20:23] = [word(rng.uniform(-1, 1)) for _ in range(3)]
        state[23] = 0x4000
        inputs = [rng.randrange(8), 0, 0, 1, 0, 0, 12345,
                  word(rng.choice((0, .5, 1))), 0, 0]
        result.append((state + inputs + [1, index % 2] + unit_words * 6, 'general-floor-slope'))
    return result


class OriginalMovement(Original):
    def __init__(self, image, control_word, unit):
        super().__init__(image, control_word)
        for page in (0x389d000, GLOBALS, 0x2bcc000, CRT_CONTROL & ~0xfff,
                     0x3b0e000, 0x3b22000, 0x3b2c000, 0x3b3c000):
            self.machine.mem_map(page, 0x1000, UC_PROT_READ | UC_PROT_WRITE)
        self.machine.mem_write(UNIT_ADDRESS, unit)
        self.machine.mem_write(0x2bcc000, image.read(0x2bcc000, 0x1000))
        self.machine.mem_write(CRT_CONTROL, struct.pack('<I', 1))
        self.query_count = 0
        self.machine.hook_add(UC_HOOK_CODE, self.count_query, begin=0x43bdc0, end=0x43bdc0)

    def count_query(self, machine, address, size, user):
        self.query_count += 1

    def run(self, words, control_word):
        self.query_count = 0
        task, motion, player = bytearray(64), bytearray(80), bytearray(0x300)
        struct.pack_into('<H', task, 4, words[0] & 0xffff)
        struct.pack_into('<3I', task, 0x14, *words[1:4])
        task[8], task[9] = words[58], words[59]
        struct.pack_into('<I', task, 0x3c, EVENT)
        struct.pack_into('<6I', motion, 4, *words[4:10])
        for offset, values in ((0x38, words[10:13]), (0x2c, words[13:16]),
                               (0x1c, words[16:17]), (0x20, words[17:20]),
                               (0x50, words[20:23]), (0x5c, words[23:24]),
                               (0x1e4, words[24:25]), (0x9c, words[25:58])):
            struct.pack_into('<' + 'I' * len(values), player, offset, *values)
        struct.pack_into('<H', player, 0xa, words[66])
        machine = self.machine
        for address, data in ((TASK, task), (MOTION, motion), (PLAYER, player)):
            machine.mem_write(address, bytes(data))
        machine.mem_write(EVENT + 0x24, bytes([words[60]]))
        for address, value in ((0x3b22dcc, words[61]), (0x3b22dec, words[62]),
                               (0x3b22dc0, words[63])):
            machine.mem_write(address, struct.pack('<H', value))
        machine.mem_write(0x3b0e7a0 + words[58] * 8, struct.pack('<II', *words[64:66]))
        machine.mem_write(0x3b3cdf0 + words[58] * 4, struct.pack('<I', PLAYER))
        machine.mem_write(0x3b2cbb0, struct.pack('<I', CAMERA))
        machine.mem_write(CAMERA + 0x18, struct.pack('<I', words[67]))
        machine.mem_write(MATRIX, struct.pack('<96I', *words[70:]))
        for address, value in ((0x3d0f818, MATRIX + (words[68] - 1) * 64),
                               (0x3d0fa28, 6), (0x3d0fa2c, words[68]),
                               (0x3d0f7ac, words[69])):
            machine.mem_write(address, struct.pack('<I', value))
        machine.mem_write(STACK, struct.pack('<4I', STOP, TASK, MOTION, PLAYER))
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.reg_write(UC_X86_REG_FPCW, control_word)
        machine.reg_write(UC_X86_REG_FPSW, 0)
        machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
        machine.emu_start(0x443f50, STOP, count=10000)
        if machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('Original movement composition did not return with cdecl ABI')
        values = struct.unpack('<6I', machine.mem_read(PLAYER + 0x38, 12) + machine.mem_read(MOTION + 4, 12))
        untouched = struct.unpack('<6I', machine.mem_read(MOTION + 0x10, 12) + machine.mem_read(PLAYER + 0x2c, 12))
        depth = struct.unpack('<I', machine.mem_read(0x3d0fa2c, 4))[0]
        frames = struct.unpack('<96I', machine.mem_read(MATRIX, 384))
        return list(values) + [self.query_count, machine.reg_read(UC_X86_REG_FPSW) & 0x3f, 0, depth] + list(untouched) + list(frames)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('native_library', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    if unicorn.__version__ != '2.1.4':
        raise RuntimeError('Requires pinned Unicorn 2.1.4')
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise RuntimeError('Original executable address-map identity mismatch')
    table, unit = image.read(TABLE_ADDRESS, 4100), image.read(UNIT_ADDRESS, 64)
    constants = b''.join(image.read(address, size) for address, size in CONSTANTS)
    fixture_cases = fixtures(struct.unpack('<33I', image.read(PARAMETER_BASE, 132)), unit)
    blob = b''.join(struct.pack('<166I', *words) for words, _ in fixture_cases)
    fixture_path = args.output.with_suffix('.cases.bin')
    fixture_path.parent.mkdir(parents=True, exist_ok=True)
    fixture_path.write_bytes(blob)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    library.sadx_native_player_movement.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_size_t] * 3 + [
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t, ctypes.c_uint16,
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    library.sadx_native_player_movement.restype = ctypes.c_int
    buffers = [(ctypes.c_ubyte * len(data)).from_buffer_copy(data) for data in (table, unit, constants)]
    report = dict(profile='PGetSpeed with real GetAnalog/task/floor/Ninja/CRT callees; no instruction overlays',
        source_sha256=image.sha256, native_library_sha256=sha(args.native_library.read_bytes()),
        table_sha256=sha(table), unit_sha256=sha(unit), constants_sha256=sha(constants),
        case_blob=dict(path=str(fixture_path.resolve()), sha256=sha(blob), size=len(blob),
                       record='166 little-endian uint32 words'), seed=hex(SEED),
        fixture_count=len(fixture_cases), overlays=[], crt_matherr_policy=1,
        control_words=[], limits=['Explicit normalized-input/runtime/matrix fixtures, not original startup state',
            'Bochs P5/P6 FPATAN model versus Unicorn host-double FPATAN; hardware unverified',
            'CRT error dispatcher bypass explicitly selected for this comparison',
            'No physical input producer, Windows startup, mod DLL execution or complete game ticks'])
    started = time.monotonic()
    for cw in (pc | rc | 0x7f for pc in (0, 0x200, 0x300) for rc in (0, 0x400, 0x800, 0xc00)):
        original = OriginalMovement(image, cw, unit)
        item = dict(control_word=hex(cw), cases=len(fixture_cases), value_mismatches=0,
                    untouched_mismatches=0, call_count_mismatches=0, stack_mismatches=0,
                    flag_mismatches=0, examples=[])
        for index, (words, group) in enumerate(fixture_cases):
            expected = original.run(words, cw)
            output = (ctypes.c_uint32 * 112)()
            status = library.sadx_native_player_movement(buffers[0], 4100, buffers[1], 64,
                buffers[2], 36, (ctypes.c_uint32 * 166)(*words), 166, cw, output, 112)
            if status or output[8]:
                raise RuntimeError(f'Native composition rejected case {index}: status={status}, failure={output[8]}')
            actual = list(output)
            changed = dict(value_mismatches=actual[:6] != expected[:6],
                untouched_mismatches=actual[10:16] != expected[10:16],
                call_count_mismatches=actual[6] != expected[6],
                stack_mismatches=actual[9] != expected[9] or actual[16:] != expected[16:],
                flag_mismatches=actual[7] != expected[7])
            for key, different in changed.items():
                item[key] += different
            # Preserve every numeric/call/frame disagreement; flags get a few
            # representative records, since they are a known oracle limitation.
            if any(v for k, v in changed.items() if k != 'flag_mismatches') or (changed['flag_mismatches'] and len(item['examples']) < 8):
                item['examples'].append(dict(case=index, group=group, original=list(map(hex, expected)),
                                             native=list(map(hex, actual)), differences=changed))
        report['control_words'].append(item)
        print(json.dumps({k: v for k, v in item.items() if k != 'examples'}), flush=True)
    report['stored_values_calls_and_matrix_match'] = all(not any(item[key] for key in (
        'value_mismatches', 'untouched_mismatches', 'call_count_mismatches', 'stack_mismatches'))
        for item in report['control_words'])
    report['emulator_exception_flags_match'] = all(item['flag_mismatches'] == 0 for item in report['control_words'])
    report['duration_seconds'] = time.monotonic() - started
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    if not report['stored_values_calls_and_matrix_match']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
