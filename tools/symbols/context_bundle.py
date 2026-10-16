#!/usr/bin/env python3
"""Assemble named recovery context without opening original binaries or starting REA."""

import argparse
import json
from pathlib import Path
from source_index import ContextIndex, ROOT


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('address')
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--database', type=Path)
    parser.add_argument('--module', default='SONIC')
    parser.add_argument('--cache-dir', type=Path)
    parser.add_argument('--end', help='exclusive function end from recovery evidence, in hex')
    args = parser.parse_args()
    try:
        index = ContextIndex(args.root, args.database)
        print(json.dumps(index.bundle(args.address, args.module, args.cache_dir, args.end), indent=2))
        return 0
    except (ValueError, OSError) as error:
        parser.exit(1, f'context_bundle: {error}\n')


if __name__ == '__main__':
    raise SystemExit(main())
