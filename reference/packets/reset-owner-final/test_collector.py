"""Fake/read-only adequacy. No model, emulation, reserved execution or asset import."""
import copy
import importlib.util
import json
import os
import selectors
import subprocess
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

PATH = Path(__file__).with_name('collector.py')
spec = importlib.util.spec_from_file_location('reset_collector_under_test', PATH)
collector = importlib.util.module_from_spec(spec)
exec(compile(PATH.read_bytes(), str(PATH), 'exec'), collector.__dict__)


class CollectorTests(unittest.TestCase):
    def test_complete_records_require_one_line(self):
        for raw in (b'', b'{}\n', b'{"case":1}\n{}\n', b'{"case":'):
            with self.subTest(raw=raw), self.assertRaises((ValueError, json.JSONDecodeError)):
                collector.one_record(raw)

    def test_unsupported_scalar_types_reject(self):
        for value in (True, 1.0, False, [True], {'field': 0.0}):
            with self.subTest(value=value), self.assertRaises(ValueError):
                collector.one_record(json.dumps({'value': value}).encode())

    def test_case_domain_is_not_counter_agreement(self):
        expected = ['first', 'second']
        for rows in ([], [{'case': 'first'}], [{'case': 'first'}] * 2,
                     [{'case': 'second'}, {'case': 'first'}]):
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                collector.validate_domain(rows, expected)
        with self.assertRaises(ValueError):
            collector.validate_domain([], [])
        collector.validate_domain([{'case': name} for name in expected], expected)

    def development_record(self, family):
        # Read retained non-reserved evidence only. Tests do not call a binding.
        folder = collector.ROOT / 'build/reset-binding-development-v1'
        row = json.loads((folder / (str(family) + '-current.stdout')).read_text())
        return row, row['case'], row['input']

    def test_real_development_records_validate_without_execution(self):
        for index in range(6):
            row, case, inputs = self.development_record(index)
            collector.validate_interface(row, case, inputs)

    def test_pending_payload_presence_and_value_are_observed(self):
        row, case, inputs = self.development_record(0)
        for pending in (None, [0x40bf7f, 0x40cff0, [0, 0]],
                        [0x40bf7f, 0x40cff0, [1, 7]]):
            changed = copy.deepcopy(row)
            changed['pending'] = pending
            with self.subTest(pending=pending), self.assertRaises(ValueError):
                collector.validate_interface(changed, case, inputs)

    def test_call_truncation_reorder_and_partial_state_reject(self):
        row, case, inputs = self.development_record(1)
        for kind in ('truncate', 'reverse', 'duplicate', 'reseed', 'early_store'):
            changed = copy.deepcopy(row)
            if kind == 'truncate':
                changed['calls'].pop()
            elif kind == 'reverse':
                changed['calls'].reverse()
            elif kind == 'duplicate':
                changed['calls'][0] = changed['calls'][1]
            elif kind == 'reseed':
                changed['calls'][0][1][4] = [1, 0xdead0cab]
            else:
                changed['calls'][0][1][0] = [1, 0]
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                collector.validate_interface(changed, case, inputs)

    def test_alias_and_receiver_identity_are_observed(self):
        row, case, inputs = self.development_record(2)
        row['calls'][26][1][3] = [1, 43]
        with self.assertRaises(ValueError):
            collector.validate_interface(row, case, inputs)
        row, case, inputs = self.development_record(5)
        row['subobservations'][0][-1][1] = 32
        with self.assertRaises(ValueError):
            collector.validate_interface(row, case, inputs)

    def test_changed_scalar_or_unknown_fact_rejects(self):
        row, case, inputs = self.development_record(3)
        for key, value in [('input', inputs[:-1] + [1]),
                           ('state', [[1, 0], [1, 0], [1, 0], [1, 43], [1, 1]])]:
            changed = copy.deepcopy(row)
            changed[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                collector.validate_interface(changed, case, inputs)

    def test_sonic_complete_result_and_order_are_observed(self):
        row = json.loads((collector.ROOT / 'build/reset-binding-development-v1/sonic-current.stdout')
                         .read_text())
        collector.validate_sonic(row, 32768)
        for field, value in ((1, 0x4258f0), (13, 87), (23, 1)):
            changed = copy.deepcopy(row)
            changed['observations'][1][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                collector.validate_sonic(changed, 32768)

    def test_original_projection_cannot_be_empty_or_drop_a_world(self):
        inputs = {'reference_inputs': {'reserved-media-failure': [99, 32768, 42, 43, 1]}}
        words = [3, 0x40bf7f, 0] + [0] * 125
        row = {'case': 'reserved-media-failure', 'kind': 'original_observer_stop',
               'source_returned': 0, 'source_stop_boundary': 0x40bf7f,
               'original_pc': 0x40cff0, 'stop_pc': 0x10000100,
               'input': [99, 32768, 42, 43, 1, 9], 'original_words': words,
               'native': {name: {'abi_return': 1, 'words': words[:]} for name in ('old', 'current')}}
        collector.validate_original(row, row['case'], inputs)
        for kind in ('truncate', 'world', 'return', 'padding', 'classification'):
            changed = copy.deepcopy(row)
            if kind == 'truncate':
                changed['original_words'].pop()
            elif kind == 'world':
                del changed['native']['old']
            elif kind == 'return':
                changed['native']['current']['abi_return'] = 0
            elif kind == 'padding':
                changed['native']['current']['words'][-1] = 7
            else:
                changed['source_returned'] = 1
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                collector.validate_original(changed, row['case'], inputs)

    def test_false_admission_rejects_before_output_or_execution(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / 'never-created'
            freeze = Path(folder) / 'freeze.json'
            freeze.write_text('{}')
            for admission in ({}, {'kind': 'reset_final_admission_v1'},
                              {'kind': 'reset_final_admission_v1', 'root_admitted': False}):
                with patch.object(collector, 'FREEZE', freeze), \
                        patch.object(collector, 'OUTPUT', output), \
                        patch.object(collector, 'committed', return_value=admission), \
                        patch.object(collector, 'execute') as execute:
                    with self.assertRaises(ValueError):
                        collector.run('a' * 40)
                    execute.assert_not_called()
                    self.assertFalse(output.exists())

    def test_complete_comparison_rejects_independently_valid_unequal_records(self):
        old = {'case': 'synthetic', 'words': [0, 1, 0xffffffff], 'optional': [1, 0]}
        current = copy.deepcopy(old)
        current['words'][-1] = 0xfffffffe
        collector.one_record(json.dumps(old).encode())
        collector.one_record(json.dumps(current).encode())
        with self.assertRaises(ValueError):
            collector.compare_complete(old, current)
        collector.compare_complete(old, copy.deepcopy(old))

    def test_positive_admission_with_null_schema_still_has_no_side_effect(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / 'never-created'
            freeze_path = Path(folder) / 'freeze.json'
            freeze_path.write_text('{}')
            admission = {'kind': 'reset_final_admission_v1', 'root_admitted': True,
                         'freeze_sha256': collector.sha(freeze_path), 'output': str(output),
                         'freeze_commit': 'b' * 40}
            malformed = {'kind': 'reset_final_freeze_v1', 'output': str(output),
                         'models': None, 'probes': None, 'pins': None,
                         'protocol': None, 'inputs': None}
            with patch.object(collector, 'FREEZE', freeze_path), \
                    patch.object(collector, 'OUTPUT', output), \
                    patch.object(collector, 'committed', side_effect=[admission, malformed]), \
                    patch.object(collector, 'execute') as execute:
                with self.assertRaises(ValueError):
                    collector.run('a' * 40)
                execute.assert_not_called()
                self.assertFalse(output.exists())

    def test_exclusive_evidence_write(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'receipt.json'
            collector.write_new(path, {'preserved': 1})
            before = path.read_bytes()
            with self.assertRaises(FileExistsError):
                collector.write_new(path, {'preserved': 0})
            self.assertEqual(path.read_bytes(), before)

    def test_parent_pipe_eof_stops_fake_without_game_code(self):
        read_fd, write_fd = os.pipe()
        process = subprocess.Popen([str(collector.ROOT / 'build/reset-final-parent-fake'),
                                    str(read_fd), str(os.getpid())], pass_fds=(read_fd,),
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            with selectors.DefaultSelector() as events:
                events.register(process.stdout, selectors.EVENT_READ)
                self.assertTrue(events.select(timeout=5), 'fake never reached guard')
            self.assertEqual(process.stdout.readline(), b'guarded\n')
            os.close(write_fd)
            write_fd = None
            process.communicate(timeout=5)
            self.assertEqual(process.returncode, 125)
        finally:
            os.close(read_fd)
            if write_fd is not None:
                os.close(write_fd)
            if process.poll() is None:
                process.kill()
                process.communicate()

    def test_wrong_parent_and_nonpipe_reject_before_fake_body(self):
        read_fd, write_fd = os.pipe()
        try:
            result = subprocess.run([str(collector.ROOT / 'build/reset-final-parent-fake'),
                                     str(read_fd), str(os.getpid() + 1)],
                                    pass_fds=(read_fd,), capture_output=True, timeout=5)
            self.assertEqual((result.returncode, result.stdout), (64, b''))
        finally:
            os.close(read_fd)
            os.close(write_fd)
        with open(os.devnull, 'rb') as stream:
            result = subprocess.run([str(collector.ROOT / 'build/reset-final-parent-fake'),
                                     str(stream.fileno()), str(os.getpid())],
                                    pass_fds=(stream.fileno(),), capture_output=True, timeout=5)
            self.assertEqual((result.returncode, result.stdout), (64, b''))

    def test_exact_commit_required(self):
        for value in ('main', 'abc', 'x' * 40, ''):
            with self.subTest(value=value), self.assertRaises(ValueError):
                collector.committed(Path('unused'), value)


if __name__ == '__main__':
    unittest.main()
