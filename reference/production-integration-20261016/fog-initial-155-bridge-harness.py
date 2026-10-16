from pathlib import Path
import subprocess, tempfile, json, hashlib
root = Path.cwd()
source = root / 'src/guest/native_game_reference.cpp'
original = source.read_text()
mutations = [
    ('ignore_native_fog', 'if (config_.fog_defaults) {', 'if (false && config_.fog_defaults) {'),
    ('cross_wire_cache', 'consumer_.cached_fog_end_at_90a09c = values.cached_end;', 'consumer_.cached_fog_end_at_90a09c = values.cached_start;'),
    ('overwrite_frame_flag', 'consumer_.fog_enabled_at_3d12798 = values.enabled;', 'consumer_.fog_enabled_at_3d12798 = values.enabled; queue_runtime_.flag_at_3abdcfe = 0;'),
    ('invent_missing_image_word', '    const auto word = [exe](std::uint32_t address) -> std::optional<std::uint32_t> {\n        return exe ? exe->u32(address) : std::nullopt;\n    };', '    const auto word = [exe](std::uint32_t address) -> std::optional<std::uint32_t> {\n        return exe ? std::optional<std::uint32_t>{exe->u32(address).value_or(0)} : std::nullopt;\n    };'),
]
rows = []
for name, before, after in [('baseline', None, None), *mutations]:
    with tempfile.TemporaryDirectory(prefix='sadx-fog-initial-mutant-') as directory:
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
                   'tests/probes/native_input_initial_state_probe.cpp', 'tests/probes/native_ninja_sine_table_probe.cpp', 'tests/probes/native_ninja_fog_initial_state_probe.cpp',
                   'build/libsadx-guest.a', 'build/libsadx-core.a', '-o', str(executable)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
        args = ['game/Sonic Adventure DX/sonic.exe', 'game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll',
                'build/engine-code-tables-poisoned.exe', 'build/session-control-poisoned.exe',
                'build/session-control-varied.exe', 'build/session-control-partial.exe',
                'build/input-initial-poisoned.exe', 'build/input-initial-poisoned.dll',
                'build/input-initial-varied.exe', 'build/input-initial-varied.dll',
                'build/input-initial-partial.exe', 'build/input-initial-partial.dll', 'build/sine-initial', 'build/fog-initial']
        result = subprocess.run([str(executable), *args], cwd=root, capture_output=True, text=True)
        assert result.returncode == (0 if before is None else 1), (name, result)
        row = dict(name=name, exit_status=result.returncode, killed=result.returncode != 0,
                   output=(result.stdout + result.stderr).strip(), replacement=[before, after])
        rows.append(row)
        print(row, flush=True)
report = dict(passed=True, source_sha256=hashlib.sha256(original.encode()).hexdigest(), baseline=rows[0],
              mutants=rows[1:], limits=['Temporary adapter copies linked before unchanged archives; no worktree mutation.',
                                      'Three-frame survey only, no sprite draws or complete native input claim.'])
(root / 'reference/production-integration-20261016/fog-initial-155-bridge-mutants.json').write_text(json.dumps(report, indent=2) + '\n')
