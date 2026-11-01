#!/usr/bin/env python3
"""Serial native ownership/attachment qualification; reserves holdouts for Root review."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
FLAGS = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
         '-Wmissing-field-initializers', '-ffp-contract=off', '-fno-fast-math',
         '-Isrc', '-Itests/fixtures']
DEPENDENCIES = ['src/game/regular_textures.cpp', 'src/game/game_cleanup.cpp',
                'src/runtime/native_menu_texture_bindings.cpp']
SOURCE = 'src/runtime/native_regular_resources.cpp'
TEST = 'tests/unit/native_regular_resources.cpp'
MUTANTS = [
    ('retry-clears-partial-groups', '    regular_groups_.resize(',
     '    regular_groups_.clear();\n    regular_groups_.resize('),
    ('drop-common-binding', 'group.common = bind(source.front());',
     'group.common = {std::nullopt, nullptr};'),
    ('lose-regular-alias', '&regular_resource_lists_.at(*source.texture_list)',
     '&regular_resource_lists_.at(0)'),
    ('skip-duplicate-guard', 'if (regular_texture_state_)', 'if (false)'),
    ('eager-group-validation', '    regular_assets_ = std::move(assets);',
     '    for (const auto &group : assets.groups)\n'
     '        if (group.empty())\n'
     '            throw std::runtime_error("regular texture group lacks its common descriptor");\n'
     '    regular_assets_ = std::move(assets);'),
    ('nonzero-scratch-default', 'regular_assets_.localizations, {}, {}});',
     'regular_assets_.localizations, {1}, {}});'),
]


def run(command):
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    return {'command': command, 'exit': result.returncode,
            'stdout': result.stdout, 'stderr': result.stderr}


def function_body(text, name):
    start = text.index('{', text.index(name))
    depth = 1
    end = start + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start + 1:end - 1]


def source_binding():
    old = subprocess.check_output(['git', 'show', 'e5cf19d:src/runtime/native_session.cpp'],
                                  cwd=ROOT, text=True)
    before = function_body(old, 'void Session::attach_regular_textures')
    fixture = function_body((ROOT / 'tests/fixtures/regular_resource_baseline.hpp').read_text(),
                            'void attach_regular_textures')
    assert before == fixture
    owner = function_body((ROOT / SOURCE).read_text(), 'void RegularResources::attach')
    owner = owner.replace('resync, language,', 'loop_.resync, text_language_,')
    split = before.index('    if (!display_)')
    assert owner.strip() == before[:split].strip()
    session = function_body((ROOT / 'src/runtime/native_session.cpp').read_text(),
                            'void Session::attach_regular_textures')
    prefix = '\n    regular_resources_.attach(std::move(assets), loop_.resync, text_language_);\n'
    assert session.startswith(prefix)
    assert session[len(prefix):] == before[split:].replace(
        'regular_texture_state_', 'regular_resources_.state()')
    return {'frozen_body_exact': True, 'extracted_owner_body_exact': True,
            'session_transport_suffix_exact': True,
            'baseline_body_sha256': hashlib.sha256(before.encode()).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=ROOT / 'build/session-regular-resources')
    args = parser.parse_args()
    folder = args.output_dir.resolve()
    folder.mkdir(parents=True, exist_ok=True)
    report = {'source_binding': source_binding(), 'builds': [], 'mutants': []}
    source = (ROOT / SOURCE).read_text()
    for baseline in [True, False]:
        binary = folder / ('prior-test' if baseline else 'current-test')
        command = FLAGS + (['-DBASELINE_ONLY'] if baseline else []) + [TEST]
        command += ([] if baseline else [SOURCE]) + DEPENDENCIES + ['-o', str(binary)]
        build = run(command)
        report['builds'].append(build)
        assert build['exit'] == 0, build
        result = run([str(binary)])
        report['builds'].append(result)
        assert result['exit'] == 0, result
        if baseline:
            assert result['stdout'] == (ROOT / 'reference/packets/session-regular-resources/native-baseline.txt').read_text()
    for name, before, after in MUTANTS:
        assert source.count(before) == 1, name
        mutant = folder / (name + '.cpp')
        mutant.write_text(source.replace(before, after))
        binary = folder / name
        build = run(FLAGS + [TEST, str(mutant)] + DEPENDENCIES + ['-o', str(binary)])
        record = {'name': name, 'build': build}
        assert build['exit'] == 0, record
        record['result'] = run([str(binary)])
        report['mutants'].append(record)
        assert record['result']['exit'] != 0, record
    report['scope'] = ('Native source-bound reduced transport host; original Windows judges separate. '
                       'No heldout/final qualification, GPU or actual full Session runtime claim.')
    (folder / 'qualification.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Source binding, 55 native observations, cleanup/lifetime and six valid mutants pass.')


if __name__ == '__main__':
    main()
