"""Verify render-queue initial values against pinned file bytes and mapped zero fill.

Selected numeric initialization only; queue allocation/device resources and live
control owners stay separate. This does not establish complete native queue behavior.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
from pe_image import PEImage
from extract import SOURCE_SHA256

FIELDS = [
    ('depth_comparison', 0x8928c8, 4), ('execution_mode', 0x3ab98ac, 4),
    ('lighting_palette', 0x3b12208, 4), ('lighting_copy', 0x3abdbc0, 1),
    ('packed_blend', 0x3ab9890, 1),
    ('primary_count', 0x3abd9d4, 4), ('primary_high_water', 0x3abd9d8, 4),
    ('secondary_count', 0x3abd9dc, 4), ('secondary_high_water', 0x3abd9e0, 4),
]
ARRAYS = [('reset_source', 0x900840), ('reset_capture', 0x3ab9898),
          ('allocation_source', 0x900858), ('allocation_capture', 0x90084c)]
SOURCE_REGIONS = [(address, size) for _, address, size in FIELDS]
SOURCE_REGIONS += [(address, 12) for _, address in ARRAYS]
for name, address in ARRAYS:
    FIELDS += [(f'{name}[{i}]', address + 4 * i, 4) for i in range(3)]
RESOURCE_FIELDS = [0x3abd960, 0x3d128b0, 0x3ce7164, 0x3ce7168, 0x3ce716c]

ROOT = Path(__file__).resolve().parents[2]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def loaded_bytes(image, address, size):
    matches = [section for section in image.sections if
               section['rva'] <= address - image.base and
               address - image.base + size <= section['rva'] + max(section['virtual_size'], section['raw_size'])]
    if len(matches) != 1:
        raise ValueError(f'queue fact not uniquely mapped: {address:#x}+{size}')
    section = matches[0]
    offset = address - image.base - section['rva']
    return bytes(image.data[section['file_offset'] + index] if index < section['raw_size'] else 0
                 for index in range(offset, offset + size))


def fixture_bytes(image, words, resources=False):
    # Reader fixture only. Materialize virtual storage without extending its mapped
    # range; original code and all other section contents are preserved.
    data = bytearray(image.data)
    pe, = image.unpack_offset('<I', 60)
    optional_size, = image.unpack_offset('<H', pe + 20)
    table = pe + 24 + optional_size
    sections = [dict(section) for section in image.sections]
    for ordinal, section in enumerate(sections):
        needs_tail = any(section['rva'] <= address - image.base and
                         section['raw_size'] < address - image.base - section['rva'] + size <=
                         section['virtual_size'] for _, address, size in FIELDS)
        if not needs_tail:
            continue
        payload = image.slice(section['file_offset'], section['raw_size'])
        payload += bytes(section['virtual_size'] - section['raw_size'])
        offset = len(data)
        data.extend(payload)
        struct.pack_into('<II', data, table + ordinal * 40 + 16, len(payload), offset)
        section['raw_size'] = len(payload)
        section['file_offset'] = offset
    spans = set()
    for (_, address, size), word in zip(FIELDS, words):
        if not 0 <= word < 1 << (size * 8):
            raise ValueError('injected value would be narrowed')
        matches = [section for section in sections if section['rva'] <= address - image.base and
                   address - image.base + size <= section['rva'] + section['raw_size']]
        if len(matches) != 1:
            raise ValueError('injection not uniquely file-backed')
        section = matches[0]
        offset = section['file_offset'] + address - image.base - section['rva']
        for byte in range(address, address + size):
            if byte in spans:
                raise ValueError('overlapping queue facts')
            spans.add(byte)
        data[offset:offset + size] = word.to_bytes(size, 'little')
    if resources:
        for ordinal, address in enumerate(RESOURCE_FIELDS):
            if any(address + i in spans for i in range(4)):
                raise ValueError('resource/data overlap')
            matches = [section for section in sections if section['rva'] <= address - image.base and
                       address - image.base + 4 <= section['rva'] + section['raw_size']]
            if len(matches) != 1:
                raise ValueError('resource injection not file-backed')
            section = matches[0]
            offset = section['file_offset'] + address - image.base - section['rva']
            data[offset:offset + 4] = (0x12345010 + 16 * ordinal).to_bytes(4, 'little')
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--poison-output', type=Path, help='reference-only PE fixture with these queue bytes XOR-poisoned')
    parser.add_argument('--varied-output', type=Path)
    parser.add_argument('--partial-output', type=Path)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('pinned source identity mismatch')
    expected = [int.from_bytes(loaded_bytes(image, address, size), 'little') for _, address, size in FIELDS]
    expected_bytes = struct.pack('<21I', *expected)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    probe = library.sadx_native_ninja_queue_initial_state
    probe.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    probe.restype = ctypes.c_size_t
    output = (ctypes.c_uint32 * len(expected))()
    if probe(0, output, len(output)) != len(expected):
        raise ValueError('probe serialization size mismatch')
    mismatches = [dict(word=index, expected=wanted, actual=actual)
                  for index, (wanted, actual) in enumerate(zip(expected, output)) if wanted != actual]
    canary = (ctypes.c_uint32 * len(expected))(*([0xdeadbeef] * len(expected)))
    if probe(2, canary, len(expected)) != 0 or probe(0, None, len(expected)) != 0 or probe(0, canary, len(expected) - 1) != 0:
        raise ValueError('probe accepted invalid output storage')
    if list(canary) != [0xdeadbeef] * len(expected):
        raise ValueError('short output storage was written')
    extra = (ctypes.c_uint32 * 22)(*([0xdeadbeef] * 22))
    if probe(0, extra, 22) != 21 or extra[-1] != 0xdeadbeef:
        raise ValueError("valid probe call exceeded its output extent")
    sources = ['src/engine/ninja_queue_initial_state.hpp', 'src/engine/ninja_queue_initial_state.cpp',
               'tests/probes/ninja_queue_initial_state_probe.hpp',
               'tests/probes/native_ninja_queue_initial_state_probe.cpp',
               'tools/oracles/verify_original_ninja_queue_initial_state.py']
    report = dict(source_sha256=image.sha256, native_library_sha256=sha(args.native_library.read_bytes()),
                  source_files={name: sha((ROOT / name).read_bytes()) for name in sources},
                  original_ranges_sha256={f'{address:x}-{address + size:x}':
                                          sha(loaded_bytes(image, address, size)) for address, size in SOURCE_REGIONS},
                  cases=1, words_compared=len(expected), expected_words_sha256=sha(expected_bytes),
                  mismatches=mismatches, returned_values_and_boundaries_match=not mismatches,
                  limits=__doc__)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    if args.poison_output and not mismatches:
        poisoned = [value ^ int.from_bytes(bytes([0x55] * size), 'little')
                    for (_, _, size), value in zip(FIELDS, expected)]
        args.poison_output.write_bytes(fixture_bytes(image, poisoned))
    if args.varied_output and not mismatches:
        varied = (ctypes.c_uint32 * len(FIELDS))()
        if probe(1, varied, len(FIELDS)) != len(FIELDS):
            raise ValueError('varied fixture serialization')
        args.varied_output.write_bytes(fixture_bytes(image, list(varied), resources=True))
    if args.partial_output and not mismatches:
        partial = bytearray(image.data)
        pe, = image.unpack_offset('<I', 60)
        optional_size, = image.unpack_offset('<H', pe + 20)
        table = pe + 24 + optional_size
        cutoff = 0x3ab98a0 + 3 # final reset-capture word lacks its fourth byte
        matches = [(i, section) for i, section in enumerate(image.sections) if
                   section['rva'] <= cutoff - image.base < section['rva'] + section['virtual_size']]
        if len(matches) != 1:
            raise ValueError('partial-fixture section not unique')
        ordinal, section = matches[0]
        size = cutoff - image.base - section['rva']
        if size <= section['raw_size']:
            raise ValueError('partial fixture must cut the original virtual tail')
        struct.pack_into('<I', partial, table + ordinal * 40 + 8, size)
        args.partial_output.write_bytes(partial)
    print(json.dumps(dict(cases=1, words_compared=len(expected), mismatches=len(mismatches))))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
