#!/usr/bin/env python3
"""Capture complete music-owner probe outputs under unchanged original judges.

Run with the pinned reference Python and freshly built libraries. Binary streams
stay local; commit hashes and explicit partial/boundary identities. This observes
all input/output arrays without changing cases, expectations or comparisons.
Actual void C exports have no return value; their incidental register is ignored.
"""
import argparse
import ctypes
import hashlib
import importlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
VOID_EXPORTS = {
    'sadx_native_menu_voice_probe', 'sadx_native_cleanup_load_save',
    'sadx_native_cleanup_helpers', 'sadx_native_title_menu',
}
GROUPS = ['music', 'menu_voice', 'game_cleanup', 'game_reset',
          'game_reset_composed', 'stream_players', 'title_menu']


class CapturedFunction:
    def __init__(self, function, name, stream):
        object.__setattr__(self, 'function', function)
        object.__setattr__(self, 'name', name)
        object.__setattr__(self, 'stream', stream)

    def __setattr__(self, key, value):
        setattr(self.function, key, value)

    def __call__(self, *args):
        def snapshot():
            return [bytes(arg) if isinstance(arg, ctypes.Array) else b'' for arg in args]

        before = snapshot()
        result = self.function(*args)
        after = snapshot()
        recorded_return = -999 if self.name in VOID_EXPORTS or result is None else result
        payload = self.name.encode() + b'\0' + struct.pack('<q', recorded_return)
        for value in before + after:
            payload += struct.pack('<I', len(value)) + value
        self.stream.write(struct.pack('<I', len(payload)) + payload)
        return result


class CapturedLibrary:
    def __init__(self, path, stream, loader):
        self.library = loader(path)
        self.stream = stream
        self.functions = {}

    def __getattr__(self, name):
        if name not in self.functions:
            self.functions[name] = CapturedFunction(
                getattr(self.library, name), name, self.stream)
        return self.functions[name]


def captured_arrays(path):
    data = path.read_bytes()
    position = 0
    while position < len(data):
        size = struct.unpack_from('<I', data, position)[0]
        record = data[position + 4:position + 4 + size]
        position += 4 + size
        cursor = record.index(b'\0') + 9  # name, terminator, int64 return
        arrays = []
        while cursor < len(record):
            size = struct.unpack_from('<I', record, cursor)[0]
            cursor += 4
            arrays.append(record[cursor:cursor + size])
            cursor += size
        yield arrays


def capture(group, args):
    module = importlib.import_module('verify_original_' + group)
    library_name = 'game_cleanup' if group == 'game_reset' else group
    path = args.output / (group + '.bin')
    source_run = None
    expected_flags = []
    if group == 'title_menu':
        source_run = module.Source.run

        def observe_source(source, values):
            expected = source_run(source, values)
            expected_flags.append(expected[135])
            return expected

        module.Source.run = observe_source

    loader = ctypes.CDLL
    arguments = sys.argv
    try:
        with path.open('wb') as stream:
            ctypes.CDLL = lambda path, *a, **kw: CapturedLibrary(path, stream, loader)
            sys.argv = [module.__file__, str(args.exe),
                        str((args.libraries / (library_name + '.dylib')).resolve()),
                        '--output', str(args.output / (group + '.json'))]
            result = module.main()
    finally:
        ctypes.CDLL = loader
        sys.argv = arguments
        if source_run is not None:
            module.Source.run = source_run

    summary = {'exit': int(result or 0), 'stream_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
               'bytes': path.stat().st_size}
    if group == 'title_menu':
        partials = []
        for index, arrays in enumerate(captured_arrays(path)):
            actual = struct.unpack('<512I', arrays[-1])
            if actual[135] != expected_flags[index]:
                partials.append({'case': index, 'source': expected_flags[index], 'native': actual[135]})
        (args.output / 'title-partials.json').write_text(json.dumps(partials) + '\n')
        summary['partial_count'] = len(partials)
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--libraries', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--groups', nargs='+', choices=GROUPS, default=GROUPS)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles')]
    summary = {}
    for group in args.groups:
        summary[group] = capture(group, args)
        print(group, summary[group], flush=True)
    (args.output / 'replay.json').write_text(json.dumps(summary, indent=2) + '\n')


if __name__ == '__main__':
    main()
