#!/usr/bin/env python3
"""Hash the differential judge. Refresh only in a separate, evidenced judge change."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = 'reference/judge-manifest.json'
DECLARATION = '.claude/judge-change.json'
DIRECTORIES = ('tools/oracles', 'tools/gate', 'tools/lib', 'tests/probes', 'tests/fixtures',
               'tests/python', 'reference/oracle-reports')
FILES = ('Makefile', 'tests/unit/manifest.tsv', 'reference/frame-control-corpus.tsv',
         'reference/frame-control-corpus.json', 'reference/keyboard-sampling-goldens.json',
         'reference/judge-hook-settings.proposed.json',
         'tools/analysis/fp_control_census.cpp', 'tools/analysis/run_fp_census.py',
         'reference/fp-control-census.json', 'tools/analysis/verify_game_mode_holdout.py')
GENERATED_SUFFIXES = ('.pyc', '.pyo')


def protected(path):
    parts = Path(path).parts
    if '__pycache__' in parts or Path(path).suffix in GENERATED_SUFFIXES or Path(path).name == '.DS_Store':
        return False
    return (path in FILES or path == MANIFEST or
            any(path.startswith(directory + '/') for directory in DIRECTORIES) or
            path.startswith('reference/frozen-corpora/') or
            re.fullmatch(r'reference/[^/]*corpus[^/]*', path) is not None or
            (path.startswith('reference/production-integration-') and
             'mutant' in Path(path).name and path.endswith('.json')))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot(root):
    root = root.resolve()
    paths = set()
    for directory in DIRECTORIES:
        paths.update(p for p in (root / directory).rglob('*') if p.is_file() or p.is_symlink())
    paths.update(root / name for name in FILES)
    paths.update(root.glob('reference/production-integration-*/**/*mutant*.json'))
    paths.update(p for p in (root / 'reference/frozen-corpora').rglob('*') if p.is_file() or p.is_symlink())
    paths.update(p for p in (root / 'reference').glob('*corpus*') if p.is_file() or p.is_symlink())
    result = {}
    for path in sorted(paths):
        relative = path.relative_to(root).as_posix()
        if not protected(relative):
            continue
        if path.is_symlink() or any(parent.is_symlink() for parent in path.parents if parent != root):
            raise ValueError(f'judge input must not be a symlink: {relative}')
        result[relative] = digest(path)
    for directory in DIRECTORIES:
        if not (root / directory).is_dir():
            raise ValueError(f'missing judge directory: {directory}')
    return result


def git(root, *arguments):
    return subprocess.run(['git', *arguments], cwd=root, capture_output=True, text=True)


def head(root):
    result = git(root, 'rev-parse', '--verify', 'HEAD')
    return result.stdout.strip() if result.returncode == 0 else None


def game_changes(root):
    if head(root) is None:
        return []
    changed = git(root, 'diff', '--name-only', '-z', 'HEAD', '--', 'src')
    added = git(root, 'ls-files', '--others', '--exclude-standard', '-z', '--', 'src')
    if changed.returncode or added.returncode:
        raise ValueError('cannot inspect game-source changes')
    return sorted(set(filter(None, (changed.stdout + added.stdout).split('\0'))))


def evidence_record(root, reason, evidence):
    if not reason or not reason.strip() or not evidence:
        raise ValueError('judge changes require --reason and --evidence (an existing file)')
    path = Path(evidence)
    if not path.is_absolute():
        path = root / path
    path = path.resolve()
    return dict(reason=reason.strip(), evidence=str(path.relative_to(root.resolve())),
                evidence_sha256=digest(path), base_commit=head(root))


def load_manifest(path):
    data = json.loads(path.read_text())
    if data.get('schema') != 1 or not isinstance(data.get('files'), dict):
        raise ValueError('invalid judge manifest schema')
    for name, value in data['files'].items():
        if (Path(name).is_absolute() or '..' in Path(name).parts or name == MANIFEST or
                not protected(name) or not isinstance(value, str) or
                re.fullmatch(r'[0-9a-f]{64}', value) is None):
            raise ValueError(f'invalid judge manifest entry: {name}')
    change = data.get('change')
    if (not isinstance(change, dict) or not isinstance(change.get('reason'), str) or
            not change['reason'].strip() or not isinstance(change.get('evidence'), str) or
            not isinstance(change.get('evidence_sha256'), str) or
            re.fullmatch(r'[0-9a-f]{64}', change['evidence_sha256']) is None):
        raise ValueError('manifest needs judge-change evidence')
    return data


def check(root):
    expected = load_manifest(root / MANIFEST)['files']
    current = snapshot(root)
    errors = []
    for path in sorted(expected.keys() | current.keys()):
        if path not in expected:
            errors.append(f'unmanifested judge input: {path}')
        elif path not in current:
            errors.append(f'missing judge input: {path}')
        elif current[path] != expected[path]:
            errors.append(f'changed judge input: {path}')
    if head(root) is not None:
        committed = git(root, 'show', f'HEAD:{MANIFEST}')
        manifest_changed = committed.returncode != 0 or committed.stdout != (root / MANIFEST).read_text()
        if manifest_changed and game_changes(root):
            errors.append('manifest update mixed with game-source changes; commit the port and judge separately')
    return errors, len(current)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    operation = parser.add_mutually_exclusive_group()
    operation.add_argument('--update', action='store_true')
    operation.add_argument('--declare-change', action='store_true')
    parser.add_argument('--reason')
    parser.add_argument('--evidence')
    args = parser.parse_args()
    root = args.root.resolve()
    try:
        if args.update or args.declare_change:
            if game_changes(root):
                raise ValueError('judge change mixed with game-source changes; commit separately')
            record = evidence_record(root, args.reason, args.evidence)
            if args.update:
                data = dict(schema=1, change=record, files=snapshot(root))
                path = root / MANIFEST
            else:
                if not record['base_commit']:
                    raise ValueError('hook declaration requires a Git HEAD')
                data, path = record, root / DECLARATION
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')
            print(f'{path.relative_to(root)} written')
        errors, count = check(root) if not args.declare_change else ([], 0)
        for error in errors:
            print('JUDGE: ' + error)
        print(f'Judge manifest: {count} files, {len(errors)} violations')
        return int(bool(errors))
    except (OSError, ValueError, TypeError) as error:
        print(f'JUDGE: {error}')
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
