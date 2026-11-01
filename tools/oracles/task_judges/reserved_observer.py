"""Additive inputs for unchanged task judges, using their existing byte recorder.

This is not an execution entry point. The final Root runner must verify its
committed dependency freeze and create its single-use admission marker BEFORE
generating cases or calling observe(). Importing this module generates nothing.
"""
import copy
import hashlib
import importlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

from reserved_inputs import arena_sequences, lifecycle_sequences


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


def observe(slug, cases, labels, definition, exe, library, output):
    """Called only after the final runner has sealed/admitted this whole closure."""
    if not cases or len(cases) != len(labels):
        raise ValueError('Missing complete labeled input bundle')
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    inputs = [dict(case=index, provenance=label, input=case)
              for index, (case, label) in enumerate(zip(cases, labels))]
    (output / 'inputs.json').write_text(json.dumps(inputs, separators=(',', ':')) + '\n')
    original = importlib.import_module('verify_original_' + slug)
    # Bind this task recorder by file, rather than an unrelated cached module
    # named "freeze". The final runner must seal this file before imports.
    spec = importlib.util.spec_from_file_location('_task_reserved_recorder',
                                                Path(__file__).with_name('freeze.py'))
    recorder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(recorder)
    result = capture_cases(slug, original, recorder, cases, definition, exe, library, output)
    report_path = output / (slug + '.json')
    if result == 0:
        report = json.loads(report_path.read_text())
        validate_report(slug, report, cases)
        validate_capture(slug, cases, report, output)
    receipt = dict(slug=slug, oracle_exit=result, generated_ids=list(range(len(cases))),
                   fresh_ids=[row['case'] for row in inputs if row['provenance'] == 'fresh'],
                   repeated_fixed_ids=[row['case'] for row in inputs
                                       if row['provenance'] == 'repeated_fixed'],
                   inputs_sha256=hashlib.sha256((output / 'inputs.json').read_bytes()).hexdigest(),
                   report_sha256=hashlib.sha256(report_path.read_bytes()).hexdigest(),
                   limits='Source returns/stops/exclusions retain the unchanged oracle policy. '
                          'Generated IDs alone do not prove attempted/completed source execution.')
    (output / 'reserved-observation.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return result
