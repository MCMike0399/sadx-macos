"""Compile faults in isolated source copies; exercise actual Gamma/accepted Sonic witnesses."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
FLAGS = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
         '-ffp-contract=off', '-fno-fast-math']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('assets', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    directory = ROOT / 'build/gamma-motion/faults'
    directory.mkdir(parents=True, exist_ok=True)
    faults = [
        ('initial-frame', 'src/player/animation/animation_engine.hpp',
         'access.state(v).frame = 0;\n        require_arena',
         'access.state(v).frame = 1;\n        require_arena', 'src/player/player_animation.cpp', False),
        ('live-pose', 'src/player/player_animation.cpp',
         'const auto &a = o.native->rotation;',
         'const auto a = std::array<std::int32_t, 3>{0, 0, 0};', None, False),
        ('decoder-count', 'src/runtime/animation_resources.cpp',
         'node.counts[channel] = word(tracks, 12 + index * 16 + channel * 4);',
         'node.counts[channel] = word(tracks, 12 + index * 16 + channel * 4) + 1;', None, True),
        ('table-identity', 'src/runtime/native_gamma_binding.cpp',
         '.animation_table = table;',
         '.animation_table = std::make_shared<animation::Table>(*table);', None, False),
        ('skip-motion', 'src/runtime/gamma_motion.cpp',
         'return animation::update(view, runtime, fp);',
         '(void)view; (void)runtime; (void)fp; animation::Outcome result; '
         'result.completed = true; return result;', None, False),
        ('charge-alias', 'src/player/player_animation.cpp',
         'return access.read(view.player.source_charge_at_0, phase);',
         'return view.player.motion.speed.x;', None, False),
    ]
    results = []
    for name, relative, old, new, translation_unit, sonic in faults:
        source = (ROOT / relative).read_text()
        assert source.count(old) == 1, name
        include = directory / name / 'include'
        copy = include / Path(relative).relative_to('src')
        copy.parent.mkdir(parents=True, exist_ok=True)
        copy.write_text(source.replace(old, new))
        obj = directory / (name + '.o')
        inputs = ['-I' + str(include), '-Isrc', '-c', translation_unit or str(copy), '-o', str(obj)]
        subprocess.run(FLAGS + inputs, cwd=ROOT, check=True, capture_output=True)
        test = 'tests/unit/sonic_animation_assets.mm' if sonic else 'tests/unit/native_gamma_motion.mm'
        binary = directory / (name + ".bin")
        subprocess.run(FLAGS + ['-Isrc', '-Itests/fixtures', '-fobjc-arc', test, str(obj),
                       'build/libsadx-native.a', '-framework', 'Foundation', '-o', str(binary)],
                       cwd=ROOT, check=True, capture_output=True)
        result = subprocess.run([str(binary), str(args.assets.resolve()), str(directory / (name + '.tsv'))],
                                cwd=ROOT, capture_output=True, text=True)
        assert result.returncode != 0, name + ' survived'
        results.append(dict(name=name, compiled=True, caught=True, exit_status=result.returncode,
                            stderr=result.stderr.strip()))
    args.output.write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
