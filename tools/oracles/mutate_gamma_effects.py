"""Compile seven discriminating faults against the unchanged new original judge."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
DIRECTORY = ROOT / 'build/gamma-effects/mutants'
FAULTS = [
    ('retained-frame-pointer', 'src/player/gamma_effects.cpp',
     'asset.burst.data(), nullptr', 'asset.hit.data(), nullptr'),
    ('discard-existing-task', 'src/player/gamma_effects.cpp',
     'if (!state.burst_task)', 'if (true)'),
    ('wrong-pool-capacity', 'src/player/gamma_effects.cpp',
     'scheduler, arena, 40, parameters', 'scheduler, arena, 39, parameters'),
    ('swap-parameters', 'src/player/gamma_effects.cpp',
     'parameters{0.03f, 0.0f}', 'parameters{0.0f, 0.03f}'),
    ('wrong-free-index', 'src/engine/particle_pool.cpp',
     'static_cast<std::uint16_t>(index);', 'static_cast<std::uint16_t>(index + 1);'),
    ('first-texture-for-all', 'src/engine/particle_pool.cpp',
     'textures.entries.at(static_cast<std::size_t>(frame->texture_index))', 'textures.entries.at(0)'),
    ('early-null-buffer-return', 'src/engine/particle_pool.cpp',
     'work->active_indices = allocate_buffer(arena, index_bytes);',
     'work->active_indices = allocate_buffer(arena, index_bytes);\n    if (!work->active_indices) return {};'),
]


def main():
    DIRECTORY.mkdir(parents=True, exist_ok=True)
    results = []
    for name, source, before, after in FAULTS:
        directory = DIRECTORY / name
        directory.mkdir(exist_ok=True)
        text = (ROOT / source).read_text()
        assert text.count(before) == 1, (name, 'fault anchor changed')
        replacement = directory / Path(source).name
        replacement.write_text(text.replace(before, after))
        sources = ['tests/probes/native_gamma_effects_probe.cpp', 'src/player/gamma_effects.cpp',
                   'src/engine/particle_pool.cpp', 'src/engine/task.cpp', 'src/engine/task_work.cpp',
                   'src/cpu/x87_math.cpp', 'src/cpu/crt_integer.cpp', 'build/softfloat/softfloat.a']
        sources = [str(replacement) if item == source else item for item in sources]
        probe = directory / 'probe.dylib'
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-fPIC', '-dynamiclib',
                   *sources, '-o', str(probe)]
        build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (directory / 'build.log').write_text(build.stdout + build.stderr)
        assert build.returncode == 0, name
        report = directory / 'original.json'
        command = [sys.executable, 'tools/oracles/verify_original_gamma_effects.py',
                   '--exe', 'game/Sonic Adventure DX/sonic.exe', '--probe', str(probe),
                   '--output', str(report), '--count', '64']
        run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (directory / 'run.log').write_text(run.stdout + run.stderr)
        observed = json.loads(report.read_text())
        caught = run.returncode == 1 and observed['value_failures'] > 0
        results.append({'name': name, 'caught': caught, 'exit_status': run.returncode,
                        'value_failures': observed['value_failures'], 'cases': observed['cases'],
                        'source_sha256': hashlib.sha256(replacement.read_bytes()).hexdigest(),
                        'probe_sha256': hashlib.sha256(probe.read_bytes()).hexdigest()})
        print(name, 'caught' if caught else 'SURVIVED', flush=True)
        assert caught, name
    output = ROOT / 'reference/packets/gamma-effects-native/mutants.json'
    output.write_text(json.dumps({'mutants': results}, indent=2) + '\n')


if __name__ == '__main__':
    main()
