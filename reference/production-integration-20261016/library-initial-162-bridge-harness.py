from pathlib import Path
import subprocess, tempfile, json, hashlib
root = Path.cwd()
source = root / 'src/guest/native_game_reference.cpp'
original = source.read_text()
mutations = [
    ('ignore_native_library', 'if (config_.library_defaults) {', 'if (false && config_.library_defaults) {'),
    ('ignore_numeric_config', 'return library_configuration_[i];', 'return config_.executable ? config_.executable->u32(address) : std::nullopt;'),
    ('invent_missing_count', 'library_.words_at_389d864[i] = word(0x389d864 + 4 * i);', 'library_.words_at_389d864[i] = word(0x389d864 + 4 * i).value_or(0);'),
    ('invent_text_resource', 'return config_.executable ? config_.executable->u32(address) : std::nullopt;', 'return config_.library_defaults ? std::optional<std::uint32_t>{0} : (config_.executable ? config_.executable->u32(address) : std::nullopt);'),
]

rows = []
for name, before, after in [('baseline', None, None), *mutations]:
    with tempfile.TemporaryDirectory(prefix='sadx-library-initial-mutant-') as directory:
        base = Path(directory)
        text = original
        if before:
            assert text.count(before) == 1, (name, text.count(before))
            text = text.replace(before, after, 1)
        cpp, executable = base / 'input.cpp', base / 'runtime-test'
        cpp.write_text(text)
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-Itests/fixtures', str(cpp),
                   'tests/unit/library_initial_boot.cpp', 'tests/probes/native_library_initial_state_probe.cpp',
                   'build/libsadx-guest.a', 'build/libsadx-core.a', '-o', str(executable)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
        args = ['game/Sonic Adventure DX/sonic.exe', 'game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll', 'build/library-initial']
        result = subprocess.run([str(executable), *args], cwd=root, capture_output=True, text=True)
        assert result.returncode == (0 if before is None else 1), (name, result)
        row = dict(name=name, exit_status=result.returncode, killed=result.returncode != 0,
                   output=(result.stdout + result.stderr).strip(), replacement=[before, after])
        rows.append(row)
        print(row, flush=True)
report = dict(passed=True, source_sha256=hashlib.sha256(original.encode()).hexdigest(), baseline=rows[0],
              mutants=rows[1:], limits=['Temporary adapter copies linked before unchanged archives; no worktree mutation.',
                                      'Three-frame survey only, no sprite draws or complete native library claim.'])
(root / 'reference/production-integration-20261016/library-initial-162-bridge-mutants.json').write_text(json.dumps(report, indent=2) + '\n')
