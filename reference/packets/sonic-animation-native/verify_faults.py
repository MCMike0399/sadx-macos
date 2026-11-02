"""Compile independent faults in disposable local copies; never edit accepted sources."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
FLAGS = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
         '-ffp-contract=off', '-fno-fast-math', '-Isrc']


def run(directory):
    output = ROOT / 'build/sonic-animation-native/faults'
    output.mkdir(parents=True, exist_ok=True)
    assets = (ROOT / 'src/runtime/sonic_animation_assets.cpp').read_text()
    binding = (ROOT / 'src/runtime/native_sonic_binding.cpp').read_text()
    faults = [
        ('speed', assets, '    table_ = std::move(table);',
         '    table->records.at(0).frame_step = 1.0f;\n    table_ = std::move(table);', False),
        ('stand-identity', assets, 'animation::initialize_sonic_table(*table, exports)',
         'animation::initialize_sonic_table(*table, (exports[1] = exports[5], exports))', False),
        ('motion-length', assets, 'animation::Motion{motion.frame_count, motion.channels,',
         'animation::Motion{motion.frame_count + 1, motion.channels,', False),
        ('key-count', assets, 'node.counts[channel] = word(tracks, 12 + index * 16 + channel * 4);',
         'node.counts[channel] = word(tracks, 12 + index * 16 + channel * 4) + 1;', False),
        ('table-copy', binding,
         'native_fields::animation_table(physics(p).animation_tail) = *context.animation_table;',
         'native_fields::animation_table(physics(p).animation_tail) = '
         'std::make_shared<animation::Table>(*std::dynamic_pointer_cast<const animation::Table>('
         '*context.animation_table));', True),
    ]
    results = []
    for name, source, old, new, is_binding in faults:
        assert source.count(old) == 1
        copy = output / (name + '.cpp')
        copy.write_text(source.replace(old, new))
        obj = copy.with_suffix('.o')
        subprocess.run(FLAGS + ['-c', str(copy), '-o', str(obj)], cwd=ROOT, check=True)
        binary = output / name
        command = FLAGS + ['-fobjc-arc', 'tests/unit/sonic_animation_assets.mm', str(obj)]
        if is_binding:
            command.append('build/sonic-animation-native/assets.o')
        command += ['build/libsadx-native.a', '-framework', 'Foundation', '-o', str(binary)]
        subprocess.run(command, cwd=ROOT, check=True)
        result = subprocess.run([str(binary), str(directory), str(output / (name + '.tsv'))],
                                cwd=ROOT, capture_output=True, text=True)
        assert result.returncode != 0, name + ' survived'
        results.append(dict(name=name, compiled=True, exit=result.returncode, caught=True,
                            diagnostic=result.stderr.strip()))
    # Preserve the initial source candidate's default-null construction as a
    # separate before witness for the newly added seventh invalid-input case.
    overlay = output / 'default-null-include/runtime'
    overlay.mkdir(parents=True, exist_ok=True)
    old_header = subprocess.run(['git', 'show',
        'a77d7110:src/runtime/sonic_animation_assets.hpp'], cwd=ROOT,
        check=True, capture_output=True).stdout
    (overlay / 'sonic_animation_assets.hpp').write_bytes(old_header)
    old_flags = FLAGS + ['-I' + str(overlay.parent)]
    # The overlay must precede src, including for quoted includes from the test.
    old_flags = [argument for argument in old_flags if argument != '-Isrc'] + ['-Isrc']
    obj = output / 'default-null.o'
    subprocess.run(old_flags + ['-c', 'src/runtime/sonic_animation_assets.cpp', '-o', str(obj)],
                   cwd=ROOT, check=True)
    binary = output / 'default-null'
    subprocess.run(old_flags + ['-fobjc-arc', 'tests/unit/sonic_animation_assets.mm', str(obj),
        'build/libsadx-native.a', '-framework', 'Foundation', '-o', str(binary)], cwd=ROOT, check=True)
    result = subprocess.run([str(binary), str(directory), str(output / 'default-null.tsv')],
                            cwd=ROOT, capture_output=True, text=True)
    assert result.returncode != 0 and 'invalid declaration must fail' in result.stderr
    results.append(dict(name='default-null-before', compiled=True, exit=result.returncode,
        caught=True, diagnostic=result.stderr.strip(), before_source='a77d7110',
        old_inputs='six previous typed negatives and all JSON inputs retained; seventh adds unfilled array'))
    return results


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('imported', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    results = run(args.imported.resolve())
    args.output.write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps(results, indent=2))
