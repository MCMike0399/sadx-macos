"""Build the exact Root owner/getter patch against copied source fixtures only."""
from pathlib import Path
import shutil
import subprocess
import json
ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/model-matrices-native/root-contract'
patch = ROOT / 'reference/packets/model-matrices-native/ROOT-OWNER.patch'
paths = [line[6:] for line in patch.read_text().splitlines() if line.startswith('--- a/')]
for path in paths:
    target = OUT / path; target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / path, target)
apply = subprocess.run(['patch', '--batch', '-p1', '-d', str(OUT), '-i', str(patch)], capture_output=True, text=True)
assert apply.returncode == 0, apply.stdout + apply.stderr
binary = OUT / 'check'
command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
           '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
           '-fobjc-arc', '-I' + str(OUT / 'src'), '-Isrc',
           'tests/fixtures/model_matrix_binding.mm',
           str(OUT / 'src/runtime/native_gamma_binding.cpp'), 'build/libsadx-native.a',
           '-framework', 'Foundation', '-o', str(binary)]
compile = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
(OUT / 'build.log').write_text(compile.stdout + compile.stderr)
assert compile.returncode == 0
run = subprocess.run([str(binary), 'game/imported'], cwd=ROOT, capture_output=True, text=True)
(OUT / 'run.log').write_text(run.stdout + run.stderr)
assert run.returncode == 0, run.stdout + run.stderr
print(run.stdout.strip())
(ROOT / 'reference/packets/model-matrices-native/ROOT-CONTRACT.json').write_text(json.dumps({
    'compiled_exact_owner_patch': True, 'native_exit_status': run.returncode,
    'shared_worktree_files_modified': False, 'qualified': 'typed getter borrow, distinct live owners and all 16 components',
    'unqualified': 'Root admission/full gate and active PJoin processing'}, indent=2) + '\n')
