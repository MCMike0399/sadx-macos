"""Revised-observer continuation of frozen consumer IDs 1..1023.

No generation, fixture repair or completed-cohort replay. Execution requires a
separate committed Root authorization after independent review of this observer.
"""
import argparse
import ctypes as C
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import traceback

ROOT = Path(__file__).resolve().parents[3]
FINAL = Path('/Users/burbujamc/.t3/worktrees/sadx-macos/t3-0f8beb2a')
PACKET = ROOT / 'docs/refactor-inventory/packets/animation-consumer-fault'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def ordered_inputs(path):
    rows = [json.loads(line) for line in path.read_text().splitlines()]
    if len(rows) != 1024 or [row['case'] for row in rows] != list(range(1024)):
        raise ValueError('Missing, duplicated, reordered or truncated consumer inputs')
    return rows


def completed_ids(records):
    ids = [record['case'] for record in records]
    if ids != list(range(1, len(ids) + 1)):
        raise ValueError('Incomplete or reordered continuation records')
    return [record['case'] for record in records
            if all(name in record for name in ('sonic', 'startup', 'consumer', 'gamma'))
            and 'error' not in record]


def execute(authorization):
    auth = json.loads(authorization.read_text())
    # Bind the authorization itself to committed bytes, never pending edits.
    relative = str(authorization.relative_to(ROOT))
    committed = subprocess.check_output(['git', 'show', 'HEAD:' + relative], cwd=ROOT)
    if committed != authorization.read_bytes() or auth['authorization'] != 'ROOT_REVISED_OBSERVER_CONTINUATION':
        raise ValueError('Missing committed continuation authorization')
    for path, digest in auth['definitions'].items():
        if sha(ROOT / path) != digest:
            raise ValueError('Changed authorized definition: ' + path)
    if auth['case_ids'] != list(range(1, 1024)):
        raise ValueError('Authorization must retain exact remaining frozen IDs')
    frozen = FINAL / 'docs/refactor-inventory/packets/animation-final-cohorts/freeze.json'
    freeze = json.loads(frozen.read_text())
    if sha(frozen) != auth['original_freeze_sha256']:
        raise ValueError('Changed original freeze')
    for group in ('definitions', 'custom_libraries'):
        for path, digest in freeze[group].items():
            if sha(FINAL / path) != digest:
                raise ValueError('Changed frozen dependency: ' + path)
    for path, record in freeze['libraries'].items():
        if sha(FINAL / path) != record['sha256']:
            raise ValueError('Changed frozen numeric library: ' + path)
    inputs = FINAL / 'build/animation-final-cohorts/consumer-attachment.inputs.jsonl'
    if sha(inputs) != auth['inputs_sha256']:
        raise ValueError('Changed frozen inputs')
    rows = ordered_inputs(inputs)
    old_output = FINAL / 'build/animation-final-cohorts/consumer-attachment.outputs.jsonl.gz'
    if sha(old_output) != auth['prior_outputs_sha256']:
        raise ValueError('Changed original failed-run output')
    # Case zero is retained, not rerun. Its exact record is linked in the receipt.
    with gzip.open(old_output, 'rt') as stream:
        prior_zero = json.loads(next(stream))
    if prior_zero['case'] != 0 or prior_zero['input'] != rows[0]:
        raise ValueError('Prior case zero does not bind this input sequence')
    module_path = FINAL / 'tools/oracles/animation_final_cohorts/run.py'
    sys.path.insert(0, str(module_path.parent))
    spec = importlib.util.spec_from_file_location('frozen_consumer_run', module_path)
    run = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(run)
    from consumer_fault_observer import SonicFaultObserver, compare_fault_contract
    _, _, sonic, startup_module = run.modules()
    from pe_image import PEImage
    from extract import SOURCE_SHA256
    image = PEImage(run.EXE)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('Wrong source executable')
    import unicorn
    if unicorn.__version__ != '2.1.4':
        raise ValueError('Wrong Unicorn version')
    observer = SonicFaultObserver(sonic.SonicOriginal(image), sonic)
    startup_oracle = startup_module.StartupOriginal(image)
    parameters = image.read(startup_module.PARAMETERS, 1056)
    constants = b''.join(image.read(address, 4) for address in sonic.CONSTANTS)
    buffer = (C.c_ubyte * len(constants)).from_buffer_copy(constants)
    native = {label: run.function(run.OUT / 'libraries' / label / 'sonic_player.dylib',
              'sadx_native_sonic_player', [run.U8, C.c_size_t, run.U32, run.U32])
              for label in ('old', 'current')}
    custom = {label: {slug: run.function(run.OUT / label / (slug + '.dylib'),
              'final_' + slug, [run.U8, C.c_size_t, run.U32] if slug == 'consumer'
              else [C.c_char_p, run.U32], C.c_char_p)
              for slug in ('consumer', 'gamma')} for label in ('old', 'current')}
    destination = ROOT / 'build/animation-consumer-continuation'
    destination.mkdir(exist_ok=True)
    marker = os.open(destination / 'EXECUTED_ONCE', os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    os.close(marker)
    records = []
    summary = dict(generated_ids=list(range(1024)), prior_completed_ids=[0], attempted_ids=[],
                   fully_observed_ids=[], source_returned_ids=[], bounded_fault_ids=[],
                   mismatch_ids=[], unattempted_ids=list(range(1, 1024)), fp_partial_ids=[],
                   fully_observed_by_component={name: [] for name in ('sonic', 'startup', 'consumer', 'gamma')},
                   established_stop_ids=[],
                   original_process_exit=1, revised_observer_continuation=True,
                   prior_case_zero=prior_zero, authorization_sha256=sha(authorization))
    with gzip.open(destination / 'outputs.jsonl.gz', 'xt') as stream:
        for fixture in rows[1:]:
            case = fixture['case']
            record = dict(case=case, input=fixture)
            summary['attempted_ids'].append(case)
            failed = False
            try:
                source = observer.run(fixture['sonic'])
                outputs = {}
                for label, function in native.items():
                    out = (C.c_uint32 * 40)()
                    rc = function(buffer, len(constants), run.array(fixture['sonic']), out)
                    outputs[label] = dict(return_code=rc, output=list(out))
                if source['original_outcome'] == 'observed_memory_fault' and source['fault_contract']:
                    comparison = compare_fault_contract(source, outputs['old'], outputs['current'])
                    failed = not comparison['qualified_bounded_fault_contract']
                    if not failed:
                        summary['bounded_fault_ids'].append(case)
                elif source['original_outcome'] in ('returned', 'established_observer_stop'):
                    current = outputs['current']['output']
                    differences = [index for index, value in source['defined_prefix'].items()
                                   if index != 17 and current[index] != value]
                    comparison = dict(defined_prefix_differences=differences,
                        complete_native_equality=outputs['old'] == outputs['current'],
                        source_native_fp_partial=current[17] != source['defined_prefix'].get(17, current[17]))
                    failed = bool(differences or outputs['old'] != outputs['current'] or
                                  any(value['return_code'] for value in outputs.values()))
                    if source['original_outcome'] == 'returned':
                        summary['source_returned_ids'].append(case)
                    else:
                        summary['established_stop_ids'].append(case)
                else:
                    comparison = dict(unexpected_original_outcome=True)
                    failed = True
                record['sonic'] = dict(source=source, observations=outputs, comparison=comparison)
                if comparison.get('source_native_fp_partial'):
                    summary['fp_partial_ids'].append([case, 'sonic'])
                # A recognized bounded Sonic fault must not suppress independent observations.
                result, bad = run.startup(fixture['startup'], bytes.fromhex(fixture['events_hex']),
                    fixture['attachment'][1], parameters, startup_oracle)
                record['startup'] = result
                failed |= bad
                if result['fp_partial']:
                    summary['fp_partial_ids'].append([case, 'startup'])
                for slug in ('consumer', 'gamma'):
                    outputs = {label: json.loads(functions[slug](buffer, len(constants),
                        run.array(fixture['attachment'])) if slug == 'consumer' else
                        functions[slug](os.fsencode(run.IMPORTS), run.array(fixture['gamma'])))
                        for label, functions in custom.items()}
                    record[slug] = outputs
                    failed |= outputs['old'] != outputs['current']
            except Exception:
                record['error'] = traceback.format_exc()
                failed = True
            record['mismatch'] = bool(failed)
            records.append(record)
            stream.write(run.dumps(record) + '\n')
            stream.flush()
            summary['fully_observed_ids'] = completed_ids(records)
            for name in summary['fully_observed_by_component']:
                if name in record:
                    summary['fully_observed_by_component'][name].append(case)
            summary['unattempted_ids'] = list(range(case + 1, 1024))
            if failed:
                summary['mismatch_ids'].append(case)
            (destination / 'summary.json').write_text(json.dumps(summary, indent=2, default=str) + '\n')
            if failed:
                return 1
    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authorization', type=Path, required=True)
    args = parser.parse_args()
    raise SystemExit(execute(args.authorization.resolve()))
