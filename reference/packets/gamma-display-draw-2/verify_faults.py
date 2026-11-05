"""Compile new isolated faults and require original-execution mismatches."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/draw-path-2/faults'


def main():
    p = argparse.ArgumentParser()
    p.add_argument('exe')
    p.add_argument('--cases', type=int, default=1024)
    args = p.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'src/engine/animated_model.cpp').read_text()
    faults = json.loads(Path(__file__).with_name('faults.json').read_text())
    results = []
    for fault in faults:
        assert source.count(fault['old']) == 1, fault['name']
        cpp = OUT / (fault['name'] + '.cpp')
        cpp.write_text(source.replace(fault['old'], fault['new'], 1))
        dylib = cpp.with_suffix('.dylib')
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64',
                   '-mmacosx-version-min=14.0', '-Isrc', '-dynamiclib',
                   'tests/probes/native_animated_model_probe.cpp', str(cpp),
                   'src/engine/object_draw.cpp', 'build/libsadx-native.a', '-o', str(dylib)]
        built = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        cpp.with_suffix('.build.log').write_text(built.stdout + built.stderr)
        row = {'name': fault['name'], 'build_command': command, 'build_exit': built.returncode}
        results.append(row)
        if built.returncode:
            break
        command = [sys.executable, 'tools/oracles/verify_original_animated_model.py',
                   args.exe, str(dylib), '--cases', str(args.cases), '--output',
                   str(cpp.with_suffix('.json'))]
        judged = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        cpp.with_suffix('.judge.log').write_text(judged.stdout + judged.stderr)
        row.update(judge_command=command, judge_exit=judged.returncode)
        if judged.returncode not in (0, 1) or not cpp.with_suffix('.json').exists():
            break
        report = json.loads(cpp.with_suffix('.json').read_text())
        row.update(caught=report['value_failures'] > 0, cases=report['cases'],
                   value_failures=report['value_failures'])
        if not row['caught']:
            break
    target = OUT / 'results.json'
    target.write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps(results, indent=2))
    assert len(results) == len(faults) and all(r.get('caught') for r in results)


if __name__ == '__main__':
    main()
