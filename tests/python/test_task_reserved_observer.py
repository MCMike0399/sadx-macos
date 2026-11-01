"""Observer plumbing tests use fake modules and tiny nonreserved input bundles."""
import importlib
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest


DIRECTORY = Path(__file__).resolve().parents[2] / 'tools/oracles/task_judges'
sys.path.insert(0, str(DIRECTORY))
try:
    OBSERVER = importlib.import_module('reserved_observer')
finally:
    sys.path.pop(0)


class TaskReservedObserver(unittest.TestCase):
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
