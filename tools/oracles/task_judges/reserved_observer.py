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
    """Successful equality requires every generated case, not a shorter capture."""
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
        validate_report(slug, json.loads(report_path.read_text()), cases)
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
