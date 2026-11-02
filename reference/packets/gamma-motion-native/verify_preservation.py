"""Replay unchanged motion/blend fixtures before and after the borrowing seam.

Imports the maintained verifiers unchanged, retaining every emitted partial ID.
The before libraries are built from git-show 3b36f63b animation sources/headers;
all other linked members are the unchanged local native support archive.
"""
import ctypes
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles')]
from pe_image import PEImage
import verify_original_player_animation as motion
import verify_original_player_animation_blend as blend


def main():
    output = Path(sys.argv[1])
    image = PEImage(ROOT / 'game/Sonic Adventure DX/sonic.exe')
    constants = (ctypes.c_float * 3).from_buffer_copy(
        b''.join(image.read(address, 4) for address in motion.CONSTANTS))
    reports = {}
    for slug, module, before_name, width, count, flag, original_type in [
        ('player_animation', motion, 'before-motion', 37, 15, 9, motion.AnimationOriginal),
        ('player_animation_blend', blend, 'before-blend', 62, 35, 30, blend.BlendOriginal),
    ]:
        before = ROOT / ('build/gamma-motion/' + before_name + '.dylib')
        after = ROOT / ('build/native-' + slug.replace('_', '-') + '-probe.dylib')
        functions = []
        libraries = []
        for path in (before, after):
            library = ctypes.CDLL(str(path))
            libraries.append(library)
            probe = getattr(library, 'sadx_native_' + slug)
            probe.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_uint32),
                              ctypes.POINTER(ctypes.c_uint32)]
            probe.restype = ctypes.c_int
            functions.append(probe)
        oracle = original_type(image)
        case_digest, stream = hashlib.sha256(), hashlib.sha256()
        partials = []
        cases = 0
        for control in (pc | rc | 0x7f for pc in (0, 0x200, 0x300)
                        for rc in (0, 0x400, 0x800, 0xc00)):
            for fixture in module.fixtures():
                fixture = [x & 0xffffffff for x in fixture]
                fixture[11 if slug == 'player_animation' else 0] = control
                raw = struct.pack('<' + 'I' * width, *fixture)
                case_digest.update(raw)
                observed = []
                for function in functions:
                    result = (ctypes.c_uint32 * count)()
                    returned = function(constants, (ctypes.c_uint32 * width)(*fixture), result)
                    assert returned == 0
                    observed.append(bytes(result))
                assert observed[0] == observed[1], (slug, cases, 'before/current output changed')
                stream.update(raw + observed[1])
                actual = struct.unpack('<' + 'I' * count, observed[1])
                expected = oracle.run(fixture)
                if slug == 'player_animation':
                    expected, boundary, returned = expected
                    assert bool(actual[13]) == returned
                    assert not boundary or actual[12] == boundary
                for index, value in enumerate(expected):
                    if actual[index] != value:
                        assert index == flag, (slug, cases, index, value, actual[index])
                        partials.append([cases, value, actual[index]])
                cases += 1
        reports[slug] = dict(cases=cases, complete_output_changes=0, case_sha256=case_digest.hexdigest(),
                             stream_sha256=stream.hexdigest(), fp_partial_count=len(partials),
                             fp_partial_identities=partials,
                             before_library_sha256=hashlib.sha256(before.read_bytes()).hexdigest(),
                             current_library_sha256=hashlib.sha256(after.read_bytes()).hexdigest())
    output.write_text(json.dumps(reports, indent=2) + '\n')
    print(json.dumps({slug: {k: v for k, v in report.items() if k != 'fp_partial_identities'}
                      for slug, report in reports.items()}, indent=2))


if __name__ == '__main__':
    main()
