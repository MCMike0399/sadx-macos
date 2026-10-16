from pathlib import Path
import subprocess, tempfile, json, hashlib
root = Path.cwd()
source = root / 'src/guest/native_game_reference.cpp'
original = source.read_text()
mutations = [
    ('ignore_native_view', 'if (config_.view_defaults) {', 'if (false && config_.view_defaults) {'),
    ('wrong_screen_distance', 'bits(values.screen.distance)', 'bits(values.screen.width)'),
    ('invent_missing_color_component',
     'if (!a || !r || !g || !b)\n            return std::nullopt;\n        return std::array<float, 4>{*a, *r, *g, *b};',
     'if (!a || !r || !g)\n            return std::nullopt;\n        return std::array<float, 4>{*a, *r, *g, b.value_or(0)};'),
    ('overwrite_pause_owner', 'byte_at_3b280a8_ = values.reset_pending;', 'byte_at_3b280a8_ = values.reset_pending; loop_.mode_at_3b28114 = 0;'),
]
rows = []
for name, before, after in [('baseline', None, None), *mutations]:
    with tempfile.TemporaryDirectory(prefix='sadx-view-initial-mutant-') as directory:
        base = Path(directory)
        text = original
        if before:
            assert text.count(before) == 1, (name, text.count(before))
            text = text.replace(before, after, 1)
        cpp, executable = base / 'input.cpp', base / 'runtime-test'
        cpp.write_text(text)
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-Itests/fixtures', str(cpp),
                   'tests/unit/game_runtime.cpp', 'tests/probes/native_session_control_defaults_probe.cpp',
                   'tests/probes/native_input_initial_state_probe.cpp', 'tests/probes/native_ninja_sine_table_probe.cpp', 'tests/probes/native_ninja_fog_initial_state_probe.cpp', 'tests/probes/native_ninja_screen_initial_state_probe.cpp', 'tests/probes/native_sound_initial_state_probe.cpp', 'tests/probes/native_game_view_initial_state_probe.cpp',
                   'build/libsadx-guest.a', 'build/libsadx-core.a', '-o', str(executable)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
        args = ['game/Sonic Adventure DX/sonic.exe', 'game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll',
                'build/engine-code-tables-poisoned.exe', 'build/session-control-poisoned.exe',
                'build/session-control-varied.exe', 'build/session-control-partial.exe',
                'build/input-initial-poisoned.exe', 'build/input-initial-poisoned.dll',
                'build/input-initial-varied.exe', 'build/input-initial-varied.dll',
                'build/input-initial-partial.exe', 'build/input-initial-partial.dll', 'build/sine-initial', 'build/fog-initial', 'build/screen-initial', 'build/sound-initial', 'build/view-initial']
        result = subprocess.run([str(executable), *args], cwd=root, capture_output=True, text=True)
        assert result.returncode == (0 if before is None else 1), (name, result)
        row = dict(name=name, exit_status=result.returncode, killed=result.returncode != 0,
                   output=(result.stdout + result.stderr).strip(), replacement=[before, after])
        rows.append(row)
        print(row, flush=True)
report = dict(passed=True, source_sha256=hashlib.sha256(original.encode()).hexdigest(), baseline=rows[0],
              mutants=rows[1:], limits=['Temporary adapter copies linked before unchanged archives; no worktree mutation.',
                                      'Three-frame survey only, no sprite draws or complete native view claim.'])
(root / 'reference/production-integration-20261016/view-initial-158-bridge-mutants.json').write_text(json.dumps(report, indent=2) + '\n')
