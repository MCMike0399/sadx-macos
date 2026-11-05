"""Compile isolated dynamic-model dispatcher faults; no production source mutations."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/draw-path-2/alternate/faults'
FAULTS = [
    ('missing-material-transition', 'auto previous_material = kMaterialMask;',
     'auto previous_material = std::uint32_t{0};'),
    ('live-material-callback', '            callback(context);',
     '            snapshot_material_callback(context)(context);'),
    ('wrong-color-table', 'RenderFlag::IgnoreVertexColors', 'RenderFlag::ConstantMaterial'),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe')
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'src/engine/dynamic_model.cpp').read_text()
    results = []
    for name, old, new in FAULTS:
        assert source.count(old) == 1, (name, source.count(old))
        cpp, dylib, report = [OUT / (name + suffix) for suffix in ('.cpp', '.dylib', '.json')]
        cpp.write_text(source.replace(old, new))
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64',
                   '-mmacosx-version-min=14.0', '-Isrc', '-Itests/probes', '-dynamiclib',
                   'tests/probes/native_dynamic_model_probe.cpp', str(cpp),
                   'build/libsadx-native.a', '-o', str(dylib)]
        compile = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if compile.returncode:
            raise RuntimeError(compile.stderr)
        judge = [sys.executable, 'tools/oracles/verify_original_dynamic_model.py', args.exe,
                 str(dylib), '--output', str(report)]
        run = subprocess.run(judge, cwd=ROOT, capture_output=True, text=True)
        row = json.loads(report.read_text())
        results.append(dict(name=name, compile_command=command, compile_exit=compile.returncode,
                            judge_command=judge, judge_exit=run.returncode,
                            value_failures=row['value_failures']))
    (OUT / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps(results))
    return 0 if all(row['value_failures'] > 0 and row['judge_exit'] == 1 for row in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
