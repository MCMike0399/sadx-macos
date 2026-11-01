#!/usr/bin/env python3
"""Additional full-output comparison around the unchanged Gamma consumer judge."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles')]
import verify_original_gamma_selection_core as judge


def load(path):
    library = ctypes.CDLL(str(path.resolve()))
    word = ctypes.c_uint32
    function = library.sadx_native_gamma_selection
    function.argtypes = [ctypes.POINTER(word), ctypes.c_size_t, ctypes.c_uint16,
                         ctypes.POINTER(word), ctypes.c_size_t, ctypes.POINTER(word)]
    return library, function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--before', type=Path, required=True)
    parser.add_argument('--current', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = judge.PEImage(args.exe)
    assert image.sha256 == judge.SOURCE_SHA256 and judge.unicorn.__version__ == '2.1.4'
    source = judge.Source(image)
    libraries = [load(path) for path in [args.before, args.current]]
    outputs = [hashlib.sha256(), hashlib.sha256()]
    identities = [[], []]
    mismatches = []
    count = value_failures = 0
    word = ctypes.c_uint32
    for cw in [0x007F, 0x027F, 0x037F, 0x047F, 0x087F, 0x0C7F]:
        for index, values in enumerate(judge.fixtures(False)):
            expected, expected_meta = source.run(values, cw)
            results = []
            for position, (_, function) in enumerate(libraries):
                buffer = (word * 8192)(); meta = (word * 4)()
                status = function((word * 32)(*values), 32, cw, buffer, 8192, meta)
                assert meta[0] <= 8192
                actual = list(buffer[:meta[0]]); metadata = list(meta)
                results.append((status, actual, metadata))
                raw = [status & 0xFFFFFFFF, len(actual), *actual, *metadata]
                outputs[position].update(struct.pack('<' + 'I' * len(raw), *raw))
                failed = status != 0 or actual != expected or metadata[:2] != expected_meta[:2] or metadata[3] != expected_meta[3]
                if failed:
                    value_failures += 1
                elif metadata[2] != expected_meta[2]:
                    identities[position].append([index, cw])
            if results[0] != results[1]:
                mismatches.append([index, cw])
            count += 1
    report = {'cases': count, 'value_failures_across_both': value_failures,
              'before_current_full_output_mismatches': mismatches,
              'full_output_sha256': [digest.hexdigest() for digest in outputs],
              'fp_partial_case_identities': identities,
              'fp_partial_identities_preserved': identities[0] == identities[1],
              'libraries': {name: hashlib.sha256(path.read_bytes()).hexdigest()
                            for name, path in [('before', args.before), ('current', args.current)]},
              'original_judge_sha256': hashlib.sha256(Path(judge.__file__).read_bytes()).hexdigest(),
              'limits': ['Additional observer around unchanged default fixtures/evaluator; FP partials remain partial.',
                         'Concrete lower-callee receipts; no recovered actor/rendering or whole Session fidelity claim.',
                         'Source-bound owner/unit and actual native journeys qualify ownership separately.']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    if mismatches or value_failures or identities[0] != identities[1]:
        return 1
    print(f'{count} full outputs identical; {len(identities[0])} FP partial identities preserved.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
