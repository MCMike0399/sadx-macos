#!/usr/bin/env python3
"""Narrow, local-only PE packet replay; never runs a gate or refreshes a manifest."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/executable-image-packet'
OUT.mkdir(parents=True, exist_ok=True)
SOURCE = ROOT / 'src/loader/executable_image.cpp'
HEADER = ROOT / 'src/loader/executable_image.hpp'
BASE = '1300815eae321d5de8e992958b2841f96ca47039'
records = []


def run(args, expected=0):
    result = subprocess.run(args, cwd=ROOT, text=True, capture_output=True)
    records.append(dict(command=args, exit=result.returncode,
                        stdout=result.stdout, stderr=result.stderr))
    if result.returncode != expected:
        raise RuntimeError(f'expected {expected}, got {result.returncode}: {args}\n{result.stderr}')
    return result


def compile_test(source, test, name, baseline=False, sanitize=False):
    binary = OUT / name
    args = ['xcrun', 'clang++', '-std=c++17', '-O1' if sanitize else '-O2',
            '-Wall', '-Wextra', '-Werror']
    if sanitize:
        args += ['-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    if baseline:
        args += ['-I' + str(OUT / 'baseline')]
    args += ['-Isrc', test, str(source), '-o', str(binary)]
    run(args)
    return str(binary)


try:
    # Exact base header AND implementation, not a new-header/old-body hybrid.
    base_header = OUT / 'baseline/loader/executable_image.hpp'
    base_header.parent.mkdir(parents=True, exist_ok=True)
    for tracked, output in [('src/loader/executable_image.hpp', base_header),
                            ('src/loader/executable_image.cpp', OUT / 'baseline.cpp')]:
        output.write_text(run(['git', 'show', BASE + ':' + tracked]).stdout)
        # Source text is already tracked; avoid duplicating it in the receipt.
        records[-1]['stdout'] = 'sha256 ' + hashlib.sha256(output.read_bytes()).hexdigest()
    for test, stem in [('tests/unit/executable_image.cpp', 'contract'),
                       ('tests/unit/executable_image_edges.cpp', 'edges')]:
        baseline = compile_test(OUT / 'baseline.cpp', test, stem + '-baseline', baseline=True)
        run([baseline, '--valid'])
        if stem == 'contract':
            run([baseline, '--p2'], expected=1)
        current = compile_test(SOURCE, test, stem + '-current')
        run([current])
        run([current, '--invalid-option'], expected=1)
        sanitized = compile_test(SOURCE, test, stem + '-sanitized', sanitize=True)
        run([sanitized])
    mutations = [
        ('ignored-directory', 'return reject(error, "relocation directory outside the image");',
         'return true;', 'tests/unit/executable_image.cpp', '--p2'),
        ('missing-delta', 'const std::uint32_t relocated = *value + delta;',
         'const std::uint32_t relocated = *value + (delta & 0u);',
         'tests/unit/executable_image.cpp', '--valid'),
        ('wrong-export', 'entry.va = base_ + *function;',
         'entry.va = base_ + *function + 1;', 'tests/unit/executable_image.cpp', '--valid'),
        ('wrong-fallback', '*original ? *original : *first;',
         '*original ? *original : 0;', 'tests/unit/executable_image.cpp', '--valid'),
        ('wrong-zero-tail', 'return std::uint8_t{0};',
         'return std::uint8_t{1};', 'tests/unit/executable_image.cpp', '--valid'),
        ('wrapping-target', 'std::uint64_t(*page) + (*entry & 0xfff)',
         'std::uint32_t(*page + (*entry & 0xfff))',
         'tests/unit/executable_image.cpp', '--malformed'),
    ]
    source_text = SOURCE.read_text()
    for name, old, new, test, mode in mutations:
        if source_text.count(old) != 1:
            raise RuntimeError('mutation anchor is not unique: ' + name)
        candidate = OUT / (name + '.cpp')
        candidate.write_text(source_text.replace(old, new))
        binary = compile_test(candidate, test, name)
        run([binary, mode], expected=1)
    # Exercise actual registered targets and CLI; this does not build archives.
    run(['make', '-j1', 'build/executable-image-test', 'build/executable-image-edges-test'])
    run(['./build/executable-image-test'])
    run(['./build/executable-image-edges-test'])
    generated = (ROOT / 'build/tests.mk').read_text()
    for name in ['executable-image', 'executable-image-edges']:
        if f'./build/{name}-test' not in generated:
            raise RuntimeError('missing generated registration: ' + name)
    print('PE packet: baseline, malformed boundaries, sanitizers, six mutants and registration passed')
finally:
    inputs = ['src/loader/executable_image.cpp', 'src/loader/executable_image.hpp',
              'tests/unit/executable_image.cpp', 'tests/unit/executable_image_edges.cpp',
              'tests/unit/manifest.tsv', 'Makefile',
              'docs/packets/executable-image/qualify.py']
    receipt = dict(base=BASE, source_commit='fc4be98',
                   inputs={p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in inputs},
                   records=records,
                   limits='Narrow local checks only; no gate, provider, download, manifest refresh, '
                          'Windows parity, fresh independent review or campaign closure.')
    (OUT / 'qualification.json').write_text(json.dumps(receipt, indent=2) + '\n')
