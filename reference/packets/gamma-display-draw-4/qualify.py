"""Focused reproducible qualification. Compiled faults stay private, never in game code."""
import json
import os
from pathlib import Path
import subprocess

OUT = Path('build/draw-path-4/qualification')
OUT.mkdir(parents=True, exist_ok=True)
ENV = dict(os.environ, PYTHONPATH='tools/lib:tools/oracles')
rows = []

def run(name, command):
    p = subprocess.run(command, capture_output=True, text=True, env=ENV)
    log = OUT / (name+'.log')
    log.write_text(p.stdout+p.stderr)
    rows.append(dict(name=name, command=command, exit=p.returncode, log=str(log)))
    (OUT/'commands.json').write_text(json.dumps(rows, indent=2)+'\n')
    print(name, p.returncode, flush=True)
    return p.returncode

common = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
          '-ffp-contract=off', '-fno-fast-math', '-dynamiclib', '-Isrc',
          'tests/probes/native_normalize_probe.cpp']
libraries = ['src/cpu/sse_estimate.cpp', 'src/cpu/sse_math.cpp', 'build/softfloat_sse/softfloat-sse.a', 'build/softfloat/softfloat.a']
oracle = ['reference/python/bin/python', 'tools/oracles/verify_original_normalize.py',
          'reference/updated-pc-reference/game/sonic.exe']
run('build', ['make','-j2','-o','Makefile','-o','build/native-controller-deps/include/SDL3/SDL.h',
              'build/sadx-native','build/native-normalize-probe.dylib','build/vector-normalize-test',
              'build/cs-private-assets/.private-native-assets.json'])
run('unit', ['./build/vector-normalize-test'])
for name, options in [('development', []), ('holdout', ['--holdout'])]:
    run(name, oracle+['build/native-normalize-probe.dylib', '--output',str(OUT/(name+'.json'))]+options)
source = Path('src/engine/vector_normalize.cpp').read_text()
faults = {
    'threshold': ('word(1.4210855e-14f)', 'word(3.5527137e-15f)'),
    'missing-refinement': ('const auto scale = arithmetic.multiply(half_reciprocal, correction);',
                           'const auto scale = reciprocal_length; (void)half_reciprocal; (void)correction;'),
    'missing-z-store': ('store(output[2], arithmetic.multiply(scale, z));', '(void)z;'),
}
for name, (before, after) in faults.items():
    if source.count(before) != 1: raise RuntimeError('fault location drift')
    path = OUT/(name+'.cpp'); path.write_text(source.replace(before, after))
    lib = OUT/(name+'.dylib')
    if run(name+'-build', common+[str(path)]+libraries+['-o',str(lib)]):
        raise RuntimeError('fault did not compile')
    run(name, oracle+[str(lib),'--output',str(OUT/(name+'.json'))])
    report=json.loads((OUT/(name+'.json')).read_text())
    if not report['value_mismatches']: raise RuntimeError('compiled fault not detected in values')
run('verdict', ['python3','tools/oracles/verdict.py',str(OUT/'development.json'),str(OUT/'holdout.json')])
run('profile', ['python3','tools/gate/check_profile_patches.py','--build-dir',str(OUT),'--json'])
run('check', ['make','-j2','check'])
