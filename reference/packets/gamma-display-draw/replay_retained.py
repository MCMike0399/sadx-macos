"""Compare native output to a retained original-observation commitment.

This does not execute the original and cannot qualify the new body cohort.
The retained passing report commits the complete old observation JSON bytes.
"""
import ctypes
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/oracles'), str(ROOT / 'tools/lib')]
from verify_original_gamma_display import fixtures, packed


def main():
    retained = json.loads(Path(sys.argv[1]).read_text())
    assert retained['status'] == 'pass' and retained['value_failures'] == 0
    library = ctypes.CDLL(str((ROOT / 'build/native-gamma-display-probe.dylib').resolve()))
    native = library.sadx_native_gamma_display
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    rows = []
    cases = fixtures(retained['cases'] - 102, retained['seed'])
    for index, case in enumerate(cases):
        output = (ctypes.c_uint32 * 1026)()
        assert native((ctypes.c_uint32 * 1032)(*case), output) == 1
        digest = hashlib.sha256(packed(list(output))).hexdigest()
        rows.append({'case': index, 'input_sha256': hashlib.sha256(packed(case)).hexdigest(),
                     'original_sha256': digest, 'native_sha256': digest,
                     'failure_prefix': output[1024]})
    data = (json.dumps(rows) + '\n').encode()
    actual = hashlib.sha256(data).hexdigest()
    report = {'cases': len(cases), 'retained_report': sys.argv[1],
              'retained_report_sha256': hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest(),
              'observations_sha256': actual,
              'retained_observations_sha256': retained['observations_sha256'],
              'match': actual == retained['observations_sha256'],
              'scope': 'native replay matches retained original aggregate commitment; no fresh original execution, placement, holdout or draw verdict'}
    out = ROOT / 'build/gamma-display-native/retained-prefix-replay.json'
    out.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    assert report['match']


if __name__ == '__main__':
    main()
