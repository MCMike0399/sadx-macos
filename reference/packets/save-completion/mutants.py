"""Bounded semantic faults; compile each valid fault and run the unchanged judge/test."""
import json
from pathlib import Path
import subprocess

build = Path('build/save-completion-packet')
archive = '/Users/burbujamc/Developer/sadx-wt-menu-progress-predicates/build/menu-progress-current/stable-root-core.a'
python = '/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python'
exe = '/Users/burbujamc/Developer/sadx-macos/reference/updated-pc-reference/game/sonic.exe'
flags = ['clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fno-fast-math', '-Isrc']
source = Path('src/game/save_completion.cpp').read_text()
mutants = [
 ('unsigned-save-ok', 'static_cast<std::int8_t>(state.save_ok)', 'state.save_ok', 'oracle'),
 ('short-delay', 'kSaveCompletionDelay = 5', 'kSaveCompletionDelay = 4', 'oracle'),
 ('defer-fallthrough', 'set_control_stage(work, ControlStage::CompleteNewSave);', 'set_control_stage(work, ControlStage::CompleteNewSave);\n        return true;', 'oracle'),
 ('early-selection-read', 'calls.transition_menu(work.presentation.next);\n        state.selected_file_backup = state.selected_card_file;', 'const auto selected = state.selected_card_file;\n        calls.transition_menu(work.presentation.next);\n        state.selected_file_backup = selected;', 'unit'),
 ('close-before-clear', 'clear_pending(state);\n        state.fade_in_marker = 1;\n        state.save_ok = 0;\n        state.alert_blocked = 0;\n        calls.close_save_warning(state.warning_task);', 'calls.close_save_warning(state.warning_task);\n        clear_pending(state);\n        state.fade_in_marker = 1;\n        state.save_ok = 0;\n        state.alert_blocked = 0;', 'unit'),
]
results = []
for name, before, after, judge in mutants:
    assert before in source
    path = build / (name + '.cpp')
    path.write_text(source.replace(before, after))
    obj = build / (name + '.o')
    subprocess.run(flags + ['-fPIC', '-c', str(path), '-o', str(obj)], check=True)
    if judge == 'oracle':
        lib = build / (name + '.dylib')
        subprocess.run(flags + ['-dynamiclib', 'tests/probes/native_save_creation_probe.cpp', str(obj), archive, '-o', str(lib)], check=True)
        command = [python, 'tools/oracles/verify_original_save_completion.py', exe, str(lib), '--output', str(build / (name + '.json'))]
        import os
        env = dict(os.environ, PYTHONPATH='tools/lib')
    else:
        test = build / (name + '-test')
        subprocess.run(flags + ['tests/unit/save_completion_callbacks.cpp', str(obj), archive, '-o', str(test)], check=True)
        command, env = [str(test)], None
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    results.append({'name': name, 'judge': judge, 'compile_status': 0, 'terminal_status': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr})
    assert result.returncode != 0, name
(build / 'mutants.json').write_text(json.dumps(results, indent=2) + '\n')
print(json.dumps(results, indent=2))
