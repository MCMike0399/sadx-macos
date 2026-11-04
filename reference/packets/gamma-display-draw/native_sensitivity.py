"""Native-only fault sensitivity; deliberately no original-body verdict."""
import ctypes
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/oracles'), str(ROOT / 'tools/lib')]
from gamma_display_body_judge import body_fixtures, SIZE
from verify_original_gamma_display import fixtures, packed


def main():
    cases = [(c, 1026) for c in fixtures(1024, 0x47fd50)]
    cases += [(c, SIZE) for c in body_fixtures(1024, 0x47fd50 ^ 0xabc)]

    def observe(path):
        library = ctypes.CDLL(str(path.resolve()))
        native = library.sadx_native_gamma_display
        native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
        rows = []
        for case, count in cases:
            output = (ctypes.c_uint32 * count)()
            assert native((ctypes.c_uint32 * 1032)(*case), output) == 1
            rows.append(hashlib.sha256(packed(list(output))).hexdigest())
        return rows

    baseline = observe(ROOT / 'build/native-gamma-display-probe.dylib')
    result = []
    for fault in json.loads(Path(__file__).with_name('faults.json').read_text()):
        rows = observe(ROOT / ('build/gamma-display-draw-faults/' + fault['name'] + '.dylib'))
        different = [i for i, (a, b) in enumerate(zip(baseline, rows)) if a != b]
        result.append({'name': fault['name'], 'native_baseline_differences': len(different),
                       'first_difference': different[0] if different else None})
    report = {'topology_cases': 1126, 'body_cases': 1024, 'cases': len(cases), 'faults': result,
              'native_baseline_digest': hashlib.sha256(json.dumps(baseline).encode()).hexdigest(),
              'scope': 'native-only sensitivity; topology baseline separately matches retained original commitment; no source body/holdout differential verdict'}
    (ROOT / 'build/gamma-display-native/native-fault-sensitivity.json').write_text(
        json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    assert all(row['native_baseline_differences'] for row in result)


if __name__ == '__main__':
    main()
