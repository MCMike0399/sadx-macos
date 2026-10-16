"""Verify maintained Ninja coefficients exactly against the pinned inline table.

This covers code data only. Original trig execution remains in the unchanged math
oracle; it does not establish complete native runtime or physical FP equivalence.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
from pe_image import PEImage
from extract import SOURCE_SHA256

ROOT = Path(__file__).resolve().parents[2]
ADDRESS, SIZE = 0x88af78, 4100
SOURCES = ['src/math/ninja_sine_table.hpp', 'src/math/ninja_sine_table.cpp',
           'tests/probes/ninja_sine_table_probe.hpp', 'tests/probes/native_ninja_sine_table_probe.cpp',
           'tools/oracles/verify_original_ninja_sine_table.py', 'tools/emit/emit_ninja_sine_table.py']


def sha(data):
    return hashlib.sha256(data).hexdigest()


def patched(image, payload):
    if len(payload) != SIZE:
        raise ValueError('wrong fixture extent')
    data = bytearray(image.data)
    start = image.file_offset(ADDRESS, SIZE)
    data[start:start + SIZE] = payload
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--fixture-prefix', type=Path)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('pinned source identity mismatch')
    source_bytes = image.read(ADDRESS, SIZE)
    expected = struct.unpack('<1025I', source_bytes)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    probe = library.sadx_native_ninja_sine_table
    probe.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    probe.restype = ctypes.c_size_t
    output = (ctypes.c_uint32 * 1025)()
    if probe(0, output, 1025) != 1025:
        raise ValueError('probe serialization size mismatch')
    mismatches = [dict(word=i, expected=a, actual=b) for i, (a, b) in
                  enumerate(zip(expected, output)) if a != b]
    canary = (ctypes.c_uint32 * 1026)(*([0xdeadbeef] * 1026))
    if probe(2, canary, 1026) or probe(0, None, 1025) or probe(0, canary, 1024):
        raise ValueError('probe accepted invalid variant or output storage')
    if list(canary) != [0xdeadbeef] * 1026:
        raise ValueError('invalid call wrote output storage')
    if probe(0, canary, 1026) != 1025 or canary[-1] != 0xdeadbeef:
        raise ValueError('valid call exceeded output bounds')
    report = dict(source_sha256=image.sha256, native_library_sha256=sha(args.native_library.read_bytes()),
                  source_files={name: sha((ROOT / name).read_bytes()) for name in SOURCES},
                  original_ranges_sha256={f'{ADDRESS:x}-{ADDRESS + SIZE:x}': sha(source_bytes)},
                  cases=1, words_compared=1025, expected_words_sha256=sha(source_bytes),
                  mismatches=mismatches, returned_values_and_boundaries_match=not mismatches,
                  limits=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    if args.fixture_prefix and not mismatches:
        prefix = str(args.fixture_prefix)
        Path(prefix + '-poisoned.exe').write_bytes(patched(image, bytes(b ^ 85 for b in source_bytes)))
        varied = (ctypes.c_uint32 * 1025)()
        if probe(1, varied, 1025) != 1025:
            raise ValueError('varied serialization size mismatch')
        Path(prefix + '-varied.exe').write_bytes(patched(image, struct.pack('<1025I', *varied)))
        # Reader-only partial section: exactly the last byte of the final knot is missing.
        data = bytearray(image.data)
        pe, = image.unpack_offset('<I', 60)
        optional_size, = image.unpack_offset('<H', pe + 20)
        table = pe + 24 + optional_size
        cutoff = ADDRESS + SIZE - 1
        matches = [(i, s) for i, s in enumerate(image.sections) if
                   s['rva'] <= cutoff - image.base < s['rva'] + max(s['virtual_size'], s['raw_size'])]
        if len(matches) != 1:
            raise ValueError('partial section not unique')
        ordinal, section = matches[0]
        extent = cutoff - image.base - section['rva']
        struct.pack_into('<I', data, table + ordinal * 40 + 8, extent)
        struct.pack_into('<I', data, table + ordinal * 40 + 16, min(extent, section['raw_size']))
        Path(prefix + '-partial.exe').write_bytes(data)
    print(json.dumps(dict(cases=1, words_compared=1025, mismatches=len(mismatches))))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
