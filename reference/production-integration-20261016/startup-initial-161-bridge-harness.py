from pathlib import Path
import subprocess, tempfile, json, hashlib
root = Path.cwd()
source = root / 'src/guest/native_game_reference.cpp'
original = source.read_text()
mutations = [
    ('ignore_native_startup', 'if (config_.startup_defaults) {', 'if (false && config_.startup_defaults) {'),
    ('truncate_assignments', '= values.button_assignments[i];', '= static_cast<std::uint8_t>(values.button_assignments[i]);'),
    ('invent_missing_binding', 'if (low && high)\n            winmain_.pad_buttons_at_389d634[i] = static_cast<std::uint16_t>(*low | (*high << 8));', 'if (low)\n            winmain_.pad_buttons_at_389d634[i] = static_cast<std::uint16_t>(*low | (high.value_or(0) << 8));'),
    ('overwrite_media_resource', 'winmain_.media_at_3ab918c=u32(0x3ab918c);', 'winmain_.media_at_3ab918c=config_.startup_defaults ? std::optional<std::uint32_t>{0} : u32(0x3ab918c);'),
]

# An earlier dead-store candidate wrote media before its independent initialization,
# which overwrote it. This candidate changes the observable resource store instead.
rows = []
for name, before, after in [('baseline', None, None), *mutations]:
    with tempfile.TemporaryDirectory(prefix='sadx-startup-initial-mutant-') as directory:
        base = Path(directory)
        text = original
        if before:
            assert text.count(before) == 1, (name, text.count(before))
            text = text.replace(before, after, 1)
        cpp, executable = base / 'input.cpp', base / 'runtime-test'
        cpp.write_text(text)
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-Itests/fixtures', str(cpp),
                   'tests/unit/startup_initial_boot.cpp', 'tests/probes/native_startup_initial_state_probe.cpp',
                   'build/libsadx-guest.a', 'build/libsadx-core.a', '-o', str(executable)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
        args = ['game/Sonic Adventure DX/sonic.exe', 'game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll', 'build/startup-initial']
        result = subprocess.run([str(executable), *args], cwd=root, capture_output=True, text=True)
        assert result.returncode == (0 if before is None else 1), (name, result)
        row = dict(name=name, exit_status=result.returncode, killed=result.returncode != 0,
                   output=(result.stdout + result.stderr).strip(), replacement=[before, after])
        rows.append(row)
        print(row, flush=True)
report = dict(passed=True, source_sha256=hashlib.sha256(original.encode()).hexdigest(), baseline=rows[0],
              mutants=rows[1:], limits=['Temporary adapter copies linked before unchanged archives; no worktree mutation.',
                                      'Three-frame survey only, no sprite draws or complete native startup claim.'])
(root / 'reference/production-integration-20261016/startup-initial-161-bridge-mutants.json').write_text(json.dumps(report, indent=2) + '\n')
