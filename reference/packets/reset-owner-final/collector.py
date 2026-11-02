"""Finite reset qualification collector. Preparation never executes a model.

Run requires a distinct affirmative committed admission and exclusive output.
Original observations, old/current interfaces and new native composition stay separate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
PACKET = ROOT / 'docs/refactor-inventory/packets/reset-owner-source'
FREEZE = PACKET / 'ROOT-FINAL-FREEZE.json'
ADMISSION = PACKET / 'ROOT-FINAL-ADMISSION.json'
OUTPUT = ROOT / 'build/reset-final-reserved-v1'
PYTHON = Path('/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python')
ASSETS = Path('/Users/burbujamc/Developer/sadx-macos/game/imported')
OLD = ROOT / 'build/reset-old'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write_new(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def source_helper():
    from types import ModuleType
    path = ROOT / 'tools/oracles/task_judges/reserved_run.py'
    module = ModuleType('reset_reviewed_task_utilities')
    module.__file__ = str(path)
    exec(compile(path.read_bytes(), str(path), 'exec'), module.__dict__)
    return module


def prepare():
    protocol = PACKET / 'ROOT-RESERVED-PROTOCOL.json'
    inputs = PACKET / 'FINAL-BINDING-INPUTS.json'
    evidence = json.loads((PACKET / 'BINDING-DEVELOPMENT-RESULT.json').read_text())
    setup_path = PACKET.parent / 'reset-owner/SETUP.json'
    setup = json.loads(setup_path.read_text())
    require(PYTHON.resolve() == Path(sys.executable).resolve(), 'Use the pinned interpreter')
    for name, expected in setup['pins'].items():
        require(sha(name) == expected['sha256'], 'Pinned setup input changed: ' + name)
    baseline = json.loads((PACKET / 'BEFORE-SOURCE.json').read_text())
    for name, expected in baseline['source_freeze'].items():
        require(sha(OLD / name) == expected, 'Immutable old source changed: ' + name)
    models = {name: value['path'] for name, value in evidence['models'].items()}
    for name, expected in evidence['binding_sources'].items():
        if name == 'Makefile':
            # Only the separately compiled no-game fake target was added after model pinning.
            rule = ('# Fake process-lifetime adequacy only; no game code is linked.\n'
                    'build/reset-final-parent-fake: tests/fixtures/reset_final_parent_fake.cpp '
                    'tests/fixtures/reset_final_parent.hpp\n'
                    '\t$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@\n')
            current = (ROOT / name).read_text()
            require(current.count(rule) == 1 and
                    hashlib.sha256(current.replace(rule, '').encode()).hexdigest() == expected,
                    'Compiled model recipe changed beyond the isolated fake target')
        else:
            require(sha(ROOT / name) == expected, 'Changed compiled binding source: ' + name)
    for name, value in evidence['models'].items():
        require(sha(value['path']) == value['sha256'], 'Frozen binding model changed: ' + name)
    helpers = source_helper()
    package = Path(sys.prefix) / 'lib' / f'python{sys.version_info.major}.{sys.version_info.minor}'
    package /= 'site-packages/unicorn'
    require(package.is_dir(), 'Use pinned reference Python with Unicorn installed')
    paths = [protocol, inputs, setup_path, PACKET / 'BEFORE-SOURCE.json',
             PACKET / 'BINDING-DEVELOPMENT-RESULT.json', ROOT / 'tools/oracles/task_judges/reserved_run.py',
             PYTHON.resolve(), ROOT / 'build/assets/sonic-player-constants.bin',
             ASSETS / 'characters/sonic-selection.actions',
             Path('/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe')]
    paths += list(Path(__file__).parent.glob('*.py'))
    paths += list((ROOT / 'tests/fixtures').glob('reset_*final*'))
    paths += [ROOT / 'tests/reference/actor_current_fixture.hpp',
              OLD / 'tests/reference/actor_current_fixture.hpp', ROOT / 'Makefile']
    for tree in (ROOT, OLD):
        paths += [p for p in (tree / 'src').rglob('*') if p.suffix in ('.hpp', '.cpp', '.mm')]
    for name in ('pe_image', 'extract'):
        paths.append(OLD / f'tools/lib/{name}.py')
    for name in ('math', 'game_reset', 'game_reset_composed'):
        paths.append(OLD / f'tools/oracles/verify_original_{name}.py')
    paths += [p for p in package.rglob('*') if p.is_file()]
    header = (ROOT / 'src/apps/native_assets.hpp').read_text()
    for name in set(re.findall(r'read_metadata\(directory / "([^"]+)"', header)):
        paths.append(ASSETS / name)
    movies = json.loads((ASSETS / 'movie-content.json').read_text())
    paths += [ASSETS / row['asset_path'] for row in movies['clips']]
    paths += [Path(p) for p in models.values()]
    probes = {'old_reset': '/Users/burbujamc/Developer/sadx-macos/build/reset-owner-baseline-8e15bbe/native-game-reset-probe.dylib',
              'old_composed': '/Users/burbujamc/Developer/sadx-macos/build/reset-owner-baseline-8e15bbe/native-game-reset-composed-probe.dylib',
              'current_reset': str(ROOT / 'build/native-game-reset-probe.dylib'),
              'current_composed': str(ROOT / 'build/native-game-reset-composed-probe.dylib')}
    paths += [Path(p) for p in probes.values()]
    pins = {str(p.absolute()): sha(p) for p in sorted(set(paths))}
    value = {'kind': 'reset_final_freeze_v1', 'root_admitted': False,
             'protocol': json.loads(protocol.read_text()), 'inputs': json.loads(inputs.read_text()),
             'models': models, 'probes': probes, 'pins': pins, 'python': str(PYTHON),
             'python_resolved': str(PYTHON.resolve()), 'unicorn_package': str(package.resolve()),
             'old_root': str(OLD), 'assets': str(ASSETS), 'output': str(OUTPUT),
             'source_loading': 'Reviewed Task source_module/origin/pinned-Unicorn utilities; no project bytecode.',
             'limits': 'No generation, compilation, library load or game behavior during preparation.'}
    write_new(FREEZE, value)
    return value


def committed(path, commit):
    require(bool(re.fullmatch('[0-9a-f]{40}', commit)), 'Exact committed admission SHA required')
    relative = path.resolve().relative_to(ROOT)
    data = subprocess.check_output(['git', 'show', f'{commit}:{relative}'], cwd=ROOT)
    require(data == path.read_bytes(), 'Admission differs from committed bytes')
    subprocess.run(['git', 'merge-base', '--is-ancestor', commit, 'HEAD'], cwd=ROOT, check=True)
    return json.loads(data)


def validate_admission(value, freeze, internal=False):
    require(type(value) is dict and type(freeze) is dict, 'Invalid admission/freeze object')
    require(value.get('kind') == 'reset_final_admission_v1' and
            value.get('root_admitted') is True, 'Distinct affirmative Root admission required')
    require(value.get('freeze_sha256') == sha(FREEZE), 'Admission freeze identity differs')
    require(value.get('output') == str(OUTPUT) == freeze.get('output'), 'Output identity differs')
    require(freeze.get('kind') == 'reset_final_freeze_v1', 'Wrong frozen protocol')
    require(type(freeze.get('models')) is dict and type(freeze.get('probes')) is dict and
            type(freeze.get('pins')) is dict and type(freeze.get('protocol')) is dict and
            type(freeze.get('inputs')) is dict, 'Incomplete closure schema')
    require(set(freeze['models']) == {'reset-reference-final-old', 'reset-reference-final',
            'reset-sonic-final-old', 'reset-sonic-final', 'reset-native-session-final'},
            'Wrong compiled binding set')
    require(set(freeze['probes']) == {'old_reset', 'current_reset', 'old_composed',
                                    'current_composed'}, 'Wrong probe set')
    evidence = json.loads((PACKET / 'BINDING-DEVELOPMENT-RESULT.json').read_text())
    require(freeze['models'] == {name: row['path'] for name, row in evidence['models'].items()},
            'Changed old/current binding identities')
    require(all(type(path) is str and path for path in
                [*freeze['models'].values(), *freeze['probes'].values()]),
            'Missing compiled model paths')
    require(freeze['protocol'] == json.loads((PACKET / 'ROOT-RESERVED-PROTOCOL.json').read_text())
            and freeze['inputs'] == json.loads((PACKET / 'FINAL-BINDING-INPUTS.json').read_text()),
            'Embedded literal protocol differs from pinned authored definition')
    require(freeze.get('protocol', {}).get('expected_order') and
            len(freeze['protocol']['expected_order']) == 8 and
            len(set(freeze['protocol']['expected_order'])) == 8, 'Expected eight unique cases')
    require(type(freeze.get('python')) is str and type(freeze.get('python_resolved')) is str,
            'Missing interpreter identities')
    require(Path(freeze['python']).resolve() == Path(freeze['python_resolved']),
            'Pinned interpreter target differs')
    required = {str(Path(__file__).resolve()), str(PYTHON.resolve()),
                str(PACKET / 'ROOT-RESERVED-PROTOCOL.json'),
                str(PACKET / 'FINAL-BINDING-INPUTS.json'),
                str(PACKET / 'BINDING-DEVELOPMENT-RESULT.json'),
                str(Path(__file__).with_name('original.py').resolve()),
                str(ROOT / 'tools/oracles/task_judges/reserved_run.py')}
    required.update(freeze['models'].values())
    required.update(freeze['probes'].values())
    for name in ('pe_image', 'extract'):
        required.add(str(Path(freeze['old_root']) / f'tools/lib/{name}.py'))
    for name in ('math', 'game_reset', 'game_reset_composed'):
        required.add(str(Path(freeze['old_root']) / f'tools/oracles/verify_original_{name}.py'))
    require(required.issubset(freeze.get('pins', {})), 'Missing required closure pins')
    require(all(type(path) is str and type(digest) is str and
                re.fullmatch('[0-9a-f]{64}', digest) for path, digest in freeze['pins'].items()),
            'Invalid closure hash/path')
    for name, expected in freeze['pins'].items():
        require(sha(name) == expected, 'Changed frozen input: ' + name)
    if not internal:
        require(not OUTPUT.exists(), 'Output already exists; preserve prior attempt')
    else:
        require((OUTPUT / 'STARTED.json').is_file() and not (OUTPUT / 'RUN.json').exists(),
                'Missing live parent execution or terminal attempt')


def strict_json(value):
    if type(value) in (str, int) or value is None:
        return
    if type(value) is list:
        for item in value:
            strict_json(item)
        return
    if type(value) is dict and all(type(key) is str for key in value):
        for item in value.values():
            strict_json(item)
        return
    raise ValueError('Unsupported observed scalar/type')


def one_record(raw):
    lines = raw.splitlines()
    require(len(lines) == 1, 'Expected one nonempty complete record')
    value = json.loads(lines[0])
    require(isinstance(value, dict) and bool(value), 'Empty observation')
    strict_json(value)
    return value


def compare_complete(old, current):
    strict_json(old)
    strict_json(current)
    require(old == current, 'Complete old/current observation differs')


def validate_domain(rows, order):
    require(bool(order) and len(rows) == len(order), 'Incomplete or empty case domain')
    require([row.get('case') for row in rows] == order, 'Changed/duplicated/reordered case domain')


def reset_calls(first, second):
    rows = [[0x40bf42, 0x40cd10, [0, 0]], [0x40bf49, 0x401410, [1, 1]],
            [0x40bf51, 0x785330, [1, 0]], [0x40bf63, 0x403e60, [0, 0]],
            [0x40bf68, 0x4257c0, [0, 0]], [0x40bf6d, 0x4257e0, [0, 0]],
            [0x40bf72, 0x424460, [0, 0]], [0x40bf79, 0x410100, [1, 0xffffffff]],
            [0x40bf7f, 0x40cff0, [1, 0]], [0x40bf86, 0x40cff0, [1, 1]],
            [0x40bf8b, 0x4208e0, [0, 0]], [0x40bf91, 0x40d490, [1, 0]],
            [0x40bf99, 0x40b1a0, [0, 0]], [0x40bf9e, 0x40b460, [0, 0]]]
    rows += [[0x40bfa6, 0x4228f0, [1, n]] for n in range(10)]
    return rows + [[0x40bfb4, 0x421fb0, [0, 0]], [0x40bfc3, 0x780fb0, [1, first]],
                   [0x40bfdb, 0x780fb0, [1, second]], [0x40c00e, 0x411af0, [0, 0]]]


def validate_interface(record, case, values):
    strict_json(record)
    require(record.get('case') == case and record.get('input') == values, 'Changed input/binding')
    if case == 'reserved-media-reacquire':
        a, b, shared, loop, first, second, mode = values
        require(mode == 1, 'Wrong media mode')
        expected = [[0, 0, 1, a, b, 0, loop, [first * 10 + 1, second * 10 + 2]],
                    [1, 3, 0, a, b, shared, loop, [first * 10 + 1]]]
        require(record.get('subobservations') == expected, 'Changed media receiver or partial state')
        return
    if case == 'reserved-level-substitution':
        first, second, callback = values[2], values[3], values[5]
        calls = [[0x42246b, 0x780fb0, [0, 0], [1, first]]] * 2
        calls += [[0x4224cb, 0x780fb0, [0, 0], [1, second]],
                  [0x42291e, callback + 1, [0, 0], [0, 0]]]
        require(record.get('result') == [3, 0x42291e, 2, 0x123, [0, 0]] and
                record.get('calls') == calls and
                record.get('state') == [[1, 0], [1, 0], [1, callback + 1]] + [[1, 0]] * 12 and
                record.get('debug_bytes') == list(b'ReleaseCamFile\n'),
                'Changed level callback/release failure contract')
        return
    calls = reset_calls(values[2], values[2])
    stop = {'reserved-media-failure': 9, 'reserved-task-failure': 14,
            'reserved-buffer-replacement': 28, 'reserved-later-buffer-unknown': 26}[case]
    require([row[0] for row in record.get('calls', [])] == calls[:stop], 'Wrong complete call ledger')
    state = [[1, 0], [1, 0], [1, values[2]], [1, values[3]], [1, values[4]]]
    if case in ('reserved-media-failure', 'reserved-task-failure'):
        expected = [3, calls[stop - 1][0], 0x123, 0]
        pending = calls[stop - 1]
    elif case == 'reserved-later-buffer-unknown':
        expected, pending = [1, 0x40bfd1, 0, 0], None
        state[2:4] = [[1, 0], [0, 0]]
    else:
        expected, pending = [0, 0, 0, 1], None
        state[2:] = [[1, 0], [1, 0], [1, 0xdead0cab]]
    require(record.get('result') == expected and record.get('pending') == pending and
            record.get('state') == state, 'Wrong result/pending payload/state')
    for index, row in enumerate(record['calls']):
        require(len(row) == 2 and len(row[1]) == 5, 'Incomplete reached state')
        snapshot = row[1]
        require(snapshot[:2] == ([[1, values[0]], [1, values[1]]] if index < 3
                                 else [[1, 0], [1, 0]]), 'Early/reordered frame/editor stores')
        require(snapshot[4] == [1, values[4]], 'Early reseed')
        if index == 25:
            require(snapshot[2] == [1, values[2]], 'First slot cleared before callback')
        if index == 26:
            require(snapshot[2:4] == [[1, 0], [1, values[2]]], 'Later slot not reacquired')


def validate_sonic(record, initial):
    strict_json(record)
    require(record.get('case') == 'reserved-editor-read' and record.get('input') == [initial, 1],
            'Changed Sonic input')
    require(record.get('initial_signed') == initial - (65536 if initial >= 32768 else 0),
            'Changed signed editor value')
    rows = record.get('observations', [])
    require(len(rows) == 2, 'Missing Sonic controls')
    for row, boundary in zip(rows, [0x4258f0, 0x4258f7]):
        expected = [1, boundary, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    1, 86, 0, 1, 1, 1, 1, 0, 0, 127, 0, 0]
        require(row == expected, 'Changed Sonic read/prefix/publication or FP contract')


def validate_original(record, case, inputs):
    strict_json(record)
    require(record.get('case') == case, 'Wrong original case identity')
    native = record.get('native', {})
    require(set(native) == {'old', 'current'}, 'Incomplete original/native worlds')
    words = record.get('original_words', [])
    width = case == 'reserved-editor-read'
    require(len(words) == (200 if width else 128) and
            all(type(word) is int and 0 <= word <= 0xffffffff for word in words),
            'Truncated/invalid original word projection')
    for world in ('old', 'current'):
        require(native[world].get('abi_return') == 1 and native[world].get('words') == words,
                'Changed full native ABI result or source comparison')
    if width:
        require(record.get('original_pc') == record.get('stop_pc') == 0x10000100,
                'Original STOP PC is not established')
        require(record.get('kind') == 'original_returned_width_subobservation' and
                record.get('source_returned') == 1 and words[:2] == [1, 0],
                'Original normal return not established')
        before = bytes.fromhex(record.get('input_pages_hex', ''))
        after = bytes.fromhex(record.get('original_pages_hex', ''))
        require(len(before) == len(after) == 14 * 4096, 'Incomplete original mapped pages')
        offset = 4 * 4096 + 0xd08
        spec = inputs['source_editor_width_subobservation']
        require(int.from_bytes(before[offset:offset + 4], 'little') ==
                spec['editor'] | spec['upper_neighbor'] << 16 and
                int.from_bytes(after[offset:offset + 4], 'little') ==
                spec['upper_neighbor'] << 16, 'Changed editor/upper adjacent word')
        expected_input = bytearray(14 * 4096)
        expected_input[offset:offset + 4] = (spec['editor'] | spec['upper_neighbor'] << 16).to_bytes(4, 'little')
        require(before == expected_input, 'Changed complete original input pages')
        for world in ('old', 'current'):
            require(native[world].get('pages_hex') == after.hex(), 'Incomplete native mapped pages')
    else:
        fail_at = 9 if case == 'reserved-media-failure' else 14
        boundary = 0x40bf7f if fail_at == 9 else 0x40bf9e
        require(record.get('original_pc') == (0x40cff0 if fail_at == 9 else 0x40b460) and
                record.get('stop_pc') == 0x10000100, 'Wrong original observer PC')
        require(record.get('kind') == 'original_observer_stop' and
                record.get('source_returned') == 0 and
                record.get('source_stop_boundary') == boundary and words[:3] == [3, boundary, 0],
                'Original observer stop/return classification differs')
        require(record.get('input') == [*inputs['reference_inputs'][case][:5], fail_at],
                'Changed original input ABI')


def execute(argv, stage, freeze, output):
    read_fd, write_fd = os.pipe()
    pipe = os.fstat(read_fd)
    token = secrets.token_hex(24)
    active = {'parent_pid': os.getpid(), 'stage': stage, 'token': token,
              'pipe_device': pipe.st_dev, 'pipe_inode': pipe.st_ino}
    temporary = output / 'ACTIVE.new.json'
    temporary.write_text(json.dumps(active))
    temporary.replace(output / 'ACTIVE.json')
    if stage.startswith('original-'):
        command = [freeze['python'], '-I', '-S', '-B', str(Path(__file__).with_name('original.py')),
                   '--admission-commit', freeze['_admission_commit'], '--case', stage[9:],
                   '--parent-fd', str(read_fd), '--stage-token', token]
    else:
        command = [*map(str, argv), str(read_fd), str(os.getpid())]
    environment = {key: os.environ[key] for key in ('HOME', 'TMPDIR', 'USER', 'LOGNAME')
                   if key in os.environ}
    environment.update(PATH='/usr/bin:/bin:/usr/sbin:/sbin', LANG='C', LC_ALL='C')
    try:
        with (output / (stage + '.stdout')).open('xb') as stdout, \
                (output / (stage + '.stderr')).open('xb') as stderr:
            result = subprocess.run(command, cwd=ROOT, env=environment, pass_fds=(read_fd,),
                                    stdout=stdout, stderr=stderr, timeout=120)
        write_new(output / (stage + '.exit.json'), {'exit': result.returncode, 'command': command})
        require(result.returncode == 0, f'{stage}: process exit {result.returncode}')
        return one_record((output / (stage + '.stdout')).read_bytes())
    finally:
        os.close(read_fd)
        os.close(write_fd)


def run(commit):
    admission = committed(ADMISSION, commit)
    freeze = committed(FREEZE, admission.get('freeze_commit', ''))
    validate_admission(admission, freeze)  # Before permanent output or any child/model load.
    OUTPUT.mkdir()
    write_new(OUTPUT / 'STARTED.json', {'admission_commit': commit, 'freeze_sha256': sha(FREEZE)})
    freeze['_admission_commit'] = commit
    results = []
    try:
        for case in freeze['protocol']['expected_order']:
            inputs = freeze['inputs']
            if case in inputs['reference_inputs']:
                values = inputs['reference_inputs'][case]
                old = execute([freeze['models']['reset-reference-final-old'], case, *values],
                              case + '-old', freeze, OUTPUT)
                current = execute([freeze['models']['reset-reference-final'], case, *values],
                                  case + '-current', freeze, OUTPUT)
                validate_interface(old, case, values)
                validate_interface(current, case, values)
                compare_complete(old, current)
                observed = {'case': case, 'old_current': 'EXACT', 'old': old, 'current': current}
            elif case == 'reserved-editor-read':
                initial = inputs['sonic_input']['editor']
                args = [ROOT / inputs['sonic_input']['constants'], initial]
                old = execute([freeze['models']['reset-sonic-final-old'], *args],
                              case + '-old', freeze, OUTPUT)
                current = execute([freeze['models']['reset-sonic-final'], *args],
                                  case + '-current', freeze, OUTPUT)
                validate_sonic(old, initial)
                validate_sonic(current, initial)
                compare_complete(old, current)
                observed = {'case': case, 'old_current': 'EXACT', 'old': old, 'current': current}
            else:
                frame = inputs['native_session_input']['frame']
                current = execute([freeze['models']['reset-native-session-final'], freeze['assets'], frame],
                                  case + '-current', freeze, OUTPUT)
                require(current == {'case': case, 'input': [frame],
                        'boundary': 'native_pending_text_window', 'same_owner': 1,
                        'frame': 0, 'vibration_calls': 1}, 'Actual Session composition differs')
                observed = {'case': case, 'current_only_composition': current}
            if case in ('reserved-media-failure', 'reserved-task-failure', 'reserved-editor-read'):
                original = execute([], 'original-' + case, freeze, OUTPUT)
                validate_original(original, case, inputs)
                observed['original_subobservation'] = original
            results.append(observed)
            write_new(OUTPUT / (case + '.json'), observed)
        validate_domain(results, freeze['protocol']['expected_order'])
    except BaseException as failure:
        write_new(OUTPUT / 'RUN.json', {'exit': 1, 'completed_ids': [r['case'] for r in results],
                  'error': repr(failure), 'uncompleted_ids': [c for c in freeze['protocol']['expected_order']
                                                           if c not in [r['case'] for r in results]]})
        raise
    write_new(OUTPUT / 'RUN.json', {'exit': 0, 'case_count': 8, 'results': results,
              'limits': 'Two original observer-stop rows; one composed returned width sub-observation; '
                        'other reference rows are interface comparisons. Actual Session is new composition. '
                        'Inherited failures/partials and unrecovered behavior remain open.'})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--prepare', action='store_true')
    group.add_argument('--admission-commit')
    args = parser.parse_args()
    if args.prepare:
        prepare()
    else:
        run(args.admission_commit)


if __name__ == '__main__':
    main()
