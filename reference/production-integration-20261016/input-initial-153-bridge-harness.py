from pathlib import Path
import subprocess, tempfile, json, hashlib
root = Path.cwd()
source = root / 'src/guest/native_input_initialization.cpp'
original = source.read_text()
mutations = [
    ('ignore_native_input', 'if (config_.input_defaults) {', 'if (false && config_.input_defaults) {'),
    ('negate_left_stick', 'static_cast<std::uint16_t>(values.left_x)', 'static_cast<std::uint16_t>(-values.left_x)'),
    ('zero_injected_resources', 'record.write_byte(i, image ? image->byte(address + i) : std::nullopt);', 'record.write_byte(i, image ? (config_.input_defaults ? std::optional<std::uint8_t>{0} : image->byte(address + i)) : std::nullopt);'),
    ('wrong_mod_raw_stride', 'load(*input_raw_[i], dll, 0x1000e748 + 52 * i);', 'load(*input_raw_[i], dll, 0x1000e748 + 96 * i);'),
]
rows = []
for name, before, after in [('baseline', None, None), *mutations]:
    with tempfile.TemporaryDirectory(prefix='sadx-input-initial-mutant-') as directory:
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
                   'tests/probes/native_input_initial_state_probe.cpp',
                   'build/libsadx-guest.a', 'build/libsadx-core.a', '-o', str(executable)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
        args = ['game/Sonic Adventure DX/sonic.exe', 'game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll',
                'build/engine-code-tables-poisoned.exe', 'build/session-control-poisoned.exe',
                'build/session-control-varied.exe', 'build/session-control-partial.exe',
                'build/input-initial-poisoned.exe', 'build/input-initial-poisoned.dll',
                'build/input-initial-varied.exe', 'build/input-initial-varied.dll',
                'build/input-initial-partial.exe', 'build/input-initial-partial.dll']
        result = subprocess.run([str(executable), *args], cwd=root, capture_output=True, text=True)
        assert result.returncode == (0 if before is None else 1), (name, result)
        row = dict(name=name, exit_status=result.returncode, killed=result.returncode != 0,
                   output=(result.stdout + result.stderr).strip(), replacement=[before, after])
        rows.append(row)
        print(row, flush=True)
report = dict(passed=True, source_sha256=hashlib.sha256(original.encode()).hexdigest(), baseline=rows[0],
              mutants=rows[1:], rejected_equivalent_mutant={'name': 'overwrite_extension_resource_before_resource_reload', 'reason': 'Writing +40 inside store_input_values is overwritten by the subsequent resource load; it has no observable constructor effect and is not a valid mutation.'}, limits=['Temporary adapter copies linked before unchanged archives; no worktree mutation.',
                                      'Three-frame survey only, no sprite draws or complete native input claim.'])
(root / 'reference/production-integration-20261016/input-initial-153-bridge-mutants.json').write_text(json.dumps(report, indent=2) + '\n')
