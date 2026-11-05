"""Compile and exercise one isolated wrong Gamma row-selection fault."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/draw-path-2/gamma-callback-faults'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=Path)
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'src/player/gamma_display_action.cpp').read_text()
    old = 'object_row(context, table, 12)'
    assert source.count(old) == 1
    changed = OUT / 'wrong-row-selection.cpp'
    changed.write_text(source.replace(old, 'object_row(context, table, 11)'))
    library = changed.with_suffix('.dylib')
    build = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
             '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64',
             '-mmacosx-version-min=14.0', '-Isrc', '-dynamiclib',
             'tests/probes/native_gamma_object_callback_probe.cpp', str(changed),
             'build/libsadx-native.a', '-o', str(library)]
    build_result = subprocess.run(build, cwd=ROOT, capture_output=True, text=True)
    judge = [str(ROOT / 'reference/python/bin/python'),
             'tools/oracles/verify_original_gamma_object_callback.py', str(args.exe.resolve()),
             str(library), '--output', str(changed.with_suffix('.json'))]
    result = dict(name='wrong-gamma-row-selection', build_command=build,
                  build_exit=build_result.returncode, build_stderr=build_result.stderr)
    if build_result.returncode == 0:
        run = subprocess.run(judge, cwd=ROOT, capture_output=True, text=True)
        result.update(judge_command=judge, judge_exit=run.returncode)
        report = json.loads(changed.with_suffix('.json').read_text())
        result.update(cases=report['cases'], value_failures=report['value_failures'],
                      fp_partials=report['fp_partials'],
                      caught=run.returncode == 1 and report['value_failures'] > 0)
    else:
        result['caught'] = False
    (OUT / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    return 0 if result['caught'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
