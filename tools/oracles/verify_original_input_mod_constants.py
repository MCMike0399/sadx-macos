"""Compare seven native Input 2.6.1 arithmetic coefficients with pinned DLL bytes.

Reader fixtures are synthetic PE mappings, never executed. Only coefficients are
made native; mutable controller data, resources and other DLL dependencies remain.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
from pe_image import PEImage

INPUT_SHA256 = 'c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5'
FIELDS = [('axes.epsilon', 0x1000bb6c), ('axes.maximum', 0x1000bbb0),
          ('axes.keyboard_scale', 0x1000bba0), ('axes.controller_scale', 0x1000bba4),
          ('axes.magnitude_cap', 0x1000bb7c), ('trigger_scale', 0x1000bba8),
          ('half_press_scale', 0x1000bb78)]
# Raw-input record0 name/extension/info provenance, separate from coefficients.
RESOURCE_FIELDS = [0x1000e748 + offset for offset in (36, 40, 48)]
ROOT = Path(__file__).resolve().parents[2]
SOURCES = ('src/input/input_mod_constants.hpp', 'src/input/input_mod_constants.cpp',
           'tests/probes/input_mod_constants_probe.hpp', 'tests/probes/native_input_mod_constants_probe.cpp',
           'src/input/input_mod_pad.hpp', 'src/input/input_mod_axes.hpp', 'src/input/input.hpp',
           'src/engine/game_controller.hpp', 'src/cpu/sse_math.hpp',
           'tools/oracles/verify_original_input_mod_constants.py')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def section_table(image):
    pe, = image.unpack_offset('<I', 60)
    optional_size, = image.unpack_offset('<H', pe + 20)
    return pe, pe + 24 + optional_size


def injected(image, words, resources=False):
    data = bytearray(image.data)
    _, table = section_table(image)
    sections = [dict(s) for s in image.sections]
    entries = [(address, value) for (_, address), value in zip(FIELDS, words)]
    if resources:
        entries += [(address, 0x12345010 + 16 * i) for i, address in enumerate(RESOURCE_FIELDS)]
    for ordinal, section in enumerate(sections):
        if not any(section['rva'] <= address - image.base and
                   section['raw_size'] < address - image.base - section['rva'] + 4 <=
                   section['virtual_size'] for address, _ in entries):
            continue
        payload = image.slice(section['file_offset'], section['raw_size'])
        payload += bytes(section['virtual_size'] - section['raw_size'])
        offset = len(data)
        data.extend(payload)
        struct.pack_into('<II', data, table + 40 * ordinal + 16, len(payload), offset)
        section['raw_size'], section['file_offset'] = len(payload), offset
    spans = set()
    for address, value in entries:
        matches = [s for s in sections if s['rva'] <= address - image.base and
                   address - image.base + 4 <= s['rva'] + s['raw_size']]
        if len(matches) != 1 or any(address + i in spans for i in range(4)):
            raise ValueError('fixture input is overlapping or not uniquely file-backed')
        spans.update(range(address, address + 4))
        section = matches[0]
        offset = section['file_offset'] + address - image.base - section['rva']
        struct.pack_into('<I', data, offset, value)
    return data


def partial_mapping(image):
    # Split .rdata around exactly one missing byte. Keep tail exports and all
    # other mapped bytes intact, unlike truncating the whole remaining section.
    data = bytearray(image.data)
    pe, table = section_table(image)
    gap = FIELDS[0][1] + 3
    matches = [(i, s) for i, s in enumerate(image.sections) if
               s['rva'] <= gap - image.base < s['rva'] + s['raw_size']]
    if len(matches) != 1:
        raise ValueError('partial byte not uniquely file-backed')
    ordinal, section = matches[0]
    count = len(image.sections)
    if table + 40 * (count + 1) > min(s['file_offset'] for s in image.sections if s['raw_size']):
        raise ValueError('no spare section header for reader fixture')
    prefix = gap - image.base - section['rva']
    tail_offset = prefix + 1
    header = bytearray(data[table + ordinal * 40:table + (ordinal + 1) * 40])
    header[:8] = b'.rdtail\0'
    struct.pack_into('<IIII', header, 8, max(0, section['virtual_size'] - tail_offset),
                     section['rva'] + tail_offset, section['raw_size'] - tail_offset,
                     section['file_offset'] + tail_offset)
    data[table + count * 40:table + (count + 1) * 40] = header
    struct.pack_into('<I', data, table + ordinal * 40 + 8, prefix)
    struct.pack_into('<I', data, table + ordinal * 40 + 16, prefix)
    struct.pack_into('<H', data, pe + 6, count + 1)
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--poison-output', type=Path)
    parser.add_argument('--varied-output', type=Path)
    parser.add_argument('--partial-output', type=Path)
    args = parser.parse_args()
    image = PEImage(args.dll)
    if image.sha256 != INPUT_SHA256:
        raise ValueError('pinned Input2.6.1 DLL identity mismatch')
    expected = [int.from_bytes(image.read(address, 4), 'little') for _, address in FIELDS]
    library = ctypes.CDLL(str(args.native_library.resolve()))
    probe = library.sadx_native_input_mod_constants
    probe.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    probe.restype = ctypes.c_size_t
    output = (ctypes.c_uint32 * 7)()
    if probe(0, output, 7) != 7:
        raise ValueError('probe serialization size mismatch')
    mismatches = [dict(word=i, expected=w, actual=v) for i, (w, v) in enumerate(zip(expected, output)) if w != v]
    canary = (ctypes.c_uint32 * 7)(*([0xdeadbeef] * 7))
    if probe(2, canary, 7) or probe(0, None, 7) or probe(0, canary, 6):
        raise ValueError('invalid output storage accepted')
    if list(canary) != [0xdeadbeef] * 7:
        raise ValueError('invalid output storage written')
    extra = (ctypes.c_uint32 * 8)(*([0xdeadbeef] * 8))
    if probe(0, extra, 8) != 7 or extra[-1] != 0xdeadbeef:
        raise ValueError('output exceeded exact extent')
    report = dict(source_sha256=image.sha256, native_library_sha256=sha(args.native_library.read_bytes()),
                  source_files={name: sha((ROOT / name).read_bytes()) for name in SOURCES},
                  original_ranges_sha256={f'{address:x}-{address + 4:x}': sha(image.read(address, 4))
                                          for _, address in FIELDS},
                  cases=1, words_compared=7, expected_words_sha256=sha(struct.pack('<7I', *expected)),
                  mismatches=mismatches, returned_values_and_boundaries_match=not mismatches, limits=__doc__)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    if not mismatches:
        if args.poison_output:
            args.poison_output.write_bytes(injected(image, [word ^ 0x55555555 for word in expected]))
        if args.varied_output:
            varied = (ctypes.c_uint32 * 7)()
            if probe(1, varied, 7) != 7:
                raise ValueError('varied serialization size')
            args.varied_output.write_bytes(injected(image, varied, resources=True))
        if args.partial_output:
            args.partial_output.write_bytes(partial_mapping(image))
    print(json.dumps(dict(cases=1, words_compared=7, mismatches=len(mismatches))))
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
