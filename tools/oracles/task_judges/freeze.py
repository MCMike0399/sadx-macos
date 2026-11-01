#!/usr/bin/env python3
"""Run an unchanged task oracle and retain every native ABI input/output byte.

No comparisons, vectors, seeds, tolerances or exclusions are replaced. A failed
oracle remains failed. Native streams are characterization, not source goldens.
"""
import argparse
import base64
import ctypes
import gzip
import hashlib
import importlib
import json
from pathlib import Path
import sys


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('slug', choices=['task_work', 'task_lifecycle', 'task_display', 'task_delete'])
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    if list(args.output.glob(args.slug + '.*')):
        parser.error('Refusing to overwrite an existing task freeze; choose a fresh output directory')
    stream = args.output / (args.slug + '.native.jsonl.gz')
    ledger = []
    real_cdll = ctypes.CDLL
    with gzip.open(stream, 'wt') as sink:
        class Function:
            def __init__(self, fn, name):
                object.__setattr__(self, 'fn', fn)
                object.__setattr__(self, 'name', name)

            def __setattr__(self, name, value):
                setattr(self.fn, name, value)

            def __call__(self, *values):
                before = {str(i): bytes(v) for i, v in enumerate(values) if isinstance(v, ctypes.Array)}
                result = self.fn(*values)
                after = {str(i): bytes(v) for i, v in enumerate(values) if isinstance(v, ctypes.Array)}
                item = dict(call=len(ledger), symbol=self.name, result=result,
                            scalar_arguments=[v if isinstance(v, int) else None for v in values],
                            before={k: base64.b64encode(v).decode() for k, v in before.items()},
                            after={k: base64.b64encode(v).decode() for k, v in after.items()})
                sink.write(json.dumps(item, separators=(',', ':')) + '\n')
                ledger.append(dict(call=item['call'], symbol=self.name, result=result,
                                   before={k: dict(bytes=len(v), sha256=sha(v)) for k, v in before.items()},
                                   after={k: dict(bytes=len(v), sha256=sha(v)) for k, v in after.items()}))
                return result

        class Library:
            def __init__(self, *a, **kw):
                self.lib = real_cdll(*a, **kw)
                self.functions = {}

            def __getattr__(self, name):
                if name not in self.functions:
                    self.functions[name] = Function(getattr(self.lib, name), name)
                return self.functions[name]

        module = importlib.import_module('verify_original_' + args.slug)
        ctypes.CDLL = Library
        sys.argv = [module.__file__, str(args.exe), str(args.library), '--output',
                    str(args.output / (args.slug + '.json'))]
        try:
            rc = module.main()
        finally:
            ctypes.CDLL = real_cdll
    receipt = dict(slug=args.slug, oracle_exit=rc, stream=str(stream),
                   compressed_sha256=sha(stream.read_bytes()), calls=ledger,
                   oracle_sha256=sha(Path(module.__file__).read_bytes()),
                   library_sha256=sha(args.library.read_bytes()))
    (args.output / (args.slug + '.identity.json')).write_text(json.dumps(receipt, indent=2) + '\n')
    return rc


if __name__ == '__main__':
    raise SystemExit(main())
