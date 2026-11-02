"""Compiled copy-order and canonical native-owner faults; each must be caught."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--output-dir', default='build/gamma-laser-targets/mutants')
    args = parser.parse_args()
    destination = ROOT / args.output_dir
    destination.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'src/player/gamma_input_buffer.cpp').read_text()
    faults = {
        'wrong_player': source.replace('store_angle(player, angle)', 'store_angle(0, angle)'),
        'missing_stroke': source.replace('access.store_stroke_bits(player, stroke);', '(void)stroke;'),
        'preloaded_stroke': source.replace('    access.store_angle(player, angle);\n'
            '    const auto stroke = access.buffered_stroke_bits();',
            '    const auto stroke = access.buffered_stroke_bits();\n'
            '    access.store_angle(player, angle);'),
        'zero_retains_stroke': source.replace('access.store_stroke_bits(player, stroke);',
                                             'if (stroke) access.store_stroke_bits(player, stroke);'),
    }
    results = []
    for name, changed in faults.items():
        assert changed != source
        path = destination / (name + '.cpp')
        path.write_text(changed)
        probe = path.with_suffix('.dylib')
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-shared', '-fPIC', '-Isrc',
                   'tests/probes/native_gamma_input_buffer_probe.cpp', str(path), '-o', str(probe)]
        build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        report_path = destination / (name + '.json')
        run = subprocess.run([sys.executable, 'tools/oracles/verify_original_gamma_input_buffer.py',
            args.exe, str(probe), '--cases', '64', '--output', str(report_path)],
            cwd=ROOT, capture_output=True, text=True) if build.returncode == 0 else None
        report = json.loads(report_path.read_text()) if report_path.exists() else {}
        caught = build.returncode == 0 and run.returncode == 1 and report.get('mismatch_count', 0) > 0
        results.append({'name': name, 'compiled': build.returncode == 0, 'caught': caught,
                        'mismatch_case_ids': [r['case_id'] for r in report.get('mismatches', [])],
                        'build_stderr': build.stderr})
    binding = (ROOT / 'src/runtime/native_gamma_binding.cpp').read_text()
    binding_faults = {
        'wrong_extra_owner': binding.replace('} access(context, work);',
            '} access(context, context.players.gamma.current_extra);\n    (void)work;'),
        'wrong_slot': binding.replace('gamma_input_buffer::restore(access, index);',
            'gamma_input_buffer::restore(access, 0);\n    (void)index;'),
        'wrong_angle_field': binding.replace('kLatestAngle = 556', 'kLatestAngle = 548'),
    }
    for name, changed in binding_faults.items():
        assert changed != binding
        path = destination / (name + '.cpp')
        path.write_text(changed)
        obj = path.with_suffix('.o')
        compile_run = subprocess.run(['xcrun', 'clang++', '-std=c++17', '-O2', '-ffp-contract=off',
            '-fno-fast-math', '-Isrc', '-c', str(path), '-o', str(obj)], cwd=ROOT,
            capture_output=True, text=True)
        executable = path.with_suffix('.test')
        link = subprocess.run(['xcrun', 'clang++', '-std=c++17', '-O2', '-fobjc-arc', '-Isrc',
            'tests/unit/native_gamma_input_buffer.mm', str(obj), 'build/libsadx-native.a',
            '-framework', 'Foundation', '-o', str(executable)], cwd=ROOT,
            capture_output=True, text=True) if compile_run.returncode == 0 else None
        run = subprocess.run([str(executable), 'game/imported'], cwd=ROOT,
            capture_output=True, text=True) if link and link.returncode == 0 else None
        compiled = compile_run.returncode == 0 and link is not None and link.returncode == 0
        caught = compiled and run.returncode != 0 and 'Assertion failed' in run.stderr
        results.append({'name': name, 'compiled': compiled, 'caught': caught,
                        'exit_status': run.returncode if run else None,
                        'stderr': run.stderr if run else compile_run.stderr + (link.stderr if link else '')})
    report = {'mutants': results, 'passed': all(r['caught'] for r in results)}
    (destination / 'summary.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({r['name']: r['caught'] for r in results}))
    return int(not report['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
