"""Verify library initial values against pinned file bytes and mapped zero fill.

Selected numeric initialization only; text/path resources and shared frame
control owners stay separate. This does not establish complete native library behavior.
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
    ('buffer_counts[0]', 0x389d864, 4),
    ('buffer_counts[1]', 0x389d868, 4),
    ('buffer_counts[2]', 0x389d86c, 4),
    ('buffer_counts[3]', 0x389d870, 4),
    ('buffer_counts[4]', 0x389d874, 4),
    ('buffer_counts[5]', 0x389d878, 4),
    ('buffer_counts[6]', 0x389d87c, 4),
    ('buffer_counts[7]', 0x389d880, 4),
    ('buffer_counts[8]', 0x389d884, 4),
    ('windowed', 0x38a5dc4, 4),
    ('back_buffer_width', 0x38a5dc8, 4),
    ('back_buffer_height', 0x38a5dcc, 4),
    ('scale', 0x3d0f800, 4),
    ('aspect', 0x3d0f9c0, 4),
    ('configuration.screen_flags', 0x892954, 4),
    ('configuration.setup_argument', 0x892958, 4),
    ('configuration.buffer_counts[0]', 0x892968, 4),
    ('configuration.buffer_counts[1]', 0x89296c, 4),
    ('configuration.buffer_counts[2]', 0x892970, 4),
    ('configuration.buffer_counts[3]', 0x892974, 4),
    ('configuration.buffer_counts[4]', 0x892978, 4),
    ('configuration.buffer_counts[5]', 0x89297c, 4),
    ('configuration.buffer_counts[6]', 0x892980, 4),
    ('configuration.buffer_counts[7]', 0x892984, 4),
    ('configuration.buffer_counts[8]', 0x892988, 4),
    ('configuration.high_precision_depth_request', 0x89298c, 4),
    ('configuration.back_buffer_width', 0x892990, 4),
    ('configuration.back_buffer_height', 0x892994, 4),
]
SOURCE_REGIONS = [(address, size) for _, address, size in FIELDS]
RESOURCE_FIELDS = [0x892940, 0x892944, 0x892948, 0x89294c]

ROOT = Path(__file__).resolve().parents[2]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def loaded_bytes(image, address, size):
    matches = [section for section in image.sections if
               section['rva'] <= address - image.base and
               address - image.base + size <= section['rva'] + max(section['virtual_size'], section['raw_size'])]
    if len(matches) != 1:
        raise ValueError(f'library fact not uniquely mapped: {address:#x}+{size}')
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
                raise ValueError('overlapping library facts')
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
    parser.add_argument('--poison-output', type=Path, help='reference-only PE fixture with these library bytes XOR-poisoned')
    parser.add_argument('--varied-output', type=Path)
    parser.add_argument('--partial-output', type=Path)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('pinned source identity mismatch')
    expected = [int.from_bytes(loaded_bytes(image, address, size), 'little') for _, address, size in FIELDS]
    expected_bytes = struct.pack('<28I', *expected)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    probe = library.sadx_native_library_initial_state
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
    extra = (ctypes.c_uint32 * 29)(*([0xdeadbeef] * 29))
    if probe(0, extra, 29) != 28 or extra[-1] != 0xdeadbeef:
        raise ValueError("valid probe call exceeded its output extent")
    sources = ['src/game/library_initial_state.hpp', 'src/game/library_initial_state.cpp',
               'tests/probes/library_initial_state_probe.hpp',
               'tests/probes/native_library_initial_state_probe.cpp',
               'tools/oracles/verify_original_library_initial_state.py']
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
        cutoff = 0x389d887 # final cold count lacks its fourth byte
        matches = [(i, section) for i, section in enumerate(image.sections) if
                   section['rva'] <= cutoff - image.base < section['rva'] + section['virtual_size']]
        if len(matches) != 1:
            raise ValueError('partial-fixture section not unique')
        ordinal, section = matches[0]
        size = cutoff - image.base - section['rva']
        struct.pack_into('<I', partial, table + ordinal * 40 + 8, size)
        struct.pack_into('<I', partial, table + ordinal * 40 + 16, min(size, section['raw_size']))
        args.partial_output.write_bytes(partial)
    print(json.dumps(dict(cases=1, words_compared=len(expected), mismatches=len(mismatches))))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
