"""Rerun the immutable reviewed strip packet using only this worktree's private outputs."""
import argparse
import hashlib
import importlib.util
import json
import os
import subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--strip-packet', type=Path, required=True)
    p.add_argument('--python', type=Path, required=True)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--softfloat', type=Path, required=True)
    a = p.parse_args()
    build = ROOT/'build/polybuff-strip-dependency'
    build.mkdir(parents=True, exist_ok=True)
    engine = ROOT/'src/engine/normal_uv_strip.cpp'
    assert engine.read_bytes() == (a.strip_packet/'src/engine/normal_uv_strip.cpp').read_bytes()
    assert (ROOT/'src/engine/normal_uv_strip.hpp').read_bytes() == \
        (a.strip_packet/'src/engine/normal_uv_strip.hpp').read_bytes()
    spec = importlib.util.spec_from_file_location('frozen_strip_checker',
                 a.strip_packet/'tools/oracles/check_normal_uv_strip_packet.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    flags = ['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror',
             '-ffp-contract=off','-fno-fast-math','-Isrc','-I'+str(a.strip_packet/'tests/probes')]
    common = [a.strip_packet/'tests/probes/native_normal_uv_strip_probe.cpp',
              'src/cpu/x87_math.cpp',a.softfloat]
    env = dict(os.environ,PYTHONPATH=str(a.strip_packet/'tools/lib')+':'+
                                   str(a.strip_packet/'tools/oracles'))
    def run(cmd, name):
        r = subprocess.run([str(x) for x in cmd],cwd=ROOT,env=env,text=True,
                           stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (build/(name+'.log')).write_text(r.stdout)
        return r.returncode
    lib = build/'probe.dylib'
    assert run(flags+['-dynamiclib',engine]+common+['-o',lib],'compile') == 0
    unit = build/'unit'
    assert run(flags+[a.strip_packet/'tests/unit/normal_uv_strip.cpp',engine]+common+
               ['-o',unit],'unit-compile') == 0
    assert run([unit],'unit') == 0
    def judge(lib, name, seed, count):
        report = build/(name+'.json')
        code = run([a.python,a.strip_packet/'tools/oracles/verify_original_normal_uv_strip.py',
                    a.exe,lib,'--output',report,'--seed',seed,'--cases',count],name)
        return code,json.loads(report.read_text())
    dev_code,dev = judge(lib,'dev',783590,400)
    assert dev_code == 2 and dev['values_match'] and dev['flag_mismatches'] == 154
    mutants = {}
    source = engine.read_text()
    for name,(before,after) in module.MUTANTS.items():
        assert source.count(before) == (2 if name == 'header-mask' else 1),name
        variant,variant_lib = build/(name+'.cpp'),build/(name+'.dylib')
        variant.write_text(source.replace(before,after))
        assert run(flags+['-dynamiclib',variant]+common+['-o',variant_lib],name+'-compile') == 0
        code,report = judge(variant_lib,name,783590,80)
        mutants[name] = {'killed':code == 1 and report['value_mismatches'] > 0,
                         'value_mismatches':report['value_mismatches']}
        assert mutants[name]['killed']
    holdout_code,holdout = judge(lib,'holdout',26540917,2048)
    assert holdout_code == 2 and holdout['values_match'] and holdout['flag_mismatches'] == 198
    result = {'unit':(build/'unit.log').read_text().strip(), 'dev':dev,'holdout':holdout,
              'dev_exit_status':dev_code,'holdout_exit_status':holdout_code,'mutants':mutants,
              'engine_sha256':hashlib.sha256(engine.read_bytes()).hexdigest(),
              'limits':['Unchanged component qualification; runtime/owner binding is judged separately.']}
    (build/'proof.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['unit','dev_exit_status','holdout_exit_status','mutants']},indent=2))
if __name__ == '__main__': main()
