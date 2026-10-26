"""Fresh original CCL bodies versus typed native component; no existing judge is edited.

Numeric cases execute complete ClearInfo/CalcRange, including real njScalor/njSqrt.
Lifecycle cases execute real CCL_Init/free and real CAlloc/FreeMemory/arena allocator.
The task fixture supplies only taskwk linkage; native uses the actual TaskScheduler.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct

import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ESP,
                               UC_X86_REG_FPSW)
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, FIRST, STACK, STOP

ARENA = 0x20000000
TASK = DATA + 0x1000
PRIMARY = DATA + 0x2000
WORK = DATA + 0x3000
INFO = DATA + 0x4000
RANGES = ((0x41caf0, 0x41cbbf), (0x41c4e0, 0x41c521), (0x41caa0, 0x41cae8),
          (0x418b60, 0x418bda), (0x41bac0, 0x41bbda), (0x787cc0, 0x787ce6),
          (0x7871f0, 0x78721b), (0x40b2d0, 0x40b304), (0x40b220, 0x40b2cc),
          (0x40b310, 0x40b41e))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def call(m, address, arguments):
    m.mem_write(STACK, struct.pack('<' + 'I' * (len(arguments) + 1), STOP, *arguments))
    m.reg_write(UC_X86_REG_ESP, STACK)
    m.emu_start(address, STOP, count=6000000)
    if m.reg_read(UC_X86_REG_ESP) != STACK + 4:
        raise RuntimeError(f'ABI mismatch at {address:x}')
    return m.reg_read(UC_X86_REG_EAX)


def u32(m, address):
    return struct.unpack('<I', m.mem_read(address, 4))[0]


def floats(values):
    return b''.join(struct.pack('<f', v) if isinstance(v, float) else struct.pack('<I', v)
                    for v in values)


def record(form, values, damage=255):
    return bytes((3, form, 19, damage)) + struct.pack('<I', 0x87654321) + floats(values) + struct.pack('<3i', -1, 32768, 17)


def numeric_cases(seed, holdout):
    rng = random.Random(seed)
    special = [0, 0x80000000, 1, 0x007fffff, 0x00800000, 0x7f7fffff,
               0x7f800000, 0xff800000, 0x7fc12345, 0x7fa12345,
               0x3dcccccc, 0x3dcccccd, 0x3dcccd00, 0xbf800000]
    out = []
    for form in (0, 1, 2, 3, 4, 5, 6, 7, 255):
        for values in ([0., 0., 0., 0., 0., 0., 0.], [3., 4., 12., 2., 4., 9., -3.],
                       [0., 0., 0., .2, 0., 0., 0.], [0., 0., 0., .1, 0., 0., 0.],
                       [0., 0., 0., -.5, 2., 100., 0.]):
            out.append((1, [record(form, values)]))
        for bits in special:
            for column in range(6):
                values = [1., 2., 3., 4., 5., 6., 0.]
                values[column] = bits
                out.append((1, [record(form, values)]))
    for n in range(500 if holdout else 350):
        entries = []
        for _ in range(rng.randrange(5)):
            values = [rng.uniform(-100, 100) for _ in range(7)]
            if n % 3 == 0:
                values[rng.randrange(6)] = rng.getrandbits(32)
            entries.append(record(rng.randrange(256), values, rng.randrange(256)))
        out.append((n % 2, entries))
    out.extend((0, [record(0, [0.] * 7)] * n) for n in (0, 1, 3, 7))
    return out


def lifecycle(image, native, seed):
    rng = random.Random(seed)
    rows = []
    cases = [(op, count, remaining, ident) for op in range(4) for count in (1, 3)
             for remaining in (0xffffffff, 0, 224) for ident in (0, 2, 3, 7, 128, 255)]
    cases.extend((0, 0, rest, 0) for rest in (0xffffffff, 224))
    for op, count, remaining, ident in cases:
        original = Original(image, 0x7f)
        m = original.machine
        m.mem_map(ARENA, 0x1001000)
        m.mem_map(0x3abd000, 0x1000)
        m.reg_write(UC_X86_REG_EDX, ARENA)
        call(m, 0x40b1ce, [])
        call(m, 0x40b220, [0x34000])  # actual scheduler's task pool reservation
        primary = call(m, 0x40b2d0, [1, 64])
        m.mem_write(TASK + 32, struct.pack('<I', primary))
        status = rng.randrange(65536)
        m.mem_write(primary + 4, struct.pack('<H', status))
        source = b''.join(record(rng.randrange(8), [rng.uniform(-10, 10) for _ in range(7)])
                          for _ in range(count))
        m.mem_write(INFO, source or b'\0')
        if op:
            if call(m, 0x41caf0, [TASK, INFO, count, 7]) != 1:
                raise RuntimeError('original preinitialization failed')
        old = u32(m, primary + 56)
        old_info = u32(m, old + 12) if old else 0
        if op in (2, 3):
            m.mem_write(old + 5, bytes((ident,)))
            if ident == 128:
                m.mem_write(old + 12, bytes(4))
        if remaining != 0xffffffff:
            free_start = u32(m, 0x3abdbe8)
            free_bytes = ARENA + 0x1000000 - free_start
            if free_bytes > remaining + 32:
                if not call(m, 0x40b220, [free_bytes - remaining - 32]):
                    raise RuntimeError('original exhaustion fixture failed')
        if op < 2:
            ret = call(m, 0x41caf0, [TASK, INFO, count, ident])
        else:
            call(m, 0x41c4e0 if op == 2 else 0x41caa0, [primary if op == 2 else TASK])
            ret = 0
        current = u32(m, primary + 56)
        expected = bytearray(168 + count * 48)
        if current:
            expected[:168] = m.mem_read(current, 168)
            expected[168:] = m.mem_read(u32(m, current + 12), count * 48)
        output = (ctypes.c_ubyte * len(expected))()
        result = (ctypes.c_uint32 * 8)()
        status_code = native(op, 0x7f, (ctypes.c_ubyte * len(source)).from_buffer_copy(source),
                             count, status, ident, remaining, output, result)
        if status_code:
            raise RuntimeError(f'native lifecycle returned {status_code}')
        expected_status = struct.unpack('<H', m.mem_read(primary + 4, 2))[0]
        expected_header = [ret, expected_status, m.reg_read(UC_X86_REG_FPSW) & 63,
                           current, u32(m, 0x3abdbe8)]
        # Original free-list headers independently prove coalescing and the orphan prefix.
        cursor = ARENA
        blocks = 1
        while u32(m, cursor + 8):
            cursor = u32(m, cursor + 8)
            blocks += 1
        mismatch = bytes(output) != bytes(expected) or list(result[:2]) != expected_header[:2] or list(result[3:5]) != expected_header[3:5] or result[7] != blocks
        if op in (2, 3):
            mismatch |= result[5] != 1 or result[6] != int(bool(ident & 128) and ident != 128)
        rows.append(dict(operation=op, count=count, remaining=remaining, id=ident,
                         value_mismatch=bool(mismatch), fp_partial=result[2] != expected_header[2],
                         old=old, old_info=old_info, native=list(result), original=expected_header,
                         original_blocks=blocks))
    return rows


def scalar_cases(image, native, seed):
    rng = random.Random(seed)
    values = [0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x00800000, 0x7f7fffff,
              0x7f800000, 0xff800000, 0x7fc12345, 0x7fa12345,
              0xc0800000, 0x40800000, 0x3f800001]
    cases = [(op, floats([x, 2., 3.])) for op in (0, 1) for x in values]
    cases += [(n % 2, floats([rng.getrandbits(32) for _ in range(3)])) for n in range(120)]
    total = mismatches = partials = 0
    for control in (0x7f, 0x27f, 0x37f, 0x47f, 0x87f, 0xc7f):
        original = Original(image, control)
        m = original.machine
        for op, source in cases:
            m.mem_write(FIRST, source)
            m.reg_write(UC_X86_REG_FPSW, 0)
            expected = original.call_float(0x787cc0 if op == 0 else 0x7871f0,
                ecx=FIRST, stack_argument=None if op == 0 else struct.unpack('<I', source[:4])[0])
            output = (ctypes.c_uint32 * 2)()
            rc = native(op, control, (ctypes.c_ubyte * 12).from_buffer_copy(source), output)
            mismatches += bool(rc or output[0] != expected)
            partials += output[1] != (m.reg_read(UC_X86_REG_FPSW) & 63)
            total += 1
    return dict(cases=total, value_mismatches=mismatches, fp_partials=partials)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe', type=pathlib.Path)
    p.add_argument('native_library', type=pathlib.Path)
    p.add_argument('--output', required=True, type=pathlib.Path)
    p.add_argument('--holdout', action='store_true')
    p.add_argument('--numeric-only', action='store_true')
    a = p.parse_args()
    if unicorn.__version__ != '2.1.4':
        raise RuntimeError('requires Unicorn 2.1.4')
    image = PEImage(a.exe)
    if image.sha256 != SOURCE_SHA256:
        raise RuntimeError('original source hash changed')
    lib = ctypes.CDLL(str(a.native_library.resolve()))
    numeric = lib.sadx_collision_component_numeric
    numeric.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_ubyte),
                        ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_ubyte),
                        ctypes.POINTER(ctypes.c_uint32)]
    native_lifecycle = lib.sadx_collision_component_lifecycle
    native_lifecycle.argtypes = numeric.argtypes[:5] + [ctypes.c_uint32, ctypes.c_uint32] + numeric.argtypes[5:]
    scalar = lib.sadx_collision_component_scalar
    scalar.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_ubyte),
                       ctypes.POINTER(ctypes.c_uint32)]
    seed = 0x7193abc5 if a.holdout else 0x41caf041
    rng = random.Random(seed)
    cases = numeric_cases(seed, a.holdout)
    mismatches, partials, total, failures = 0, 0, 0, []
    corpus = hashlib.sha256()
    for control in (0x7f, 0x27f, 0x37f, 0x47f, 0x87f, 0xc7f):
        m = Original(image, control).machine
        for op, infos in cases:
            count = len(infos)
            work = bytearray(rng.randbytes(168))
            struct.pack_into('<HI', work, 6, count, 0x42424242)
            struct.pack_into('<I', work, 12, INFO)
            source = bytes(work) + b''.join(infos)
            corpus.update(struct.pack('<3I', op, control, count) + source)
            status = rng.randrange(65536)
            m.mem_write(PRIMARY + 4, struct.pack('<H', status))
            m.mem_write(PRIMARY + 56, struct.pack('<I', WORK))
            m.mem_write(WORK, bytes(work))
            m.mem_write(INFO, b''.join(infos) or b'\0')
            m.reg_write(UC_X86_REG_FPSW, 0)
            call(m, 0x418b60 if op == 0 else 0x41bac0, [PRIMARY])
            expected = bytes(m.mem_read(WORK, 168)) + bytes(m.mem_read(INFO, count * 48))
            expected_status = struct.unpack('<H', m.mem_read(PRIMARY + 4, 2))[0]
            flags = m.reg_read(UC_X86_REG_FPSW) & 63
            output = (ctypes.c_ubyte * len(source))()
            result = (ctypes.c_uint32 * 2)()
            rc = numeric(op, control, (ctypes.c_ubyte * len(source)).from_buffer_copy(source),
                         count, status, output, result)
            bad = rc != 0 or bytes(output) != expected or result[0] != expected_status
            if bad and len(failures) < 12:
                failures.append(dict(control=control, op=op, count=count, rc=rc,
                                     byte_offsets=[n for n, (x, y) in enumerate(zip(output, expected)) if x != y][:24],
                                     actual=bytes(output)[8:12].hex(), expected=expected[8:12].hex()))
            total += 1
            mismatches += bool(bad)
            partials += result[1] != flags
    lifecycle_rows = [] if a.numeric_only else lifecycle(image, native_lifecycle, seed)
    scalar_rows = scalar_cases(image, scalar, seed)
    mismatches += sum(r['value_mismatch'] for r in lifecycle_rows)
    partials += sum(r['fp_partial'] for r in lifecycle_rows)
    mismatches += scalar_rows['value_mismatches']
    partials += scalar_rows['fp_partials']
    report = dict(source_sha256=image.sha256, native_library_sha256=sha(a.native_library.read_bytes()),
                  case_blob_sha256=corpus.hexdigest(), holdout=a.holdout,
                  source_ranges={hex(x): dict(size=y-x, sha256=sha(image.read(x, y-x))) for x, y in RANGES},
                  cases=total + len(lifecycle_rows) + scalar_rows['cases'], numeric_cases=total,
                  lifecycle_cases=len(lifecycle_rows), scalar=scalar_rows,
                  value_mismatches=mismatches, fp_partials=partials, flag_mismatches=partials,
                  passed=mismatches == 0 and partials == 0,
                  values_match=mismatches == 0, failures=failures, lifecycle=lifecycle_rows,
                  limits=['FP exception flags are compared and partials remain partial; full stack/status/tag flags are not claimed',
                          'Native source closure and component only; no collision-list registration, collision solver, actor rendering or journey claim',
                          'Task linkage is an explicit original fixture; original allocation/free bodies execute unchanged',
                          'Unsupported partial info layouts and allocator wrap/corruption stop at tagged native boundaries'])
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('cases', 'value_mismatches', 'fp_partials', 'failures')}), flush=True)
    return 1 if mismatches else 2 if partials else 0


if __name__ == '__main__':
    raise SystemExit(main())
