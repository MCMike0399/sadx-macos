from pathlib import Path
import subprocess, tempfile, json, hashlib
root = Path.cwd()
source = root / 'src/game/game_view_initial_state.cpp'
original = source.read_text()
mutations = [
    ('pixel_width_as_stretch', 'values.stretch_x = 640.0F;'),
    ('premature_screen_distance', 'values.screen.distance = 1.0F;'),
    ('opaque_load_color', 'values.view_color.alpha = 1.0F;'),
]
rows = []
for name, mutation in [('baseline', None), *mutations]:
    with tempfile.TemporaryDirectory(prefix='sadx-view-numeric-') as directory:
        base = Path(directory)
        text = original
        if mutation:
            assert text.count('return {};') == 1
            text = text.replace('return {};', 'GameViewInitialState values{}; ' + mutation + ' return values;')
        cpp, library, report = base / 'input.cpp', base / 'probe.dylib', base / 'oracle.json'
        cpp.write_text(text)
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-dynamiclib', str(cpp),
                   'tests/probes/native_game_view_initial_state_probe.cpp', '-o', str(library)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
        command = ['reference/python/bin/python3', 'tools/oracles/verify_original_game_view_initial_state.py',
                   'game/Sonic Adventure DX/sonic.exe', str(library), '--output', str(report)]
        result = subprocess.run(command, cwd=root, env={**__import__('os').environ, 'PYTHONPATH': 'tools/lib'}, capture_output=True, text=True)
        assert result.returncode == (0 if mutation is None else 1), (name, result.stderr)
        data = json.loads(report.read_text())
        row = dict(name=name, exit_status=result.returncode, killed=result.returncode != 0,
                   mismatches=data['mismatches'], mutation=mutation)
        rows.append(row)
        print(row, flush=True)
assert source.read_text() == original
(root / 'reference/production-integration-20261016/view-initial-158-numeric-mutants.json').write_text(
    json.dumps(dict(passed=True, source_sha256=hashlib.sha256(original.encode()).hexdigest(),
                    baseline=rows[0], mutants=rows[1:], worktree_mutated=False), indent=2) + '\n')
