#!/usr/bin/env python3
"""Compile private wrapper mutants and require the original-instruction judge to reject each."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('mutation anchor is not unique: ' + old)
    return text.replace(old, new)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('--output-dir', type=Path, default=Path('build/character-textures-core/mutants'))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    source_path = root / 'src/game/character_textures.cpp'
    source = source_path.read_text()
    variants = {
        'negative-is-cold': replace_once(source, 'if (loaded == 0)', 'if (loaded <= 0)'),
        'null-table-increments': replace_once(source, 'if (table.entries == nullptr)\n        return;',
            'if (table.entries == nullptr) {\n        catalog.loaded[character] = 42;\n        return;\n    }'),
        'counter-does-not-wrap': replace_once(source, '? std::numeric_limits<std::int8_t>::min()',
            '? std::numeric_limits<std::int8_t>::max()'),
        'cached-counter': replace_once(replace_once(source, 'auto &loaded = catalog.loaded[character];',
            'auto &loaded = catalog.loaded[character];\n    const auto initial = loaded;'),
            'loaded = loaded == std::numeric_limits<std::int8_t>::max()',
            'loaded = initial == std::numeric_limits<std::int8_t>::max()').replace(
            'static_cast<std::int8_t>(loaded + 1)', 'static_cast<std::int8_t>(initial + 1)'),
        'amy-on-warm': replace_once(source,
            '        if (character == kAmy)\n            load_egg_robo_textures(state, services);\n    }',
            '    }\n    if (character == 5)\n        load_egg_robo_textures(state, services);').replace(
            '        constexpr std::size_t kAmy = 5; // Characters_Amy\n', ''),
        'amy-skips-empty': replace_once(source, 'if (character == kAmy)',
            'if (character == kAmy && table.count != 0 && table.entries[0].textures != nullptr)'),
        'named-uses-nameless': replace_once(source,
            'load_archive(*binding.archive, binding.textures, state, services);',
            'load_no_name(*binding.textures, state, services);'),
        'prefetch-next': replace_once(replace_once(source,
            'const auto &binding = table.entries[index];',
            'const auto &binding = table.entries[index];\n'
            '            const auto *next = index + 1 < table.count ? table.entries[index + 1].textures : nullptr;'),
            '            // The next entry is read only after the lower loader returns.',
            '            if (next == nullptr) break;'),
        'recapture-table': replace_once(replace_once(source, 'const auto table =', 'auto table ='),
            '            // The next entry is read only after the lower loader returns.',
            '            table = catalog.tables[character];\n            if (!table.entries) break;'),
        'ignore-current-null': replace_once(source,
            'if (binding.textures == nullptr)\n                break;',
            'if (binding.textures == nullptr)\n                continue;'),
    }
    args.output_dir.mkdir(parents=True, exist_ok=True)
    results = []
    for name, text in variants.items():
        cpp = args.output_dir / (name + '.cpp')
        dylib = args.output_dir / (name + '.dylib')
        report = args.output_dir / (name + '.json')
        cpp.write_text(text)
        subprocess.run(['clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Isrc', '-dynamiclib',
                        str(cpp), 'tests/probes/native_character_textures_probe.cpp', '-o', str(dylib)],
                       check=True, cwd=root)
        result = subprocess.run([sys.executable, str(root / 'tools/oracles/verify_original_character_textures.py'),
                                 str(args.executable), str(dylib.resolve()), '--output', str(report.resolve())],
                                cwd=root, capture_output=True, text=True)
        # A compile failure or crash is not counted as a killed behavioral mutant.
        receipt = json.loads(report.read_text()) if report.exists() else {}
        killed = result.returncode == 1 and receipt.get('value_mismatches', 0) > 0
        results.append({'name': name, 'killed': killed, 'exit_status': result.returncode,
                        'first_mismatch_case': receipt.get('failures', [{}])[0].get('case') if killed else None,
                        'source_sha256': hashlib.sha256(text.encode()).hexdigest()})
        print(name, 'killed' if killed else 'NOT KILLED', flush=True)
    summary = {'mutants': results, 'killed': sum(x['killed'] for x in results),
               'count': len(results), 'original_source_sha256': hashlib.sha256(source.encode()).hexdigest()}
    (args.output_dir / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    return 0 if summary['killed'] == len(results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
