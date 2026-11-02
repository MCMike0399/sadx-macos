#!/usr/bin/env python3
"""Compile bounded product faults and run the unchanged new weld comparison."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('--output', default='build/gamma-weld/mutations.json')
    args = parser.parse_args()
    source = (ROOT / 'src/player/player_vertex_weld.cpp').read_text()
    faults = [
        ('destination-depth', 'join.destination_depth = 0;', 'join.destination_depth = 1;'),
        ('normal-owner', 'model_vector(source, true,', 'model_vector(source, false,'),
        ('failed-backup-publication', 'join.original_vertices = allocation.work;',
         'join.original_vertices = {};'),
        ('skip-free', 'arena.release(allocation)',
         'TaskWorkResult{TaskWorkFailure::None, {}}'),
        ('drop-restored-z', '(*words)[destination] = word;',
         'if (component != 2) (*words)[destination] = word;'),
        ('no-restoration', '(*words)[destination] = word;', '(void)word;'),
        ('node-order', '->child', '->sibling'),
    ]
    results = []
    directory = ROOT / 'build/gamma-weld/mutants'
    directory.mkdir(parents=True, exist_ok=True)
    for name, before, after in faults:
        assert before in source, name
        if name == 'node-order':
            changed = source.replace('->child', '->temporary').replace('->sibling', '->child')
            changed = changed.replace('->temporary', '->sibling')
        else:
            changed = source.replace(before, after)
        product = directory / f'{name}.cpp'
        probe = directory / f'{name}.dylib'
        report = directory / f'{name}.json'
        product.write_text(changed)
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64',
                   '-mmacosx-version-min=14.0', '-dynamiclib', '-Isrc', '-Itests/probes',
                   'tests/probes/native_player_vertex_weld_probe.cpp', str(product),
                   'src/engine/character_pack.cpp', 'src/engine/character_geometry.cpp',
                   'src/engine/task_work.cpp', '-o', str(probe)]
        compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        result = {'name': name, 'compile_exit': compiled.returncode,
                  'compile_stderr': compiled.stderr, 'command': command}
        if compiled.returncode == 0:
            check = subprocess.run([sys.executable, 'tools/oracles/verify_original_player_vertex_weld.py',
                                    args.exe, str(probe), '--cases', '1024', '--output', str(report)],
                                   cwd=ROOT, capture_output=True, text=True)
            result.update(check_exit=check.returncode, stdout=check.stdout, stderr=check.stderr)
            if report.exists():
                data = json.loads(report.read_text())
                result['mismatch_count'] = data['mismatch_count']
            result['caught'] = check.returncode == 1 and result.get('mismatch_count', 0) > 0
        else:
            result['caught'] = False
        results.append(result)
        print(name, 'caught' if result['caught'] else 'NOT CAUGHT', flush=True)
    output = ROOT / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps({'all_caught': all(row['caught'] for row in results),
                                 'faults': results}, indent=2) + '\n')
    return 0 if all(row['caught'] for row in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
