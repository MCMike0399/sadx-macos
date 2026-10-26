"""Private build, original instruction judge, meaningful mutants and independent holdout.

Every output is beneath the selected worktree build directory. Compilation is sequential;
no project archive/app/gate, Ghidra worker or shared build output is used.
"""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MUTANTS = {
    'remaining-equality': ('bytes < buffer.remaining_bytes', 'bytes <= buffer.remaining_bytes'),
    'header-mask': ('& kStripLengthMask', '& (kStripLengthMask | 0x4000U)'),
    'signed-index': ('const auto index = mesh.indices.at(index_cursor);',
                     'const auto index = static_cast<std::uint16_t>(mesh.indices.at(index_cursor));'),
    'uv-divisor': ('* kUvScale', '* (1.0f / 256.0f)'),
    'initial-degenerate': ('vertex(context, output, input, points, normals, cursor, uv_cursor, color);',
                           '/* initial duplicate suppressed */'),
    'trailing-uv': ('cursor - 1, uv_cursor - 2, color', 'cursor - 1, uv_cursor, color'),
    'argument-store-order': (
        'argument(context, buffer, row, ArgumentField::PrimitiveCount, vertices - 2);\n    argument(context, buffer, row, ArgumentField::StartVertex, offset / stride);',
        'argument(context, buffer, row, ArgumentField::StartVertex, offset / stride);\n    argument(context, buffer, row, ArgumentField::PrimitiveCount, vertices - 2);'),
}

def run(command, log):
    result = subprocess.run([str(x) for x in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_text(result.stdout)
    return result.returncode

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--python', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build/normal-uv-strip')
    args = parser.parse_args()
    build = args.build_dir.resolve()
    if not build.is_relative_to(ROOT):
        parser.error('private build directory must be inside this worktree')
    build.mkdir(parents=True, exist_ok=True)
    archive = build / 'softfloat/softfloat.a'
    if not archive.exists():
        code = run(['sh', 'third_party/softfloat/build.sh', archive.parent], build / 'softfloat.log')
        if code:
            raise RuntimeError('private SoftFloat build failed')
    flags = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
             '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-Itests/probes']
    engine = ROOT / 'src/engine/normal_uv_strip.cpp'
    probe = ROOT / 'tests/probes/native_normal_uv_strip_probe.cpp'
    shared = ['src/cpu/x87_math.cpp', probe, archive]
    library = build / 'probe.dylib'
    if run(flags + ['-dynamiclib', engine] + shared + ['-o', library], build / 'compile-probe.log'):
        raise RuntimeError('probe compile failed')
    unit = build / 'normal-uv-strip-test'
    if run(flags + ['tests/unit/normal_uv_strip.cpp', engine] + shared + ['-o', unit], build / 'compile-unit.log'):
        raise RuntimeError('unit compile failed')
    unit_code = run([unit], build / 'unit.log')
    if unit_code:
        raise RuntimeError('native unit failed')
    oracle = ROOT / 'tools/oracles/verify_original_normal_uv_strip.py'
    def judge(lib, name, seed, count):
        report = build / f'{name}.json'
        code = run([args.python, oracle, args.exe, lib, '--output', report,
                    '--seed', seed, '--cases', count], build / f'{name}.log')
        if not report.exists():
            raise RuntimeError(f'{name} oracle failed before reporting')
        return code, json.loads(report.read_text())
    dev_code, dev = judge(library, 'dev', 783590, 400)
    if dev_code not in (0, 2) or not dev['values_match']:
        raise RuntimeError('development value judge failed')
    source = engine.read_text()
    mutant_results = {}
    for name, (before, after) in MUTANTS.items():
        expected_occurrences = 2 if name == 'header-mask' else 1
        if source.count(before) != expected_occurrences:
            raise RuntimeError(f'mutant {name} no longer anchors uniquely')
        mutated = build / f'{name}.cpp'
        mutated.write_text(source.replace(before, after))
        mutant_library = build / f'{name}.dylib'
        if run(flags + ['-dynamiclib', mutated] + shared + ['-o', mutant_library], build / f'{name}-compile.log'):
            raise RuntimeError(f'mutant {name} did not compile')
        code, report = judge(mutant_library, name, 783590, 80)
        killed = code == 1 and report['value_mismatches'] > 0
        mutant_results[name] = {'killed': killed, 'value_mismatches': report['value_mismatches']}
        if not killed:
            raise RuntimeError(f'mutant {name} survived')
    holdout_code, holdout = judge(library, 'holdout', 26540917, 2048)
    summary = {
        'base_commit': '04c40ae0b0840b9982cda95452057c1d678eeec1',
        'source_sha256': dev['source_sha256'], 'unit_exit_status': unit_code,
        'unit_output': (build / 'unit.log').read_text().strip(),
        'dev': {key: dev[key] for key in ('passed', 'values_match', 'cases', 'value_mismatches',
                 'flag_mismatches', 'ordered_receipts', 'vertex_word_stores', 'reference_state_api_vertex_sha256',
                 'native_state_api_vertex_sha256', 'input_sha256', 'flag_patterns')},
        'holdout': {key: holdout[key] for key in ('passed', 'values_match', 'cases', 'value_mismatches',
                     'flag_mismatches', 'ordered_receipts', 'vertex_word_stores', 'reference_state_api_vertex_sha256',
                 'native_state_api_vertex_sha256', 'input_sha256', 'flag_patterns')},
        'dev_exit_status': dev_code, 'holdout_exit_status': holdout_code, 'mutants': mutant_results,
        'source_ranges': dev['source_ranges'],
        'engine_sha256': hashlib.sha256(engine.read_bytes()).hexdigest(),
        'limits': dev['limits'], 'full_gate_run': False,
    }
    (build / 'proof.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary, indent=2))
    return 1 if holdout_code not in (0, 2) else 0

if __name__ == '__main__':
    sys.exit(main())
