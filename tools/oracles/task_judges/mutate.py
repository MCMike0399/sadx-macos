#!/usr/bin/env python3
"""Compile faults in immutable Git task.cpp copies; never edit maintained source."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

BASE = '99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3'
FLAGS = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
         '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-Itests/fixtures']
FAULTS = [
    ("early-walk-reclamation", "if (--scheduler_.walk_depth_ == 0)",
     "if (--scheduler_.walk_depth_ <= 1)", "deferred_walk"),
    ('unavailable-default-is-null', 'else return {TaskFailure::default_field30_unobserved, task, {}, 0x40b918};',
     'else task->field30.reset();', 'availability'),
    ('slot-zero-is-null', 'if(task->source_slot_){const auto current=source_slots_',
     'if(task->source_slot_ && *task->source_slot_ != 0){const auto current=source_slots_', 'saved_next'),
    ('stale-saved-generation', 'if(task->source_slot_){const auto current=source_slots_',
     'if(false && task->source_slot_){const auto current=source_slots_', 'saved_next'),
    ('missing-function-context', 'if(!state.children)context_.current_function=callback;',
     '(void)callback;', 'contexts'),
    ('missing-context-clear', 'context_.field30.reset();if(!state.children)',
     'if(!state.children)', 'contexts'),
    ('skip-unlink-state', 'task->linked_ = false;\n        task->destruction_stage_ = Stage::collision;',
     'task->destruction_stage_ = Stage::collision;', 'read_failures'),
    ('skip-native-aux-release', 'if (field.stage == Stage::auxiliary)\n            task->auxiliary_native_work.reset();',
     'if (false) task->auxiliary_native_work.reset();', 'read_failures'),
    ('discard-callback-failure', 'catch(const SourceTaskFailure& failed){return failed.result;}',
     'catch(const SourceTaskFailure& failed){(void)failed;return {};}', 'callback_failures'),
    ('conflicting-context-owner', 'TaskCallbackContextFailure::conflicting_owner};',
     'TaskCallbackContextFailure::none};', 'contexts'),
    ('canonical-argument-not-retained', 'const TaskHandle task = requested_task;',
     'const TaskHandle& task = requested_task;', 'canonical'),
    ('historical-skip-destructor', 'if(task->destructor)return {{},task,task->destructor,TaskLifetimeCallee::destructor,false};',
     'if(false && task->destructor)return {{},task,task->destructor,TaskLifetimeCallee::destructor,false};', 'historical'),
    ('historical-retain-exec', 'task->exec=task->display=task->destructor=nullptr;',
     'task->display=task->destructor=nullptr;', 'historical'),
    ('historical-skip-auxiliary-free', 'if (work) {\n            const auto released = arena_',
     'if (work && field.stage != Stage::auxiliary) {\n            const auto released = arena_', 'historical'),
]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--python', type=Path, required=True)
    p.add_argument('--out', type=Path, default=Path('build/task-judges/mutants'))
    p.add_argument("--only", default="")
    args = p.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    source = subprocess.check_output(['git', 'show', BASE + ':src/engine/task.cpp']).decode()
    assert Path('src/engine/task.cpp').read_text() == source
    rows = []
    for name, old, new, scenario in FAULTS:
        if args.only and name != args.only: continue
        assert source.count(old) >= 1, name
        folder = args.out / name
        folder.mkdir(exist_ok=True)
        copied = folder / 'task.cpp'
        copied.write_text(source.replace(old, new, 1))
        if scenario == 'historical':
            test = 'tests/probes/native_task_lifecycle_probe.cpp'
            binary = folder / 'probe.dylib'
            command = FLAGS + ['-dynamiclib', test, str(copied), 'src/engine/task_work.cpp', '-o', str(binary)]
            run = [str(args.python), 'tools/oracles/verify_original_task_lifecycle.py', str(args.exe), str(binary),
                   '--output', str(folder / 'report.json')]
        else:
            test = 'tests/unit/task_callback_lifetime.cpp' if scenario == 'canonical' else 'tests/unit/task_scheduler_contract.cpp'
            binary = folder / 'test'
            command = FLAGS + [test, str(copied), 'src/engine/task_work.cpp', '-o', str(binary)]
            run = [str(binary)] + ([] if scenario == 'canonical' else [scenario])
        built = subprocess.run(command, capture_output=True, text=True)
        (folder / 'compile.log').write_text(built.stdout + built.stderr)
        row = dict(name=name, scenario=scenario, old=old, new=new, compile_command=command,
                   compiled=built.returncode == 0, compile_exit=built.returncode,
                   copied_sha256=hashlib.sha256(copied.read_bytes()).hexdigest(), caught=False)
        if built.returncode == 0:
            try:
                result = subprocess.run(run, capture_output=True, text=True, timeout=180,
                                        env=os.environ | {'PYTHONPATH': 'tools/lib:tools/oracles'})
                (folder / 'run.log').write_text(result.stdout + result.stderr)
                row.update(run_command=run, run_exit=result.returncode, caught=result.returncode != 0,
                           stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),
                           stderr=result.stderr)
                if scenario == 'historical' and (folder / 'report.json').exists():
                    report = json.loads((folder / 'report.json').read_text())
                    row['sequences'] = report['sequences']
                    row['inactive_destroy_boundary'] = report['inactive_destroy_boundary']
            except subprocess.TimeoutExpired:
                row['timeout'] = True
                row['caught'] = False  # a timeout is not a discriminating mismatch
        rows.append(row)
        (args.out / 'receipts.json').write_text(json.dumps(rows, indent=2) + '\n')
        print(name, 'compiled', row['compiled'], 'caught', row['caught'], flush=True)
    assert Path('src/engine/task.cpp').read_text() == source
    return int(not all(row['compiled'] and row['caught'] for row in rows))


if __name__ == '__main__':
    raise SystemExit(main())
