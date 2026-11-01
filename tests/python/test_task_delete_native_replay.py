"""Fake-only adequacy checks; no game libraries or reserved inputs are loaded."""
import base64
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / 'reference/packets/task-delete-native-replay/replay.py'
SPEC = importlib.util.spec_from_file_location('deletion_replay', SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
exec(compile(SCRIPT.read_bytes(), str(SCRIPT), 'exec'), MODULE.__dict__)


def digest(data):
    return hashlib.sha256(data).hexdigest()


class DeleteNativeReplay(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)
        self.inputs = [{'case': i, 'provenance': 'fresh', 'input': [i]} for i in range(3)]
        self.events, self.records = [], []
        self.zeros = bytes(8192)
        self.changed = struct.pack('<I', 116) + bytes(8188)
        for i in range(3):
            value = [i]
            event = {'case': i, 'input_sha256': digest(json.dumps(value, separators=(',', ':')).encode()),
                     'memory_faults': [], 'original_outcome': 'returned',
                     'projection': 'excluded' if i == 2 else 'produced', 'expected_words': [0] * 2048}
            if i == 2:
                event.update(original_outcome='established_observer_stop', exclusion='source reentrant destructor boundary')
            self.events.append(event)
            if i == 2:
                continue
            data = struct.pack('<I', i)
            output = self.changed if i == 1 else self.zeros
            encode = lambda x: base64.b64encode(x).decode()
            self.records.append({'call': i, 'symbol': 'sadx_native_task_delete', 'result': 1,
                                 'scalar_arguments': [None, 1, None, 2048],
                                 'before': {'0': encode(data), '2': encode(self.zeros)},
                                 'after': {'0': encode(data), '2': encode(output)}})
        self.excluded = [{'case': 2, 'input': [2], 'reason': 'source reentrant destructor boundary'}]
        self.mismatches = [{'case': 1, 'input': [1], 'diff': [[0, 0, 116]]}]
        self.observation = {'generated_ids': [0, 1, 2], 'attempted_source_ids': [0, 1, 2],
                            'unattempted_source_ids': [], 'error': None, 'source_observations': self.events}
        self.report = {'source_unsupported': self.excluded, 'attempted_cases': 3, 'cases': 2,
                       'source_unsupported_cases': 1, 'mismatches': 1, 'first_mismatches': self.mismatches}

    def write(self):
        def save(name, data):
            (self.folder / name).write_text(json.dumps(data))
        save('inputs.json', self.inputs)
        self.observation['inputs_sha256'] = MODULE.sha(self.folder / 'inputs.json')
        save('reserved-observation.json', self.observation)
        save('task_delete.json', self.report)
        path = self.folder / 'task_delete.native.jsonl.gz'
        with gzip.open(path, 'wt') as stream:
            for record in self.records:
                stream.write(json.dumps(record) + '\n')
        calls = []
        for record in self.records:
            row = {key: record[key] for key in ('call', 'symbol', 'result')}
            for phase in ('before', 'after'):
                row[phase] = {}
                for key, encoded in record[phase].items():
                    data = base64.b64decode(encoded)
                    row[phase][key] = {'bytes': len(data), 'sha256': digest(data)}
            calls.append(row)
        save('task_delete.identity.json', {'slug': 'task_delete', 'oracle_exit': 1,
             'compressed_sha256': MODULE.sha(path), 'calls': calls})

    def reject(self):
        self.write()
        with self.assertRaises((ValueError, KeyError, IndexError)):
            MODULE.contracts(self.folder, self.pins())

    def pins(self):
        return {p.name: MODULE.sha(p) for p in self.folder.iterdir() if p.is_file()}

    def test_failed_original_capture_is_preserved_separately(self):
        self.write()
        prepared, exclusions, mismatches = MODULE.contracts(self.folder, self.pins())
        self.assertEqual([row[0] for row in prepared], [0, 1])
        self.assertEqual(exclusions, self.excluded)
        self.assertEqual(mismatches, self.mismatches)
        outputs = {row[1]: row[3] for row in prepared}
        results = MODULE.replay(prepared, lambda data, before: (1, data, outputs[data]))
        self.assertTrue(all(row['old_current_equal'] for row in results))
        self.assertEqual(results[1]['original_differences'], [[0, 0, 116]])

    def test_empty_native_capture(self):
        self.records = []
        self.reject()

    def test_truncated_native_capture(self):
        self.records.pop()
        self.reject()

    def test_reordered_native_calls(self):
        self.records.reverse()
        self.reject()

    def test_duplicate_native_call(self):
        self.records[1] = self.records[0]
        self.reject()

    def test_coherently_truncated_source_ledger(self):
        self.events.pop()
        self.reject()

    def test_changed_input_with_coherent_capture(self):
        self.inputs[0]['input'] = [42]
        self.reject()

    def test_dropped_buffer_extent(self):
        self.records[0]['after']['2'] = base64.b64encode(bytes(8191)).decode()
        self.reject()

    def test_dropped_original_mismatch(self):
        self.report['mismatches'] = 0
        self.report['first_mismatches'] = []
        self.reject()

    def test_changed_exclusion_reason(self):
        self.excluded[0]['reason'] = 'guessed exception'
        self.reject()

    def test_memory_fault_never_accepted(self):
        self.events[0]['memory_faults'] = [{'address': 6}]
        self.reject()

    def test_wrong_case_mapping_with_coherent_call_numbers(self):
        self.records.reverse()
        for index, record in enumerate(self.records):
            record['call'] = index
        self.reject()

    def test_changed_padding_does_not_hide_in_same_mismatch_count(self):
        changed = bytearray(self.changed)
        changed[-1] = 1
        self.records[1]['after']['2'] = base64.b64encode(changed).decode()
        self.reject()

    def test_changed_source_expectation_same_mismatch_count(self):
        self.events[1]['expected_words'][0] = 42
        self.reject()

    def test_removed_exclusion(self):
        self.excluded.clear()
        self.report['source_unsupported_cases'] = 0
        self.report['cases'] = 3
        self.reject()

    def test_artifact_change_after_pin(self):
        self.write()
        pins = self.pins()
        self.inputs[0]['input'] = [999]
        self.write()
        with self.assertRaisesRegex(ValueError, 'artifact pin differs'):
            MODULE.contracts(self.folder, pins)

    def test_candidate_change_stops_at_first_unequal_call(self):
        self.write()
        prepared, _, _ = MODULE.contracts(self.folder, self.pins())
        called = []
        def invoke(data, before):
            called.append(data)
            return 1, data, self.changed
        result = MODULE.replay(prepared, invoke)
        self.assertEqual(len(called), 1)
        self.assertFalse(result[0]['old_current_equal'])

    def test_candidate_return_and_input_changes_are_observed(self):
        self.write()
        prepared, _, _ = MODULE.contracts(self.folder, self.pins())
        for invoke in (lambda data, before: (0, data, self.zeros),
                       lambda data, before: (1, bytes(4), self.changed)):
            result = MODULE.replay(prepared[1:], invoke)
            self.assertFalse(result[0]['old_current_equal'])

    def test_committed_admission_rejects_coherent_replacement(self):
        root = self.folder
        subprocess.run(['git', 'init', '-q', str(root)], check=True)
        admission = root / 'admission.json'
        admission.write_text('{"pins":{"original":"abc"}}\n')
        subprocess.run(['git', '-C', str(root), 'add', 'admission.json'], check=True)
        subprocess.run(['git', '-C', str(root), '-c', 'user.name=Test', '-c',
                        'user.email=test@example.invalid', 'commit', '-qm', 'Freeze fake admission'], check=True)
        commit = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
        self.assertEqual(MODULE.admitted(admission, commit, root), {'pins': {'original': 'abc'}})
        admission.write_text('{"pins":{"replacement":"def"}}\n')
        with self.assertRaisesRegex(ValueError, 'committed Root proof'):
            MODULE.admitted(admission, commit, root)

    def test_missing_or_ambiguous_admission_commit(self):
        for commit in ('', 'HEAD', 'a' * 39, 'z' * 40):
            with self.assertRaisesRegex(ValueError, 'Exact admission'):
                MODULE.admitted(self.folder / 'absent.json', commit, self.folder)


if __name__ == '__main__':
    unittest.main()
