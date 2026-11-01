"""Serial local reference probes from registered per-object Source builds.

Run with the pinned reference Python. No Main archives or reserved cohorts.
The old loop body is extracted from immutable Main into ignored build storage;
headers remain checked against the baseline. Controls inherits a strict FP partial.
"""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys

root = Path('build/save-source/consumer-probes')
root.mkdir(parents=True, exist_ok=True)
rows = []
env = {**os.environ, 'PYTHONPATH': 'tools/lib:tools/oracles', 'PYTHONDONTWRITEBYTECODE': '1'}
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
def run(name, command, expected=0):
    result = subprocess.run(command, capture_output=True, text=True, env=env)
    (root / (name + '.log')).write_text(result.stdout + result.stderr)
    rows.append(dict(name=name, command=command, exit=result.returncode))
    (root / 'commands.json').write_text(json.dumps(rows, indent=2) + '\n')
    assert result.returncode == expected, (name, result.returncode)

objects = ['game/file_select_controls', 'game/title_menu', 'math/ninja_math', 'math/ninja_sine_table', 'cpu/x87_math']
run('objects', ['make', '-j1', *['build/obj/' + p + '.o' for p in objects], 'build/softfloat/softfloat.a'])
run('archive', ['xcrun', 'libtool', '-static', '-o', str(root / 'controls-support.a'), *['build/obj/' + p + '.o' for p in objects]])
flags = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0', '-Isrc', '-Itests/fixtures', '-dynamiclib', '-undefined', 'dynamic_lookup', '-Wl,-dead_strip']
old = root / 'old-loop.cpp'
old.write_bytes(subprocess.check_output(['git', 'show', 'a799c518a2dc4ae9fceb3aeb05f7a41f270abb93:src/guest/game_loop.cpp']))
exe = '/Users/burbujamc/Developer/sadx-macos/reference/updated-pc-reference/game/sonic.exe'
compare_path = Path('reference/packets/save-io-frame-judges/compare.py')
spec = importlib.util.spec_from_file_location('compare', compare_path)
compare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compare)
comparisons = []
for name, slug, sources in [
    ('loop-old', 'game_loop', ['tests/probes/native_game_loop_probe.cpp', str(old)]),
    ('loop-new', 'game_loop', ['tests/probes/native_game_loop_probe.cpp', 'src/guest/game_loop.cpp']),
    ('controls', 'file_select_controls', ['tests/probes/native_file_select_controls_probe.cpp', str(root / 'controls-support.a'), 'build/softfloat/softfloat.a']),
]:
    export = root / (name + '.exports')
    export.write_text('_sadx_native_' + ('game_loop' if slug == 'game_loop' else 'file_select_controls_probe') + '\n')
    library = root / (name + '.dylib')
    run(name + '-build', flags + ['-Wl,-exported_symbols_list,' + str(export), *sources, '-o', str(library)])
    directory = root / name
    run(name + '-original', [sys.executable, 'reference/packets/save-io-frame-judges/observe.py', 'tools/oracles/verify_original_' + slug + '.py', exe, str(library.resolve()), str(directory)], 1 if slug == 'file_select_controls' else 0)
    stream = directory / ('verify_original_' + slug + '.abi.pkl.gz')
    historical = Path('/Users/burbujamc/.t3/worktrees/sadx-macos/t3-5acabf9a/build/save-io-judges') / ('loop-original' if slug == 'game_loop' else 'controls-original') / stream.name
    difference = compare.compare(compare.records(historical), compare.records(stream))
    assert difference is None, (name, difference)
    receipt = json.loads((directory / ('verify_original_' + slug + '.receipt.json')).read_text())
    comparisons.append(dict(name=name, first_defined_difference=difference, receipt=receipt, historical=str(historical), historical_sha256=sha(historical)))
(root / 'comparison.json').write_text(json.dumps(comparisons, indent=2) + '\n')
