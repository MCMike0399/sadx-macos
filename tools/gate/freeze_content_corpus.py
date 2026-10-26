#!/usr/bin/env python3
"""Archive qualified replay inputs by their existing hashes, without blessing new output."""
import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/journey'))
from content_corpus import archive_artifact


def expected_inputs():
    evidence = json.loads((ROOT / 'reference/frozen-corpora/capture-content-evidence.json').read_text())
    fixture = json.loads((ROOT / 'reference/frozen-corpora/capture-content.json').read_text())
    pins = dict(evidence['completion_final']['offline_replay_artifacts_sha256'])
    for row in evidence['completed_native_content']:
        pins[row['capture']] = row['capture_sha256']
    for row in evidence['completion_final']['return_transitions']:
        provenance = row['provenance']
        for key in ('capture', 'state', 'log', 'script', 'input_metadata'):
            pins[provenance[key]] = provenance[key + '_sha256']
    for name in ('title', 'character-select'):
        provenance = fixture['references'][name]['provenance']
        pins[provenance['capture']] = provenance['capture_sha256']
    return pins


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--corpus', type=Path, default=ROOT)
    args = parser.parse_args()
    pins = expected_inputs()
    for name, expected in sorted(pins.items()):
        archive_artifact(args.corpus, name, expected)
    print(f'Qualified content corpus: {len(pins)} unchanged inputs archived; no expected hash updated')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
