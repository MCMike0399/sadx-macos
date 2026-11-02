"""Bounded source and binding faults; never edits shipping source or the actual probe."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
MUTANTS = [
    ('miles-speed', 'slot == 1 ? 0.5f : 0.25f', 'slot == 1 ? 0.25f : 0.25f'),
    ('miles-flag', 'kMilesCounterFlag = 0x20000000', 'kMilesCounterFlag = 0x10000000'),
    ('captured-primary', 'calls.action_head(primary)', 'calls.action_head(calls.primary(task))'),
    ('terminal-loop', 'calls.action(slot, work.motion_counts[slot]).identity == 0',
     'calls.action(slot, work.motion_counts[slot]).identity != 0'),
    ('consume-selection', 'work.selected_motion = 0;', 'work.selected_motion = 2;'),
    ('tail-traversal', 'while (auto *next = calls.next_action(tail))',
     'if (auto *next = calls.next_action(tail))'),
    ('selection-gate', 'selected == slot', 'selected != slot'),
    ('retained-tails', 'constexpr std::int8_t kEventInput = 12;',
     'state.tails = {}; constexpr std::int8_t kEventInput = 12;'),
    ('non-miles-speed', 'slot == 1 ? 0.5f : 0.25f', 'slot == 1 ? 0.5f : 0.125f'),
    ('texture-identity', 'const auto action = calls.action(slot, 0);\n            auto &textures = calls.textures(slot);',
     'const auto action = calls.action(slot, 0);\n            auto &textures = calls.textures((slot + 1) % 6);'),
    ('four-node-cap', 'while (auto *next = calls.next_action(tail))\n            tail = next;',
     'unsigned traversed = 1; while (auto *next = calls.next_action(tail)) {\n'
     '            if (++traversed > 4) break; tail = next; }'),
    ('registered-counter', 'return *registered_primaries[1]->source_counter_at_c;',
     'return *primaries[1]->source_counter_at_c;', 'probe'),
    ('registered-event-list', 'return heads[n];',
     'return n == 1 && in[39] ? &registered_nodes[0] : heads[n];', 'probe'),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe'); parser.add_argument('--output-dir', default='build/preview-actions/mutants')
    args = parser.parse_args()
    out = ROOT / args.output_dir
    out.mkdir(parents=True, exist_ok=True)
    original = (ROOT / 'src/game/selection_preview_actions.cpp').read_text()
    original_probe = (ROOT / 'tests/probes/native_selection_preview_actions_probe.cpp').read_text()
    results = []
    for mutation in MUTANTS:
        name, old, new = mutation[:3]
        binding_fault = len(mutation) == 4 and mutation[3] == 'probe'
        text = original_probe if binding_fault else original
        assert text.count(old) == 1, name
        source, library, report = out / (name+'.cpp'), out / (name+'.dylib'), out / (name+'.json')
        source.write_text(text.replace(old, new))
        core = 'src/game/selection_preview_actions.cpp' if binding_fault else str(source)
        probe = str(source) if binding_fault else 'tests/probes/native_selection_preview_actions_probe.cpp'
        command = ['clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-dynamiclib', core,
                   'src/engine/task.cpp', 'src/engine/task_work.cpp',
                   probe, '-o', str(library)]
        subprocess.run(command, cwd=ROOT, check=True, capture_output=True)
        result = subprocess.run([sys.executable, 'tools/oracles/verify_original_selection_preview_actions.py',
                                 args.exe, str(library), '--output', str(report), '--cases', '100'],
                                cwd=ROOT, capture_output=True, text=True)
        assert report.exists(), result.stderr
        body = json.loads(report.read_text())
        row = {'compiled_source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
               'mutated_target': 'probe binding' if binding_fault else 'core',
               'name': name, 'caught': not body['passed'], 'exit_status': result.returncode,
               'failed_cases': len(body['failures']), 'first_case': body['failures'][0] if body['failures'] else None}
        results.append(row)
        print(name, 'caught' if row['caught'] else 'SURVIVED', flush=True)
    (out / 'results.json').write_text(json.dumps(results, indent=2)+'\n')
    return not all(row['caught'] for row in results)

if __name__ == '__main__':
    raise SystemExit(main())
