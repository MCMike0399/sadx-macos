#!/usr/bin/env python3
"""Bounded signature census. Existing readability metrics are deliberately untouched.

All unregistered callable names are review candidates, never semantic proof. Their
per-file/name counts also ratchet, so adding/renaming a helper cannot silently fall
out of the census. This intentionally asks for review of ordinary semantic calls.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from check_readability import LAYERS, NON_CODE, ROOT, SOURCE_SUFFIXES

BASELINE = 'reference/address-dispatch-baseline-v1.json'
REGISTRY = 'tools/gate/address_dispatch_signatures_v1.json'
VERSION = 'signature-dispatch-v1'
CALL = re.compile(r'\b([A-Za-z_]\w*)\s*\(')
# Keywords are syntax, not potential helper names. Casts remain candidates.
KEYWORDS = {'if', 'for', 'while', 'switch', 'catch', 'sizeof', 'alignof', 'decltype',
            'noexcept', 'static_assert', 'return'}
MAX_SPAN = 65536
MAX_DEPTH = 128


def masked(source):
    return NON_CODE.sub(lambda m: re.sub(r'[^\n]', ' ', m.group()), source)


def arguments(code, opening):
    """Balance (), [] and {}; angle brackets are intentionally not parsed.

    Template commas at top argument depth are unsupported. Registered call sites
    using those must wrap the expression in parentheses. Fail closed on bounds.
    """
    stack = ['(']
    start = opening + 1
    spans = []
    for pos in range(start, min(len(code), opening + MAX_SPAN)):
        char = code[pos]
        if char in '([{':
            stack.append(char)
            if len(stack) > MAX_DEPTH:
                raise ValueError('lexical nesting bound exceeded')
        elif char in ')]}':
            if not stack or stack.pop() != {')': '(', ']': '[', '}': '{'}[char]:
                raise ValueError('unbalanced callable expression')
            if not stack:
                if code[start:pos].strip() or spans:
                    spans.append((start, pos))
                return spans, pos + 1
        elif char == ',' and len(stack) == 1:
            spans.append((start, pos))
            start = pos + 1
    raise ValueError('unterminated callable expression or lexical span bound exceeded')


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def census(source, path, signatures):
    code = masked(source)
    sites = []
    candidates = {}
    definitions = []
    helpers = {}
    for spec in signatures:
        if spec['path'] != path:
            continue
        # Exact evidence includes declaration and body. A move, rename, changed
        # signature/body or deletion requires an explicit reviewed registry change.
        anchor = spec['definition']
        start = source.find(anchor)
        if (start < 0 or source.find(anchor, start + 1) >= 0 or
                code[start:start + len(anchor)] != masked(anchor)):
            raise ValueError(f'{path}: registered helper evidence changed/moved: {spec["helper"]}')
        definitions.append(start + spec['definition_name_offset'])
        if spec['helper'] in helpers:
            raise ValueError(f'{path}: ambiguous registered helper')
        helpers[spec['helper']] = spec
        for declaration in re.finditer(r'\b' + re.escape(spec['helper']) + r'\s*=\s*\[', code):
            if not start <= declaration.start() < start + len(anchor):
                raise ValueError(f'{path}: unregistered lambda definition: {spec["helper"]}')
    for match in CALL.finditer(code):
        helper = match[1]
        if helper in KEYWORDS or match.start() in definitions:
            continue
        prefix = re.search(r'((?:\w+\s*(?:\.|->|::)\s*)+)$',
                           code[max(0, match.start() - 256):match.start()])
        receiver = re.sub(r'\s+', '', prefix[0]) if prefix else ''
        if not receiver and code[:match.start()].rstrip().endswith(('.', '->', '::')):
            receiver = '<unsupported receiver>'
        if helper not in helpers or receiver not in helpers[helper]['receivers']:
            # All callable-name tokens, including declarations, are intentionally
            # broad candidates. No hex/range/name guess establishes semantics.
            candidates[helper] = candidates.get(helper, 0) + 1
            continue
        spec = helpers[helper]
        spans, end = arguments(code, match.end() - 1)
        if code[end:].lstrip().startswith('{'):
            raise ValueError(f'{path}: unregistered overload definition: {helper}')
        if not spec['min_args'] <= len(spans) <= spec['max_args']:
            raise ValueError(f'{path}:{source.count(chr(10), 0, match.start()) + 1}: '
                             f'{helper}: unsupported arity / possible new overload')
        a, b = spans[spec['target_argument'] - 1]
        sites.append({'path': path, 'line': source.count('\n', 0, match.start()) + 1,
                      'column': match.start() - source.rfind('\n', 0, match.start()),
                      'helper': helper, 'target_argument': spec['target_argument'],
                      'target_expression': code[a:b].strip()})
    return {'dispatch_sites': len(sites), 'candidate_names': candidates}, sites


def git(root, *args):
    result = subprocess.run(['git', *args], cwd=root, capture_output=True, text=True)
    if result.returncode:
        raise ValueError('Git HEAD required: ' + ' '.join(args))
    return result.stdout


def sources(root, head=False):
    if head:
        paths = git(root, 'ls-tree', '-r', '--name-only', 'HEAD', '--', 'src').splitlines()
    else:
        paths = [p.relative_to(root).as_posix() for layer in LAYERS
                 for p in sorted((root / 'src' / layer).rglob('*')) if p.is_file()]
    result = {}
    for path in paths:
        parts = Path(path).parts
        if len(parts) >= 3 and parts[1] in LAYERS and Path(path).suffix in SOURCE_SUFFIXES:
            result[path] = git(root, 'show', f'HEAD:{path}') if head else (root / path).read_text()
    return result


def scan(texts, registry):
    # Check every registered path, even after deletion/move out of game tier.
    missing = {s['path'] for s in registry['signatures']} - texts.keys()
    if missing:
        raise ValueError('registered helper path removed/moved: ' + ', '.join(sorted(missing)))
    rows, sites = {}, []
    for path, source in sorted(texts.items()):
        rows[path], found = census(source, path, registry['signatures'])
        sites.extend(found)
    return rows, sites


def validate(data, registry_hash):
    if (data.get('schema') != 1 or data.get('measurement') != VERSION or
            data.get('registry_sha256') != registry_hash or not isinstance(data.get('files'), dict)):
        raise ValueError('invalid signature baseline/version/registry; named migration required')
    for path, row in data['files'].items():
        parts = Path(path).parts
        if (len(parts) < 3 or parts[0] != 'src' or parts[1] not in LAYERS or '..' in parts or
                set(row) != {'dispatch_sites', 'candidate_names'} or
                type(row['dispatch_sites']) is not int or row['dispatch_sites'] < 0 or
                not isinstance(row['candidate_names'], dict) or
                any(not re.fullmatch(r'[A-Za-z_]\w*', k) or type(v) is not int or v < 0
                    for k, v in row['candidate_names'].items())):
            raise ValueError('invalid signature baseline row: ' + path)
    return data['files']


def increases(current, prior, label):
    errors = []
    for path, row in current.items():
        before = prior.get(path, {})
        if row['dispatch_sites'] > before.get('dispatch_sites', 0):
            errors.append(f'{path}: dispatch_sites {row["dispatch_sites"]} > '
                          f'{before.get("dispatch_sites", 0)} ({label})')
        for name, count in row['candidate_names'].items():
            limit = before.get('candidate_names', {}).get(name, 0)
            if count > limit:
                errors.append(f'{path}: candidate review required: {name} {count} > {limit} ({label})')
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--migrate-signature-v1', action='store_true')
    parser.add_argument('--write-baseline', action='store_true')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    try:
        registry_text = (root / REGISTRY).read_text()
        registry_hash = digest(registry_text)
        registry = json.loads(registry_text)
        if registry.get('schema') != 1 or registry.get('measurement') != VERSION:
            raise ValueError('unsupported signature registry')
        current, sites = scan(sources(root), registry)
        target = root / BASELINE
        committed = subprocess.run(['git', 'show', f'HEAD:{BASELINE}'], cwd=root,
                                   capture_output=True, text=True)
        if committed.returncode == 0:
            prior = validate(json.loads(committed.stdout), registry_hash)
            migration = False
        else:
            # Missing baseline in an existing HEAD is the sole v1 migration.
            # A Git read error must not become a bootstrap opportunity.
            tracked = git(root, 'ls-tree', 'HEAD', '--', BASELINE)
            if tracked.strip() or not args.migrate_signature_v1:
                raise ValueError('missing committed signature baseline; explicit --migrate-signature-v1 required')
            prior, _ = scan(sources(root, head=True), registry)
            migration = True
            print('MEASUREMENT CHANGE: signature-dispatch-v1 seeded only from immutable HEAD; '
                  'legacy readability metrics/allowances unchanged')
        errors = increases(current, prior, 'immutable HEAD migration' if migration else 'HEAD baseline')
        if target.exists():
            working = validate(json.loads(target.read_text()), registry_hash)
            errors += increases(working, prior, 'baseline increase against HEAD')
            errors += increases(current, working, 'working baseline')
        elif not migration:
            raise ValueError('missing working signature baseline')
        if args.report:
            args.report.write_text(json.dumps({'measurement': VERSION, 'files': current,
                                               'sites': sites, 'errors': errors}, indent=2) + '\n')
        if errors:
            print('\n'.join(errors))
            return 1
        if args.write_baseline or args.migrate_signature_v1:
            # Migration preserves HEAD allowances exactly; ordinary writes only lower.
            rows = prior if migration else current
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(json.dumps({'schema': 1, 'measurement': VERSION,
                                          'registry_sha256': registry_hash, 'files': rows}, indent=2) + '\n')
        print(f'Signature dispatch: {len(sites)} proven sites; '
              f'{sum(sum(r["candidate_names"].values()) for r in current.values())} '
              'unresolved candidate tokens; 0 increases')
        return 0
    except (ValueError, OSError, KeyError, TypeError) as error:
        print('ADDRESS DISPATCH: ' + str(error))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
