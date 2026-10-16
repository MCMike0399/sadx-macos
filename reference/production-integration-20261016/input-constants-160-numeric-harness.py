from pathlib import Path
import subprocess, tempfile, json, hashlib
root = Path.cwd()
source = root / 'src/input/input_mod_constants.cpp'
original = source.read_text()
mutations = [
    ('wrong_epsilon', 'sse_literal(1.1920929e-7F)', 'sse_literal(0.000001F)'),
    ('keyboard_controller_scale_confusion', 'sse_literal(127.0F)', 'sse_literal(128.0F)'),
    ('lost_half_press', 'sse_literal(0.5F)', 'sse_literal(1.0F)'),
    ('numeric_conversion_instead_of_bits', 'std::memcpy(&result.bits, &value, sizeof(value));', 'result.bits = static_cast<std::uint32_t>(value);'),
]
rows = []
for name, before, after in [('baseline', None, None), *mutations]:
    with tempfile.TemporaryDirectory(prefix='sadx-input-constants-numeric-') as directory:
        base = Path(directory)
        text = original
        if before:
            assert text.count(before) == 1, (name, text.count(before))
            text = text.replace(before, after, 1)
        cpp, library, report = base / 'input.cpp', base / 'probe.dylib', base / 'oracle.json'
        cpp.write_text(text)
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-dynamiclib', str(cpp),
                   'tests/probes/native_input_mod_constants_probe.cpp', '-o', str(library)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
        command = ['reference/python/bin/python3', 'tools/oracles/verify_original_input_mod_constants.py',
                   'game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll', str(library), '--output', str(report)]
        result = subprocess.run(command, cwd=root, env={**__import__('os').environ, 'PYTHONPATH': 'tools/lib'}, capture_output=True, text=True)
        assert result.returncode == (0 if before is None else 1), (name, result.stderr)
        data = json.loads(report.read_text())
        row = dict(name=name, exit_status=result.returncode, killed=result.returncode != 0,
                   mismatches=data['mismatches'], replacement=[before, after])
        rows.append(row)
        print(row, flush=True)
assert source.read_text() == original
(root / 'reference/production-integration-20261016/input-constants-160-numeric-mutants.json').write_text(
    json.dumps(dict(passed=True, source_sha256=hashlib.sha256(original.encode()).hexdigest(),
                    baseline=rows[0], mutants=rows[1:], worktree_mutated=False), indent=2) + '\n')
