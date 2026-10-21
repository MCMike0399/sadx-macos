"""Run the unchanged FOV judge against the native screen-distance implementation."""
import hashlib
import json
from pathlib import Path
import sys
import verify_original_fov as judge


def main():
    code = judge.main()
    output = Path(sys.argv[sys.argv.index('--output') + 1])
    report = json.loads(output.read_text())
    root = Path(__file__).resolve().parents[2]
    names = ('src/engine/ninja_view_native.hpp', 'src/engine/ninja_view_native.cpp',
             'tests/probes/native_screen_distance_probe.cpp',
             'tools/oracles/verify_original_fov.py', 'tools/oracles/verify_original_screen_distance.py')
    report['source_files'] = {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in names}
    report['limits'] += '\n' + __doc__
    output.write_text(json.dumps(report, indent=2) + '\n')
    return code


if __name__ == '__main__':
    raise SystemExit(main())
