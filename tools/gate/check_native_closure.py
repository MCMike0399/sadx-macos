#!/usr/bin/env python3
"""Check every native archive member, including members normal linking never pulls in.

The companion force-load link resolves OS/C++ dependencies against the deployment
SDK. nm independently checks project symbol ownership and forbidden runtime imports.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
LAYERS = {'cpu', 'math', 'engine', 'input', 'player', 'game', 'runtime'}


def symbols(text):
    defined, undefined = set(), set()
    for line in text.splitlines():
        fields = line.split()
        if len(fields) < 2 or len(fields[1]) != 1:
            continue  # archive member heading
        name, kind = fields[:2]
        if kind == 'U':
            undefined.add(name)
        elif kind.upper() in {'T', 'D', 'S', 'B', 'C', 'W', 'V', 'A', 'I'}:
            defined.add(name)
    return defined, undefined


def problems_for(tables, sources):
    problems = []
    for name in sources:
        path = Path(name)
        if (len(path.parts) != 3 or path.parts[0] != 'src' or
                path.parts[1] not in LAYERS or path.suffix != '.cpp' or
                not re.fullmatch(r'[a-z][a-z0-9_]*\.cpp', path.name)):
            problems.append(f'forbidden native source: {name}')
    defined, needed = set(), set()
    for table in tables:
        provided, required = symbols(table)
        defined.update(provided)
        needed.update(required)
    for name in sorted(needed - defined):
        if '4sadx' in name:
            problems.append(f'unresolved project symbol: {name}')
    for name in sorted(defined | needed):
        if re.match(r'_?uc_', name):
            problems.append(f'forbidden Unicorn symbol: {name}')
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--platform', type=Path, nargs='+', required=True)
    parser.add_argument('--sources', nargs='+', required=True)
    parser.add_argument('--linked', type=Path, required=True,
                        help='executable produced with -force_load of the native archive')
    parser.add_argument('--output', type=Path, default=ROOT / 'build/native-closure.json')
    args = parser.parse_args()
    try:
        if not args.linked.is_file():
            raise ValueError('missing force-load linked executable')
        inputs = [args.archive, *args.platform]
        tables = [subprocess.check_output(['xcrun', 'nm', '-g', '-P', str(path)],
                                         text=True) for path in inputs]
        problems = problems_for(tables, args.sources)
        report = {'passed': not problems, 'problems': problems,
                  'inputs': [str(path) for path in inputs],
                  'sources': args.sources, 'force_load_executable': str(args.linked),
                  'limits': ['Link closure proves dependencies, not recovered behavior.']}
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + '\n')
        print('Native closure: ' + ('PASS' if not problems else '; '.join(problems)))
        return int(bool(problems))
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f'Native closure: {error}')
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
