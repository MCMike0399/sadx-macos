#!/usr/bin/env python3
"""Build native Input2.6.1 constants in an isolated closure and check their verified fingerprint."""
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

SOURCES = ('src/input/input_mod_constants.hpp', 'src/input/input_mod_constants.cpp',
           'tests/probes/input_mod_constants_probe.hpp', 'tests/probes/native_input_mod_constants_probe.cpp',
           'src/input/input_mod_pad.hpp', 'src/input/input_mod_axes.hpp', 'src/input/input.hpp',
           'src/engine/game_controller.hpp', 'src/cpu/sse_math.hpp',
           'src/engine/controller_values.hpp')
UNITS = (SOURCES[1], SOURCES[3])
WORKER = r'''
import ctypes,hashlib,json,struct,sys
library=ctypes.CDLL(sys.argv[1])
probe=library.sadx_native_input_mod_constants
probe.argtypes=[ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t]
probe.restype=ctypes.c_size_t
words=(ctypes.c_uint32*7)()
if probe(0,words,7)!=7:raise RuntimeError('native serialization size')
print(json.dumps(dict(words=7,sha256=hashlib.sha256(struct.pack('<7I',*words)).hexdigest())))
'''


def check(cxx):
    compiler = shutil.which(cxx)
    if not compiler:
        raise RuntimeError(f'compiler unavailable: {cxx}')
    reference = json.loads((ROOT / 'reference/oracle-reports/input_mod_constants.json').read_text())
    if (reference.get('returned_values_and_boundaries_match') is not True or
            reference.get('mismatches') or reference.get('words_compared') != 7):
        raise RuntimeError('source-data comparison is not exact')
    for name in SOURCES:
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != reference['source_files'][name]:
            raise RuntimeError(f'stale source-data comparison: {name}')
    with tempfile.TemporaryDirectory(prefix='sadx-native-input-mod-constants-') as temporary:
        isolated = Path(temporary).resolve()
        closure = compile_closure(compiler, isolated, SOURCES, UNITS)
        darwin = platform.system() == 'Darwin'
        name = 'input-mod-constants.dylib' if darwin else 'input-mod-constants.so'
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
                    limits=['Seven arithmetic coefficients only; consumer algorithms, mutable pad data and resources remain outside this source closure.',
                            'Isolated source/dependency closure, not an operating-system access sandbox.',
                            'Only the verified fingerprint is read before isolation; no original binary or table data is copied.'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--report', type=Path, default=ROOT / 'build/native-input-mod-constants.json')
    args = parser.parse_args()
    try:
        report = check(args.cxx)
    except (OSError, RuntimeError, ValueError, KeyError, subprocess.TimeoutExpired) as error:
        report = dict(passed=False, error=str(error))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f"Native Input2.6.1 constants: {'PASS' if report['passed'] else 'FAIL'} ({args.report})")
    if not report['passed']:
        print(report['error'])
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
