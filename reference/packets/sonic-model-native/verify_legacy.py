"""Preserve every default Sonic action observation and individual source discrepancy."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles'),
                str(ROOT / 'tools/oracles/sonic_controller')]
from pe_image import PEImage
from observe import invoke
import verify_original_sonic_actions as judge

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('old', type=Path)
    parser.add_argument('current', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    image = PEImage(ROOT / 'game/Sonic Adventure DX/sonic.exe')
    assert image.sha256 == judge.SOURCE_SHA256
    old, new = ctypes.CDLL(str(args.old.resolve())), ctypes.CDLL(str(args.current.resolve()))
    source = judge.ActionsOriginal(image)
    constants = list(b''.join(image.read(address, 4) for address in judge.CONSTANTS))
    differences, partials = [], []
    inputs = hashlib.sha256()
    observations = hashlib.sha256()
    case = 0
    for precision in (0, 512, 768):
        for rounding in (0, 1024, 2048, 3072):
            for row in judge.fixtures():
                row[1] = precision | rounding | 127
                inputs.update(struct.pack('<52I', *row))
                expected, boundary, kind = source.run(row)
                arguments = [constants, len(constants), row, 52, [0] * 32, 32]
                before = invoke(old, 'sadx_native_sonic_action', arguments)
                after = invoke(new, 'sadx_native_sonic_action', arguments)
                if before != after:
                    differences.append(case)
                delta = [[field, value, after[1][field]] for field, value in expected.items()
                         if value != after[1][field]]
                if delta:
                    partials.append(dict(case=case, input_sha256=hashlib.sha256(
                        json.dumps(row, separators=(',', ':')).encode()).hexdigest(), differences=delta))
                observations.update(json.dumps([case, before, after, boundary, kind],
                                               separators=(',', ':')).encode())
                case += 1
    baseline = json.loads((ROOT / 'docs/refactor-inventory/packets/sonic-controller-native/sonic_actions-partial-identities.json').read_text())
    report = dict(records=case, input_sha256=inputs.hexdigest(), native_differences=differences,
                  complete_observation_sha256=observations.hexdigest(),
                  inherited_partials=len(partials), individual_partials_exact=partials == baseline['differences'],
                  individual_partial_sha256=hashlib.sha256(json.dumps(partials, separators=(',', ':')).encode()).hexdigest(),
                  input_exact=inputs.hexdigest() == baseline['case_sha256'],
                  limits=['Unchanged original fixtures and run method; compares every old/current output word.',
                          'Source partials remain partial; exact means old/current observation preservation.'])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    assert not differences and report['individual_partials_exact'] and report['input_exact']
    print(json.dumps(report))

if __name__ == '__main__':
    main()
