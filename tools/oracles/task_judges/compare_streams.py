#!/usr/bin/env python3
"""Compare every captured task ABI byte, scalar, status and record identity."""
import argparse
import base64
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import sys


def validated_records(folder, slug):
    """Bind each capture to its complete, ordered freeze ledger before comparing."""
    path = folder / (slug + '.native.jsonl.gz')
    identity = json.loads((folder / (slug + '.identity.json')).read_text())
    calls = identity['calls']
    if identity['slug'] != slug or identity['oracle_exit'] != 0 or not calls:
        raise ValueError(f'{folder}: missing successful nonempty {slug} capture')
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    if digest != identity['compressed_sha256']:
        raise ValueError(f'{path}: capture digest differs from freeze ledger')
    count = 0
    with gzip.open(path, 'rb') as stream:
        for index, raw in enumerate(stream):
            if index >= len(calls):
                raise ValueError(f'{path}: extra record {index}')
            row = json.loads(raw)
            call = calls[index]
            if (type(row['call']) is not int or row['call'] != index or
                    type(call['call']) is not int or call['call'] != index):
                raise ValueError(f'{path}: incomplete or reordered call identity at {index}')
            if not row['symbol'] or any(row[key] != call[key] for key in ('symbol', 'result')):
                raise ValueError(f'{path}: call {index} differs from freeze ledger')
            for phase in ('before', 'after'):
                observed = {}
                for key, encoded in row[phase].items():
                    data = base64.b64decode(encoded, validate=True)
                    observed[key] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
                if observed != call[phase]:
                    raise ValueError(f'{path}: call {index} {phase} differs from freeze ledger')
            count += 1
            yield raw
    if count != len(calls):
        raise ValueError(f'{path}: truncated capture ({count}/{len(calls)} records)')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('baseline', type=Path)
    p.add_argument('candidate', type=Path)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--slug', choices=['task_work', 'task_lifecycle', 'task_display', 'task_delete'])
    args = p.parse_args()
    rows = []
    for slug in ([args.slug] if args.slug else ['task_work', 'task_lifecycle', 'task_display', 'task_delete']):
        mismatch = []
        count = 0
        a_hash, b_hash = hashlib.sha256(), hashlib.sha256()
        a = validated_records(args.baseline, slug)
        b = validated_records(args.candidate, slug)
        for index, (x, y) in enumerate(itertools.zip_longest(a, b)):
            count += 1
            if x: a_hash.update(x)
            if y: b_hash.update(y)
            if x is None or y is None or json.loads(x) != json.loads(y):
                mismatch.append(index)
        # gzip timestamps and library identities are not ABI records. No ABI
        # field, padding byte, failure status or excluded-cohort record is masked.
        a_report = json.loads((args.baseline / (slug + '.json')).read_text())
        b_report = json.loads((args.candidate / (slug + '.json')).read_text())
        exclusions_equal = a_report.get('source_unsupported') == b_report.get('source_unsupported')
        rows.append(dict(slug=slug, records=count, mismatch_ids=mismatch,
                         baseline_stream_sha256=a_hash.hexdigest(), candidate_stream_sha256=b_hash.hexdigest(),
                         exclusions_equal=exclusions_equal))
    args.output.write_text(json.dumps(rows, indent=2) + '\n')
    return int(any(r['mismatch_ids'] or not r['exclusions_equal'] for r in rows))


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError, TypeError, EOFError) as error:
        print(f'Task capture validation failed: {error}', file=sys.stderr)
        raise SystemExit(1)
