"""Development-only binding characterization; never reads the reserved literal manifest."""
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUTPUT = ROOT / 'build/reset-binding-development-v1'
# Existing development values, distinct from every frozen reserved literal vector.
REFERENCE_INPUTS = [
    ('reserved-media-failure', [99, 32768, 42, 43, 1, 99, 0]),
    ('reserved-task-failure', [99, 32768, 42, 43, 1, 99, 0]),
    ('reserved-buffer-replacement', [99, 32768, 42, 43, 1, 99, 0]),
    ('reserved-later-buffer-unknown', [99, 32768, 42, 43, 1, 99, 0]),
    ('reserved-level-substitution', [99, 32768, 42, 43, 1, 99, 0]),
    ('reserved-media-reacquire', [2, 3, 5, 7, 3, 4, 1]),
]


def invoke(name, arguments, label):
    read_fd, write_fd = os.pipe()
    environment = {key: os.environ[key] for key in ('HOME', 'TMPDIR', 'USER', 'LOGNAME')
                   if key in os.environ}
    environment.update(PATH='/usr/bin:/bin:/usr/sbin:/sbin', LANG='C', LC_ALL='C')
    command = [str(ROOT / 'build' / name), *map(str, arguments), str(read_fd), str(os.getpid())]
    try:
        result = subprocess.run(command, cwd=ROOT, env=environment, pass_fds=(read_fd,),
                                capture_output=True, timeout=120)
    finally:
        os.close(read_fd)
        os.close(write_fd)
    (OUTPUT / (label + '.stdout')).write_bytes(result.stdout)
    (OUTPUT / (label + '.stderr')).write_bytes(result.stderr)
    (OUTPUT / (label + '.exit')).write_text(str(result.returncode) + '\n')
    if result.returncode:
        raise RuntimeError(f'{label}: exit {result.returncode}')
    lines = result.stdout.splitlines()
    if len(lines) != 1:
        raise ValueError(f'{label}: expected one complete observation')
    record = json.loads(lines[0])
    if not isinstance(record, dict) or not record:
        raise ValueError(f'{label}: empty observation')
    return record


def main():
    OUTPUT.mkdir()  # Exclusive: preserve failed attempts and all raw outputs.
    rows = []
    try:
        for index, (case, values) in enumerate(REFERENCE_INPUTS):
            old = invoke('reset-reference-final-old', [case, *values], f'{index}-old')
            current = invoke('reset-reference-final', [case, *values], f'{index}-current')
            if old != current or current['case'] != case or current['input'] != values:
                raise ValueError(f'{case}: old/current development observations differ')
            if index < 2:
                if current['result'] != [3, 0x40bf7f if index == 0 else 0x40bf9e, 0x123, 0]:
                    raise ValueError('missing development failure prefix')
                if len(current['calls']) != (9 if index == 0 else 14):
                    raise ValueError('wrong development failure call count')
            if index == 2 and (len(current['calls']) != 28 or current['state'][2:4] != [[1, 0]] * 2):
                raise ValueError('missing development buffer alias/replacement')
            if index == 3 and current['result'] != [1, 0x40bfd1, 0, 0]:
                raise ValueError('missing reached second-buffer unknown')
            if index == 4 and current['result'][:4] != [3, 0x42291e, 2, 0x123]:
                raise ValueError('missing substituted callback failure')
            if index == 5 and current['subobservations'] != [
                    [0, 0, 1, 2, 3, 0, 7, [31, 42]],
                    [1, 3, 0, 2, 3, 5, 7, [31]]]:
                raise ValueError('missing real media receiver/partial-output observations')
            rows.append({'family': case, 'development_input': values, 'equal': True})
        constants = ROOT / 'build/assets/sonic-player-constants.bin'
        old = invoke('reset-sonic-final-old', [constants, 32768], 'sonic-old')
        current = invoke('reset-sonic-final', [constants, 32768], 'sonic-current')
        if old != current or current['initial_signed'] != -32768:
            raise ValueError('Sonic developmental signed-value contract differs')
        if [row[1] for row in current['observations']] != [0x4258f0, 0x4258f7]:
            raise ValueError('Sonic forced/editor read order differs')
        if any(row[13] != 86 or row[-1] != 0 for row in current['observations']):
            raise ValueError('Sonic action published or external call reached')
        rows.append({'family': 'reserved-editor-read', 'development_editor': 32768, 'equal': True})
        session = invoke('reset-native-session-final', [
            '/Users/burbujamc/Developer/sadx-macos/game/imported', 99], 'session-current')
        if session != {'case': 'reserved-native-frame-alias', 'input': [99],
                       'boundary': 'native_pending_text_window', 'same_owner': 1,
                       'frame': 0, 'vibration_calls': 1}:
            raise ValueError('actual Session development alias contract differs')
        rows.append({'family': 'reserved-native-frame-alias', 'development_frame': 99,
                     'current_composition': True})
    except BaseException as failure:
        (OUTPUT / 'RESULT.json').write_text(json.dumps({
            'exit': 1, 'completed': rows, 'error': repr(failure),
            'reserved_inputs_consumed': False}, indent=2) + '\n')
        raise
    (OUTPUT / 'RESULT.json').write_text(json.dumps({
        'exit': 0, 'development_families': rows, 'program_calls': 15,
        'original_executions': 0, 'reserved_inputs_consumed': False,
        'limits': 'Development domain only; not final admission, source fidelity or integration.'},
        indent=2) + '\n')


if __name__ == '__main__':
    main()
