"""Compare the retained reference constants API with its immutable before-source library.

These are compatibility observations, not patched-original or hardware claims.
Native product access continues to use pinned literals in the same shared engine.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def load(path):
    library = ctypes.CDLL(str(path.resolve()))
    function = library.sadx_native_player_animation
    function.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_uint32),
                         ctypes.POINTER(ctypes.c_uint32)]
    function.restype = ctypes.c_int
    return function


def fixture(kind, control):
    values = [0] * 37
    values[0] = 7 if kind == 'reverse-zero' else 6
    values[7] = bits(4.0)
    values[9] = bits(1.0)
    values[11] = control
    values[12] = 0 if kind == 'reverse-zero' else 1
    for index in range(4):
        values[13 + index] = values[0]
        values[17 + index] = (index + 1) % 4
        values[21 + index] = bits(0.25)
        values[25 + index] = bits(1.0)
        values[29 + index] = 0x8000000a if kind == 'unsigned-reset' else 10
        values[33 + index] = 3
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=Path)
    parser.add_argument('current', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Use a new receipt path; previous comparisons are retained')
    old, new = load(args.before), load(args.current)
    constants = [
        [bits(0), bits(1), bits(4294967296)],
        [bits(0), bits(2), bits(4294967296)],
        [bits(100), bits(1), bits(4294967296)],
        [bits(-100), bits(1), bits(0)],
        [bits(0), bits(-2), bits(1024)],
        [0x7fc12345, bits(1), bits(4294967296)],
        [bits(0), 0x7f812345, bits(4294967296)],
    ]
    rows, mismatches = [], []
    for kind in ('reverse-reset', 'reverse-zero', 'unsigned-reset'):
        for precision in (0, 2, 3):
            for rounding in range(4):
                control = 0x7f | (precision << 8) | (rounding << 10)
                values = fixture(kind, control)
                for words in constants:
                    supplied = (ctypes.c_float * 3).from_buffer_copy(struct.pack('<3I', *words))
                    source = (ctypes.c_uint32 * 37)(*values)
                    a, b = (ctypes.c_uint32 * 15)(), (ctypes.c_uint32 * 15)()
                    left, right = old(supplied, source, a), new(supplied, source, b)
                    row = dict(id=len(rows), kind=kind, constants=words, input=values,
                               before_return=left, current_return=right,
                               before=list(a), current=list(b))
                    if left != right or list(a) != list(b):
                        mismatches.append(row['id'])
                    rows.append(row)
    encoded = json.dumps(rows, separators=(',', ':')).encode()
    report = dict(cases=len(rows), mismatch_ids=mismatches, rows_sha256=hashlib.sha256(encoded).hexdigest(),
                  before_library_sha256=hashlib.sha256(args.before.read_bytes()).hexdigest(),
                  current_library_sha256=hashlib.sha256(args.current.read_bytes()).hexdigest(),
                  rows=rows, limits=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, separators=(',', ':')) + '\n')
    print(f'{len(rows)} reference constant observations; {len(mismatches)} mismatches')
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
