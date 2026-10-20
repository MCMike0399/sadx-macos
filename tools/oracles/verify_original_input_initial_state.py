"""Verify load-time input values against both pinned images, before startup/polling.

Resource-pointer fields are excluded. No full native input/session independence claim.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_session_control_defaults import loaded_bytes

ROOT = Path(__file__).resolve().parents[2]
INPUT_SHA256 = 'c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5'
# Stable serialization: edges, template, four engine records, eight mod raw records,
# eight DreamPad input records. Name/extend/info remain outside these regions.
REGIONS = [('game', 'raw_edges', 0x3b0e3e2, 768),
           ('game', 'accumulated_edges', 0x3b0eaf0, 256),
           ('game', 'special', 0x3b0e3ba, 1),
           ('game', 'accumulated_special', 0x3b0eae8, 1),
           ('game', 'clear_prefix', 0x3b0e3e0, 2),
           ('game', 'cleared_neighbor', 0x3b0e3b9, 1)]
RECORDS = [('game', 'template', 0x3b0eaa8)]
RECORDS += [('game', f'engine_raw_{i}', 0x3b0e9c8 + 52 * i) for i in range(4)]
RECORDS += [('mod', f'mod_raw_{i}', 0x1000e748 + 52 * i) for i in range(8)]
RECORDS += [('mod', f'dream_pad_{i}', 0x1000e950 + 96 * i) for i in range(8)]
for image, name, address in RECORDS:
    REGIONS += [(image, name + '_values', address, 36),
                (image, name + '_previous_held', address + 44, 4)]
SOURCES = ['src/engine/controller_values.hpp', 'src/input/input_initial_state.hpp', 'src/input/input_initial_state.cpp',
           'tests/probes/input_initial_state_probe.hpp',
           'tests/probes/native_input_initial_state_probe.cpp',
           'tools/oracles/verify_original_input_initial_state.py']


def sha(data):
    return hashlib.sha256(data).hexdigest()


def table_offset(image):
    pe, = image.unpack_offset('<I', 60)
    optional_size, = image.unpack_offset('<H', pe + 20)
    return pe + 24 + optional_size


def fixture(image, regions, payload, resources=False):
    """Reader-only PE fixture; materialize virtual storage, preserve mapping extent."""
    data = bytearray(image.data)
    sections = [dict(section) for section in image.sections]
    for ordinal, section in enumerate(sections):
        selected = [(a, n) for a, n in regions]
        if resources:
            selected += [(a + 36, 16) for source, _, a in RECORDS
                         if (source == 'game') == (image.base == 0x400000)]
        if not any(section['rva'] <= a - image.base and
                   section['raw_size'] < a - image.base - section['rva'] + n <= section['virtual_size']
                   for a, n in selected):
            continue
        original = image.slice(section['file_offset'], section['raw_size'])
        original += bytes(section['virtual_size'] - section['raw_size'])
        offset = len(data)
        data.extend(original)
        struct.pack_into('<II', data, table_offset(image) + ordinal * 40 + 16, len(original), offset)
        section.update(raw_size=len(original), file_offset=offset)
    def write(address, value):
        matches = [s for s in sections if s['rva'] <= address - image.base and
                   address - image.base + len(value) <= s['rva'] + s['raw_size']]
        if len(matches) != 1:
            raise ValueError('fixture injection is not uniquely file-backed')
        s = matches[0]
        at = s['file_offset'] + address - image.base - s['rva']
        data[at:at + len(value)] = value
    next_byte = 0
    touched = set()
    for address, size in regions:
        for a in range(address, address + size):
            if a in touched:
                raise ValueError('overlapping data regions')
            touched.add(a)
        write(address, payload[next_byte:next_byte + size])
        next_byte += size
    if next_byte != len(payload):
        raise ValueError('fixture payload length mismatch')
    if resources:
        for source, _, address in RECORDS:
            if (source == 'game') != (image.base == 0x400000):
                continue
            for offset in [36, 40, 48]:
                if any(address + offset + i in touched for i in range(4)):
                    raise ValueError('resource and data overlap')
                write(address + offset, bytes((offset + i) & 255 for i in range(4)))
    return data


def partial_fixture(image, cutoff):
    data = bytearray(image.data)
    matches = [(i, s) for i, s in enumerate(image.sections) if
               s['rva'] <= cutoff - image.base < s['rva'] + max(s['virtual_size'], s['raw_size'])]
    if len(matches) != 1:
        raise ValueError('partial-image section not unique')
    ordinal, section = matches[0]
    size = cutoff - image.base - section['rva']
    struct.pack_into('<I', data, table_offset(image) + ordinal * 40 + 8, size)
    struct.pack_into('<I', data, table_offset(image) + ordinal * 40 + 16, min(size, section['raw_size']))
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('mod', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--fixture-prefix', type=Path)
    args = parser.parse_args()
    images = {'game': PEImage(args.exe), 'mod': PEImage(args.mod)}
    if images['game'].sha256 != SOURCE_SHA256 or images['mod'].sha256 != INPUT_SHA256:
        raise ValueError('pinned image identity mismatch')
    expected = b''.join(loaded_bytes(images[source], address, size)
                        for source, _, address, size in REGIONS)
    if len(expected) != 1869:
        raise ValueError('unexpected source-data extent')
    library = ctypes.CDLL(str(args.native_library.resolve()))
    probe = library.sadx_native_input_initial_state
    probe.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
    probe.restype = ctypes.c_size_t
    output = (ctypes.c_uint8 * 1869)()
    if probe(0, output, len(output)) != len(output):
        raise ValueError('probe serialization size mismatch')
    mismatches = [dict(byte=i, expected=a, actual=b)
                  for i, (a, b) in enumerate(zip(expected, output)) if a != b]
    canary = (ctypes.c_uint8 * 1870)(*([173] * 1870))
    if probe(2, canary, 1870) or probe(0, None, 1869) or probe(0, canary, 1868):
        raise ValueError('probe accepted invalid storage/variant')
    if list(canary) != [173] * 1870:
        raise ValueError('invalid call wrote storage')
    if probe(0, canary, 1870) != 1869 or canary[-1] != 173:
        raise ValueError('valid call exceeded output bounds')
    report = dict(source_sha256=images['game'].sha256, input_mod_sha256=images['mod'].sha256,
                  source_files={name: sha((ROOT / name).read_bytes()) for name in SOURCES},
                  native_library_sha256=sha(args.native_library.read_bytes()),
                  original_ranges_sha256={f'{address:x}-{address + size:x}':
                      sha(loaded_bytes(images[source], address, size))
                      for source, _, address, size in REGIONS}, cases=1,
                  bytes_compared=1869, expected_bytes_sha256=sha(expected), mismatches=mismatches,
                  returned_values_and_boundaries_match=not mismatches, limits=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    if args.fixture_prefix and not mismatches:
        varied = (ctypes.c_uint8 * 1869)()
        if probe(1, varied, 1869) != 1869:
            raise ValueError('varied serialization size mismatch')
        for source, suffix, cutoff in [('game', 'exe', 0x3b0eaaa), ('mod', 'dll', 0x1000e74a)]:
            regions = [(address, size) for kind, _, address, size in REGIONS if kind == source]
            positions = []
            start = 0
            for kind, _, _, size in REGIONS:
                if kind == source:
                    positions.extend(range(start, start + size))
                start += size
            poisoned = bytes(expected[i] ^ 85 for i in positions)
            injected = bytes(varied[i] for i in positions)
            prefix = str(args.fixture_prefix)
            Path(f'{prefix}-poisoned.{suffix}').write_bytes(fixture(images[source], regions, poisoned))
            Path(f'{prefix}-varied.{suffix}').write_bytes(fixture(images[source], regions, injected, resources=True))
            Path(f'{prefix}-partial.{suffix}').write_bytes(partial_fixture(images[source], cutoff))
    print(json.dumps(dict(cases=1, bytes_compared=1869, mismatches=len(mismatches))))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
