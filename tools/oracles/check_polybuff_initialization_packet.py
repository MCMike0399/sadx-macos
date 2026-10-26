"""Private sequential candidate verification; never runs apps, gate or Ghidra."""
import argparse
import hashlib
import json
import subprocess
import struct
import sys
import re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
MUTANTS = {
    'omit-ninth-record': ('index != kBufferCount;', 'index != kBufferCount - 1;'),
    'wrong-scratch-capacity': ('vertex_count * kScratchVertexBytes', 'vertex_count * 32U'),
    'wrong-count-owner-slot': ('configured_count(context, index)',
                              'configured_count(context, (index + 1) % kBufferCount)'),
    'retain-null-base': ('retained.value = base;', 'retained.value = nullptr;\n    (void)base;'),
    'short-scratch-clear': ('index != kScratchWords;', 'index != kScratchWords - 1;'),
}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--python', required=True, type=Path)
    parser.add_argument('--softfloat', required=True, type=Path)
    args = parser.parse_args()
    sys.path.insert(0, str(ROOT / 'tools/lib'))
    from pe_image import PEImage
    image = PEImage(args.exe)
    entries = re.findall(r'\{(\d+), (\d+), "([^"]+)"\}',
                         (ROOT / 'src/engine/polybuff_initialization.cpp').read_text())
    words = struct.unpack('<27I', image.read(0x389d888, 108))
    expected = [(str(words[i]), str(words[9+i]),
                 image.read(words[18+i], 64).split(b'\0')[0].decode()) for i in range(9)]
    assert entries == expected, 'maintained table differs from pinned source'
    table_hash = hashlib.sha256(image.read(0x389d888, 108)).hexdigest()
    build = ROOT / 'build/polybuff-initialization'
    build.mkdir(parents=True, exist_ok=True)
    def run(command, name):
        r = subprocess.run([str(x) for x in command], cwd=ROOT, text=True,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (build / (name + '.log')).write_text(r.stdout)
        return r.returncode
    flags = ['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror',
             '-ffp-contract=off','-fno-fast-math','-Isrc']
    engine = ROOT / 'src/engine/polybuff_initialization.cpp'
    dependencies = ['src/engine/normal_uv_strip.cpp',
                    'tests/probes/native_polybuff_initialization_probe.cpp',
                    'src/cpu/x87_math.cpp', args.softfloat]
    library = build / 'probe.dylib'
    assert run(flags + ['-dynamiclib',engine] + dependencies + ['-o',library], 'compile') == 0
    unit = build / 'unit'
    assert run(flags + ['-DPOLYBUFF_INITIALIZATION_NATIVE_BINDING',engine,
               'src/engine/polybuff_initialization_binding.cpp','tests/unit/polybuff_initialization.cpp'] +
               dependencies + ['-o',unit], 'unit-compile') == 0
    assert run([unit], 'unit') == 0
    def judge(lib, name, seed, count):
        path = build / (name + '.json')
        code = run([args.python, 'tools/oracles/verify_original_polybuff_initialization.py',
                    args.exe, lib, '--output',path,'--seed',seed,'--cases',count], name)
        return code, json.loads(path.read_text())
    dev_code, dev = judge(library, 'dev', 789720, 256)
    assert dev_code == 0 and dev['values_match']
    algorithm = ROOT / 'src/engine/polybuff_initialization.hpp'
    source = algorithm.read_text()
    mutants = {}
    for name, (before, after) in MUTANTS.items():
        assert source.count(before) == 1, name
        overlay = build / name
        path, lib = overlay / 'engine/polybuff_initialization.hpp', build / (name+'.dylib')
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(source.replace(before,after))
        variant_flags = flags.copy()
        variant_flags.insert(variant_flags.index('-Isrc'), '-I'+str(overlay))
        assert run(variant_flags + ['-dynamiclib',engine] + dependencies + ['-o',lib],
                   name+'-compile') == 0
        code, report = judge(lib, name, 789720, 32)
        mutants[name] = {'killed': code == 1 and report['value_mismatches'] > 0,
                         'value_mismatches': report['value_mismatches']}
        assert mutants[name]['killed'], name
    holdout_code, holdout = judge(library, 'holdout', 91465327, 1024)
    assert holdout_code == 0 and holdout['values_match']
    summary = {'status':'verified-full-initialization-core',
        'dev_exit_status':dev_code,'holdout_exit_status':holdout_code,
        'dev':dev,'holdout':holdout,'mutants':mutants,
        'description_table_sha256':table_hash,
        'unit':(build/'unit.log').read_text().strip(), 'full_gate_run':False,
        'files': {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
                  [engine,ROOT/'src/engine/polybuff_initialization.hpp',
                   ROOT/'src/engine/polybuff_initialization_binding.cpp']}}
    (build/'proof.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps({k:summary[k] for k in ('status','unit','mutants')},indent=2))
if __name__ == '__main__': main()
