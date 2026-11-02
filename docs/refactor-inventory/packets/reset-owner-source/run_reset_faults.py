"""Finite reset fault qualification; compile errors never count as semantic kills.

Only the shared reset helper is temporarily changed. Every attempt and build log
is retained, source is restored even on error, and the restored targets are rebuilt.
"""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys

SOURCE = Path('src/game/game_cleanup_internal.hpp')
PYTHON = '/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python'
PE = '/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe'
MAKE = ['make', '-j2', '-o', 'Makefile', '-o', 'build/softfloat/softfloat.a',
        '-o', 'build/softfloat_sse/softfloat-sse.a', '-o', 'build/bochs_fpatan/libbochs_fpatan.a',
        'UNICORN_DIR=/Users/burbujamc/Developer/sadx-macos/reference/python/lib/python3.14/site-packages/unicorn',
        'build/reset-order-characterization-test', 'build/reset-native-contract-test',
        'build/native-game-reset-composed-probe.dylib']
FAULTS = [
    ('timer-before-flags', '        const auto flags = state.sound_flags(index);\n        state.clear_sound_timer(index);',
     '        state.clear_sound_timer(index);\n        const auto flags = state.sound_flags(index);'),
    ('late-list-read', '    const auto present = state.list_present();\n    state.clear_reset_work();\n    state.clear_loop_count();',
     '    state.clear_reset_work();\n    state.clear_loop_count();\n    const auto present = state.list_present();'),
    ('media-stop-instead-of-reset', '        state.reset_music();', '        state.stop_music();'),
    ('clear-before-callback', '    state.invoke_level_destructor();\n    state.clear_level_destructor();',
     '    state.clear_level_destructor();\n    state.invoke_level_destructor();'),
    ('clear-before-free', '        operations.release_buffer(buffer);\n        state.clear_temporary_buffer();',
     '        state.clear_temporary_buffer();\n        operations.release_buffer(buffer);'),
    ('reread-wrong-buffer', 'if (auto *buffer = state.secondary_buffer())',
     'if (auto *buffer = state.temporary_buffer())'),
    ('omit-fog', '    operations.enable_fog();', '    // injected omission of fog'),
    ('wrong-seed', 'state.seed_random(kResetRandomSeed);', 'state.seed_random(kResetRandomSeed + 1U);'),
    ('wrong-mission-guard', 'mode == 5 && state.is_mission_destructor()', 'mode == 4 && state.is_mission_destructor()'),
    ('vibration-threshold', 'state.first_vibration_entry() < 3', 'state.first_vibration_entry() < 2'),
]


def main():
    destination = Path(sys.argv[1])
    destination.mkdir(parents=True)  # Refuse overwriting earlier fault attempts.
    original = SOURCE.read_bytes()
    frozen = hashlib.sha256(original).hexdigest()
    expected = Path('docs/refactor-inventory/packets/reset-owner-source/old-order.txt').read_bytes()
    environment = dict(os.environ, PYTHONPATH='tools/lib:tools/oracles')
    results = []
    try:
        for name, before, after in FAULTS:
            assert original.decode().count(before) == 1, name
            SOURCE.write_text(original.decode().replace(before, after))
            directory = destination / name
            directory.mkdir()
            with (directory / 'build.log').open('wb') as log:
                built = subprocess.run(MAKE, stdout=log, stderr=subprocess.STDOUT)
            record = {'name': name, 'build_exit': built.returncode,
                      'mutated_source_sha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest()}
            if built.returncode:
                record['status'] = 'SETUP_ERROR_NOT_A_KILL'
                results.append(record)
                break
            matrix = subprocess.run(['build/reset-order-characterization-test'], capture_output=True)
            native = subprocess.run(['build/reset-native-contract-test'], capture_output=True)
            (directory / 'matrix.out').write_bytes(matrix.stdout)
            (directory / 'matrix.err').write_bytes(matrix.stderr)
            (directory / 'native.out').write_bytes(native.stdout + native.stderr)
            command = [PYTHON, 'tools/oracles/verify_original_game_reset_composed.py', PE,
                       'build/native-game-reset-composed-probe.dylib', '--output', str(directory / 'original.json')]
            with (directory / 'original.log').open('wb') as log:
                oracle = subprocess.run(command, env=environment, stdout=log, stderr=subprocess.STDOUT)
            report = json.loads((directory / 'original.json').read_text())
            record.update(matrix_exit=matrix.returncode, matrix_equal=matrix.stdout == expected,
                          native_exit=native.returncode, oracle_exit=oracle.returncode,
                          original_cases=report['cases'], original_mismatches=report['mismatch_count'])
            killed = matrix.returncode == 0 and matrix.stdout != expected
            killed |= native.returncode != 0
            killed |= report['cases'] == 300 and report['mismatch_count'] > 0
            record['status'] = 'CAUGHT' if killed else 'SURVIVED'
            results.append(record)
            SOURCE.write_bytes(original)
    finally:
        SOURCE.write_bytes(original)
        with (destination / 'restore-build.log').open('wb') as log:
            restored = subprocess.run(MAKE, stdout=log, stderr=subprocess.STDOUT)
        assert hashlib.sha256(SOURCE.read_bytes()).hexdigest() == frozen
        (destination / 'results.json').write_text(json.dumps({
            'source_sha256': frozen, 'results': results, 'restored_build_exit': restored.returncode,
            'limits': 'Finite valid semantic faults only; no reserved cohort or new seed.'}, indent=2) + '\n')
    print([(r['name'], r['status']) for r in results])
    return int(len(results) != len(FAULTS) or restored.returncode != 0 or
               any(r['status'] != 'CAUGHT' for r in results))


if __name__ == '__main__':
    raise SystemExit(main())
