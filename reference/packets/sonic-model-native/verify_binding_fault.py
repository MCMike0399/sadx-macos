"""The real callback must detect a wrong canonical parameter in its model binding."""
import json
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[3]
out = ROOT / 'build/sonic-model/binding-fault'
out.mkdir(parents=True, exist_ok=True)
source = (ROOT / 'src/runtime/native_sonic_binding.cpp').read_text()
old = 'collision->info[0].a = player.record->parameters.height;'
assert source.count(old) == 1
mutant = out / 'binding.cpp'
mutant.write_text(source.replace(old, 'collision->info[0].a = player.record->parameters.center_height;'))
command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
           '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
           '-Isrc', '-c', str(mutant), '-o', str(out / 'binding.o')]
receipts = []
def run(command):
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    receipts.append(dict(command=command, exit=result.returncode, stdout=result.stdout, stderr=result.stderr))
    return result
assert run(command).returncode == 0
dry = run(['make', '-n', '-W', 'tests/unit/sonic_model_native.mm',
           '-o', 'build/native-controller-deps/include/SDL3/SDL.h', 'build/sonic-model-native-test'])
assert dry.returncode == 0
lines = [line for line in dry.stdout.splitlines() if line.startswith('xcrun clang++ ') and
         'tests/unit/sonic_model_native.mm' in line]
assert len(lines) == 1
command = shlex.split(lines[0])
command.insert(command.index('build/sonic-model-callback.o'), str(out / 'binding.o'))
command[-1] = str(out / 'test')
assert run(command).returncode == 0
failed = run([str(out / 'test'), 'game/imported'])
assert failed.returncode != 0 and 'collision->info[0].a == player->parameters.height' in failed.stderr
(out / 'receipt.json').write_text(json.dumps(dict(caught=True, commands=receipts), indent=2) + '\n')
print('Canonical height binding fault caught by actual callback')
