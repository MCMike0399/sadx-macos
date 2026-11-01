#!/usr/bin/env python3
"""Replay retained deletion ABI calls; source failures remain separate failures."""
import argparse
import base64
import ctypes
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def words(data):
    return list(struct.unpack('<' + 'I' * (len(data) // 4), data))


def admitted(path, commit, root):
    """The invocation names the immutable Root admission, not mutable copied pins."""
    if len(commit) != 40 or any(c not in '0123456789abcdef' for c in commit):
        raise ValueError('Exact admission commit required')
    relative = path.resolve().relative_to(root.resolve())
    frozen = subprocess.run(['git', '-C', str(root), 'show', commit + ':' + relative.as_posix()],
                            check=True, capture_output=True).stdout
    if path.read_bytes() != frozen:
        raise ValueError('Admission differs from committed Root proof')
    return json.loads(frozen)


def contracts(folder, pins):
    """Validate the complete source/capture binding before calling any library."""
    inputs = json.loads((folder / 'inputs.json').read_text())
    observation = json.loads((folder / 'reserved-observation.json').read_text())
    report = json.loads((folder / 'task_delete.json').read_text())
    validator_path = Path(__file__).resolve().parents[3] / 'tools/oracles/task_judges/compare_streams.py'
    spec = importlib.util.spec_from_file_location('task_capture_validator', validator_path)
    validator = importlib.util.module_from_spec(spec)
    # Load source explicitly so a stale bytecode cache cannot replace the reviewed body.
    exec(compile(validator_path.read_bytes(), str(validator_path), 'exec'), validator.__dict__)
    records = [json.loads(raw) for raw in
               validator.validated_failed_delete_records(folder, pins)]
    ids = list(range(len(inputs)))
    events = observation['source_observations']
    if (not ids or [row['case'] for row in inputs] != ids or
            observation['generated_ids'] != ids or observation['attempted_source_ids'] != ids or
            observation['unattempted_source_ids'] or observation['error'] is not None or
            [event['case'] for event in events] != ids):
        raise ValueError('Incomplete ordered source/input observations')
    if observation['inputs_sha256'] != sha(folder / 'inputs.json'):
        raise ValueError('Input receipt differs')
    excluded = report['source_unsupported']
    excluded_ids = [row['case'] for row in excluded]
    if (len(set(excluded_ids)) != len(excluded_ids) or
            excluded_ids != [event['case'] for event in events if event['projection'] == 'excluded'] or
            report['attempted_cases'] != len(ids) or report['cases'] != len(ids) - len(excluded_ids) or
            report['source_unsupported_cases'] != len(excluded_ids)):
        raise ValueError('Exclusion or report count differs')
    for row in excluded:
        event = events[row['case']]
        if row['input'] != inputs[row['case']]['input'] or row['reason'] != event['exclusion']:
            raise ValueError('Excluded input/reason differs')
    if len(records) != len(ids) - len(excluded_ids):
        raise ValueError('Incomplete native capture')
    prepared, source_mismatches = [], []
    call = 0
    for row, event in zip(inputs, events):
        if event['memory_faults'] or event['original_outcome'] not in ('returned', 'established_observer_stop'):
            raise ValueError('Unexpected source outcome')
        data = struct.pack('<' + 'I' * len(row['input']), *row['input'])
        input_digest = hashlib.sha256(json.dumps(row['input'], separators=(',', ':')).encode()).hexdigest()
        if event['input_sha256'] != input_digest:
            raise ValueError('Source input binding differs')
        if row['case'] in excluded_ids:
            continue
        if event['projection'] != 'produced' or len(event['expected_words']) != 2048:
            raise ValueError('Missing complete defined source projection')
        record = records[call]
        if (record['call'] != call or record['symbol'] != 'sadx_native_task_delete' or
                type(record['result']) is not int or record['result'] != 1 or
                record['scalar_arguments'] != [None, len(row['input']), None, 2048]):
            raise ValueError('Native call identity differs')
        decoded = {}
        for phase in ('before', 'after'):
            if set(record[phase]) != {'0', '2'}:
                raise ValueError('Native buffer keys differ')
            decoded[phase] = {key: base64.b64decode(value, validate=True)
                              for key, value in record[phase].items()}
            if decoded[phase]['0'] != data or len(decoded[phase]['2']) != 8192:
                raise ValueError('Native input or output extent differs')
        diffs = [[index, x, y] for index, (x, y) in
                 enumerate(zip(event['expected_words'], words(decoded['after']['2']))) if x != y]
        if diffs:
            source_mismatches.append({'case': row['case'], 'input': row['input'], 'diff': diffs})
        prepared.append((row['case'], data, decoded['before']['2'], decoded['after']['2'],
                         event['expected_words']))
        call += 1
    if report['mismatches'] != len(source_mismatches) or report['first_mismatches'] != source_mismatches:
        raise ValueError('Retained source mismatch identities differ')
    return prepared, excluded, source_mismatches


def replay(prepared, invoke):
    results = []
    for case, data, before, old, expected in prepared:
        result, after_input, output = invoke(data, before)
        if type(result) is not int or len(output) != 8192:
            raise ValueError('Invalid candidate return or output extent')
        equal = result == 1 and after_input == data and output == old
        differences = [[i, x, y] for i, (x, y) in enumerate(zip(expected, words(output))) if x != y]
        results.append({'case': case, 'return': result, 'old_current_equal': equal,
                        'output_sha256': hashlib.sha256(output).hexdigest(),
                        'original_differences': differences})
        if not equal:
            break
    return results


def main():
    if not (sys.flags.isolated and sys.flags.no_site and sys.flags.dont_write_bytecode):
        raise ValueError('Use pinned Python with -I -S -B')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('admission', type=Path)
    parser.add_argument('--admission-commit', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    admission = admitted(args.admission, args.admission_commit, root)
    folder = Path(admission['capture_folder'])
    library_path = Path(admission['candidate_library'])
    validator_path = Path(__file__).resolve().parents[3] / 'tools/oracles/task_judges/compare_streams.py'
    required = {str(Path(__file__).resolve()), str(validator_path), str(library_path),
                str(Path(sys.executable).resolve())}
    required.update(str(folder / name) for name in ('inputs.json', 'reserved-observation.json',
                    'task_delete.json', 'task_delete.identity.json', 'task_delete.native.jsonl.gz'))
    if not required.issubset(admission['pins']):
        raise ValueError('Missing required admission pin')
    for name, expected in admission['pins'].items():
        if sha(Path(name)) != expected:
            raise ValueError('Admission pin differs: ' + name)
    artifact_pins = {Path(name).name: expected for name, expected in admission['pins'].items()
                     if Path(name).parent == folder}
    prepared, excluded, mismatches = contracts(folder, artifact_pins)
    expected = admission['expected']
    if (len(prepared) != expected['produced_calls'] or
            [row['case'] for row in excluded] != expected['excluded_ids'] or
            [row['case'] for row in mismatches] != expected['source_mismatch_ids']):
        raise ValueError('Committed cohort accounting differs')
    output = Path(admission['output'])
    output.mkdir(exist_ok=False)
    (output / 'STARTED.json').write_text(json.dumps({'admission_sha256': sha(args.admission)}) + '\n')
    library = ctypes.CDLL(str(library_path))
    fn = library.sadx_native_task_delete
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
                   ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    fn.restype = ctypes.c_int

    attempted, error, results = [], None, []
    with gzip.open(output / 'native-calls.jsonl.gz', 'wt') as stream:
        def invoke(data, before):
            case = prepared[len(attempted)][0]
            attempted.append(case)
            values = (ctypes.c_uint32 * (len(data) // 4)).from_buffer_copy(data)
            out = (ctypes.c_uint32 * 2048).from_buffer_copy(before)
            result = fn(values, len(values), out, 2048)
            stream.write(json.dumps({'case': case, 'result': result,
                         'input_before': base64.b64encode(data).decode(),
                         'output_before': base64.b64encode(before).decode(),
                         'input_after': base64.b64encode(bytes(values)).decode(),
                         'output_after': base64.b64encode(bytes(out)).decode()}) + '\n')
            return result, bytes(values), bytes(out)

        try:
            results = replay(prepared, invoke)
        except BaseException as failure:
            error = {'type': type(failure).__name__, 'message': str(failure)}
    equal = len(results) == len(prepared) and all(row['old_current_equal'] for row in results)
    candidate_mismatches = [row for row in results if row['original_differences']]
    receipt = {'complete_old_current_native_equal': equal, 'compared': len(results),
               'expected': len(prepared), 'excluded': excluded,
               'attempted_native_ids': attempted, 'error': error,
               'unattempted_native_ids': [row[0] for row in prepared if row[0] not in attempted],
               'candidate_source_matches': [row['case'] for row in results if not row['original_differences']],
               'candidate_source_mismatches': candidate_mismatches,
               'source_fidelity': 'FAIL' if candidate_mismatches else 'INCOMPLETE',
               'retained_original_mismatches': mismatches, 'cases': results,
               'limits': 'Native-only replay of captured inputs. No original execution. '
                         'Original mismatch remains a failure; this is not an original pass.'}
    (output / 'RESULT.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return int(not equal)


if __name__ == '__main__':
    raise SystemExit(main())
