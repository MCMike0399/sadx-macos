#!/usr/bin/env python3
"""Check task packet anchors against immutable Git blobs, not editable copies."""
import hashlib
import json
from pathlib import Path
import subprocess

BASE = '99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3'
PACKET = Path('docs/refactor-inventory/packets/task-judges')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    frozen = json.loads((PACKET / 'source-freeze.json').read_text())
    assert frozen['base'] == BASE
    for name, expected in frozen['files'].items():
        blob = subprocess.check_output(['git', 'show', BASE + ':' + name])
        assert sha(blob) == expected, ('mutable anchor', name)
        assert Path(name).read_bytes() == blob, ('current source differs from baseline', name)
    manifest = json.loads(subprocess.check_output(['git', 'show', BASE + ':reference/judge-manifest.json']))
    for row in json.loads((PACKET / 'inherited-protected.json').read_text()):
        name = row['path']
        assert row['expected_sha256'] == manifest['files'][name]
        # Registration is permitted to append only. Other old judge inputs remain
        # exact. New parent manifest union/seal is Root's responsibility.
        if name in ['Makefile', 'tests/unit/manifest.tsv']:
            before = subprocess.check_output(['git', 'show', BASE + ':' + name])
            assert Path(name).read_bytes().startswith(before), name
        else:
            assert sha(Path(name).read_bytes()) == row['expected_sha256'], name
    print('Immutable base source anchors and all inherited judge inputs verified')


if __name__ == '__main__':
    main()
