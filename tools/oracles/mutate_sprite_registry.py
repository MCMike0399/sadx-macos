"""Seven constructor mutants; new files only, no shared judge edits."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--exe', required=True)
    a = p.parse_args()
    original = (ROOT / 'src/engine/sprite_registry.cpp').read_text()
    capture = '    const auto count = state.count;\n    const auto old_head = state.head;\n'
    mutants = {
        'null_guard': original.replace('if (!definition)', 'if (false)'),
        'allocation_size': original.replace('extra_bytes + 28', 'extra_bytes + 24'),
        'preallocation_capture': original.replace(capture, '').replace(
            '    const auto allocation =', capture + '    const auto allocation ='),
        'count_wrap': original.replace('count + 1', 'count == 4294967295u ? count : count + 1'),
        'publication_order': original.replace('    state.head = node;\n    state.count = count + 1;', '    state.count = count + 1;\n    state.head = node;'),
        'negative_extra_skipped': original.replace('extra_bytes != 0', 'static_cast<std::int32_t>(extra_bytes) > 0'),
        'lost_definition': original.replace('node->definition = definition;', 'node->definition = nullptr;'),
    }
    destination = ROOT / 'build/sprite-registry-mutants'
    destination.mkdir(parents=True, exist_ok=True)
    results = []
    for name, text in mutants.items():
        assert text != original
        source = destination / (name + '.cpp')
        probe = destination / (name + '.dylib')
        source.write_text(text)
        # Mutants can introduce unused captures; warnings are not their judge.
        subprocess.run(['c++', '-std=c++17', '-O2', '-shared', '-fPIC', '-Isrc',
                        'tests/probes/native_sprite_registry_probe.cpp', str(source),
                        'src/engine/task_work.cpp', '-o', str(probe)], cwd=ROOT, check=True)
        prefix = destination / (name + '-prefix.dylib')
        subprocess.run([sys.executable, 'tools/oracles/build_sprite_registry_probe.py',
                        '--source', str(source), '--output', str(prefix)], cwd=ROOT, check=True)
        report = destination / (name + '.json')
        run = subprocess.run([sys.executable, 'tools/oracles/verify_original_sprite_registry.py',
                              '--exe', a.exe, '--probe', str(probe), '--output', str(report),
                              '--prefix-probe', str(prefix), '--count', '64'], cwd=ROOT, capture_output=True, text=True)
        result = json.loads(report.read_text()) if report.exists() else {}
        caught = run.returncode == 1 and result.get('mismatch_count', 0) > 0
        results.append({'name': name, 'exit_status': run.returncode, 'caught': caught,
                        'mismatch_count': result.get('mismatch_count'), 'stderr': run.stderr})
        print(name, 'caught' if caught else 'SURVIVED/ERROR', flush=True)
    output = {'mutants': results, 'passed': all(r['caught'] for r in results)}
    (destination / 'summary.json').write_text(json.dumps(output, indent=2) + '\n')
    return not output['passed']


if __name__ == '__main__':
    sys.exit(main())
