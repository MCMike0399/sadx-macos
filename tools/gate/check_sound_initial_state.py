#!/usr/bin/env python3
"""Build native sound initial state in an isolated closure and check their verified fingerprint."""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile

# Reuse the pilot's dependency, environment, symbol and library guards unchanged.
from check_native_frame_control import compile_closure, inspect_libraries, FORBIDDEN, ROOT, run

SOURCES = ('src/game/sound_initial_state.hpp', 'src/game/sound_initial_state.cpp',
           'tests/probes/sound_initial_state_probe.hpp', 'tests/probes/native_sound_initial_state_probe.cpp')
UNITS = (SOURCES[1], SOURCES[-1])
WORKER = r'''
import ctypes,hashlib,json,struct,sys
library=ctypes.CDLL(sys.argv[1])
probe=library.sadx_native_sound_initial_state
probe.argtypes=[ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t]
probe.restype=ctypes.c_size_t
words=(ctypes.c_uint32*329)()
if probe(0,words,329)!=329:raise RuntimeError('native serialization size')
print(json.dumps(dict(words=329,sha256=hashlib.sha256(struct.pack('<329I',*words)).hexdigest())))
'''


def check(cxx):
    compiler = shutil.which(cxx)
    if not compiler:
        raise RuntimeError(f'compiler unavailable: {cxx}')
    reference = json.loads((ROOT / 'reference/oracle-reports/sound_initial_state.json').read_text())
    if (reference.get('returned_values_and_boundaries_match') is not True or
            reference.get('mismatches') or reference.get('words_compared') != 329):
        raise RuntimeError('source-data comparison is not exact')
    for name in SOURCES:
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != reference['source_files'][name]:
            raise RuntimeError(f'stale source-data comparison: {name}')
    with tempfile.TemporaryDirectory(prefix='sadx-native-sound-initial-state-') as temporary:
        isolated = Path(temporary).resolve()
        closure = compile_closure(compiler, isolated, SOURCES, UNITS)
        darwin = platform.system() == 'Darwin'
        name = 'sound-initial-state.dylib' if darwin else 'sound-initial-state.so'
        identity = '@rpath/' + name if darwin else None
        flags = ['-dynamiclib', '-Wl,-install_name,' + identity] if darwin else ['-shared']
        command = [compiler, *flags, *closure['objects'].values(), '-o', name]
        run(command, isolated, closure['env'])
        binary = isolated / name
        symbols = run(['/usr/bin/nm', '-C', str(binary)], isolated, closure['env'])
        if FORBIDDEN.search(symbols):
            raise RuntimeError('reference-runtime symbol in native table library')
        _, libraries = inspect_libraries(binary, isolated, closure['env'], identity)
        result = json.loads(run([sys.executable, '-I', '-c', WORKER, str(binary)],
                                isolated, closure['env']))
        if result['sha256'] != reference['expected_words_sha256']:
            raise RuntimeError('isolated native table fingerprint differs from verified original words')
        return dict(passed=True, source_sha256=closure['files'], compiler_dependencies=closure['dependencies'],
                    toolchain_metadata_sha256=closure['toolchain'], linked_libraries=libraries,
                    reference_runtime_symbols_found=False, commands=[*closure['commands'], command],
                    binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                    serialized=result, copied_original_binary_inputs=[], copied_fixtures=[],
                    inherited_environment=False, core_archive_linked=False,
                    limits=['Load-time numeric sound values only; algorithms, identities, tasks, listener and audio resources remain outside this source closure.',
                            'Isolated source/dependency closure, not an operating-system access sandbox.',
                            'Only the verified fingerprint is read before isolation; no original binary or table data is copied.'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--report', type=Path, default=ROOT / 'build/native-sound-initial-state.json')
    args = parser.parse_args()
    try:
        report = check(args.cxx)
    except (OSError, RuntimeError, ValueError, KeyError, subprocess.TimeoutExpired) as error:
        report = dict(passed=False, error=str(error))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f"Native sound initial state: {'PASS' if report['passed'] else 'FAIL'} ({args.report})")
    if not report['passed']:
        print(report['error'])
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
