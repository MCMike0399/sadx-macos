"""Verify screen load-time values against pinned file bytes and PE virtual zero fill.

Only the listed screen facts are covered. Resources, capabilities and remaining
initialization families are outside this oracle; no complete native-session claim.
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
    ('clip.near_distance', 0x3d0f808, 4),
    ('clip.far_distance', 0x3d0f968, 4),
    ('clip.range_near', 0x90082c, 4),
    ('clip.range_far', 0x900830, 4),
    ('clip.cached_near', 0x3aad0c0, 4),
    ('clip.cached_far', 0x3aad0c4, 4),
    ('setup_argument', 0x3d0f804, 4),
    ('flags', 0x3d0f964, 4),
    ('timing_multiplier', 0x3d0f814, 4),
    ('width_pixels', 0x3d0fa20, 4),
    ('height_pixels', 0x3d0f838, 4),
    ('logical_height', 0x3d0f9e8, 4),
    ('viewport_width', 0x3d0fa04, 4),
    ('viewport_height', 0x3d0fa08, 4),
    ('screen_bounds.left', 0x3d0f820, 4),
    ('screen_bounds.top', 0x3d0f824, 4),
    ('screen_bounds.right', 0x3d0f828, 4),
    ('screen_bounds.bottom', 0x3d0f82c, 4),
    ('transform_invalid', 0x3d0fd1c, 4),
    ('polygon_setup_word', 0x3ce7160, 4),
    ('polygon_count', 0x3ce7170, 4),
    ('viewport.x', 0x3d12780, 4),
    ('viewport.y', 0x3d12784, 4),
    ('viewport.width', 0x3d12788, 4),
    ('viewport.height', 0x3d1278c, 4),
    ('viewport.min_z', 0x3d12790, 4),
    ('viewport.max_z', 0x3d12794, 4),
    ('half_width', 0x3d12948, 4),
    ('half_height', 0x3d1294c, 4),
    ('viewport_bounds.left', 0x3d0fd20, 4),
    ('viewport_bounds.top', 0x3d0fd24, 4),
    ('viewport_bounds.right', 0x3d0fd28, 4),
    ('viewport_bounds.bottom', 0x3d0fd2c, 4),
    ('projection_width', 0x3d0f9e4, 4),
    ('polygon_points[0].x', 0x3ce7184, 4),
    ('polygon_points[0].y', 0x3ce7188, 4),
    ('polygon_points[1].x', 0x3ce718c, 4),
    ('polygon_points[1].y', 0x3ce7190, 4),
    ('polygon_points[2].x', 0x3ce7194, 4),
    ('polygon_points[2].y', 0x3ce7198, 4),
    ('polygon_points[3].x', 0x3ce719c, 4),
    ('polygon_points[3].y', 0x3ce71a0, 4),
]
ARRAYS = [
    ('projection', 0x3d129c0),
    ('work_transform', 0x3d0fd80),
    ('base_transform', 0x389d318),
]
SOURCE_REGIONS = [(address, 4) for _, address, _ in FIELDS]
SOURCE_REGIONS += [(address, 64) for _, address in ARRAYS]
for name, address in ARRAYS:
    FIELDS += [(f"{name}_{i}", address + 4 * i, 4) for i in range(16)]
RESOURCE_FIELDS = [0x3ce7164, 0x3ce7168, 0x3ce716c]

ROOT = Path(__file__).resolve().parents[2]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def loaded_bytes(image, address, size):
    matches = [section for section in image.sections if
               section['rva'] <= address - image.base and
               address - image.base + size <= section['rva'] + max(section['virtual_size'], section['raw_size'])]
    if len(matches) != 1:
        raise ValueError(f'screen fact not uniquely mapped: {address:#x}+{size}')
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
                raise ValueError('overlapping screen facts')
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
    parser.add_argument('--poison-output', type=Path, help='reference-only PE fixture with these screen bytes XOR-poisoned')
    parser.add_argument('--varied-output', type=Path)
    parser.add_argument('--partial-output', type=Path)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('pinned source identity mismatch')
    expected = [int.from_bytes(loaded_bytes(image, address, size), 'little') for _, address, size in FIELDS]
    expected_bytes = struct.pack('<90I', *expected)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    probe = library.sadx_native_ninja_screen_initial_state
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
    extra = (ctypes.c_uint32 * 91)(*([0xdeadbeef] * 91))
    if probe(0, extra, 91) != 90 or extra[-1] != 0xdeadbeef:
        raise ValueError("valid probe call exceeded its output extent")
    sources = ['src/engine/ninja_screen_initial_state.hpp', 'src/engine/ninja_screen_initial_state.cpp',
               'tests/probes/ninja_screen_initial_state_probe.hpp',
               'tests/probes/native_ninja_screen_initial_state_probe.cpp',
               'tools/oracles/verify_original_ninja_screen_initial_state.py']
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
        cutoff = 0x3d129fc + 3 # final projection-matrix float lacks its fourth byte
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
