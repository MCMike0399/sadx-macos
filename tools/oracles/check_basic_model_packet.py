"""Private <=2-job basic-model build, original oracle, holdout and value mutants.

Builds source dependencies into this worktree only. No Root Make/registration,
legacy judge/probe edits, shared build products or runtime app invocation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import os

ROOT = Path(__file__).resolve().parents[2]
MUTANTS = [
    ('normalized-material', 'float(color & kColorByteMask)', 'float(color & kColorByteMask) / 255.0f'),
    ('destination-blend-field', '(attributes >> 26) & kBlendIndexMask',
     '(attributes >> 29) & kBlendIndexMask'),
    ('environment-off-skipped', '} else if (has(saved_flags, RenderFlag::Environment)) {',
     '} else if (false && has(saved_flags, RenderFlag::Environment)) {'),
    ('callback-reloaded', '                callback(context);', '                snapshot_material_callback(context)(context);'),
    ('reset-world-alias', 'set_world_transform(context, owners.reset_world);',
     'set_world_transform(context, owners.inverse_view);'),
    ('constant-channel-order', 'owners.constant_material[3 - index]) * 255.0f',
     'owners.constant_material[index]) * 255.0f'),
    ('environment-generator', 'kCameraSpaceNormalGenerator = 1U << 16',
     'kCameraSpaceNormalGenerator = 1U << 17'),
]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--python', required=True, type=Path)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/basic-model-core')
    parser.add_argument('--mutants', action='store_true')
    args = parser.parse_args()
    out = args.output.resolve()
    if not out.is_relative_to(ROOT / 'build'):
        parser.error('output must stay inside this worktree/build')
    out.mkdir(parents=True, exist_ok=True)
    def run(command, log, expected=0, env=None):
        with log.open('w') as stream:
            result = subprocess.run([str(x) for x in command], cwd=ROOT, env=env,
                                    stdout=stream, stderr=subprocess.STDOUT)
        if result.returncode != expected:
            raise RuntimeError(f'{command[0]} exited {result.returncode}: {log}')
        return result.returncode
    flags = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
             '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-fPIC']
    softfloat = out / 'softfloat/softfloat.a'
    if not softfloat.exists():
        run(['sh', 'third_party/softfloat/build.sh', out / 'softfloat'], out / 'softfloat-build.log')
    objects = []
    sources = ['src/engine/basic_model.cpp', 'tests/probes/native_basic_model_probe.cpp',
               'src/cpu/x87_math.cpp', 'src/cpu/crt_integer.cpp', 'src/engine/character_pack.cpp']
    for source in sources:
        obj = out / (Path(source).stem + '.o')
        run(flags + ['-c', source, '-o', obj], obj.with_suffix('.build.log'))
        objects.append(obj)
    def link(core, library):
        run(['xcrun', 'clang++', '-dynamiclib', core, *objects[1:], softfloat, '-o', library],
            library.with_suffix('.link.log'))
    def judge(library, report, holdout=False, expected=2):
        command = [args.python, 'tools/oracles/verify_original_basic_model.py', args.exe,
                   library, '--output', report]
        if holdout:
            command += ['--cases', '1800', '--seed', '7914850']
        run(command, report.with_suffix('.log'), expected)
        return json.loads(report.read_text())
    library = out / 'probe.dylib'
    link(objects[0], library)
    dev = judge(library, out / 'dev.json')
    holdout = judge(library, out / 'holdout.json', holdout=True)
    run(flags + ['tests/unit/basic_model.cpp', objects[0], *objects[2:], softfloat,
                 '-o', out / 'unit-test'], out / 'unit-build.log')
    run([out / 'unit-test'], out / 'unit.log')
    mutants = []
    if args.mutants:
        source = (ROOT / sources[0]).read_text()
        for name, before, after in MUTANTS:
            assert source.count(before) == 1, name
            directory = out / 'mutants' / name
            directory.mkdir(parents=True, exist_ok=True)
            cpp, obj, library = directory / 'basic_model.cpp', directory / 'core.o', directory / 'probe.dylib'
            cpp.write_text(source.replace(before, after))
            run(flags + ['-c', cpp, '-o', obj], directory / 'build.log')
            link(obj, library)
            report = judge(library, directory / 'verdict.json', expected=1)
            assert report['mismatch_count'] > 0 and not report['values_match'], name
            mutants.append(dict(name=name, value_mismatches=report['mismatch_count'],
                                exit_status=1, source_sha256=hashlib.sha256(cpp.read_bytes()).hexdigest()))
    def summary(report):
        return {key: report[key] for key in ('cases', 'mismatch_count', 'fp_status_mismatches',
                                            'values_match', 'passed', 'exit_status', 'case_sha256')}
    result = dict(dev=summary(dev), holdout=summary(holdout), unit_exit_status=0, mutants=mutants,
                  limits=dev['limits'])
    (out / 'summary.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
