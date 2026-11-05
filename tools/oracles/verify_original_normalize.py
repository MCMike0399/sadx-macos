"""Whole D3DXVec3Normalize_SSE body, including scalar RSQRTSS, no seeded leaves.

Masked exceptions only. Values, control and sticky flags are compared separately;
Unicorn's missing SSE flags remain strict partials. Independent RSQRTSS fixtures
measure the reference estimate; no physical Intel estimate is claimed.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EIP,
                              UC_X86_REG_ESP, UC_X86_REG_MXCSR, UC_X86_REG_XMM0)
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP, DATA, CODE

START, END = 0x775630, 0x7756f6
VECTOR, OUTPUT = DATA + 0x200, DATA + 0x300

def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]

def cases(holdout):
    rng = random.Random(0x775630 + (7919 if holdout else 0))
    vectors = [(0, 0, 0), (0x80000000, 0, 0), (bits(1), 0, 0),
               (bits(1), bits(2), bits(3)), (bits(2**-24), 0, 0),
               (bits(2**-23)-1, 0, 0), (bits(2**-23), 0, 0),
               (bits(2**-23)+1, 0, 0), (1, 0, 0), (0x7fffff, 0, 0),
               (0x7f7fffff, 0, 0), (bits(1e19), bits(-1e19), bits(1e19))]
    for special in (0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345):
        for axis in range(3):
            v = [bits(1), bits(-2), bits(3)]; v[axis] = special
            vectors.append(tuple(v))
    for _ in range(3000 if holdout else 1000):
        exponent = rng.randrange(-30, 65)
        vectors.append(tuple(bits(rng.uniform(-1, 1) * 2**exponent) for _ in range(3)))
    for i, vector in enumerate(vectors):
        # All RC modes; vary DAZ, FTZ, incoming sticky flags and exact aliasing.
        for rc in range(4):
            yield vector, 0x1f80 | (rc << 13) | ((i % 2) << 6) | (((i//2) % 2) << 15) | (i % 64), i % 2

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--holdout', action='store_true')
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('pinned source/Unicorn mismatch')
    m = Original(image, 0x7f).machine
    m.mem_map(0x3d13000, 4096)
    m.mem_write(0x3d13180, image.read(0x3d13180, 12))
    clean = m.context_save()
    lib = ctypes.CDLL(str(args.native_library.resolve()))
    native = lib.sadx_native_normalize
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32, ctypes.c_uint,
                       ctypes.POINTER(ctypes.c_uint32)]
    native.restype = ctypes.c_int
    estimate = lib.sadx_native_normalize_rsqrt
    estimate.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    estimate.restype = ctypes.c_uint32
    # Execute the exact opcode from the source independently, including non-RN RC.
    opcode = image.read(0x7756b2, 4)
    if opcode.hex() != 'f30f52db': raise RuntimeError('RSQRTSS bytes changed')
    # Change only the register encoding to XMM0 for the isolated fixture.
    m.mem_write(CODE, bytes.fromhex('f30f52c0'))
    rng = random.Random(0x525351 + args.holdout)
    measurements = dict(cases=0, value_mismatches=0, flag_mismatches=0, examples=[])
    inputs = [0, 0x80000000, 1, 0x7fffff, 0x800000, 0x40000000, 0x40400000,
              0x7f800000, 0xbf800000, 0x7fc12345, 0x7f812345]
    inputs += [rng.randrange(0x800000, 0x7f800000) for _ in range(2000)]
    for value in inputs:
        for mode in range(16):
            control = 0x1f80 | ((mode & 3) << 13) | (((mode >> 2) & 1) << 6) | (((mode >> 3) & 1) << 15)
            m.context_restore(clean)
            m.reg_write(UC_X86_REG_MXCSR, control)
            m.reg_write(UC_X86_REG_XMM0, value)
            m.emu_start(CODE, CODE+4, count=1)
            expected = m.reg_read(UC_X86_REG_XMM0) & 0xffffffff
            status = ctypes.c_uint32()
            actual = estimate(value, control, ctypes.byref(status))
            measurements['cases'] += 1
            measurements['value_mismatches'] += actual != expected
            measurements['flag_mismatches'] += status.value != m.reg_read(UC_X86_REG_MXCSR)
            if len(measurements['examples']) < 12 and (value in inputs[:11]):
                measurements['examples'].append(dict(input=hex(value), mxcsr=hex(control),
                                                     original=hex(expected), native=hex(actual)))
    report = dict(cases=0, value_mismatches=0, control_mismatches=0, flag_mismatches=0,
                  examples=[], flag_examples=[])
    digest = hashlib.sha256()
    for vector, control, alias in cases(args.holdout):
        m.context_restore(clean)
        m.mem_write(VECTOR, struct.pack('<3I', *vector))
        m.mem_write(OUTPUT, b'\xa5'*12)
        target = VECTOR if alias else OUTPUT
        m.mem_write(STACK, struct.pack('<3I', STOP, target, VECTOR))
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.reg_write(UC_X86_REG_EBX, 0x12345678)
        m.reg_write(UC_X86_REG_MXCSR, control)
        m.emu_start(START, STOP, count=200)
        if (m.reg_read(UC_X86_REG_EIP) != STOP or m.reg_read(UC_X86_REG_ESP) != STACK+12
            or m.reg_read(UC_X86_REG_EAX) != target or m.reg_read(UC_X86_REG_EBX) != 0x12345678):
            raise RuntimeError('return/preserved register/stack differs')
        expected = list(struct.unpack('<3I', m.mem_read(target, 12)))
        status = m.reg_read(UC_X86_REG_MXCSR)
        output = (ctypes.c_uint32*4)(*[0xa5a5a5a5]*4)
        if native((ctypes.c_uint32*3)(*vector), control, alias, output):
            raise RuntimeError('masked native execution trapped')
        actual = list(output[:3])
        report['cases'] += 1
        report['value_mismatches'] += expected != actual
        report['control_mismatches'] += (status & ~63) != (output[3] & ~63)
        report['flag_mismatches'] += (status & 63) != (output[3] & 63)
        digest.update(struct.pack('<10I', *vector, control, alias, *expected, status, output[3]))
        if actual != expected and len(report['examples']) < 10:
            report['examples'].append(dict(vector=[hex(x) for x in vector], mxcsr=hex(control),
                                          original=[hex(x) for x in expected], native=[hex(x) for x in actual]))
        if status != output[3] and len(report['flag_examples']) < 5:
            report['flag_examples'].append(dict(vector=[hex(x) for x in vector], original=hex(status), native=hex(output[3])))
    result = dict(report, rsqrt_measurement=measurements, cohort='holdout' if args.holdout else 'development',
                  observation_sha256=digest.hexdigest(),
                  native_probe_sha256=hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
                  original_ranges_sha256={f'{START:#x}-{END:#x}': hashlib.sha256(image.read(START, END-START)).hexdigest()},
                  source_constants={hex(a): image.read(a, 4).hex() for a in (0x3d13180, 0x3d13184, 0x3d13188)},
                  values_match=not (report['value_mismatches'] or report['control_mismatches'] or measurements['value_mismatches']),
                  limits=['Masked exceptions only; no unmasked trap/partial-store qualification.',
                          'Unicorn 2.1.4 reference estimate: binary32 sqrt then binary32 division, not physical Intel RSQRTSS.',
                          'Unicorn does not publish SSE exception flags; all flag differences remain strict partials.',
                          'No Windows CPU or raster parity claim.'])
    root = Path(__file__).resolve().parents[2]
    names = ['src/cpu/sse_estimate.cpp', 'src/cpu/sse_estimate.hpp', 'src/cpu/sse_math.cpp', 'src/cpu/sse_math.hpp',
             'src/engine/vector_normalize.cpp', 'src/engine/vector_normalize.hpp', 'tests/probes/native_normalize_probe.cpp',
             'tools/oracles/verify_original_normalize.py']
    result['source_files'] = {name: hashlib.sha256((root/name).read_bytes()).hexdigest() for name in names}
    result['passed'] = result['values_match'] and not report['flag_mismatches'] and not measurements['value_mismatches'] and not measurements['flag_mismatches']
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('examples','flag_examples','rsqrt_measurement','limits')}, indent=2))
    return 1 if not result['values_match'] or measurements['value_mismatches'] else (2 if report['flag_mismatches'] or measurements['flag_mismatches'] else 0)

if __name__ == '__main__':
    raise SystemExit(main())
