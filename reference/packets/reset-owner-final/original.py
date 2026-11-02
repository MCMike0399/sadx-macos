"""Internal live-parent-only original/reset-probe observations; no generator or build."""
import argparse
import ctypes
import json
from pathlib import Path
import struct
from types import ModuleType

PATH = Path(__file__).with_name('collector.py').resolve()
collector = ModuleType('reset_frozen_collector')
collector.__file__ = str(PATH)
exec(compile(PATH.read_bytes(), str(PATH), 'exec'), collector.__dict__)


def reset_observation(freeze, case, modules, image):
    values = freeze['inputs']['reference_inputs'][case]
    fail_at = 9 if case == 'reserved-media-failure' else 14
    data = [*values[:5], fail_at]
    source = modules['reset'].Source(image)
    expected = source.run(data)
    boundary = 0x40bf7f if fail_at == 9 else 0x40bf9e
    collector.require(expected[:3] == [3, boundary, 0], 'Unexpected original stop contract')
    native = {}
    for world in ('old', 'current'):
        library = ctypes.CDLL(freeze['probes'][world + '_reset'])
        function = library.sadx_native_game_reset
        function.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
                             ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
        function.restype = ctypes.c_int
        output = (ctypes.c_uint32 * 128)()
        result = function((ctypes.c_uint32 * 6)(*data), 6, output, 128)
        collector.require(result == 1 and list(output) == expected,
                          world + ' full reset ABI/source prefix differs')
        native[world] = {'abi_return': result, 'words': list(output)}
    return {'case': case, 'kind': 'original_observer_stop', 'input': data,
            'source_returned': 0, 'source_stop_boundary': boundary,
            'original_pc': source.m.reg_read(modules['reset'].UC_X86_REG_EIP),
            'stop_pc': modules['reset'].STOP,
            'original_words': expected, 'native': native}


def width_observation(freeze, modules, image):
    module = modules['composed']
    inputs = freeze['inputs']['source_editor_width_subobservation']
    pages = bytearray(len(module.PAGES) * 4096)
    offset = module.PAGES.index(0x3b29000) * 4096 + 0xd08
    struct.pack_into('<I', pages, offset, inputs['editor'] | inputs['upper_neighbor'] << 16)
    source = module.Source(image)
    expected_pages, expected_words = source.run(bytes(pages), 0)
    collector.require(expected_words[0:2] == [1, 0], 'Original did not reach normal return')
    collector.require(struct.unpack_from('<I', expected_pages, offset)[0] ==
                      inputs['upper_neighbor'] << 16, 'Original editor width contract differs')
    native = {}
    for world in ('old', 'current'):
        library = ctypes.CDLL(freeze['probes'][world + '_composed'])
        function = library.sadx_native_game_reset_composed
        function.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_size_t, ctypes.c_uint32,
                             ctypes.POINTER(ctypes.c_ubyte), ctypes.POINTER(ctypes.c_uint32),
                             ctypes.c_size_t]
        function.restype = ctypes.c_int
        output_pages = (ctypes.c_ubyte * len(pages))()
        output_words = (ctypes.c_uint32 * 200)()
        result = function((ctypes.c_ubyte * len(pages)).from_buffer_copy(pages), len(pages), 0,
                          output_pages, output_words, 200)
        collector.require(result == 1 and bytes(output_pages) == expected_pages and
                          list(output_words) == expected_words,
                          world + ' full composed pages/ABI differ')
        native[world] = {'abi_return': result, 'pages_hex': bytes(output_pages).hex(),
                         'words': list(output_words)}
    return {'case': 'reserved-editor-read', 'kind': 'original_returned_width_subobservation',
            'source_returned': 1, 'original_pc': source.m.reg_read(module.UC_X86_REG_EIP),
            'stop_pc': module.STOP, 'input_pages_hex': bytes(pages).hex(),
            'original_pages_hex': expected_pages.hex(), 'original_words': expected_words,
            'native': native}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--admission-commit', required=True)
    parser.add_argument('--case', choices=['reserved-media-failure', 'reserved-task-failure',
                                          'reserved-editor-read'], required=True)
    parser.add_argument('--parent-fd', type=int, required=True)
    parser.add_argument('--stage-token', required=True)
    args = parser.parse_args()
    helper = collector.source_helper()
    helper.OUT = collector.OUTPUT
    helper.admit_child('original-' + args.case, args.parent_fd, args.stage_token)
    admission = collector.committed(collector.ADMISSION, args.admission_commit)
    freeze = collector.committed(collector.FREEZE, admission.get('freeze_commit', ''))
    collector.validate_admission(admission, freeze, internal=True)
    import sys
    old = Path(freeze['old_root'])
    package = Path(freeze['unicorn_package'])
    sys.path.insert(0, str(package.parent))
    helper.pinned_unicorn(package)
    modules = {}
    modules['pe'] = helper.source_module('pe_image', old / 'tools/lib/pe_image.py')
    extract = helper.source_module('extract', old / 'tools/lib/extract.py')
    helper.source_module('verify_original_math', old / 'tools/oracles/verify_original_math.py')
    modules['reset'] = helper.source_module(
        'verify_original_game_reset', old / 'tools/oracles/verify_original_game_reset.py')
    modules['composed'] = helper.source_module(
        'verify_original_game_reset_composed', old / 'tools/oracles/verify_original_game_reset_composed.py')
    helper.verify_origins(old, package)
    image = modules['pe'].PEImage(
        '/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe')
    collector.require(image.sha256 == extract.SOURCE_SHA256, 'Pinned original image differs')
    if args.case == 'reserved-editor-read':
        result = width_observation(freeze, modules, image)
    else:
        result = reset_observation(freeze, args.case, modules, image)
    print(json.dumps(result, separators=(',', ':')))


if __name__ == '__main__':
    main()
