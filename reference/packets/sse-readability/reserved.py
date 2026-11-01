"""Root's once-only fresh SSE cohorts; freeze before generating any inputs.

The original opcode observer stays unchanged except its distinct random seed.
The native supplement reuses the pre-source contract with fresh operand arrays;
its old-vector digest assertion is replaced by the unchanged record-size check.
All semantic assertions remain. Complete immutable-old/current bytes must match.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import random
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
PACKET = Path(__file__).resolve().parent
FREEZE = PACKET / 'reserved-freeze.json'
OUT = ROOT / 'build/sse-reserved'
BASE = 'db2f1588122acf6aeb5e1cfa7484ab4da09904c3'
SEEDS = {'scalar': 0x53534591, 'native': 0x535345d7}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def freeze():
    artifacts = json.loads((PACKET / 'local-artifacts.json').read_text())
    inputs = {str(p.relative_to(ROOT)): sha(p) for p in (ROOT / 'src').rglob('*') if p.is_file()}
    for name in ('tests/unit/sse_math_contract.cpp', 'tools/oracles/verify_original_sse_math.py',
                 'reference/packets/sse-readability/capture_sse.py',
                 'reference/packets/sse-readability/reserved.py'):
        inputs[name] = sha(ROOT / name)
    for group in ('archives', 'libraries'):
        for name, digest in artifacts[group].items():
            if sha(ROOT / name) != digest:
                raise ValueError('Changed qualified artifact: ' + name)
            inputs[name] = digest
    record = dict(base=BASE, candidate=git('rev-parse', 'HEAD').decode().strip(),
                  seeds=SEEDS, inputs=inputs, python=str(Path(sys.executable).resolve()),
                  python_sha256=sha(Path(sys.executable).resolve()),
                  old_sse_sha256=hashlib.sha256(git('show', BASE + ':src/cpu/sse_math.cpp')).hexdigest(),
                  generation='Scalar: unchanged nine-opcode judge with reserved seed. Native: 18 fresh uint32 and 18 fresh uint64 operands from Random(native), all original controls/masks/sticky/status/thread/TLS assertions retained.',
                  expected_scalar_records=312112, expected_native_bytes=199068120,
                  policy='One execution after committed freeze. Stop on unexpected error or any old/current difference. Original disagreements remain classified failures, never normalized or promoted. No Source changes or reserved-input retries.')
    with FREEZE.open('x') as stream:
        stream.write(json.dumps(record, indent=2) + '\n')


def command(arguments, log):
    with log.open('w') as stream:
        result = subprocess.run(arguments, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f'Command failed ({result.returncode}): {arguments}; see {log}')


def capture_binary(arguments, target):
    digest = hashlib.sha256()
    count = 0
    with target.with_suffix('.stderr').open('wb') as errors:
        process = subprocess.Popen(arguments, cwd=ROOT, stdout=subprocess.PIPE, stderr=errors)
        with gzip.open(target, 'wb') as stream:
            while chunk := process.stdout.read(1024 * 1024):
                stream.write(chunk)
                digest.update(chunk)
                count += len(chunk)
        code = process.wait()
    if code:
        raise RuntimeError(f'Native contract exited {code}: {arguments}')
    return dict(bytes=count, sha256=digest.hexdigest(), exit=code)


def run():
    frozen = json.loads(FREEZE.read_text())
    if git('show', 'HEAD:' + str(FREEZE.relative_to(ROOT))) != FREEZE.read_bytes():
        raise ValueError('Reserved freeze must be committed unchanged')
    for name, digest in frozen['inputs'].items():
        if sha(ROOT / name) != digest:
            raise ValueError('Frozen input changed: ' + name)
    if str(Path(sys.executable).resolve()) != frozen['python'] or sha(Path(sys.executable).resolve()) != frozen['python_sha256']:
        raise ValueError('Wrong pinned interpreter')
    old_source = git('show', BASE + ':src/cpu/sse_math.cpp')
    if hashlib.sha256(old_source).hexdigest() != frozen['old_sse_sha256']:
        raise ValueError('Old Source snapshot changed')
    OUT.mkdir(exist_ok=False)
    (OUT / 'EXECUTED_ONCE').write_text('Reserved definitions consumed; never retry or regenerate.\n')
    scalar = (PACKET / 'capture_sse.py').read_text()
    anchor = "sys.argv=[judge.__file__,a.library,'--output',str(out/'report.json')]"
    if scalar.count(anchor) != 1:
        raise ValueError('Unexpected opcode observer structure')
    scalar = scalar.replace(anchor, "assert source.count('rng=random.Random(0x535345)') == 1\nsource=source.replace('rng=random.Random(0x535345)', 'rng=random.Random(' + str(" + str(SEEDS['scalar']) + ") + ')')\n" + anchor)
    scalar_path = OUT / 'scalar_observer.py'
    scalar_path.write_text(scalar)
    results = dict(scalar={}, native={}, original_source_unchanged=True)
    for label in ('baseline', 'current'):
        command([sys.executable, str(scalar_path), str(ROOT / 'build/sse-readability' / label / 'probes.dylib'),
                 str(OUT / label / 'scalar')], OUT / (label + '-scalar.log'))
        results['scalar'][label] = json.loads((OUT / label / 'scalar/identities.json').read_text())
    if results['scalar']['baseline'] != results['scalar']['current']:
        raise ValueError('Fresh scalar Source/native output or classification identity changed')
    if results['scalar']['baseline']['records'] != frozen['expected_scalar_records']:
        raise ValueError('Missing scalar observations')
    rng = random.Random(SEEDS['native'])
    single = [rng.getrandbits(32) for _ in range(18)]
    wide = [rng.getrandbits(64) for _ in range(18)]
    (OUT / 'native-inputs.json').write_text(json.dumps(dict(single=single, wide=wide), indent=2) + '\n')
    contract = (ROOT / 'tests/unit/sse_math_contract.cpp').read_text()
    for name, width, words in (('single', 32, single), ('wide', 64, wide)):
        pattern = rf'const std::array<std::uint{width}_t,18> {name}\{{\{{.*?\}}\}};'
        replacement = f'const std::array<std::uint{width}_t,18> {name}' + '{{' + ','.join(hex(x) + ('ULL' if width == 64 else 'U') for x in words) + '}};'
        contract, count = re.subn(pattern, replacement, contract, flags=re.S)
        if count != 1:
            raise ValueError('Unexpected native operand-array definition')
    old_assertion = 'assert(count==199068120 && digest==4458530012893139995ULL);'
    if contract.count(old_assertion) != 1:
        raise ValueError('Unexpected frozen-contract digest assertion')
    contract = contract.replace(old_assertion, 'assert(count==199068120);')
    contract_path = OUT / 'native_contract.cpp'
    contract_path.write_text(contract)
    (OUT / 'old_sse.cpp').write_bytes(old_source)
    flags = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
             '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0', '-Isrc', '-I.', '-Isrc/cpu']
    archives = ['build/sse-readability/pinned/sse/softfloat-sse.a', 'build/sse-readability/pinned/base/softfloat.a']
    for label, source in (('baseline', OUT / 'old_sse.cpp'), ('current', ROOT / 'src/cpu/sse_math.cpp')):
        executable = OUT / (label + '-native')
        arguments = flags + [str(contract_path), str(source), 'src/cpu/x87_math.cpp'] + archives + ['-o', str(executable)]
        command(arguments, OUT / (label + '-native-build.log'))
        results['native'][label] = capture_binary([str(executable), '--capture'], OUT / (label + '-native.bin.gz'))
        if results['native'][label]['bytes'] != frozen['expected_native_bytes']:
            raise ValueError('Missing native contract records')
    if results['native']['baseline'] != results['native']['current']:
        raise ValueError('Fresh native full output changed')
    results.update(freeze_sha256=sha(FREEZE), scalar_equal=True, native_equal=True,
                   holdout=True, hardware_equivalence_claimed=False,
                   generated_files={p.name: sha(p) for p in (scalar_path, contract_path, OUT / 'native-inputs.json')})
    (OUT / 'result.json').write_text(json.dumps(results, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation', choices=('freeze', 'run'))
    args = parser.parse_args()
    freeze() if args.operation == 'freeze' else run()
