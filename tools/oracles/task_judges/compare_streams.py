#!/usr/bin/env python3
"""Compare every captured task ABI byte, scalar, status and record identity."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('baseline', type=Path)
    p.add_argument('candidate', type=Path)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--slug', choices=['task_work', 'task_lifecycle', 'task_display', 'task_delete'])
    args = p.parse_args()
    rows = []
    for slug in ([args.slug] if args.slug else ['task_work', 'task_lifecycle', 'task_display', 'task_delete']):
        left = args.baseline / (slug + '.native.jsonl.gz')
        right = args.candidate / (slug + '.native.jsonl.gz')
        mismatch = []
        count = 0
        a_hash, b_hash = hashlib.sha256(), hashlib.sha256()
        with gzip.open(left, 'rb') as a, gzip.open(right, 'rb') as b:
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
    raise SystemExit(main())
