"""Read-only original runtime witness with a private overlay/output directory.

Run only after Root explicitly releases the serialized build/reference slot.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[3]

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('game', type=Path); p.add_argument('system', type=Path)
    p.add_argument('--output', type=Path, default=ROOT / 'build/model-matrices-native/initialization')
    a = p.parse_args(); a.output.mkdir(parents=True, exist_ok=True)
    assert not (a.output / 'overlay').exists(), 'use a fresh private output directory'
    exe = a.game / 'sonic.exe'
    assert sha(exe) == '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
    make_command = ['make', '-j2', '-o', 'build/native-controller-deps/include/SDL3/SDL.h',
                    'build/libsadx-core.a', 'build/libsadx-guest.a']
    input_paths = [exe, *sorted(a.system.glob('*.dll')),
                   a.game / 'mods/.modloader/SADXModLoader.dll',
                   a.game / 'mods/sadx-input-mod/sadx-input-mod.dll',
                   *sorted(a.game.rglob('sadx-speedrun-mod.dll')),
                   *sorted(a.game.glob('*.ini')), *sorted((a.game / 'mods').glob('*.ini')),
                   *sorted((a.game / 'savedata').glob('*'))]
    inputs_before = {str(p.resolve()): sha(p) for p in input_paths if p.is_file()}
    subprocess.run(make_command, cwd=ROOT, check=True)
    unicorn = ROOT / 'reference/python/lib/python3.14/site-packages/unicorn'
    binary = a.output / 'reference-witness'
    command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
               '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
               '-Isrc', '-I' + str(unicorn / 'include'), 'tests/probes/model_matrix_initialization.cpp',
               'build/libsadx-guest.a', 'build/libsadx-core.a', str(unicorn / 'lib/libunicorn.a'),
               '-lpthread', '-o', str(binary)]
    subprocess.run(command, cwd=ROOT, check=True)
    raw = a.output / 'original.json'
    run_command = [str(binary), str(a.game.resolve()), str(a.system.resolve()),
                   str((a.output / 'overlay').resolve()), str(raw.resolve())]
    run = subprocess.run(run_command, cwd=ROOT,
                         capture_output=True, text=True)
    (a.output / 'run.log').write_text(run.stdout + run.stderr)
    summary = {'exit_status': run.returncode, 'source_sha256': sha(exe),
               'witness_source_sha256': sha(ROOT / 'tests/probes/model_matrix_initialization.cpp'),
               'private_output': str(a.output), 'runtime_qualified': False,
               'make_command': make_command, 'compile_command': command, 'run_command': run_command,
               'key_events': [[frame, 13, down] for press in (500, 750, 950, 1050, 1300, 1500, 1700, 1900)
                              for down, frame in ((True, press), (False, press + 1))],
               'input_sha256': inputs_before,
               'inputs_unchanged': inputs_before == {p: sha(Path(p)) for p in inputs_before},
               'binary_sha256': sha(binary)}
    if raw.exists():
        summary['raw_report_sha256'] = sha(raw)
        try:
            result = json.loads(raw.read_text())
        except json.JSONDecodeError:
            result = {}
        summary['finished'] = result.get('finished', False)
        summary['frames'] = result.get('frames')
        if result.get('finished'):
            summary['getter_result'] = result['getter_result']
            summary['getter_index'] = result['getter_index']
            summary['getter_indices'] = result.get('getter_indices', [])
            summary['destination_preserved'] = result['destination_before'] == result['destination_after']
            summary['snapshots'] = [{k: v for k, v in row.items() if k not in ('buffer', 'gamma_destinations', 'weld_scratch')}
                                    | {'buffer_sha256': hashlib.sha256(json.dumps(row['buffer']).encode()).hexdigest(),
                                       'buffer_nonzero_words': sum(x != 0 for x in row['buffer']),
                                       'gamma_nonzero_words': sum(x != 0 for matrix in row['gamma_destinations'] for x in matrix),
                                       'weld_nonzero_words': sum(x != 0 for matrix in row['weld_scratch'] for x in matrix)}
                                    for row in result['snapshots']]
            summary['runtime_qualified'] = run.returncode == 0
    (a.output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary, indent=2))
    return run.returncode
if __name__ == '__main__': raise SystemExit(main())
