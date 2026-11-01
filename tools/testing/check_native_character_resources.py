#!/usr/bin/env python3
"""Verify the native character-owner extraction against its frozen Session body."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/runtime/native_character_resources.cpp'
TEST = 'tests/unit/native_character_resources.cpp'
FLAGS = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
         '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-Itests/fixtures']
MUTANTS = [
    ('collapse-character-alias', '&character_texture_lists_[character];', '&character_texture_lists_[0];'),
    ('allow-zero-declaration', 'if (declarations[index].texture_count == 0)', 'if (false)'),
    ('publish-before-lists', '        auto &lists = character_textures_[character];',
     '        character_texture_bindings_[character] = &character_texture_lists_[character];\n'
     '        auto &lists = character_textures_[character];'),
    ('invent-loaded-byte', '    for (std::size_t character = 0;',
     '    character_textures_loaded_[0] = 1;\n    for (std::size_t character = 0;'),
    ('omit-final-character', 'character < texture_assets_.size();', 'character + 1 < texture_assets_.size();'),
]


def body(text, name):
    start = text.index('{', text.index(name)); end = start + 1; depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}'); end += 1
    return text[start + 1:end - 1]


def source_binding():
    def old(path):
        return subprocess.check_output(['git', 'show', '92dbd07:' + path], cwd=ROOT, text=True)
    before = old('src/runtime/native_session.cpp')
    start = before.index('    for (std::size_t character = 0; character < texture_assets_.size(); ++character)')
    end = before.index('    apply_frame_rate_setting(frame_rate_setting);', start)
    loop = before[start:end]
    fixture = body((ROOT / 'tests/fixtures/character_resource_baseline.hpp').read_text(), 'void initialize()')
    current = body((ROOT / SOURCE).read_text(), 'void CharacterResources::initialize()')
    assert fixture.strip() == current.strip() == loop.strip()
    expected = before.replace('texture_assets_(textures) {', 'character_resources_(textures) {')
    expected = expected.replace(loop, '    character_resources_.initialize();\n')
    expected = expected.replace('character_texture_bindings_,', 'character_resources_.bindings(),')
    expected = expected.replace('character_textures_loaded_};', 'character_resources_.loaded()};')
    assert expected == (ROOT / 'src/runtime/native_session.cpp').read_text()
    header = old('src/runtime/native_session.hpp')
    start = header.index('    CharacterTextureAssets texture_assets_;')
    end = header.index('    RegularResources regular_resources_;', start)
    assert header[start:end] in (ROOT / 'src/runtime/native_character_resources.hpp').read_text()
    expected = header[:start] + '    CharacterResources character_resources_;\n' + header[end:]
    expected = expected.replace('#include "runtime/character_texture_assets.hpp"',
                                '#include "runtime/native_character_resources.hpp"')
    expected = expected.replace('return character_textures_loaded_;', 'return character_resources_.loaded();')
    assert expected == (ROOT / 'src/runtime/native_session.hpp').read_text()
    expected = old('src/runtime/native_selection_preview.cpp')
    for name, member in [('texture_assets_', 'assets()'), ('character_textures_', 'lists()'),
                         ('character_textures_loaded_', 'loaded()')]:
        expected = expected.replace('scene.session_.' + name, 'scene.session_.character_resources_.' + member)
    assert expected == (ROOT / 'src/runtime/native_selection_preview.cpp').read_text()
    return {'loop_sha256': hashlib.sha256(loop.encode()).hexdigest(),
            'frozen_body_exact': True, 'owner_fields_exact': True,
            'session_and_preview_forwarding_exact': True,
            'metadata_copy_and_body_initialization_boundaries_preserved': True}


def run(command):
    result = subprocess.run([str(x) for x in command], cwd=ROOT, capture_output=True, text=True)
    return {'command': [str(x) for x in command], 'exit': result.returncode,
            'stdout': result.stdout, 'stderr': result.stderr}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/character-resources/check')
    folder = parser.parse_args().output.resolve(); folder.mkdir(parents=True, exist_ok=True)
    report = {'source_binding': source_binding(), 'builds': [], 'mutants': []}
    for baseline in [True, False]:
        binary = folder / ('baseline' if baseline else 'current')
        command = FLAGS + (['-DCHARACTER_RESOURCES_BASELINE_ONLY'] if baseline else [])
        command += [TEST, 'src/game/game_cleanup.cpp'] + ([] if baseline else [SOURCE]) + ['-o', binary]
        built = run(command); report['builds'].append(built); assert built['exit'] == 0, built
        result = run([binary]); report['builds'].append(result); assert result['exit'] == 0, result
        assert result['stdout'] == (ROOT / 'reference/packets/session-character-resources/native-baseline.txt').read_text()
    original = (ROOT / SOURCE).read_text()
    for name, before, after in MUTANTS:
        assert original.count(before) == 1, name
        source = folder / (name + '.cpp'); source.write_text(original.replace(before, after))
        binary = folder / name
        built = run(FLAGS + [TEST, 'src/game/game_cleanup.cpp', source, '-o', binary])
        record = {'name': name, 'build': built}; assert built['exit'] == 0, record
        record['run'] = run([binary]); assert record['run']['exit'] != 0, record
        report['mutants'].append(record)
    report['limits'] = ['Native reduced-host binding baseline; not Windows attachment or actual Session runtime.',
                        'No full native build, original oracle, final holdout or integration gate run by this checker.']
    (folder / 'qualification.json').write_text(json.dumps(report, indent=2) + '\n')
    print('25 observations match; exact binding/forwarding source preserved; five compiled mutants caught.')


if __name__ == '__main__':
    main()
