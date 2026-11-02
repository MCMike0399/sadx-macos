#!/usr/bin/env python3
"""Actual caller and arena reentry faults; no replacement actor/controller logic."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assets', default='game/imported')
    parser.add_argument('--output', default='build/gamma-weld/binding-mutations.json')
    args = parser.parse_args()
    original = (ROOT / 'src/runtime/native_gamma_binding.cpp').read_text()
    first = original.index('void join_vertices(Context &context')
    last = original.index('// NOTSA: native boundary for GammaUpdateLaserTargets', first)
    body = original[first:last]
    initialize = 'vertex_weld::initialize(**table, arena(context), context.scene);'
    assert initialize in body
    faults = [
        ('captured-null-alias', body.replace('bind_join_vertex_mode(player)',
                                            'bind_join_vertex_mode(context.players.current.player ? '
                                            'context.players.registry.allocations.at('
                                            'context.players.current.player.get()) : Player{})')),
        ('copied-weld-table', body.replace(initialize,
             'auto copied_table = **table;\n        '
             'vertex_weld::initialize(copied_table, arena(context), context.scene);')),
        ('skipped-initialization', body.replace(initialize, '(void)table;')),
    ]
    directory = ROOT / 'build/gamma-weld/binding-faults'
    directory.mkdir(parents=True, exist_ok=True)
    results = []
    flags = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
             '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64',
             '-mmacosx-version-min=14.0', '-Isrc', '-Itests/fixtures']
    for name, replacement in faults:
        source = directory / f'{name}.cpp'
        obj = directory / f'{name}.o'
        executable = directory / name
        source.write_text(original[:first] + replacement + original[last:])
        compile_command = flags + ['-c', str(source), '-o', str(obj)]
        compiled = subprocess.run(compile_command, cwd=ROOT, capture_output=True, text=True)
        row = {'name': name, 'compile_exit': compiled.returncode,
               'compile_stderr': compiled.stderr, 'compile_command': compile_command}
        if compiled.returncode == 0:
            link_command = flags + ['-fobjc-arc', 'tests/unit/native_gamma_vertex_weld.mm',
                                   str(obj), 'build/libsadx-native.a', '-framework', 'Foundation',
                                   '-o', str(executable)]
            linked = subprocess.run(link_command, cwd=ROOT, capture_output=True, text=True)
            row.update(link_exit=linked.returncode, link_stderr=linked.stderr,
                       link_command=link_command)
            if linked.returncode == 0:
                command = [str(executable), args.assets, str(directory / f'{name}.tsv')]
                run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
                row.update(run_exit=run.returncode, stdout=run.stdout, stderr=run.stderr,
                           command=command)
        row['caught'] = row.get('run_exit') == -6
        results.append(row)
        print(name, 'caught' if row['caught'] else 'NOT CAUGHT', flush=True)

    # FreeMemory receives a captured pointer. Reentry can replace the row's org
    # field during metadata stores; freeing through that field by reference is wrong.
    source = (ROOT / 'src/player/player_vertex_weld.cpp').read_text()
    before = 'const auto result = arena.release(allocation);'
    assert before in source
    source = source.replace(before, 'const auto result = arena.release(join.original_vertices);')
    # Remove the now-unused capture so -Werror does not substitute for the observer.
    source = source.replace('const auto allocation = join.original_vertices;\n        const auto result',
                            'const auto result')
    product = directory / 'uncaptured-free.cpp'
    obj = directory / 'uncaptured-free.o'
    executable = directory / 'uncaptured-free'
    product.write_text(source)
    compiled = subprocess.run(flags + ['-c', str(product), '-o', str(obj)],
                              cwd=ROOT, capture_output=True, text=True)
    row = {'name': 'uncaptured-free', 'compile_exit': compiled.returncode,
           'compile_stderr': compiled.stderr}
    if compiled.returncode == 0:
        linked = subprocess.run(flags + ['tests/unit/player_vertex_weld.cpp', str(obj),
                                'build/libsadx-native.a', '-o', str(executable)],
                                cwd=ROOT, capture_output=True, text=True)
        row.update(link_exit=linked.returncode, link_stderr=linked.stderr)
        if linked.returncode == 0:
            run = subprocess.run([str(executable), str(Path(args.assets) /
                                 'characters/sonic-selection.actions')], cwd=ROOT,
                                 capture_output=True, text=True)
            row.update(run_exit=run.returncode, stdout=run.stdout, stderr=run.stderr)
    row['caught'] = row.get('run_exit') == -6
    results.append(row)
    print(row['name'], 'caught' if row['caught'] else 'NOT CAUGHT', flush=True)
    output = ROOT / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps({'all_caught': all(row['caught'] for row in results),
                                 'faults': results}, indent=2) + '\n')
    return 0 if all(row['caught'] for row in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
