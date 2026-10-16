#!/usr/bin/env python3
"""Look up hexadecimal addresses, preserving symbol alternatives and provenance."""

import argparse
import json
from pathlib import Path
from source_index import ContextIndex, ROOT


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('addresses', nargs='+')
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--database', type=Path)
    parser.add_argument('--module', default='SONIC')
    args = parser.parse_args()
    try:
        index = ContextIndex(args.root, args.database, with_inventory=False)
        print(json.dumps([index.lookup(at, args.module) for at in args.addresses], indent=2))
        return 0
    except (ValueError, OSError) as error:
        parser.exit(1, f'lookup: {error}\n')


if __name__ == '__main__':
    raise SystemExit(main())
