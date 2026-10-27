#!/usr/bin/env python3
"""Build isolated source mutations of Gamma cursors and run the original dev judge."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--directory', default='build/gamma-weld-cursors/mutations')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    original = (root / 'src/player/gamma_cursors.cpp').read_text()
    specs = json.loads((root / 'reference/production-integration-20261026/gamma-weld-cursors/mutants.json').read_text())
    directory = root / args.directory
    directory.mkdir(parents=True, exist_ok=True)
    results = []
    for number, (name, before, after) in enumerate(specs):
        if original.count(before) != 1:
            raise RuntimeError(f'mutant anchor is not unique: {name}')
        source = directory / f'cursor-{number}.cpp'
        library = directory / f'probe-{number}.dylib'
        report = directory / f'report-{number}.json'
        log = directory / f'build-{number}.log'
        source.write_text(original.replace(before, after, 1))
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-dynamiclib', '-Isrc',
                   'tests/probes/native_gamma_cursors_probe.cpp', str(source),
                   'src/engine/task_work.cpp', '-o', str(library)]
        with log.open('w') as stream:
            compiled = subprocess.run(command, cwd=root, stdout=stream, stderr=subprocess.STDOUT)
        row = {'name': name, 'compile_exit_status': compiled.returncode, 'caught': False}
        if compiled.returncode == 0:
            judged = subprocess.run([sys.executable, 'tools/oracles/verify_original_gamma_cursors.py',
                                     '--exe', args.exe, '--probe', str(library),
                                     '--output', str(report)], cwd=root, capture_output=True, text=True)
            row['oracle_exit_status'] = judged.returncode
            if report.exists():
                verdict = json.loads(report.read_text())
                row['mismatch_count'] = verdict['mismatch_count']
                row['caught'] = judged.returncode == 1 and verdict['mismatch_count'] > 0
            else:
                row['error'] = judged.stderr[-1000:]
        results.append(row)
        print(json.dumps(row), flush=True)
    result = {'source_sha256': hashlib.sha256(original.encode()).hexdigest(),
              'mutants': results, 'caught': sum(row['caught'] for row in results),
              'total': len(results), 'source_modified': False,
              'limits': ['Only development cases are used; holdout remains untouched.',
                         'A failed compile or crashed harness is not counted as caught.']}
    Path(args.output).write_text(json.dumps(result, indent=2) + '\n')
    return int(result['caught'] != result['total'])


if __name__ == '__main__':
    raise SystemExit(main())
