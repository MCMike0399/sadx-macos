"""Additive inputs for unchanged task judges, using their existing byte recorder.

This is not an execution entry point. The final Root runner must verify its
committed dependency freeze and create its single-use admission marker BEFORE
generating cases or calling observe(). Importing this module generates nothing.
"""
import copy
from contextlib import contextmanager
import hashlib
import importlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

from reserved_inputs import arena_sequences, lifecycle_sequences


def input_digest(value):
    return hashlib.sha256(json.dumps(value, separators=(',', ':')).encode()).hexdigest()


@contextmanager
def record_faults(original, machine, events, read_ip):
    """Observe invalid emulated accesses; never map memory or resume a fault."""
    def fault(current, access, address, size, value, unused):
        events.append(dict(pc=read_ip(current), access=access, address=address, size=size, value=value))
        return False
    hook = machine.hook_add(original.unicorn.UC_HOOK_MEM_INVALID, fault)
    try:
        yield
    finally:
        machine.hook_del(hook)


def traversal_outcome(original, source, pc):
    """Classify only the unchanged traversal observer's established hook stops."""
    if source.failure == 0 and pc == original.STOP:
        return 'returned'
    callback_sites = {original.CODE + offset for offset in (0x10, 0x20, 0x30)}
    if source.failure == 1 and pc in callback_sites | {0x4128f0}:
        return 'established_observer_stop'
    if source.failure == 2 and pc == source.boundary and pc in (0x40b510, 0x40b140, 0x40b750):
        return 'established_observer_stop'
    raise ValueError(f'Unestablished traversal outcome: failure={source.failure}, pc={pc:x}')


@contextmanager
def audit_traversal(slug, original, cases, events, read_ip):
    previous = original.Source.run

    def run(source, values):
        index = len(events)
        if index >= len(cases) or values != cases[index]:
            raise ValueError('Original case order/input differs from generated bundle')
        event = dict(case=index, input_sha256=input_digest(values), projection='attempted', memory_faults=[])
        events.append(event)
        try:
            with record_faults(original, source.m, event['memory_faults'], read_ip):
                expected = previous(source, values)
        except Exception as error:
            event.update(error_type=type(error).__name__, error=str(error))
            event['projection'] = 'unexpected_error'
            event['pc'] = read_ip(source.m)
            if event['memory_faults']:
                raise
            if (slug == 'task_delete' and isinstance(error, RuntimeError) and
                    str(error) in ('source free list cycle', 'source reentrant destructor boundary')):
                pc = read_ip(source.m)
                if str(error) == 'source reentrant destructor boundary':
                    if pc not in {original.CODE + offset for offset in (0x10, 0x20, 0x30)}:
                        raise ValueError('Unestablished reentrant destructor boundary') from error
                    outcome = 'established_observer_stop'
                else:
                    outcome = traversal_outcome(original, source, pc)
                event.update(pc=pc, original_outcome=outcome, projection='excluded',
                             exclusion=str(error))
            raise
        pc = read_ip(source.m)
        if event['memory_faults']:
            raise ValueError('Original memory fault was suppressed')
        event.update(pc=pc, source_failure=source.failure, source_boundary=source.boundary,
                     projection='unexpected_stop')
        event.update(original_outcome=traversal_outcome(original, source, pc),
                     expected_words=expected, projection='produced')
        return expected

    try:
        original.Source.run = run
        yield
        if len(events) != len(cases):
            raise ValueError('Incomplete original traversal attempts')
    finally:
        original.Source.run = previous


@contextmanager
def audit_helpers(slug, original, cases, events, read_ip):
    previous = original.call
    active = None
    machine = None

    def call(current, entry, arguments):
        nonlocal active, machine
        if entry == 0x40b1ce:
            index = len(events)
            control = slug == 'task_lifecycle' and index == len(cases)
            if index >= len(cases) and not control:
                raise ValueError('Unexpected original helper sequence')
            active = dict(case=None if control else index, calls=[],
                          kind='repeated_inactive_control' if control else 'generated_sequence')
            events.append(active)
        if active is None:
            raise ValueError('Original helper before sequence initialization')
        machine = current
        # The lifecycle pool initializer uses emu_start directly between these
        # Python helpers. Observe its actual end PC before the first operation.
        if slug == 'task_lifecycle' and len(active['calls']) == 1:
            active['pool_initializer_end_pc'] = read_ip(current)
            if read_ip(current) != original.STOP:
                raise ValueError('Original pool initializer did not reach STOP')
        row = dict(entry=entry, arguments=list(arguments), outcome='attempted', memory_faults=[])
        active['calls'].append(row)
        try:
            with record_faults(original, current, row['memory_faults'], read_ip):
                result = previous(current, entry, arguments)
        except Exception as error:
            row.update(outcome='unexpected_error', error_type=type(error).__name__, error=str(error),
                       pc=read_ip(current))
            raise
        pc = read_ip(current)
        if row['memory_faults']:
            raise ValueError('Original helper memory fault was suppressed')
        row.update(pc=pc, result=result, outcome='returned' if pc == original.STOP else 'unexpected_stop')
        if pc != original.STOP:
            raise ValueError('Original helper did not reach STOP')
        return result

    try:
        original.call = call
        yield
        expected_count = len(cases) + (slug == 'task_lifecycle')
        if len(events) != expected_count:
            raise ValueError('Incomplete original helper sequence attempts')
        op_entries = {0: 0x40b030, 1: 0x40b860, 2: 0x40b940, 3: 0x40b570,
                      4: 0x40b6c0, 5: 0x40b0c0, 6: 0x40b7e0}
        for event, case in zip(events, cases):
            entries = [0x40b1ce]
            for op in case:
                if slug == 'task_work':
                    entries.append(0x40b220 if op[0] == 0 else 0x40b310)
                elif op[0] == 9:
                    entries.extend([0x40b220] * 7)
                elif op[0] not in (7, 8, 10):
                    entries.append(op_entries[op[0]])
            if [row['entry'] for row in event['calls']] != entries:
                raise ValueError('Original helper operation sequence changed')
            event.update(input_sha256=input_digest(case), original_outcome='helpers_returned')
        if slug == 'task_lifecycle':
            control = events[-1]
            pc = read_ip(machine)
            if pc != 0x401000 or [row['entry'] for row in control['calls']] != [0x40b1ce, 0x40b860]:
                raise ValueError('Inactive control did not reach established diagnostic stop')
            control.update(original_outcome='established_observer_stop', pc=pc)
    finally:
        original.call = previous


def generate(slug, definition, original):
    """Generate only when called by the admitted runner; keep fixed rows labeled."""
    seed = definition['seed']
    if slug in ('task_work', 'task_lifecycle'):
        generator = arena_sequences if slug == 'task_work' else lifecycle_sequences
        cases = generator(seed, definition['sequences'],
                          definition['operations_before_final_release'])
        labels = ['fresh'] * len(cases)
    elif slug == 'task_display':
        count = definition['random_cases']
        cases = original.fixtures(seed, repetitions=count)
        if len(cases) != count + 44:
            raise ValueError('Display generator fixed-row count changed')
        labels = ['fresh'] * count + ['repeated_fixed'] * 44
    elif slug == 'task_delete':
        count = definition['random_cases']
        cases = original.fixtures(seed, count)
        if len(cases) != 58 + count:
            raise ValueError('Delete generator fixed-row count changed')
        labels = ['repeated_fixed'] * 58 + ['fresh'] * count
    else:
        raise ValueError('Unknown task cohort: ' + slug)
    return cases, labels


def capture_cases(slug, original, recorder, cases, definition, exe, library, output):
    """Only substitute additive inputs, then call the unmodified main/comparator.

    The delete judge's supported CLI seed/count must agree with its report even
    though its fixture function receives the already generated, frozen cases.
    All monkeypatches are local to this serial observer and restored on failure.
    """
    previous_fixtures, previous_main, previous_argv = original.fixtures, original.main, sys.argv
    supplied = 0

    def fixtures(*args, **kwargs):
        nonlocal supplied
        supplied += 1
        return copy.deepcopy(cases)

    def main():
        if slug == 'task_delete':
            sys.argv += ['--seed', str(definition['seed']),
                         '--cases', str(definition['random_cases'])]
        return previous_main()

    try:
        original.fixtures, original.main = fixtures, main
        sys.argv = [recorder.__file__, slug, str(exe), str(library), str(output)]
        result = recorder.main()
        if supplied != 1:
            raise ValueError(f'Expected one complete fixture bundle, supplied {supplied}')
        return result
    finally:
        original.fixtures, original.main, sys.argv = previous_fixtures, previous_main, previous_argv


def validate_report(slug, report, cases):
    """Require complete report identities; validate_capture binds actual records."""
    if slug in ('task_work', 'task_lifecycle'):
        rows = report['sequences']
        if len(rows) != len(cases):
            raise ValueError('Incomplete sequence comparison')
        for index, (row, operations) in enumerate(zip(rows, cases)):
            if row['sequence'] != index or row['operations'] != len(operations):
                raise ValueError('Sequence identity/order changed')
    elif slug == 'task_display':
        if report['cases'] != len(cases):
            raise ValueError('Incomplete display comparison')
    elif slug == 'task_delete':
        excluded = report['source_unsupported']
        ids = [row['case'] for row in excluded]
        if (report['attempted_cases'] != len(cases) or len(set(ids)) != len(ids) or
                report['cases'] + len(ids) != len(cases)):
            raise ValueError('Incomplete delete comparison')
        for row in excluded:
            index = row['case']
            if type(index) is not int or not 0 <= index < len(cases) or row['input'] != cases[index]:
                raise ValueError('Delete exclusion identity changed')
            if row['reason'] not in ('source free list cycle', 'source reentrant destructor boundary'):
                raise ValueError('Unknown delete exclusion')
    else:
        raise ValueError('Unknown task cohort: ' + slug)


def expected_calls(slug, cases, report):
    """Derive the exact unchanged probe ABI sequence independently of its recorder."""
    arena_bytes = 16 * 1024 * 1024 + 32

    def primary(symbol, words, trace_words=None):
        command = struct.pack('<' + 'I' * len(words), *words)
        if trace_words is None:
            scalars = [None, len(words), None, 2048]
            buffers = {'0': len(command), '2': 2048 * 4}
            returned = 1
        else:
            scalars = [None, len(words), None, arena_bytes, None, trace_words]
            buffers = {'0': len(command), '2': arena_bytes, '4': trace_words * 4}
            returned = 0
        return dict(symbol=symbol, scalars=scalars, buffers=buffers, result=returned,
                    input=dict(bytes=len(command), sha256=hashlib.sha256(command).hexdigest()))

    if slug == 'task_work':
        return [primary('sadx_native_task_work', [v for op in case for v in op], len(case) * 3)
                for case in cases]
    if slug == 'task_lifecycle':
        result = []
        for case, row in zip(cases, report['sequences']):
            result.append(primary('sadx_native_task_lifecycle', [v for op in case for v in op],
                                  len(case) * 3 + 11))
            callback_words = row['callback_count'] * 4
            capacity = callback_words + 16
            result.append(dict(symbol='sadx_native_task_callback_trace',
                               scalars=[None, capacity], buffers={'0': capacity * 4},
                               result=callback_words, input=None))
        # These two calls are the original repeated inactive-membership control,
        # not fresh generated sequences or a normally returned original destroy.
        result.append(primary('sadx_native_task_lifecycle', [1, 2, 7, 0, 3, 0, 0, 0], 17))
        result.append(dict(symbol='sadx_native_task_inactive_boundary', scalars=[None, 4],
                           buffers={'0': 16}, result=0, input=None))
        return result
    if slug == 'task_display':
        return [primary('sadx_native_task_display', case) for case in cases]
    if slug == 'task_delete':
        excluded = {row['case'] for row in report['source_unsupported']}
        return [primary('sadx_native_task_delete', case)
                for index, case in enumerate(cases) if index not in excluded]
    raise ValueError('Unknown task cohort: ' + slug)


def validate_capture(slug, cases, report, output):
    """Bind complete recorded bytes to independent case/symbol/extent expectations."""
    spec = importlib.util.spec_from_file_location('_task_capture_validator',
                                                Path(__file__).with_name('compare_streams.py'))
    validator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(validator)
    expected = expected_calls(slug, cases, report)
    identity = json.loads((output / (slug + '.identity.json')).read_text())
    count = 0
    for index, raw in enumerate(validator.validated_records(output, slug)):
        if index >= len(expected):
            raise ValueError('Extra native capture call')
        row, contract = json.loads(raw), expected[index]
        if (row['symbol'] != contract['symbol'] or type(row['result']) is not int or
                row['result'] != contract['result'] or row['scalar_arguments'] != contract['scalars']):
            raise ValueError(f'Native capture call binding differs at {index}')
        call = identity['calls'][index]
        for phase in ('before', 'after'):
            sizes = {key: value['bytes'] for key, value in call[phase].items()}
            if sizes != contract['buffers']:
                raise ValueError(f'Native capture buffer extents differ at {index}')
            if contract['input'] is not None and call[phase]['0'] != contract['input']:
                raise ValueError(f'Native capture input differs at {index}')
        count += 1
    if count != len(expected):
        raise ValueError(f'Native capture call count {count} differs from expected {len(expected)}')


def validate_source_report(slug, report, cases, events, definition):
    generated = [event for event in events if event['case'] is not None]
    if [event['case'] for event in generated] != list(range(len(cases))):
        raise ValueError('Incomplete ordered source attempt ledger')
    if slug == 'task_display':
        outcomes = {}
        for event in generated:
            key = str(event['source_failure'])
            outcomes[key] = outcomes.get(key, 0) + 1
        if report['source_outcomes'] != outcomes:
            raise ValueError('Source outcome counts differ from observed stops/returns')
    elif slug == 'task_delete':
        excluded = [dict(case=event['case'], input=cases[event['case']], reason=event['exclusion'])
                    for event in generated if event['projection'] == 'excluded']
        if report['source_unsupported'] != excluded or report['seed'] != definition['seed']:
            raise ValueError('Source exclusions/seed differ from actual observations')
    elif slug == 'task_lifecycle':
        control = report['inactive_destroy_boundary']
        if control['original_stop'] != '0x401000' or control['original_return_observed'] is not False:
            raise ValueError('Repeated inactive stop mislabeled as returned')


def observe(slug, cases, labels, definition, exe, library, output):
    """Called only after the final runner has sealed/admitted this whole closure."""
    if (not cases or len(cases) != len(labels) or
            any(label not in ('fresh', 'repeated_fixed') for label in labels)):
        raise ValueError('Missing complete labeled input bundle')
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    inputs = [dict(case=index, provenance=label, input=case)
              for index, (case, label) in enumerate(zip(cases, labels))]
    (output / 'inputs.json').write_text(json.dumps(inputs, separators=(',', ':')) + '\n')
    events, result, error, validated = [], None, None, False
    report_path = output / (slug + '.json')
    try:
        original = importlib.import_module('verify_original_' + slug)
        # Bind the recorder by file, not an unrelated cached "freeze" module.
        spec = importlib.util.spec_from_file_location('_task_reserved_recorder',
                                                    Path(__file__).with_name('freeze.py'))
        recorder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(recorder)
        register = getattr(original, 'UC_X86_REG_EIP', None)
        if register is None:
            from unicorn.x86_const import UC_X86_REG_EIP
            register = UC_X86_REG_EIP
        audit = audit_helpers if slug in ('task_work', 'task_lifecycle') else audit_traversal
        with audit(slug, original, cases, events, lambda machine: machine.reg_read(register)):
            result = capture_cases(slug, original, recorder, cases, definition, exe, library, output)
        if result == 0:
            report = json.loads(report_path.read_text())
            validate_report(slug, report, cases)
            validate_source_report(slug, report, cases, events, definition)
            validate_capture(slug, cases, report, output)
            validated = True
        return result
    except BaseException as failure:
        error = dict(type=type(failure).__name__, message=str(failure))
        raise
    finally:
        attempted = [event['case'] for event in events if event['case'] is not None]
        receipt = dict(slug=slug, oracle_exit=result, error=error, validated=validated,
                       generated_ids=list(range(len(cases))), attempted_source_ids=attempted,
                       unattempted_source_ids=sorted(set(range(len(cases))) - set(attempted)),
                       fresh_ids=[row['case'] for row in inputs if row['provenance'] == 'fresh'],
                       repeated_fixed_ids=[row['case'] for row in inputs
                                           if row['provenance'] == 'repeated_fixed'],
                       inputs_sha256=hashlib.sha256((output / 'inputs.json').read_bytes()).hexdigest(),
                       report_sha256=hashlib.sha256(report_path.read_bytes()).hexdigest()
                       if report_path.exists() else None,
                       source_observations=events,
                       limits='Helper sequences, normally returned traversals, established hook stops '
                              'and excluded projections are distinct. No unexpected memory fault is accepted. '
                              'A source return with an excluded projection is not a completed comparison.')
        (output / 'reserved-observation.json').write_text(json.dumps(receipt, separators=(',', ':')) + '\n')
