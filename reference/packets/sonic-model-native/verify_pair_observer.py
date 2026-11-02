"""Demonstrate the former pair blind spot against identical compiled product faults."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUTPUT = ROOT / 'build/sonic-model/observer-correction'
FLAGS = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
         '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
         '-fobjc-arc', '-Isrc', '-dynamiclib']


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    old_probe = OUTPUT / 'probe-before.mm'
    old_judge = OUTPUT / 'verify_source_before.py'
    for path, original in [(old_probe, 'tests/probes/native_sonic_model_probe.mm'),
                           (old_judge, 'reference/packets/sonic-model-native/verify_source.py')]:
        path.write_bytes(subprocess.check_output(['git', 'show', '4ec83dab:' + original], cwd=ROOT))
    body = (ROOT / 'src/runtime/sonic_model_assets.cpp').read_text()
    faults = {
        'same-length-wrong-pair': (
            'vertex.vertex_pairs = &declarations_.pairs[row.pairs - 1];',
            'vertex.vertex_pairs = &declarations_.pairs[(row.pairs == 1 ? 2 : row.pairs) - 1];'),
        'changed-pair-content': (
            '    state_.joins = std::move(table);',
            '    declarations_.pairs[0][0] ^= 1;\n    state_.joins = std::move(table);'),
    }
    receipts = []
    for name, (before, after) in faults.items():
        assert body.count(before) == 1
        source = OUTPUT / (name + '.cpp')
        source.write_text(body.replace(before, after))
        checks = []
        for schema, probe, judge in [
            ('before', old_probe, old_judge),
            ('after', ROOT / 'tests/probes/native_sonic_model_probe.mm',
             ROOT / 'reference/packets/sonic-model-native/verify_source.py'),
        ]:
            library = OUTPUT / (name + '-' + schema + '.dylib')
            command = FLAGS + [str(probe), str(source), 'build/libsadx-native.a',
                               '-framework', 'Foundation', '-o', str(library)]
            build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
            assert build.returncode == 0, build.stderr
            report = OUTPUT / (name + '-' + schema + '.json')
            run = subprocess.run([sys.executable, str(judge), str(library), '--output', str(report)],
                                 cwd=ROOT, capture_output=True, text=True)
            observed = json.loads(report.read_text())
            expected_exit = 2 if schema == 'before' else 1
            assert run.returncode == expected_exit, run.stderr
            assert bool(observed['failures']) == (schema == 'after')
            checks.append(dict(schema=schema, command=command, exit_status=run.returncode,
                               failures=len(observed['failures']),
                               partials=len(observed['partials']),
                               first_failure=observed['failures'][:1],
                               library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),
                               report_sha256=hashlib.sha256(report.read_bytes()).hexdigest()))
        receipts.append(dict(name=name, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                             checks=checks))
        print(name, 'hidden before, caught after', flush=True)
    (OUTPUT / 'pair-faults.json').write_text(json.dumps(receipts, indent=2) + '\n')


if __name__ == '__main__':
    main()
