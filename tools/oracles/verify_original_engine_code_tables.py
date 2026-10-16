"""Compare every immutable engine code-table word with pinned source data.

This checks initialized table/constant encodings, not all behavior of their consumers.
The independent native probe builds without an executable, asset pack or reference runtime.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
from pe_image import PEImage
from extract import SOURCE_SHA256

# In probe serialization order. Keyboard words retain kind/code and zero source padding;
# no native struct layout or padding is serialized.
RANGES = [(0x88ad64, 22), (0x389d918, 237), (0x389d650, 16),
          (0x7db2c4, 1), (0x7dce6c, 1), (0x7e9b40, 1), (0x7dcb2c, 1),
          (0x7dcb10, 1), (0x7dcb0c, 1), (0x7dce50, 1),
          (0x7dcb28, 1), (0x7dcb24, 1), (0x7dcc98, 1), (0x7dd594, 1), (0x7dd590, 1)]
ROOT = Path(__file__).resolve().parents[2]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--poison-output', type=Path, help='reference-only PE fixture with these table bytes XOR-poisoned')
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('pinned source identity mismatch')
    expected_bytes = b''.join(image.read(address, count * 4) for address, count in RANGES)
    expected = list(struct.unpack('<' + str(len(expected_bytes) // 4) + 'I', expected_bytes))
    library = ctypes.CDLL(str(args.native_library.resolve()))
    probe = library.sadx_native_engine_code_tables
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    probe.restype = ctypes.c_size_t
    output = (ctypes.c_uint32 * len(expected))()
    if probe(output, len(output)) != len(expected):
        raise ValueError('probe serialization size mismatch')
    mismatches = [dict(word=index, expected=wanted, actual=actual)
                  for index, (wanted, actual) in enumerate(zip(expected, output)) if wanted != actual]
    canary = (ctypes.c_uint32 * len(expected))(*([0xdeadbeef] * len(expected)))
    if probe(None, len(expected)) != 0 or probe(canary, len(expected) - 1) != 0:
        raise ValueError('probe accepted invalid output storage')
    if list(canary) != [0xdeadbeef] * len(expected):
        raise ValueError('short output storage was written')
    sources = ['src/runtime/engine_code_tables.hpp', 'src/runtime/engine_code_tables.cpp',
               'tests/probes/native_engine_code_tables_probe.cpp',
               'tools/oracles/verify_original_engine_code_tables.py']
    report = dict(source_sha256=image.sha256, native_library_sha256=sha(args.native_library.read_bytes()),
                  source_files={name: sha((ROOT / name).read_bytes()) for name in sources},
                  original_ranges_sha256={f'{address:x}-{address + count * 4:x}':
                                          sha(image.read(address, count * 4)) for address, count in RANGES},
                  cases=1, words_compared=len(expected), expected_words_sha256=sha(expected_bytes),
                  mismatches=mismatches, returned_values_and_boundaries_match=not mismatches,
                  limits=__doc__)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    if args.poison_output and not mismatches:
        poisoned = bytearray(image.data)
        for address, count in RANGES:
            offset = image.file_offset(address, count * 4)
            for index in range(offset, offset + count * 4):
                poisoned[index] ^= 0x55
        args.poison_output.write_bytes(poisoned)
    print(json.dumps(dict(cases=1, words_compared=len(expected), mismatches=len(mismatches))))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
