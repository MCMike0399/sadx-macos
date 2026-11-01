#!/usr/bin/env python3
"""Run the unchanged Input axes judge and retain every per-case native word.

FP status differences stay partial. This capture adds no source expectations,
input cases, tolerances or hardware claims to the inherited instruction judge.
"""
import argparse
import ctypes
import gzip
import hashlib
import json
from pathlib import Path
import sys

import verify_original_input_mod_axes as judge


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Use a new capture directory; frozen streams are never overwritten')
    args.output.mkdir(parents=True)
    original_run, original_load = judge.Original.run, ctypes.CDLL
    pending, partial_ids, value_ids = [], [], []
    stream_hash = hashlib.sha256()
    count = 0

    def source_run(source, values, mode, mxcsr):
        result = original_run(source, values, mode, mxcsr)
        if pending:
            raise RuntimeError('A source result was not paired with a native invocation')
        pending.append((list(values), mode, mxcsr, result))
        return result

    with gzip.open(args.output / 'full-cases.jsonl.gz', 'wb') as stream:
        class Function:
            def __init__(self, function):
                object.__setattr__(self, 'function', function)

            def __setattr__(self, name, value):
                setattr(self.function, name, value)

            def __call__(self, words, mode, constants, mxcsr, output):
                nonlocal count
                result = self.function(words, mode, constants, mxcsr, output)
                if len(pending) != 1:
                    raise RuntimeError('Native call has no unique preceding source observation')
                values, source_mode, control, (expected, flags) = pending.pop()
                if (mode, mxcsr, list(words)) != (source_mode, control, [v & 0xffffffff for v in values]):
                    raise RuntimeError('Source/native case identity changed')
                actual = list(output)
                row = dict(id=count, mode=mode, input=values, mxcsr=mxcsr,
                           constants_hex=bytes(constants).hex(), original=expected,
                           original_mxcsr=flags, native_return=result, native=actual)
                encoded = (json.dumps(row, separators=(',', ':')) + '\n').encode()
                stream.write(encoded)
                stream_hash.update(encoded)
                if actual[:len(expected)] != expected or result or actual[7]:
                    value_ids.append(count)
                if actual[6] != flags:
                    partial_ids.append(count)
                count += 1
                return result

        class Library:
            def __init__(self, library):
                self.library = library

            def __getattr__(self, name):
                function = getattr(self.library, name)
                return Function(function) if name == 'sadx_native_input_mod_axes' else function

        def load(path, *positional, **keywords):
            library = original_load(path, *positional, **keywords)
            return Library(library) if Path(path).resolve() == args.library.resolve() else library

        old_argv = sys.argv
        try:
            judge.Original.run, ctypes.CDLL = source_run, load
            sys.argv = [judge.__file__, str(args.dll), str(args.library), '--output',
                        str(args.output / 'original-report.json')]
            judge.main()
        finally:
            judge.Original.run, ctypes.CDLL, sys.argv = original_run, original_load, old_argv
    if pending:
        raise RuntimeError('An unpaired source result remains')
    report = json.loads((args.output / 'original-report.json').read_text())
    receipt = dict(records=count, full_stream_sha256=stream_hash.hexdigest(),
                   value_failure_ids=value_ids, fp_partial_ids=partial_ids,
                   original_full_state_passed=report['passed'],
                   original_report_sha256=hashlib.sha256((args.output / 'original-report.json').read_bytes()).hexdigest(),
                   library_sha256=hashlib.sha256(args.library.read_bytes()).hexdigest(),
                   scope=__doc__)
    (args.output / 'identity.json').write_text(json.dumps(receipt, separators=(',', ':')) + '\n')
    print(f'{count} complete records; {len(value_ids)} value failures; {len(partial_ids)} FP partials')
    return int(bool(value_ids))


if __name__ == '__main__':
    raise SystemExit(main())
