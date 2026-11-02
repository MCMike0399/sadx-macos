"""Qualify the Sonic mode-zero continuation and setup/common-link source prefixes."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('probe', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
    packet = ROOT / 'reference/packets/sonic-initialization-native'
    reports = {}
    for name in ['source', 'helpers']:
        report = args.output.with_name(args.output.stem + '-' + name + '.json')
        command = [sys.executable, str(packet / ('verify_' + name + '.py')),
                   str(args.probe), '--output', str(report)]
        result = subprocess.run(command, cwd=ROOT, check=False)
        assert result.returncode == 0, (command, result.returncode)
        reports[name] = json.loads(report.read_text())
    context = json.loads((packet / 'CONTEXT.json').read_text())
    result = dict(passed=True, values_match=True, cases=(reports['source']['cases'] + reports['helpers']['common_cases'] +
                        len(reports['helpers']['setup_cases']) + 1), mismatch_count=0, development=reports,
                  original_ranges_sha256=context['original_ranges_sha256'],
                  limits=['Controlled source-site lower calls, not complete actor execution.',
                          'Production callback and inherited model/FP checks are separate packet evidence.'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    return 0
if __name__ == '__main__':
    raise SystemExit(main())
