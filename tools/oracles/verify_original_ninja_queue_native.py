"""Native queue ownership using the unchanged lifecycle instruction judge.

The child report retains its original comparisons/cases and explicitly reports
FP partials. This wrapper additionally fingerprints the new maintained owner.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
from verify_original_ninja_queue_lifecycle import main as lifecycle

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    child = args.output.with_suffix('.lifecycle.json')
    sys.argv = [sys.argv[0], str(args.exe), str(args.native_library), '--output', str(child)]
    status = lifecycle()
    report = json.loads(child.read_text())
    root = Path(__file__).resolve().parents[2]
    names = ['src/engine/ninja_queue_native.hpp', 'src/engine/ninja_queue_native.cpp',
             'tests/probes/native_ninja_queue_native_probe.cpp',
             'tools/oracles/verify_original_ninja_queue_native.py']
    report['native_queue_sources'] = {
        name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in names}
    report['native_queue_scope'] = __doc__
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    return status
if __name__ == '__main__':
    raise SystemExit(main())
