"""Fault signatures and defined prefixes fail closed, without executing game code."""
import copy
import importlib.util
from pathlib import Path
import unittest
import tempfile
import json
import hashlib

PATH = Path(__file__).resolve().parents[2] / 'tools/oracles/animation_judges/consumer_fault_observer.py'
spec = importlib.util.spec_from_file_location('consumer_fault', PATH)
observer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(observer)


continuation_spec = importlib.util.spec_from_file_location('consumer_continuation',
    PATH.with_name('consumer_continuation.py'))
continuation = importlib.util.module_from_spec(continuation_spec)
continuation_spec.loader.exec_module(continuation)


class ConsumerAccountingTests(unittest.TestCase):
    def test_imported_helpers_and_content_fail_closed_on_drift(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            helper = root / 'tools/oracles/original.py'
            helper.parent.mkdir(parents=True)
            helper.write_text('frozen helper')
            seal = dict(files={'tools/oracles/original.py': hashlib.sha256(helper.read_bytes()).hexdigest()})
            continuation.verify_imported_helpers(root, seal)
            helper.write_text('changed comparator')
            with self.assertRaisesRegex(ValueError, 'Changed consumed dependency'):
                continuation.verify_imported_helpers(root, seal)
            helper.write_text('frozen helper')
            extra = helper.with_name('new_helper.py'); extra.write_text('new import')
            with self.assertRaisesRegex(ValueError, 'additional imported Python helper'):
                continuation.verify_imported_helpers(root, seal)
            extra.unlink(); helper.unlink()
            with self.assertRaisesRegex(ValueError, 'Missing or additional'):
                continuation.verify_imported_helpers(root, seal)
            asset = root / 'gamma.json'; asset.write_text('frozen asset')
            hashes = {str(asset): hashlib.sha256(asset.read_bytes()).hexdigest()}
            continuation.verify_hashes(hashes)
            asset.write_text('different table')
            with self.assertRaisesRegex(ValueError, 'Changed consumed dependency'):
                continuation.verify_hashes(hashes)

    def test_all_frozen_ids_must_be_present_once_in_order(self):
        rows = [dict(case=index) for index in range(1024)]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'inputs.jsonl'
            for values, valid in ((rows, True), ([], False), (rows[:-1], False),
                                  ([rows[1], rows[0]] + rows[2:], False),
                                  (rows + [rows[-1]], False)):
                path.write_text(''.join(json.dumps(row) + '\n' for row in values))
                if valid:
                    self.assertEqual(continuation.ordered_inputs(path), rows)
                else:
                    with self.assertRaises(ValueError):
                        continuation.ordered_inputs(path)

    def test_full_observation_requires_each_independent_component(self):
        record = dict(case=1, sonic={}, startup={}, consumer={}, gamma={})
        self.assertEqual(continuation.completed_ids([record]), [1])
        for key in ('sonic', 'startup', 'consumer', 'gamma'):
            changed = dict(record); del changed[key]
            self.assertEqual(continuation.completed_ids([changed]), [])
        self.assertEqual(continuation.completed_ids([dict(record, error='stopped')]), [])
        for records in ([dict(record, case=2)], [record, record]):
            with self.assertRaises(ValueError):
                continuation.completed_ids(records)


class ConsumerFaultTests(unittest.TestCase):
    def setUp(self):
        self.fixture = [0] * 36
        self.fixture[0], self.fixture[30] = 2, 1
        self.fault = dict(kind='read_unmapped', pc=0x49aa50, address=6, size=2, ebp=0)
        self.source = dict(original_outcome='observed_memory_fault', fault_contract=True,
                           semantic_boundary=0x49aa50, original_fp_flags=0,
                           writes=[dict(address=0x3abdf64, size=4, value=0)],
                           defined_prefix={2: 32768, 3: 87, 5: 0, 18: 1151, 19: 1, 20: 1,
                                           21: 1, 22: 87, 23: 31, 24: 123456})
        values = [0] * 40
        for index, value in self.source['defined_prefix'].items():
            values[index] = value
        values[0], values[1] = 2, 0x49aa50
        self.native = dict(return_code=0, output=values)

    def test_original_return_requires_actual_stop_pc(self):
        stop = 0x1234
        self.assertEqual(observer.original_outcome(stop, 0, stop, {0x49ab30}), 'returned')
        self.assertEqual(observer.original_outcome(0x49ab30, 0x49ab30, stop, {0x49ab30}),
                         'established_observer_stop')
        for pc, boundary in ((0x49aa20, 0), (stop, 0x49ab30),
                             (0x49ab30, 0x49ab30), (0x49ab30, 0x49ab31)):
            self.assertEqual(observer.original_outcome(pc, boundary, stop, set()), 'unexpected_error')

    def test_only_established_reached_signature_is_accepted(self):
        self.assertTrue(observer.established_null_player_fault(self.fixture, self.fault))
        for key, value in [('kind', 'write_unmapped'), ('pc', 0x49a9c0),
                           ('address', 0), ('size', 4), ('ebp', 1)]:
            changed = dict(self.fault, **{key: value})
            self.assertFalse(observer.established_null_player_fault(self.fixture, changed))
        for index, value in ((0, 1), (30, 0), (35, 1)):
            changed = self.fixture[:]
            changed[index] = value
            self.assertFalse(observer.established_null_player_fault(changed, self.fault))

    def test_equal_native_values_do_not_excuse_changed_source_prefix(self):
        good = observer.compare_fault_contract(self.source, self.native, self.native)
        self.assertTrue(good['qualified_bounded_fault_contract'])
        self.assertFalse(good['original_returned'])
        for index in self.source['defined_prefix']:
            changed = copy.deepcopy(self.native)
            changed['output'][index] ^= 1
            result = observer.compare_fault_contract(self.source, changed, changed)
            self.assertFalse(result['qualified_bounded_fault_contract'])

    def test_native_boundary_full_result_and_return_are_independent(self):
        for index in (0, 1, 39):
            changed = copy.deepcopy(self.native)
            changed['output'][index] ^= 1
            result = observer.compare_fault_contract(self.source, self.native, changed)
            self.assertFalse(result['qualified_bounded_fault_contract'])
        changed = dict(self.native, return_code=1)
        self.assertFalse(observer.compare_fault_contract(self.source, changed, changed)
                         ['qualified_bounded_fault_contract'])
        with self.assertRaises(ValueError):
            observer.compare_fault_contract(self.source, self.native,
                                            dict(return_code=0, output=self.native['output'][:39]))

    def test_written_null_requires_the_actual_publication_event(self):
        for writes in ([], [dict(address=0x3abdf60, size=4, value=0)],
                       [dict(address=0x3abdf64, size=2, value=0)],
                       [dict(address=0x3abdf64, size=4, value=1)]):
            result = observer.compare_fault_contract(dict(self.source, writes=writes),
                                                     self.native, self.native)
            self.assertFalse(result['qualified_bounded_fault_contract'])

    def test_fp_difference_remains_a_separate_partial(self):
        changed = copy.deepcopy(self.native)
        changed['output'][17] = 32
        result = observer.compare_fault_contract(self.source, changed, changed)
        self.assertTrue(result['qualified_bounded_fault_contract'])
        self.assertTrue(result['source_native_fp_partial'])
        with self.assertRaises(ValueError):
            observer.compare_fault_contract(dict(self.source, fault_contract=False),
                                            self.native, self.native)


if __name__ == '__main__':
    unittest.main()
