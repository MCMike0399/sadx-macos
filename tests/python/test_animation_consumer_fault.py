"""Fault signatures and defined prefixes fail closed, without executing game code."""
import copy
import importlib.util
from pathlib import Path
import unittest

PATH = Path(__file__).resolve().parents[2] / 'tools/oracles/animation_judges/consumer_fault_observer.py'
spec = importlib.util.spec_from_file_location('consumer_fault', PATH)
observer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(observer)


class ConsumerFaultTests(unittest.TestCase):
    def setUp(self):
        self.fixture = [0] * 36
        self.fixture[0], self.fixture[30] = 2, 1
        self.fault = dict(kind='read_unmapped', pc=0x49aa50, address=6, size=2, ebp=0)
        self.source = dict(original_outcome='observed_memory_fault', fault_contract=True,
                           semantic_boundary=0x49aa50, original_fp_flags=0,
                           defined_prefix={2: 32768, 3: 87, 5: 0, 18: 1151, 19: 1, 20: 1,
                                           21: 1, 22: 87, 23: 31, 24: 123456})
        values = [0] * 40
        for index, value in self.source['defined_prefix'].items():
            values[index] = value
        values[0], values[1] = 2, 0x49aa50
        self.native = dict(return_code=0, output=values)

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
