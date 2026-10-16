from pathlib import Path
import subprocess, tempfile, json, hashlib
root = Path.cwd()
source = root / 'src/guest/native_game_reference.cpp'
original = source.read_text()
mutations = [
    ('ignore_native_screen', 'if (config_.screen_defaults) {', 'if (false && config_.screen_defaults) {'),
    ('convert_depth_instead_of_bits', 'device_.viewport_at_3d12780[5] = bits(values.viewport.max_z);', 'device_.viewport_at_3d12780[5] = static_cast<std::uint32_t>(values.viewport.max_z);'),
    ('cross_wire_texture_resource', 'screen_.word_at_3ce716c=u32(0x3ce716c);', 'screen_.word_at_3ce716c=u32(0x3ce7164);'),
    ('invent_missing_projection_word', 'screen_.projection_at_3d129c0[i] = word(0x3d129c0 + 4 * i);', 'screen_.projection_at_3d129c0[i] = exe ? std::optional<std::uint32_t>{word(0x3d129c0 + 4 * i).value_or(0)} : std::nullopt;'),
]
rows = []
for name, before, after in [('baseline', None, None), *mutations]:
    with tempfile.TemporaryDirectory(prefix='sadx-screen-initial-mutant-') as directory:
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
                   'tests/probes/native_input_initial_state_probe.cpp', 'tests/probes/native_ninja_sine_table_probe.cpp', 'tests/probes/native_ninja_fog_initial_state_probe.cpp', 'tests/probes/native_ninja_screen_initial_state_probe.cpp',
                   'build/libsadx-guest.a', 'build/libsadx-core.a', '-o', str(executable)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
        args = ['game/Sonic Adventure DX/sonic.exe', 'game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll',
                'build/engine-code-tables-poisoned.exe', 'build/session-control-poisoned.exe',
                'build/session-control-varied.exe', 'build/session-control-partial.exe',
                'build/input-initial-poisoned.exe', 'build/input-initial-poisoned.dll',
                'build/input-initial-varied.exe', 'build/input-initial-varied.dll',
                'build/input-initial-partial.exe', 'build/input-initial-partial.dll', 'build/sine-initial', 'build/fog-initial', 'build/screen-initial']
        result = subprocess.run([str(executable), *args], cwd=root, capture_output=True, text=True)
        assert result.returncode == (0 if before is None else 1), (name, result)
        row = dict(name=name, exit_status=result.returncode, killed=result.returncode != 0,
                   output=(result.stdout + result.stderr).strip(), replacement=[before, after])
        rows.append(row)
        print(row, flush=True)
report = dict(passed=True, source_sha256=hashlib.sha256(original.encode()).hexdigest(), baseline=rows[0],
              mutants=rows[1:], limits=['Temporary adapter copies linked before unchanged archives; no worktree mutation.',
                                      'Three-frame survey only, no sprite draws or complete native input claim.'])
(root / 'reference/production-integration-20261016/screen-initial-156-bridge-mutants.json').write_text(json.dumps(report, indent=2) + '\n')
