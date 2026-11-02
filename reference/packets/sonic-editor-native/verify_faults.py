"""Compile isolated faults; never edits maintained source or judge inputs."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
FLAGS = ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
         '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0', '-Isrc']

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('assets', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    work = ROOT / 'build/editor-faults'
    work.mkdir(exist_ok=True)
    results = []
    def run(command, log):
        result = subprocess.run(list(map(str, command)), cwd=ROOT,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        log.write_bytes(result.stdout)
        return result.returncode
    def retain(name, commands, changed, observed, log):
        assert observed != 0, f'{name}: fault escaped'
        results.append(dict(name=name, commands=[list(map(str, c)) for c in commands],
                            changed_source_sha256=sha(changed.encode()), exit=observed,
                            output_sha256=sha(log.read_bytes()), caught=True))
    header = (ROOT / 'src/game/editor_state.hpp').read_text()
    faults = {
        'zero-extended-words': header.replace('std::int16_t', 'std::uint16_t'),
        'normalized-bool': header.replace(
            'return static_cast<std::int32_t>(mission) | static_cast<std::int32_t>(editor);',
            'return (static_cast<std::int32_t>(mission) | static_cast<std::int32_t>(editor)) != 0;'),
        'omit-editor-word': header.replace(
            'static_cast<std::int32_t>(mission) | static_cast<std::int32_t>(editor)',
            'static_cast<std::int32_t>(mission) | (static_cast<std::int32_t>(editor) & 0)'),
    }
    for name, changed in faults.items():
        directory = work / name
        (directory / 'game').mkdir(parents=True, exist_ok=True)
        (directory / 'game/editor_state.hpp').write_text(changed)
        library = directory / 'probe.dylib'
        compile_command = ['xcrun', 'clang++', '-I' + str(directory)] + FLAGS + [
            '-dynamiclib', 'tests/probes/native_editor_state_probe.cpp', '-o', library]
        log = directory / 'compile.log'
        assert run(compile_command, log) == 0, log.read_text()
        report = directory / 'comparison.json'
        command = [sys.executable, Path(__file__).with_name('verify_source.py'), args.exe,
                   library, ROOT / 'build/editor-native.tsv', '--output', report]
        log = directory / 'run.log'
        code = run(command, log)
        assert json.loads(report.read_text())['mismatch_count'] > 0
        retain(name, [compile_command, command], changed, code, log)
    binding = (ROOT / 'src/runtime/native_sonic_binding.cpp').read_text()
    preview = (ROOT / 'src/runtime/native_selection_preview.cpp').read_text()
    gamma = (ROOT / 'src/runtime/native_gamma_binding.cpp').read_text()
    swapped = binding.replace('state.mission_status', 'state.TEMP').replace(
        'state.status', 'state.mission_status').replace('state.TEMP', 'state.status')
    snapshot = binding.replace('[state] {\n        return static_cast<std::int16_t>(state.mission_status);',
        '[value = static_cast<std::int16_t>(state.mission_status)] {\n        return value;').replace(
        '[state] {\n        return static_cast<std::int16_t>(state.status);',
        '[value = static_cast<std::int16_t>(state.status)] {\n        return value;')
    unbound = preview.replace('    sonic::bind_editor_state(context,\n'
        '        {scene.session_.editor_status_, scene.session_.mission_editor_status_});\n', '')
    alternate = preview.replace('    context.alternate_model = &scene.session_.soft_reset_.metal_sonic;\n', '')
    gamma_bool = gamma.replace('return editor::check_mode(*context.editor_state);',
                               'return editor::check_mode(*context.editor_state) != 0;')
    for name, original, changed in [
            ('swapped-native-owners', binding, swapped),
            ('snapshot-native-words', binding, snapshot),
            ('remove-production-editor-binding', preview, unbound),
            ('remove-production-metal-binding', preview, alternate),
            ('normalize-gamma-result', gamma, gamma_bool)]:
        assert changed != original, name
        directory = work / name
        directory.mkdir(exist_ok=True)
        source = directory / 'fault.cpp'
        source.write_text(changed)
        obj = directory / 'fault.o'
        binary = directory / 'test'
        compile_command = ['xcrun', 'clang++'] + FLAGS + ['-c', source, '-o', obj]
        link_command = ['xcrun', 'clang++'] + FLAGS + ['-fobjc-arc',
            'tests/unit/native_editor_binding.mm', obj, 'build/libsadx-native.a',
            '-framework', 'Foundation', '-o', binary]
        log = directory / 'compile.log'
        assert run(compile_command, log) == 0, log.read_text()
        assert run(link_command, log) == 0, log.read_text()
        command = [binary, args.assets, directory / 'native.tsv']
        log = directory / 'run.log'
        retain(name, [compile_command, link_command, command], changed, run(command, log), log)
    args.output.write_text(json.dumps(dict(faults=results, count=len(results),
                                           caught=all(r['caught'] for r in results)), indent=2) + '\n')
    print(f'{len(results)} compiled faults caught')

if __name__ == '__main__':
    main()
