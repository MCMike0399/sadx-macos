"""Build and qualify the isolated C++17 collision packet without shared build writes.

Source mutations are compiled from private copies. Original game inputs are read-only.
The main gate, apps, build registration and independent review remain Root-owned.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
CPP = ['src/engine/collision_component.cpp', 'src/math/ninja_length.cpp',
       'src/engine/task.cpp', 'src/engine/task_work.cpp', 'src/cpu/x87_math.cpp']
FLAGS = ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-Wmissing-field-initializers',
         '-ffp-contract=off', '-fno-fast-math', '-Isrc']
MUTANTS = [
    ('sphere_extent', CPP[0], 'return exact(context, center_length) + info.a;',
     'return exact(context, center_length) + info.b;'),
    ('capsule_extent', CPP[0], 'return (exact(context, center_length) + info.b) + info.a;',
     'return exact(context, center_length) + info.a;'),
    ('cylinder_ignores_c', CPP[0],
     'info.form == Form::Cylinder || info.form == Form::CylinderAlternate', 'false'),
    ('range_fallback', CPP[0], 'compare(0.1f)', 'compare(25.0f)'),
    ('length_sum_order', CPP[1], '((zz + yy) + xx).rounded()', '((xx + yy) + zz).rounded()'),
    ('clear_primary_status', CPP[0], 'status_flags &= kRetainedPrimaryFlags;',
     'status_flags |= kRetainedPrimaryFlags;'),
    ('allocation_orphan', CPP[0], 'free_collision(arena, task);',
     'free_collision(arena, task);\n        arena.release(allocation.work);'),
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command(args, log, allowed=(0,)):
    with log.open('w') as stream:
        completed = subprocess.run([str(a) for a in args], cwd=ROOT, stdout=stream,
                                   stderr=subprocess.STDOUT)
    if completed.returncode not in allowed:
        raise RuntimeError(f'{log}: exit {completed.returncode}')
    return completed.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--reference-python', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/collision-component-core')
    parser.add_argument('--skip-mutants', action='store_true')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    compiler = os.environ.get('CXX', 'clang++')
    archive = out / 'softfloat/softfloat.a'
    command(['sh', 'third_party/softfloat/build.sh', archive.parent], out / 'softfloat.log')

    def library(destination, replacements=None):
        sources = [(replacements or {}).get(p, p) for p in CPP]
        command([compiler, *FLAGS, '-fPIC', '-dynamiclib',
                 'tests/probes/native_collision_component_probe.cpp', *sources, archive,
                 '-o', destination], destination.with_suffix('.build.log'))

    library(out / 'probe.dylib')
    command([compiler, *FLAGS, 'tests/unit/collision_component.cpp', *CPP,
             'src/player/player_startup.cpp', 'src/player/player_state.cpp', archive,
             '-o', out / 'unit'], out / 'unit.build.log')
    command([out / 'unit'], out / 'unit.log')

    def oracle(library_path, report, extra=()):
        # Import paths use the existing harness; no shared library or report is modified.
        env = os.environ.copy()
        env['PYTHONPATH'] = str(ROOT / 'tools/lib') + os.pathsep + str(ROOT / 'tools/oracles')
        with report.with_suffix('.log').open('w') as stream:
            result = subprocess.run([str(args.reference_python),
                'tools/oracles/verify_original_collision_component.py', str(args.exe),
                str(library_path), '--output', str(report), *extra], cwd=ROOT, env=env,
                stdout=stream, stderr=subprocess.STDOUT)
        if result.returncode not in (0, 1, 2):
            raise RuntimeError(f'{report}: oracle crashed, exit {result.returncode}')
        payload = json.loads(report.read_text())
        return result.returncode, payload

    dev_rc, dev = oracle(out / 'probe.dylib', out / 'dev.json')
    if dev['value_mismatches']:
        raise RuntimeError('development mismatch')
    mutants = []
    if not args.skip_mutants:
        for name, source, before, after in MUTANTS:
            text = (ROOT / source).read_text()
            if text.count(before) != 1:
                raise RuntimeError(f'mutant {name}: expected one exact site')
            folder = out / 'mutants' / name
            folder.mkdir(parents=True, exist_ok=True)
            candidate = folder / Path(source).name
            candidate.write_text(text.replace(before, after))
            lib = folder / 'probe.dylib'
            library(lib, {source: candidate})
            rc, report = oracle(lib, folder / 'verdict.json',
                                () if name == 'allocation_orphan' else ('--numeric-only',))
            mutants.append(dict(name=name, oracle_exit=rc, value_mismatches=report['value_mismatches'],
                                caught=report['value_mismatches'] > 0))
            print(json.dumps(mutants[-1]), flush=True)
        if not all(row['caught'] for row in mutants):
            raise RuntimeError('surviving mutant')
    # Untouched holdout is run only after the production implementation and dev checks freeze.
    holdout_rc, holdout = oracle(out / 'probe.dylib', out / 'holdout.json', ('--holdout',))
    if holdout['value_mismatches']:
        raise RuntimeError('holdout mismatch')
    summary = dict(compiler=compiler, compiler_flags=FLAGS, private_output=str(out),
        source_hashes={p: digest(ROOT / p) for p in CPP + [
            'src/engine/collision_component.hpp', 'src/math/ninja_length.hpp',
            'src/engine/task_work.hpp', 'src/cpu/fidelity_float.hpp',
            'tests/unit/collision_component.cpp', 'tests/probes/native_collision_component_probe.cpp',
            'tools/oracles/verify_original_collision_component.py']},
        unit_exit=0, dev_exit=dev_rc, holdout_exit=holdout_rc,
        dev={k: dev[k] for k in ('cases', 'value_mismatches', 'fp_partials', 'scalar')},
        holdout={k: holdout[k] for k in ('cases', 'value_mismatches', 'fp_partials', 'scalar')},
        mutants=mutants, full_gate='not run; Root owned', apps='not run',
        verdict='partial' if dev_rc == 2 or holdout_rc == 2 else 'pass')
    (out / 'proof.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary), flush=True)


if __name__ == '__main__':
    main()
