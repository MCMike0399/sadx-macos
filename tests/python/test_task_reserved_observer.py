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
    def source_fixture(self, failure=0, pc=0x1234, reason=None, fault=False):
        class Machine:
            def __init__(self):
                self.ip = pc
                self.callback = None
            def hook_add(self, kind, callback):
                self.callback = callback
                return 1
            def hook_del(self, hook):
                self.callback = None
            def emu_start(self, entry, stop, *args, **kwargs):
                self.ip = entry
                if getattr(self, 'fault_entry', None) == entry:
                    assert self.callback is not None
                    assert self.callback(self, 19, 6, 2, 0, None) is False
                    raise RuntimeError('synthetic direct lifecycle fault')
                self.ip = stop

        class Source:
            def __init__(self):
                self.m = Machine()
                self.failure = failure
                self.boundary = pc if failure == 2 else 0
            def run(self, values):
                if fault:
                    assert self.m.callback(self.m, 19, 6, 2, 0, None) is False
                    raise RuntimeError('synthetic memory fault')
                if reason:
                    raise RuntimeError(reason)
                return [failure]

        original = SimpleNamespace(Source=Source, STOP=0x1234, CODE=0x23000000,
                                   unicorn=SimpleNamespace(UC_HOOK_MEM_INVALID=1))
        original.Original = lambda: SimpleNamespace(machine=Machine())
        return original, Machine

    def test_actual_stop_required_for_return(self):
        original, _ = self.source_fixture(pc=0x999)
        events = []
        with self.assertRaisesRegex(ValueError, 'Unestablished'):
            with OBSERVER.audit_traversal('task_display', original, [[5]], events, lambda m: m.ip):
                original.Source().run([5])
        self.assertEqual(events[0]['pc'], 0x999)
        self.assertEqual(events[0]['projection'], 'unexpected_stop')

    def test_source_return_and_established_stops_are_distinct(self):
        for failure, pc, outcome in [(0, 0x1234, 'returned'),
                                     (1, 0x23000010, 'established_observer_stop'),
                                     (2, 0x40b510, 'established_observer_stop')]:
            original, _ = self.source_fixture(failure, pc)
            events = []
            with OBSERVER.audit_traversal('task_display', original, [[5]], events, lambda m: m.ip):
                original.Source().run([5])
            self.assertEqual(events[0]['original_outcome'], outcome)

    def test_unknown_stop_site_rejected(self):
        original, _ = self.source_fixture(failure=1, pc=0x999)
        with self.assertRaisesRegex(ValueError, 'Unestablished'):
            with OBSERVER.audit_traversal('task_display', original, [[5]], [], lambda m: m.ip):
                original.Source().run([5])

    def test_exclusion_requires_actual_callback_boundary(self):
        for pc, accepted in [(0x23000010, True), (0x999, False)]:
            original, _ = self.source_fixture(pc=pc, reason='source reentrant destructor boundary')
            events = []
            if accepted:
                with OBSERVER.audit_traversal('task_delete', original, [[5]], events, lambda m: m.ip):
                    with self.assertRaisesRegex(RuntimeError, 'reentrant'):
                        original.Source().run([5])
                self.assertEqual(events[0]['projection'], 'excluded')
            else:
                with self.assertRaisesRegex(ValueError, 'Unestablished'):
                    with OBSERVER.audit_traversal('task_delete', original, [[5]], events, lambda m: m.ip):
                        original.Source().run([5])

    def test_source_return_with_excluded_projection_stays_excluded(self):
        original, _ = self.source_fixture(reason='source free list cycle')
        events = []
        with OBSERVER.audit_traversal('task_delete', original, [[5]], events, lambda m: m.ip):
            with self.assertRaisesRegex(RuntimeError, 'free list cycle'):
                original.Source().run([5])
        self.assertEqual(events[0]['original_outcome'], 'returned')
        self.assertEqual(events[0]['projection'], 'excluded')

    def test_unexpected_memory_fault_signature_retained(self):
        original, _ = self.source_fixture(pc=0x49aa50, fault=True)
        events = []
        with self.assertRaisesRegex(RuntimeError, 'synthetic memory fault'):
            with OBSERVER.audit_traversal('task_display', original, [[5]], events, lambda m: m.ip):
                original.Source().run([5])
        self.assertEqual(events[0]['projection'], 'unexpected_error')
        self.assertEqual(events[0]['memory_faults'],
                         [dict(pc=0x49aa50, access=19, address=6, size=2, value=0)])

    def test_helper_return_and_lifecycle_inactive_control(self):
        original, machine_type = self.source_fixture()
        def call(machine, entry, arguments):
            machine.ip = original.STOP
            return 0
        original.call = call
        events = []
        with OBSERVER.audit_helpers('task_lifecycle', original, [[(1, 2, 0, 0)]],
                                    events, lambda m: m.ip):
            for _ in range(2):
                machine = original.Original().machine
                original.call(machine, 0x40b1ce, [])
                machine.emu_start(0x40b466, original.STOP)
                original.call(machine, 0x40b860, [2, 0, 0])
            machine.emu_start(0x40b570, 0x401000)
        self.assertEqual(events[0]['original_outcome'], 'helpers_returned')
        self.assertEqual(events[1]['kind'], 'repeated_inactive_control')
        self.assertIsNone(events[1]['case'])
        self.assertEqual(events[1]['original_outcome'], 'established_observer_stop')
        self.assertIs(original.call, call)

    def test_helper_stack_return_alone_does_not_prove_stop(self):
        original, machine_type = self.source_fixture(pc=0x999)
        original.call = lambda machine, entry, arguments: 0
        with self.assertRaisesRegex(ValueError, 'did not reach STOP'):
            with OBSERVER.audit_helpers('task_work', original, [[(0, 1)]], [], lambda m: m.ip):
                original.call(original.Original().machine, 0x40b1ce, [])

    def test_direct_lifecycle_faults_retain_exact_failure_evidence(self):
        for phase in ('pool', 'inactive'):
            original, _ = self.source_fixture()
            def call(machine, entry, arguments):
                machine.ip = original.STOP
                return 0
            original.call = call
            prior_constructor = original.Original
            events = []
            with self.assertRaisesRegex(RuntimeError, 'direct lifecycle fault'):
                with OBSERVER.audit_helpers('task_lifecycle', original, [[(1, 2, 0, 0)]],
                                            events, lambda m: m.ip):
                    machine = original.Original().machine
                    original.call(machine, 0x40b1ce, [])
                    if phase == 'pool':
                        machine.fault_entry = 0x40b466
                    machine.emu_start(0x40b466, original.STOP)
                    original.call(machine, 0x40b860, [2, 0, 0])
                    machine = original.Original().machine
                    original.call(machine, 0x40b1ce, [])
                    machine.emu_start(0x40b466, original.STOP)
                    original.call(machine, 0x40b860, [2, 0, 0])
                    machine.fault_entry = 0x40b570
                    machine.emu_start(0x40b570, 0x401000)
            event = events[-1]
            self.assertEqual(event['original_outcome'], 'unexpected_error')
            self.assertEqual(event['direct'][-1]['outcome'], 'unexpected_error')
            expected_pc = 0x40b466 if phase == 'pool' else 0x40b570
            self.assertEqual(event['memory_faults'],
                             [dict(pc=expected_pc, access=19, address=6, size=2, value=0)])
            self.assertEqual(event['direct'][-1]['pc'], expected_pc)
            self.assertIsNone(machine.callback)
            self.assertIs(original.call, call)
            self.assertIs(original.Original, prior_constructor)

    def test_constructor_failure_belongs_to_its_own_setup_attempt(self):
        for successful in (0, 1):
            original, _ = self.source_fixture()
            factory = original.Original
            created = 0
            def constructor():
                nonlocal created
                if created == successful:
                    raise RuntimeError('synthetic constructor setup failure')
                created += 1
                return factory()
            original.Original = constructor
            def call(machine, entry, arguments):
                machine.ip = original.STOP
                return 0
            original.call = call
            events = []
            with self.assertRaisesRegex(RuntimeError, 'constructor setup failure'):
                with OBSERVER.audit_helpers('task_work', original, [[(0, 1)], [(0, 1)]],
                                            events, lambda m: m.ip):
                    for _ in range(successful + 1):
                        machine = original.Original().machine
                        original.call(machine, 0x40b1ce, [])
                        original.call(machine, 0x40b220, [1])
            self.assertEqual(len(events), successful + 1)
            self.assertEqual(events[-1]['case'], successful)
            self.assertIsNone(events[-1]['pc'])
            self.assertFalse(events[-1]['source_attempted'])
            if successful:
                self.assertNotIn('error', events[0])
                self.assertTrue(all(row['outcome'] == 'returned' for row in events[0]['calls']))
            self.assertIs(original.Original, constructor)

    def test_hook_installation_failure_is_a_setup_error(self):
        original, _ = self.source_fixture()
        factory = original.Original
        def constructor():
            instance = factory()
            def fail(*args):
                raise RuntimeError('synthetic hook install failure')
            instance.machine.hook_add = fail
            return instance
        original.Original = constructor
        original.call = lambda *args: 0
        events = []
        with self.assertRaisesRegex(RuntimeError, 'hook install failure'):
            with OBSERVER.audit_helpers('task_work', original, [[(0, 1)]], events, lambda m: m.ip):
                original.Original()
        self.assertEqual(events[0]['setup'], 'constructed')
        self.assertFalse(events[0]['source_attempted'])
        self.assertEqual(events[0]['calls'], [])
        self.assertIs(original.Original, constructor)

    def observe_fake_display(self, mode, captured_errors=None):
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
            module.STOP, module.CODE, module.UC_X86_REG_EIP = 0x1234, 0x23000000, 0
            module.unicorn = SimpleNamespace(UC_HOOK_MEM_INVALID=1)

            class Machine:
                def __init__(self):
                    self.ip = module.STOP
                def reg_read(self, register):
                    return self.ip
                def hook_add(self, kind, callback):
                    return 1
                def hook_del(self, hook):
                    pass

            class Source:
                def __init__(self):
                    self.m = Machine()
                    self.failure = self.boundary = 0
                def run(self, values):
                    if mode == 'source_error':
                        self.m.ip = 0x999
                        raise RuntimeError('synthetic source error')
                    return [0] * 2048

            module.Source = Source
            library = root / 'library'
            library.write_bytes(b'fake-only library identity')

            def main():
                cases = module.fixtures()
                native = ctypes.CDLL(str(library))
                source = module.Source()
                for index, values in enumerate(cases):
                    source.run(values)
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
                try:
                    result = OBSERVER.observe('task_display', [[1], [2]], ['fresh', 'fresh'],
                                              {'seed': 7, 'random_cases': 2},
                                              root / 'unused-exe', library, root / 'capture')
                except BaseException:
                    if captured_errors is not None:
                        captured_errors.append(json.loads(
                            (root / 'capture/reserved-observation.json').read_text()))
                    raise
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

    def test_interrupted_observation_preserves_actual_attempt_prefix(self):
        receipts = []
        with self.assertRaisesRegex(RuntimeError, 'synthetic source error'):
            self.observe_fake_display('source_error', receipts)
        receipt = receipts[0]
        self.assertFalse(receipt['validated'])
        self.assertIsNone(receipt['oracle_exit'])
        self.assertEqual(receipt['attempted_source_ids'], [0])
        self.assertEqual(receipt['unattempted_source_ids'], [1])
        self.assertEqual(receipt['source_observations'][0]['pc'], 0x999)

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
