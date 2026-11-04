"""Compile isolated display faults; never rewrite shipping source or old mutants."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/gamma-display-draw-faults'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe')
    parser.add_argument('--compile-only', action='store_true')
    args = parser.parse_args()
    OUT.mkdir(exist_ok=True)
    source = (ROOT / 'src/player/gamma_display.cpp').read_text()
    results = []
    for fault in json.loads(Path(__file__).with_name('faults.json').read_text()):
        assert fault['old'] in source, fault['name']
        cpp = OUT / (fault['name'] + '.cpp')
        cpp.write_text(source.replace(fault['old'], fault['new'], 1))
        dylib = cpp.with_suffix('.dylib')
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64',
                   '-mmacosx-version-min=14.0', '-Isrc', '-dynamiclib',
                   'tests/probes/native_gamma_display_probe.cpp', str(cpp),
                   'build/libsadx-native.a', '-o', str(dylib)]
        build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        cpp.with_suffix('.build.log').write_text(build.stdout + build.stderr)
        row = {'name': fault['name'], 'build_command': command, 'build_exit': build.returncode}
        results.append(row)
        if build.returncode:
            break
        if not args.compile_only:
            command = [sys.executable, 'tools/oracles/verify_original_gamma_display.py',
                       args.exe, str(dylib), '--body', '--output', str(cpp.with_suffix('.json'))]
            judged = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
            cpp.with_suffix('.judge.log').write_text(judged.stdout + judged.stderr)
            row.update(judge_command=command, judge_exit=judged.returncode)
            if judged.returncode not in (0, 1):
                break
            report = json.loads(cpp.with_suffix('.json').read_text())
            row['caught'] = bool(report['value_failures'] or report['body']['value_failures'])
            if not row['caught']:
                break
    (OUT / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    assert len(results) == 5 and all(r['build_exit'] == 0 for r in results)
    if not args.compile_only:
        assert all(r.get('caught') for r in results)


if __name__ == '__main__':
    main()
