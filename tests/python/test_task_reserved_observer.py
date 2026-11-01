"""Observer plumbing tests use fake modules and tiny nonreserved input bundles."""
import importlib
import ctypes
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from types import ModuleType, SimpleNamespace
import unittest
from unittest.mock import patch


DIRECTORY = Path(__file__).resolve().parents[2] / 'tools/oracles/task_judges'
sys.path.insert(0, str(DIRECTORY))
try:
    OBSERVER = importlib.import_module('reserved_observer')
finally:
    sys.path.pop(0)


class TaskReservedObserver(unittest.TestCase):
    def observe_fake_display(self, mode):
        """Use the real recorder; a Python callable stands in for the native ABI."""
        class Function:
            def __call__(self, values, count, output, capacity):
                return 1

        class Library:
            def __init__(self, *args, **kwargs):
                self.sadx_native_task_display = Function()

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            module = ModuleType('verify_original_task_display')
            module.__file__ = str(root / 'original.py')
            Path(module.__file__).write_text('# fake-only original module\n')
            module.fixtures = lambda: []
            library = root / 'library'
            library.write_bytes(b'fake-only library identity')

            def main():
                cases = module.fixtures()
                native = ctypes.CDLL(str(library))
                for index, values in enumerate(cases):
                    actual = [99] if mode == 'wrong_input' and index == 1 else values
                    commands = (ctypes.c_uint32 * len(actual))(*actual)
                    words = 2047 if mode == 'short_output' else 2048
                    output = (ctypes.c_uint32 * words)()
                    if mode == 'missing_output':
                        output = ctypes.cast(output, ctypes.POINTER(ctypes.c_uint32))
                    fn = native.sadx_native_task_display
                    if mode == 'bypass' and index == 1:
                        fn = native.lib.sadx_native_task_display
                    fn(commands, len(actual), output, 2048)
                path = Path(sys.argv[sys.argv.index('--output') + 1])
                path.write_text(json.dumps(dict(cases=len(cases), source_outcomes={'0': len(cases)})))
                return 0

            module.main = main
            with patch.dict(sys.modules, {'verify_original_task_display': module}), \
                    patch.object(ctypes, 'CDLL', Library):
                result = OBSERVER.observe('task_display', [[1], [2]], ['fresh', 'fresh'],
                                          {'seed': 7, 'random_cases': 2},
                                          root / 'unused-exe', library, root / 'capture')
            # A valid transcript must also pass the unchanged full-record comparison.
            compared = subprocess.run(
                [sys.executable, str(DIRECTORY / 'compare_streams.py'),
                 str(root / 'capture'), str(root / 'capture'), '--slug', 'task_display',
                 '--output', str(root / 'comparison.json')], capture_output=True, text=True)
            self.assertEqual(compared.returncode, 0, compared.stderr)
            return result

    def test_real_recorder_complete_fake_integration(self):
        self.assertEqual(self.observe_fake_display('complete'), 0)

    def test_real_recorder_bypassed_call_rejected(self):
        with self.assertRaisesRegex(ValueError, 'capture|call'):
            self.observe_fake_display('bypass')

    def test_real_recorder_shrunken_output_rejected(self):
        with self.assertRaisesRegex(ValueError, 'capture|buffer|extent'):
            self.observe_fake_display('short_output')

    def test_real_recorder_missing_output_rejected(self):
        with self.assertRaisesRegex(ValueError, 'capture|buffer|extent'):
            self.observe_fake_display('missing_output')

    def test_real_recorder_wrong_input_rejected(self):
        with self.assertRaisesRegex(ValueError, 'capture|input'):
            self.observe_fake_display('wrong_input')

    def test_work_contract_extents(self):
        calls = OBSERVER.expected_calls('task_work', [[(0, 1), (1, 0)]], {})
        self.assertEqual(len(calls), 1)
        self.assertEqual(calls[0]['symbol'], 'sadx_native_task_work')
        self.assertEqual(calls[0]['scalars'], [None, 4, None, 16777248, None, 6])
        self.assertEqual(calls[0]['buffers'], {'0': 16, '2': 16777248, '4': 24})

    def test_lifecycle_contract_includes_callback_and_inactive_controls(self):
        calls = OBSERVER.expected_calls('task_lifecycle', [[(1, 2, 0, 0)]],
                                       {'sequences': [{'callback_count': 1}]})
        self.assertEqual([c['symbol'] for c in calls],
                         ['sadx_native_task_lifecycle', 'sadx_native_task_callback_trace',
                          'sadx_native_task_lifecycle', 'sadx_native_task_inactive_boundary'])
        self.assertEqual(calls[0]['scalars'][-1], 14)
        self.assertEqual(calls[1]['result'], 4)
        self.assertEqual(calls[1]['buffers'], {'0': 80})
        self.assertEqual(calls[2]['scalars'][-1], 17)
        self.assertEqual(calls[3]['buffers'], {'0': 16})

    def test_delete_contract_omits_only_exact_excluded_ids(self):
        calls = OBSERVER.expected_calls('task_delete', [[1], [2], [3]],
                                       {'source_unsupported': [{'case': 1}]})
        self.assertEqual(len(calls), 2)
        self.assertEqual(calls[0]['symbol'], 'sadx_native_task_delete')
        self.assertEqual(calls[1]['scalars'], [None, 1, None, 2048])
        self.assertNotEqual(calls[0]['input']['sha256'], calls[1]['input']['sha256'])

    def test_display_fixed_rows_are_separate(self):
        original = SimpleNamespace(fixtures=lambda seed, repetitions: [[seed]] * (repetitions + 44))
        cases, labels = OBSERVER.generate('task_display', {'seed': 7, 'random_cases': 3}, original)
        self.assertEqual(len(cases), 47)
        self.assertEqual(labels, ['fresh'] * 3 + ['repeated_fixed'] * 44)

    def test_delete_fixed_rows_are_separate(self):
        original = SimpleNamespace(fixtures=lambda seed, count: [[seed]] * (count + 58))
        cases, labels = OBSERVER.generate('task_delete', {'seed': 7, 'random_cases': 3}, original)
        self.assertEqual(len(cases), 61)
        self.assertEqual(labels, ['repeated_fixed'] * 58 + ['fresh'] * 3)

    def test_changed_fixed_generator_shape_rejected(self):
        original = SimpleNamespace(fixtures=lambda *args, **kwargs: [])
        for slug in ('task_display', 'task_delete'):
            with self.assertRaises(ValueError):
                OBSERVER.generate(slug, {'seed': 7, 'random_cases': 3}, original)

    def test_capture_restores_originals_and_supplies_exact_delete_metadata(self):
        previous_argv = sys.argv
        cases, seen = [[1, 2]], []

        def original_main():
            seen.append(list(sys.argv))
            supplied = original.fixtures(999, 999)
            self.assertEqual(supplied, cases)
            supplied[0][0] = 999
            return 0

        original = SimpleNamespace(fixtures=lambda: 'old', main=original_main)
        old_fixtures = original.fixtures

        def freeze_main():
            sys.argv = ['original.py', 'exe', 'library', '--output', 'report']
            return original.main()

        recorder = SimpleNamespace(__file__='capture.py', main=freeze_main)
        self.assertEqual(OBSERVER.capture_cases('task_delete', original, recorder, cases,
                                              {'seed': 7, 'random_cases': 3},
                                              'exe', 'library', 'out'), 0)
        self.assertEqual(cases, [[1, 2]])
        self.assertEqual(seen[0][-4:], ['--seed', '7', '--cases', '3'])
        self.assertIs(original.fixtures, old_fixtures)
        self.assertIs(original.main, original_main)
        self.assertIs(sys.argv, previous_argv)

    def test_capture_restores_originals_after_failure(self):
        def fail():
            raise RuntimeError('synthetic failure')
        original = SimpleNamespace(fixtures=lambda: [], main=fail)
        old_fixtures, old_argv = original.fixtures, sys.argv
        recorder = SimpleNamespace(__file__='capture.py', main=lambda: original.main())
        with self.assertRaisesRegex(RuntimeError, 'synthetic failure'):
            OBSERVER.capture_cases('task_work', original, recorder, [[(0, 1)]], {}, 'e', 'l', 'o')
        self.assertIs(original.fixtures, old_fixtures)
        self.assertIs(original.main, fail)
        self.assertIs(sys.argv, old_argv)

    def test_empty_fixture_consumption_rejected(self):
        original = SimpleNamespace(fixtures=lambda: [], main=lambda: 0)
        recorder = SimpleNamespace(__file__='capture.py', main=lambda: original.main())
        with self.assertRaisesRegex(ValueError, 'supplied 0'):
            OBSERVER.capture_cases('task_work', original, recorder, [[(0, 1)]], {}, 'e', 'l', 'o')

    def test_complete_sequence_report_and_order(self):
        cases = [[(0, 1)], [(0, 2), (1, 0)]]
        report = {'sequences': [{'sequence': 0, 'operations': 1},
                                {'sequence': 1, 'operations': 2}]}
        OBSERVER.validate_report('task_work', report, cases)
        for rows in ([], report['sequences'][:1], list(reversed(report['sequences']))):
            with self.assertRaises(ValueError):
                OBSERVER.validate_report('task_work', {'sequences': rows}, cases)

    def test_exact_delete_exclusions(self):
        cases = [[1], [2]]
        report = dict(attempted_cases=2, cases=1,
                      source_unsupported=[dict(case=1, input=[2], reason='source free list cycle')])
        OBSERVER.validate_report('task_delete', report, cases)
        for change in (dict(attempted_cases=1), dict(cases=0),
                       dict(source_unsupported=[dict(case=1, input=[1], reason='source free list cycle')]),
                       dict(source_unsupported=[dict(case=1, input=[2], reason='unknown')])):
            with self.assertRaises(ValueError):
                OBSERVER.validate_report('task_delete', dict(report, **change), cases)


if __name__ == '__main__':
    unittest.main()
