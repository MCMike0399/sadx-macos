from pathlib import Path
import subprocess, tempfile, json, hashlib
root = Path.cwd()
source = root / 'src/engine/ninja_queue_initial_state.cpp'
original = source.read_text()
mutations = [
    ('wrong_depth_index', 'values.depth_comparison = 0;'),
    ('premature_reset_capture', 'values.reset_capture = values.reset_source;'),
    ('allocation_capture_endpoint', 'values.allocation_capture[2] = -1600.0F;'),
]
rows = []
for name, mutation in [('baseline', None), *mutations]:
    with tempfile.TemporaryDirectory(prefix='sadx-queue-numeric-') as directory:
        base = Path(directory)
        text = original
        if mutation:
            assert text.count('return {};') == 1
            text = text.replace('return {};', 'NinjaQueueInitialState values{}; ' + mutation + ' return values;')
        cpp, library, report = base / 'input.cpp', base / 'probe.dylib', base / 'oracle.json'
        cpp.write_text(text)
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-dynamiclib', str(cpp),
                   'tests/probes/native_ninja_queue_initial_state_probe.cpp', '-o', str(library)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
        command = ['reference/python/bin/python3', 'tools/oracles/verify_original_ninja_queue_initial_state.py',
                   'game/Sonic Adventure DX/sonic.exe', str(library), '--output', str(report)]
        result = subprocess.run(command, cwd=root, env={**__import__('os').environ, 'PYTHONPATH': 'tools/lib'}, capture_output=True, text=True)
        assert result.returncode == (0 if mutation is None else 1), (name, result.stderr)
        data = json.loads(report.read_text())
        row = dict(name=name, exit_status=result.returncode, killed=result.returncode != 0,
                   mismatches=data['mismatches'], mutation=mutation)
        rows.append(row)
        print(row, flush=True)
assert source.read_text() == original
(root / 'reference/production-integration-20261016/queue-initial-159-numeric-mutants.json').write_text(
    json.dumps(dict(passed=True, source_sha256=hashlib.sha256(original.encode()).hexdigest(),
                    baseline=rows[0], mutants=rows[1:], worktree_mutated=False), indent=2) + '\n')
