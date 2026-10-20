"""Run the unchanged original view judge against the semantic native view probe.

Case generation, comparisons, numeric models and exit status are delegated intact.
Only source identities are amended to describe the implementation actually linked.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    judge = root / 'tools/oracles/verify_original_ninja_view.py'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    result = subprocess.run([sys.executable, str(judge), str(args.exe),
                             str(args.native_library), '--output', str(args.output)], check=False)
    if not args.output.exists():
        return result.returncode or 1
    report = json.loads(args.output.read_text())
    report['legacy_judge_source_files'] = report.pop('source_files')
    sources = ['src/engine/ninja_view_native.hpp', 'src/engine/ninja_view_native.cpp',
               'src/cpu/fidelity_float.hpp', 'tests/probes/native_ninja_view_native_probe.cpp']
    report['native_sources'] = {name: hashlib.sha256((root / name).read_bytes()).hexdigest()
                                for name in sources}
    report['unchanged_judge_sha256'] = hashlib.sha256(judge.read_bytes()).hexdigest()
    report['source_identity_adapter'] = __doc__.strip()
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
