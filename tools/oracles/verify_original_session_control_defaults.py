"""Verify scalar load-time controls against pinned file bytes and PE virtual zero fill.

Only the listed control facts are covered. Resources, callbacks and remaining
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
    ('mode', 0x3abdc7c, 4),
    ('pause', 0x3b28114, 4),
    ('save_phase', 0x3b291a4, 4),
    ('save_ready', 0x3b291ad, 1),
    ('action_restart_requested', 0x3abdf74, 1),
    ('title_mode', 0x3abdcfc, 1),
    ('title_load_phase', 0x3abdcfd, 1),
    ('playing_super_sonic', 0x3b18db4, 1),
    ('tutorial_variant', 0x3b2a2fa, 1),
    ('alert_flag', 0x3b22e1e, 1),
    ('alert_blocked', 0x3abdf78, 1),
    ('movie_number', 0x3b2c45c, 4),
    ('menu_message_enabled', 0x3b290e4, 4),
    ('frame_multiplier', 0x3b11188, 4),
    ('logic_increment', 0x3b11178, 4),
    ('pending_logic_ticks', 0x3b1117c, 4),
    ('counter_frequency_low', 0x3d08538, 4),
    ('counter_frequency_high', 0x3d0853c, 4),
    ('frame_interval_low', 0x3d084a8, 4),
    ('frame_interval_high', 0x3d084ac, 4),
    ('interval_multiplier', 0x389d7dc, 4),
    ('configured_interval', 0x89295c, 4),
    ('previous_counter_low', 0x3d08550, 4),
    ('previous_counter_high', 0x3d08554, 4),
    ('extra_ticks', 0x3b11180, 4),
    ('logic_tick_count', 0x3b1118c, 4),
    ('loop_starting', 0x3b11198, 4),
    ('demo_playing', 0x3b2a2e4, 4),
    ('logic_entry_flag', 0x3abdf70, 4),
    ('previous_loop_starting', 0x3b11194, 4),
    ('exit_requested', 0x3abdf7c, 4),
    ('window_active', 0x8929d4, 4),
    ('loading_file', 0x3abdf68, 1),
    ('special_render_flags', 0x909eb4, 1),
    ('fog_profile', 0x3abdc6c, 1),
    ('queue_fog_enabled', 0x3abdcfe, 1),
    ('active_fog_near_distance', 0x3abdca0, 4),
    ('active_fog_far_distance', 0x3abdca4, 4),
    ('restored_fog_near_distance', 0x3abdc70, 4),
    ('restored_fog_far_distance', 0x3abdc74, 4),
    ('save_count', 0x3b291b2, 1),
    ('save_pending', 0x3abdf76, 1),
    ('texture_pending', 0x3abdf79, 1),
    ('late_frame_work_enabled', 0x3abdf80, 4),
    ('game_frame_count', 0x3abdf58, 4),
    ('local_frame_count', 0x3abdf5c, 4),
    ('shutdown_cleanup_enabled', 0x3c85804, 4),
    ('game_mode', 0x3b22de4, 2),
    ('demo_cutscene', 0x3b2a2e8, 4),
    ('soft_reset_flags', 0x3b0eaa0, 1),
    ('reset_input_mask', 0x3b0e3b8, 1),
]
ROOT = Path(__file__).resolve().parents[2]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def loaded_bytes(image, address, size):
    matches = [section for section in image.sections if
               section['rva'] <= address - image.base and
               address - image.base + size <= section['rva'] + max(section['virtual_size'], section['raw_size'])]
    if len(matches) != 1:
        raise ValueError(f'control fact not uniquely mapped: {address:#x}+{size}')
    section = matches[0]
    offset = address - image.base - section['rva']
    return bytes(image.data[section['file_offset'] + index] if index < section['raw_size'] else 0
                 for index in range(offset, offset + size))


def fixture_bytes(image, words):
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
                raise ValueError('overlapping control facts')
            spans.add(byte)
        data[offset:offset + size] = word.to_bytes(size, 'little')
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--poison-output', type=Path, help='reference-only PE fixture with these control bytes XOR-poisoned')
    parser.add_argument('--varied-output', type=Path)
    parser.add_argument('--partial-output', type=Path)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('pinned source identity mismatch')
    expected = [int.from_bytes(loaded_bytes(image, address, size), 'little') for _, address, size in FIELDS]
    expected_bytes = struct.pack('<51I', *expected)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    probe = library.sadx_native_session_control_defaults
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
    sources = ['src/runtime/session_control_defaults.hpp', 'src/runtime/session_control_defaults.cpp',
               'src/engine/game_frame_control.hpp', 'src/game/game_mode.hpp',
               'tests/probes/session_control_probe.hpp',
               'tests/probes/native_session_control_defaults_probe.cpp',
               'tools/oracles/verify_original_session_control_defaults.py']
    report = dict(source_sha256=image.sha256, native_library_sha256=sha(args.native_library.read_bytes()),
                  source_files={name: sha((ROOT / name).read_bytes()) for name in sources},
                  original_ranges_sha256={f'{address:x}-{address + size:x}':
                                          sha(loaded_bytes(image, address, size)) for _, address, size in FIELDS},
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
        args.varied_output.write_bytes(fixture_bytes(image, list(varied)))
    if args.partial_output and not mismatches:
        partial = bytearray(image.data)
        pe, = image.unpack_offset('<I', 60)
        optional_size, = image.unpack_offset('<H', pe + 20)
        table = pe + 24 + optional_size
        cutoff = 0x3d0853c # frequency high word and previous counter become unavailable
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
