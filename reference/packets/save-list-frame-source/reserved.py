"""Root reserved save qualification. Freeze first; execute once after gate admission.

Build both immutable old and reviewed Source directly. Existing observers and
defined-ABI comparison remain unchanged, including unknown void-return registers.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
PACKET = Path(__file__).resolve().parent
FREEZE = PACKET / 'reserved-freeze.json'
OUT = ROOT / 'build/save-reserved'
BASE = 'a799c518a2dc4ae9fceb3aeb05f7a41f270abb93'
EXE = Path('/Users/burbujamc/Developer/sadx-macos/reference/updated-pc-reference/game/sonic.exe')
DLL = EXE.parent / 'mods/sadx-input-mod/sadx-input-mod.dll'
SLUGS = ('save_file_list', 'load_selected_save', 'save_write', 'save_frame',
         'save_creation', 'save_data', 'save_completion', 'file_select_controls')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def freeze():
    inputs = {}
    for directory in ('src', 'tools/lib', 'tools/oracles', 'tests/probes', 'tests/fixtures',
                      'reference/packets/save-io-frame-judges', 'reference/packets/save-list-frame-source'):
        for path in (ROOT / directory).rglob('*'):
            if path.is_file() and '__pycache__' not in path.parts and path != FREEZE:
                inputs[str(path.relative_to(ROOT))] = sha(path)
    for name in ('tests/unit/save_io_characterization.cpp', 'tests/unit/save_frame_characterization.cpp',
                 'build/softfloat/softfloat.a'):
        inputs[name] = sha(ROOT / name)
    record = dict(base=BASE, candidate=git('rev-parse', 'HEAD').decode().strip(), inputs=inputs,
                  exe=dict(path=str(EXE), sha256=sha(EXE)), dll=dict(path=str(DLL), sha256=sha(DLL)),
                  python=str(Path(sys.executable).resolve()), python_sha256=sha(Path(sys.executable).resolve()),
                  cohorts=json.loads((ROOT / 'reference/packets/save-io-frame-judges/reserved-cohorts.json').read_text()),
                  policy='Execute once only after explicit --admitted and committed freeze. Generate no fresh vectors before the marker. Stop on build, observer, value/prefix or defined-ABI difference. Preserve all attempts; never retry reserved inputs or normalize defined values.')
    with FREEZE.open('x') as stream:
        stream.write(json.dumps(record, indent=2) + '\n')


def run_command(command, cwd, log, expected=0):
    env = dict(os.environ, PYTHONPATH=str(ROOT / 'tools/lib') + ':' + str(ROOT / 'tools/oracles'))
    with log.open('w') as stream:
        result = subprocess.run(command, cwd=cwd, env=env, stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode != expected:
        raise RuntimeError(f'{command}: exit {result.returncode}, expected {expected}; {log}')


def build_world(tree, label):
    directory = OUT / label
    directory.mkdir()
    flags = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
             '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
             '-Isrc', '-Itests/fixtures', '-Wl,-dead_strip']
    common = ['src/game/save_write.cpp', 'src/game/save_file_list.cpp', 'src/game/save_data.cpp', 'src/game/file_select.cpp']
    for slug, sources in [('io', ['tests/unit/save_io_characterization.cpp', *common]),
                          ('frame', ['tests/unit/save_frame_characterization.cpp', 'src/game/save_frame.cpp', 'src/guest/game_loop.cpp'])]:
        run_command(flags + sources + ['-o', str(directory / slug)], tree, directory / (slug + '-build.log'))
    library = directory / 'save.dylib'
    run_command([sys.executable, str(ROOT / 'reference/packets/save-io-frame-judges/build.py'), str(library)],
                tree, directory / 'save-build.log')
    # Controls has an existing, independently observed x87 consumer closure.
    controls = directory / 'controls.dylib'
    sources = ['tests/probes/native_file_select_controls_probe.cpp', 'src/game/file_select_controls.cpp',
               'src/game/title_menu.cpp', 'src/math/ninja_math.cpp', 'src/math/ninja_sine_table.cpp',
               'src/cpu/x87_math.cpp', 'src/cpu/crt_integer.cpp']
    exports = directory / 'controls.exports'
    exports.write_text('_sadx_native_file_select_controls_probe\n')
    run_command(flags + ['-dynamiclib', '-undefined', 'dynamic_lookup', '-Wl,-exported_symbols_list,' + str(exports),
                        *sources, str(ROOT / 'build/softfloat/softfloat.a'), '-o', str(controls)],
                tree, directory / 'controls-build.log')
    return directory, library, controls


def run(admitted):
    if not admitted:
        raise ValueError('Heavy work requires explicit gate admission')
    if git('show', 'HEAD:' + str(FREEZE.relative_to(ROOT))) != FREEZE.read_bytes():
        raise ValueError('Freeze must be committed unchanged')
    frozen = json.loads(FREEZE.read_text())
    for name, digest in frozen['inputs'].items():
        if sha(ROOT / name) != digest:
            raise ValueError('Frozen dependency changed: ' + name)
    for record in (frozen['exe'], frozen['dll']):
        if sha(Path(record['path'])) != record['sha256']:
            raise ValueError('Wrong original input')
    if str(Path(sys.executable).resolve()) != frozen['python'] or sha(Path(sys.executable).resolve()) != frozen['python_sha256']:
        raise ValueError('Wrong pinned interpreter')
    OUT.mkdir(exist_ok=False)
    (OUT / 'EXECUTED_ONCE').write_text('Reserved definitions consumed; no retries or regeneration.\n')
    old = OUT / 'old-source'
    old.mkdir()
    archive = OUT / 'old-source.tar'
    with archive.open('wb') as stream:
        subprocess.run(['git', 'archive', BASE, 'src', 'tests', 'third_party'], cwd=ROOT, stdout=stream, check=True)
    subprocess.run(['tar', '-xf', str(archive), '-C', str(old)], check=True)
    archive.unlink()
    spec = importlib.util.spec_from_file_location('defined_comparison', ROOT / 'reference/packets/save-io-frame-judges/compare.py')
    comparison = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(comparison)
    results = dict(native={}, original={}, fixed_defaults_rerun=False)
    worlds = {}
    for label, tree in (('old', old), ('current', ROOT)):
        worlds[label] = build_world(tree, label)
    cohorts = frozen['cohorts']
    for slug, cohort in (('io', 'native_io'), ('frame', 'native_frame')):
        outputs = {}
        for label, (directory, _, _) in worlds.items():
            path = directory / (slug + '-capture.txt')
            run_command([str(directory / slug), *cohorts[cohort]['arguments']], ROOT, path)
            outputs[label] = dict(path=str(path), bytes=path.stat().st_size, sha256=sha(path))
        if Path(outputs['old']['path']).read_bytes() != Path(outputs['current']['path']).read_bytes():
            raise ValueError('Reserved native stream mismatch: ' + slug)
        results['native'][slug] = outputs
    seed, count = cohorts['original_extra_seed'], cohorts['original_extra_cases']
    for slug in SLUGS:
        streams = {}
        for label, (directory, library, controls) in worlds.items():
            destination = directory / slug
            command = [sys.executable, str(ROOT / 'reference/packets/save-io-frame-judges/observe.py'),
                       'tools/oracles/verify_original_' + slug + '.py', str(EXE),
                       str(controls if slug == 'file_select_controls' else library), str(destination),
                       '--seed', str(seed), '--cases', str(count)]
            if slug in ('save_data', 'load_selected_save'):
                command += ['--input-dll', str(DLL)]
            run_command(command, ROOT, directory / (slug + '.log'), 1 if slug == 'file_select_controls' else 0)
            receipt = json.loads((destination / ('verify_original_' + slug + '.receipt.json')).read_text())
            if receipt['native_calls'] == 0 or receipt['source_returns'] == 0:
                raise ValueError('Empty reserved observation: ' + slug)
            streams[label] = destination / ('verify_original_' + slug + '.abi.pkl.gz')
        difference = comparison.compare(comparison.records(streams['old']), comparison.records(streams['current']))
        results['original'][slug] = dict(first_defined_difference=difference,
            captures={label: dict(path=str(path), sha256=sha(path)) for label, path in streams.items()})
        (OUT / 'progress.json').write_text(json.dumps(results, indent=2) + '\n')
        if difference is not None:
            raise ValueError(f'Reserved defined ABI mismatch: {slug} record {difference}')
    results.update(exit=0, freeze_sha256=sha(FREEZE), source_tree=git('rev-parse', 'HEAD:src').decode().strip(),
                   void_return_policy=sorted(comparison.VOID_EXPORTS), no_expected_values_changed=True)
    (OUT / 'result.json').write_text(json.dumps(results, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation', choices=('freeze', 'run'))
    parser.add_argument('--admitted', action='store_true')
    arguments = parser.parse_args()
    freeze() if arguments.operation == 'freeze' else run(arguments.admitted)
