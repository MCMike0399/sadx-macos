"""Seven compiled setup faults, including distinct non-null definition forwarding."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--output-dir', default='build/gamma-setup/mutants')
    args = parser.parse_args()
    destination = ROOT / args.output_dir
    destination.mkdir(parents=True, exist_ok=True)
    original = (ROOT / 'src/player/gamma_setup_resource.cpp').read_text()
    assignment = 'state.sprite = sprite_registry::register_sprite(registry, arena, definition, 0);'
    faults = {
        'extra_allocation': (assignment, assignment.replace('definition, 0', 'definition, 1')),
        'null_retains_previous_result': ('const auto *definition = state.definition;',
                                       'const auto *definition = state.definition;\n    if (!definition) return;'),
        'allocation_failure_retains_result': (assignment,
            'const auto created = sprite_registry::register_sprite(registry, arena, definition, 0);\n'
            '    if (created) state.sprite = created;'),
        'clears_registry': ('const auto *definition = state.definition;',
                           'const auto *definition = state.definition;\n    registry = {};'),
        'premature_result_publication': (assignment, 'state.sprite = {};\n    ' + assignment),
        'releases_previous_allocation': ('const auto *definition = state.definition;',
            'const auto *definition = state.definition;\n'
            '    if (state.sprite && state.sprite->allocation) arena.release(state.sprite->allocation);'),
        'wrong_nonnull_definition': ('const auto *definition = state.definition;',
            'const auto *definition = state.definition && registry.head && registry.head->definition\n'
            '        ? registry.head->definition : state.definition;'),
    }
    results = []
    for name, (before, after) in faults.items():
        assert original.count(before) == 1
        source = destination / (name + '.cpp')
        source.write_text(original.replace(before, after))
        probe = destination / (name + '.dylib')
        build = subprocess.run(['c++', '-std=c++17', '-O2', '-shared', '-fPIC', '-Isrc',
            'tests/probes/native_gamma_setup_resource_probe.cpp', str(source),
            'src/engine/sprite_registry.cpp', 'src/engine/task_work.cpp', '-o', str(probe)], cwd=ROOT)
        report = destination / (name + '.json')
        run = subprocess.run([sys.executable, 'tools/oracles/verify_original_gamma_setup_resource.py',
            '--exe', args.exe, '--probe', str(probe), '--output', str(report), '--count', '64'],
            cwd=ROOT, capture_output=True, text=True) if build.returncode == 0 else None
        result = json.loads(report.read_text()) if report.exists() else {}
        failures = result.get('mismatches', [])
        caught = build.returncode == 0 and run.returncode == 1 and bool(failures)
        results.append({'name': name, 'compiled': build.returncode == 0,
                        'exit_status': run.returncode if run else None, 'caught': caught,
                        'mismatch_case_ids': [case['case_id'] for case in failures],
                        'report_sha256': hashlib.sha256(report.read_bytes()).hexdigest() if report.exists() else None,
                        'stderr': run.stderr if run else 'build failed'})
        print(name, 'caught' if caught else 'SURVIVED/ERROR', flush=True)
    summary = {'mutants': results, 'passed': all(result['caught'] for result in results)}
    (destination / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    return int(not summary['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
