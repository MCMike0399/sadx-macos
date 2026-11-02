"""Assert the real first original Gamma fan entry/return and read-only inputs.

The C++ original-runtime witness is compiled/run with the exact serialized argv
in COMMANDS.tsv. Its overlay/output directory must be fresh on a new execution.
"""
import argparse
import hashlib
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/gamma-fan-native'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--raw', type=Path, default=OUT / 'original.json')
    parser.add_argument('--before', type=Path, default=OUT / 'original-inputs-before.json')
    parser.add_argument('--after', type=Path, default=OUT / 'original-inputs-after.json')
    args = parser.parse_args()
    result = json.loads(args.raw.read_text())
    snapshots = {row['phase']: row for row in result['snapshots']}
    before, after = snapshots['fan_input'], snapshots['fan_output']
    entry, returned = snapshots['first_gamma_fan_entry'], snapshots['first_gamma_fan_return']
    assert result['finished'] and result['frames'] == 1334
    assert result['getter_indices'] == [54, 30, 39, 2, 37]
    assert before['current_player'] == 0 and before['player'] and before['extra'] and before['primary']
    assert before['speed'] == after['speed'] == [0, 0, 0]
    assert len(before['extra_words']) == len(after['extra_words']) == 145
    assert before['extra_words'][6:11] == [0] * 5
    assert after['extra_words'][6:11] == [1044891052, 2048, 0, 0, 0]
    changed = [i for i, pair in enumerate(zip(before['extra_words'], after['extra_words'])) if pair[0] != pair[1]]
    assert changed == [6, 7]
    assert entry['count'] == entry['limit'] == entry['producer_calls'] == 0
    assert len(entry['buffer']) == 2048 and not any(entry['buffer'])
    expected = [[0] * 16 for _ in range(5)]
    expected[2][4:8] = [2147483648] * 4
    assert entry['gamma_destinations'] == expected
    assert entry['weld_scratch'] == [[0] * 16 for _ in range(4)]
    assert entry['stack_depth'] == 1
    for key in ['count', 'limit', 'producer_calls', 'buffer', 'gamma_destinations',
                'weld_scratch', 'stack_depth', 'current_matrix_pointer']:
        assert entry[key] == returned[key], key
    input_hashes = json.loads(args.before.read_text())
    assert input_hashes == json.loads(args.after.read_text())
    assert all(sha(Path(path)) == value for path, value in input_hashes.items())
    summary = {
        'exit_status': int((OUT / 'original-witness.exit').read_text()),
        'source_sha256': input_hashes['/Users/burbujamc/Developer/sadx-macos/reference/updated-pc-reference/game/sonic.exe'],
        'frames': result['frames'], 'changed_extra_words': changed,
        'extra_input_words': before['extra_words'], 'extra_output_words': after['extra_words'],
        'speed_words': before['speed'], 'current_player': before['current_player'],
        'player_identity': before['player'], 'extra_identity': before['extra'],
        'primary_identity': before['primary'], 'input_sha256': input_hashes,
        'inputs_unchanged': True, 'producer_calls': entry['producer_calls'],
        'matrix_state_unchanged': True, 'stack_unchanged': True,
        'getter_indices': result['getter_indices'], 'raw_sha256': sha(args.raw),
        'raw_path': str(args.raw.relative_to(ROOT)),
        'witness_source_sha256': sha(ROOT / 'tests/probes/gamma_fan_initialization.cpp'),
        'limits': ['Actual original first Gamma common-tail fan call only; display remains unrecovered.',
                   'Reference timing 1334 differs from native 1332.']}
    assert summary['exit_status'] == 0
    assert summary['source_sha256'] == '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
    destination = ROOT / 'reference/packets/gamma-fan-native/INITIALIZATION.json'
    destination.write_text(json.dumps(summary, indent=2) + '\n')
    print('Original first fan: exact scalar words, identities, all matrices/stack and unchanged inputs')
if __name__ == '__main__':
    main()
