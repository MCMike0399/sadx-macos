"""Compiled faults in the exact native getter adapter, using distinct live owners."""
import hashlib
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[3]
CONTRACT = ROOT / 'build/model-matrices-native/root-contract'
OUT = ROOT / 'build/model-matrices-native/binding-faults'
BODY = '(void)model_matrices::get(*context.model_matrices, node, destination);'
FAULTS = [
    ('wrong-state-owner', '{ model_matrices::State wrong; (void)model_matrices::get(wrong, node, destination); }'),
    ('rejected-destination-write', 'if (!model_matrices::get(*context.model_matrices, node, destination)) destination[15] = 1;'),
    ('unsigned-index-truncation', '(void)model_matrices::get(*context.model_matrices, node % 128, destination);'),
]
def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source = (CONTRACT / 'src/runtime/native_gamma_binding.cpp').read_text()
    assert source.count(BODY) == 1
    rows = []
    for name, replacement in FAULTS:
        directory = OUT / name; directory.mkdir(exist_ok=True)
        path = directory / 'native_gamma_binding.cpp'; path.write_text(source.replace(BODY, replacement))
        binary = directory / 'test'
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
                   '-fobjc-arc', '-I' + str(CONTRACT / 'src'), '-Isrc',
                   'tests/fixtures/model_matrix_binding.mm', str(path), 'build/libsadx-native.a',
                   '-framework', 'Foundation', '-o', str(binary)]
        build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (directory / 'build.log').write_text(build.stdout + build.stderr)
        assert build.returncode == 0, build.stderr
        run = subprocess.run([str(binary), 'game/imported'], cwd=ROOT, capture_output=True, text=True)
        (directory / 'run.log').write_text(run.stdout + run.stderr)
        assert run.returncode != 0, name
        rows.append({'name': name, 'compiled': True, 'exit_status': run.returncode,
                     'caught': True, 'failure': run.stderr.strip(),
                     'mutated_source_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    header = (CONTRACT / 'src/runtime/native_display.hpp').read_text()
    operation = 'coordinates_->rotate_z(matrix, angle, arithmetic_);'
    assert header.count(operation) == 1
    for name, replacement in [
        ('wrong-rotation-axis', 'coordinates_->rotate_y(matrix, angle, arithmetic_);'),
        ('rotates-stack-instead-of-destination', '(void)matrix; coordinates_->rotate_z(coordinates_->current_matrix(), angle, arithmetic_);')
    ]:
        directory = OUT / name; directory.mkdir(parents=True, exist_ok=True)
        path = directory / 'src/runtime/native_display.hpp'; path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(header.replace(operation, replacement))
        binary = directory / 'test'
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
                   '-fobjc-arc', '-I' + str(directory / 'src'), '-I' + str(CONTRACT / 'src'), '-Isrc',
                   'tests/fixtures/model_matrix_binding.mm', str(CONTRACT / 'src/runtime/native_gamma_binding.cpp'),
                   'build/libsadx-native.a', '-framework', 'Foundation', '-o', str(binary)]
        build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (directory / 'build.log').write_text(build.stdout + build.stderr)
        assert build.returncode == 0, build.stderr
        run = subprocess.run([str(binary), 'game/imported'], cwd=ROOT, capture_output=True, text=True)
        (directory / 'run.log').write_text(run.stdout + run.stderr)
        assert run.returncode != 0, name
        rows.append({'name': name, 'compiled': True, 'exit_status': run.returncode,
                     'caught': True, 'failure': run.stderr.strip(),
                     'mutated_source_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    (ROOT / 'reference/packets/model-matrices-native/BINDING-FAULTS.json').write_text(
        json.dumps({'faults': rows, 'passed': True}, indent=2) + '\n')
    print(f'{len(rows)} compiled native getter/rotation binding faults caught')
if __name__ == '__main__': main()
