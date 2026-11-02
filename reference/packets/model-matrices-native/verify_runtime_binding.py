"""Qualify exact Root patches in private compiled source/header fixtures.

No shared source file, Main input/cache, or checkout is modified. Each compiled
native executable has one Session model-matrix owner. Build concurrency is two.
"""
import concurrent.futures
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/model-matrices-native/runtime-binding'
PACKET = ROOT / 'reference/packets/model-matrices-native'
FLAGS = ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
         '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0']
FRAMEWORKS = ['Foundation', 'AppKit', 'Metal', 'MetalKit', 'QuartzCore', 'CoreGraphics',
              'ImageIO', 'GameController', 'AudioToolbox', 'AVFoundation', 'CoreMedia', 'CoreVideo']

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def verify_proof(proof_path):
    proof = json.loads(proof_path.read_text())
    assert proof['source_sha256'] == '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
    assert proof['exit_status'] == 0 and proof['inputs_unchanged']
    assert proof['getter_index'] == 54 and proof['getter_result'] == 0 and proof['destination_preserved']
    at_get = next(row for row in proof['snapshots'] if row['phase'] == 'first_gamma_get_entry')
    assert all(at_get[field] == 0 for field in ('count', 'limit', 'producer_calls',
               'buffer_nonzero_words', 'gamma_nonzero_words', 'weld_nonzero_words'))
    raw_path = ROOT / proof['private_output'] / 'original.json'
    assert sha(raw_path) == proof['raw_report_sha256']
    raw = json.loads(raw_path.read_text())
    assert raw['finished'] and raw['getter_index'] == 54 and raw['getter_result'] == 0
    assert raw['destination_before'] == raw['destination_after']
    entry = next(row for row in raw['snapshots'] if row['phase'] == 'first_gamma_get_entry')
    assert entry['count'] == entry['limit'] == entry['producer_calls'] == 0
    assert len(entry['buffer']) == 2048 and not any(entry['buffer'])
    for name, count in [('gamma_destinations', 5), ('weld_scratch', 4)]:
        assert len(entry[name]) == count
        assert all(len(matrix) == 16 and not any(matrix) for matrix in entry[name])
    assert raw['getter_indices'] == [54, 30, 39, 2, 37]
    tail = next(row for row in raw['snapshots'] if row['phase'] == 'first_gamma_fan_entry')
    assert tail['count'] == tail['limit'] == tail['producer_calls'] == 0
    assert tail['stack_depth'] == entry['stack_depth'] == 1
    expected = [[0] * 16 for _ in range(5)]
    expected[2][4:8] = [2147483648] * 4
    assert tail['gamma_destinations'] == expected

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--proof', type=Path,
                        default=ROOT / 'build/model-matrices-native/initialization-through-fan/summary.json')
    parser.add_argument('--check-proof-only', action='store_true')
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    proof_path = args.proof
    verify_proof(proof_path)
    if args.check_proof_only:
        print('Original first Gamma getter state supports zero owner initialization; exact raw snapshot hash verified')
        return
    patches = [PACKET / 'ROOT-OWNER.patch', PACKET / 'ROOT-ACTIVATION.patch']
    paths = sorted({line[6:] for patch in patches for line in patch.read_text().splitlines()
                    if line.startswith('--- a/')})
    original = {p: sha(ROOT / p) for p in paths}
    for path in paths:
        target = OUT / path; target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / path, target)
    for patch in patches:
        subprocess.run(['patch', '--batch', '-p1', '-d', str(OUT), '-i', str(patch)], check=True)
    includes = ['-I' + str(OUT / 'src'), '-Isrc', '-Itests/fixtures']
    sources = set((ROOT / 'native-sources.txt').read_text().splitlines())
    sources.update(str(p.relative_to(ROOT)) for pattern in ('src/engine/*_native.cpp', 'src/runtime/native_*.cpp')
                   for p in ROOT.glob(pattern))
    sources = sorted(p for p in sources if p.strip() and not p.startswith('#'))
    def compile_source(path):
        source = OUT / path if path in paths else ROOT / path
        obj = OUT / 'objects' / Path(path).relative_to('src').with_suffix('.o')
        obj.parent.mkdir(parents=True, exist_ok=True)
        extra = ['-Ithird_party/bochs_fpatan', '-Ithird_party/bochs_fpatan/source',
                 '-Ithird_party/bochs_fpatan/source/softfloat3e/include'] if path == 'src/cpu/x87_transcendental.cpp' else []
        command = ['xcrun', 'clang++', *FLAGS, *includes, *extra, '-c', str(source), '-o', str(obj)]
        run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        obj.with_suffix('.log').write_text(run.stdout + run.stderr)
        if run.returncode:
            raise RuntimeError(f'{path}: {run.stderr}')
        return {'path': path, 'source_sha256': sha(source), 'object': str(obj), 'command': command}
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        compiled = list(pool.map(compile_source, sources))
    archive = OUT / 'libsadx-native.a'
    libraries = ['build/softfloat_sse/softfloat-sse.a', 'build/softfloat/softfloat.a', 'build/bochs_fpatan/libbochs_fpatan.a']
    subprocess.run(['xcrun', 'libtool', '-static', '-o', str(archive),
                    *[row['object'] for row in compiled], *libraries], cwd=ROOT, check=True)
    platforms = ['build/obj/platform/mac/' + name + '.o' for name in
                 ['mac_clock', 'mac_audio', 'hid_switch_controller', 'mac_movie', 'metal_device_frame',
                  'mac_audio_playback', 'mac_sound_bank', 'mac_sound_effects', 'mac_save_files', 'mac_save_writer']]
    frameworks = [part for name in FRAMEWORKS for part in ('-framework', name)]
    links = []
    for force in (False, True):
        binary = OUT / ('sadx-native-closure' if force else 'sadx-native')
        command = ['xcrun', 'clang++', *FLAGS, '-fobjc-arc', *includes, 'src/apps/sadx_native.mm',
                   *platforms, '-Wl,-force_load,' + str(archive) if force else str(archive),
                   *frameworks, '-o', str(binary)]
        subprocess.run(command, cwd=ROOT, check=True)
        links.append({'command': command, 'exit_status': 0, 'binary_sha256': sha(binary)})
    assert original == {p: sha(ROOT / p) for p in paths}
    summary = {'shared_source_unchanged': True, 'source_initialization_proof_sha256': sha(proof_path),
               'patches': {str(p.relative_to(ROOT)): sha(p) for p in patches},
               'max_parallel_compiles': 2, 'compiled_sources': compiled, 'links': links}
    (OUT / 'build.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(f'Private Root-patch native app and force-load closure: {len(compiled)} sources, shared files unchanged')
if __name__ == '__main__': main()
