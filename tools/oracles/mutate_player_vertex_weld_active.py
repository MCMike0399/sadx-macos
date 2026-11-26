#!/usr/bin/env python3
"""Compile bounded active-weld faults and run the unchanged differential oracle.

Each spec under reference/production-integration-20261110/ names one textual
fault in src/player/player_vertex_weld.cpp. The runner links the probe against
the maintained native archive with the original weld object removed, so the
faulted translation unit is the only implementation under test.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
ARCHIVE = ROOT / 'build/libsadx-native.a'
SOFTFLOAT = [ROOT / 'build/softfloat_sse/softfloat-sse.a', ROOT / 'build/softfloat/softfloat.a',
             ROOT / 'build/bochs_fpatan/libbochs_fpatan.a']
FLAGS = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
         '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64',
         '-mmacosx-version-min=14.0', '-Isrc', '-Itests/fixtures']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('--spec', default='reference/production-integration-20261110/'
                                          'player-vertex-weld-active-mutants.json')
    parser.add_argument('--output', default='reference/production-integration-20261110/'
                                            'player-vertex-weld-active-mutants-results.json')
    parser.add_argument('--active-cases', type=int, default=64)
    parser.add_argument('--cases', type=int, default=8)
    args = parser.parse_args()
    spec = json.loads((ROOT / args.spec).read_text())
    source_path = ROOT / spec['source']
    source = source_path.read_text()
    directory = ROOT / 'build/weld-active-mutants'
    if directory.exists():
        shutil.rmtree(directory)
    directory.mkdir(parents=True)
    # Extract the maintained archive without the weld object under test.
    extracted = directory / 'archive'
    extracted.mkdir()
    subprocess.run(['ar', '-x', str(ARCHIVE)], cwd=extracted, check=True)
    (extracted / 'player_vertex_weld.o').unlink()
    base_archive = directory / 'base.a'
    objects = sorted(str(path) for path in extracted.glob('*.o'))
    subprocess.run(['ar', '-rcs', str(base_archive), *objects], cwd=extracted, check=True)
    results = []
    for mutant in spec['mutants']:
        before, after = mutant['before'], mutant['after']
        if before not in source:
            results.append({'name': mutant['name'], 'error': 'source pattern absent'})
            print(mutant['name'], 'PATTERN ABSENT', flush=True)
            continue
        changed = source.replace(before, after, 1)
        product = directory / f"{mutant['name']}.cpp"
        product.write_text(changed)
        obj = directory / f"{mutant['name']}.o"
        compiled = subprocess.run(FLAGS + ['-c', str(product), '-o', str(obj)], cwd=ROOT,
                                  capture_output=True, text=True)
        row = {'name': mutant['name'], 'compile_exit': compiled.returncode,
               'compile_stderr': compiled.stderr, 'before': before, 'after': after}
        if compiled.returncode == 0:
            probe = directory / f"{mutant['name']}.dylib"
            linked = subprocess.run(FLAGS + ['-dynamiclib',
                str(ROOT / spec['probe']), str(obj), str(base_archive), *map(str, SOFTFLOAT),
                '-o', str(probe)], cwd=ROOT, capture_output=True, text=True)
            row.update(link_exit=linked.returncode, link_stderr=linked.stderr)
            if linked.returncode == 0:
                report = directory / f"{mutant['name']}.json"
                python = ROOT / 'reference/python/bin/python'
                command = [str(python) if python.exists() else sys.executable,
                           str(ROOT / spec['oracle']), args.exe, str(probe),
                           '--cases', str(args.cases), '--active-cases', str(args.active_cases),
                           '--output', str(report)]
                check = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                                       env={'PYTHONPATH': 'tools/lib:tools/oracles',
                                            'PATH': '/usr/bin:/bin'})
                row.update(check_exit=check.returncode, stdout=check.stdout,
                           stderr=check.stderr)
                if report.exists():
                    data = json.loads(report.read_text())
                    row['active_mismatch_count'] = data['active']['mismatch_count']
                    row['mismatch_count'] = data['mismatch_count']
                row['caught'] = (check.returncode == 1 and
                                 row.get('active_mismatch_count', 0) > 0)
        else:
            row['caught'] = False
        results.append(row)
        print(mutant['name'], 'caught' if row.get('caught') else 'NOT CAUGHT', flush=True)
    output = ROOT / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps({'all_caught': all(row.get('caught') for row in results),
                                  'faults': results}, indent=2) + '\n')
    return 0 if all(row.get('caught') for row in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
