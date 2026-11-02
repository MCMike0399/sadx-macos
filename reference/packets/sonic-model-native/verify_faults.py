"""Compile faults in private source files, keeping shipping code and old judges intact."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
FAULTS = {
    'metal_guard': ('if (!alternate)', 'if (alternate)'),
    'equal_sibling_guard': ('if (object.sibling != attach)', 'if (true)'),
    'splice_identity': ('object.sibling = attach;', 'object.sibling = root;'),
    'join_destination_neighbor': ('rows.at(11).destination_object', 'rows.at(12).destination_object'),
    'join_mode': ('player.join_mode = 0;', 'player.join_mode = 2;'),
    'cache_prefix': ('    if (!alternate) {', '    state.model_sibling = root;\n    if (!alternate) {'),
    'primary_mode': ('player.mode = 1;', 'player.mode = 2;'),
}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    body = (ROOT / 'src/player/sonic_model.cpp').read_text()
    receipts = []
    for name, (old, new) in FAULTS.items():
        assert body.count(old) == 1
        source = args.output / (name + '.cpp')
        source.write_text(body.replace(old, new))
        library = args.output / (name + '.dylib')
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
                   '-fobjc-arc', '-Isrc', '-dynamiclib', 'tests/probes/native_sonic_model_probe.mm',
                   str(source), 'build/libsadx-native.a', '-framework', 'Foundation', '-o', str(library)]
        compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        assert compiled.returncode == 0, compiled.stderr
        report = args.output / (name + '.json')
        judged = subprocess.run([sys.executable, str(Path(__file__).with_name('verify_source.py')),
                                 str(library), '--output', str(report)], cwd=ROOT,
                                capture_output=True, text=True)
        result = json.loads(report.read_text())
        assert judged.returncode == 1 and result['failures'], name
        receipts.append(dict(name=name, command=command, compile_exit=compiled.returncode,
                             judge_exit=judged.returncode, failures=len(result['failures']),
                             first_failure=result['failures'][0]))
        (args.output / 'faults.json').write_text(json.dumps(receipts, indent=2) + '\n')
        print(name, 'caught', flush=True)

if __name__ == '__main__':
    main()
