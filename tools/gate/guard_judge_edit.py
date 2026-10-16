#!/usr/bin/env python3
"""Proposed Claude PreToolUse Edit/Write guard; inactive until settings are approved."""

import json
from pathlib import Path
import subprocess
import sys

from check_judge import DECLARATION, ROOT, digest, game_changes, head, protected


def guard(payload, root):
    if payload.get('tool_name') not in {'Edit', 'Write'}:
        return
    name = payload.get('tool_input', {}).get('file_path')
    if not isinstance(name, str) or not name:
        raise ValueError('Edit/Write payload is missing file_path')
    path = Path(name)
    if not path.is_absolute():
        path = Path(payload.get('cwd', root)) / path
    # Check lexical and resolved forms so a symlink cannot hide a protected path.
    candidates = [path.absolute(), path.resolve()]
    selected = False
    for candidate in candidates:
        try:
            selected |= protected(candidate.relative_to(root).as_posix())
        except ValueError:
            continue
    if not selected:
        return
    declaration = json.loads((root / DECLARATION).read_text())
    if not declaration.get('reason') or declaration.get('base_commit') != head(root) or not head(root):
        raise ValueError('declare a judge change for the current HEAD before editing judge inputs')
    evidence = (root / declaration['evidence']).resolve()
    evidence.relative_to(root)
    if digest(evidence) != declaration.get('evidence_sha256'):
        raise ValueError('judge-change evidence changed after declaration')
    if game_changes(root):
        raise ValueError('judge edits must be separate from game-source changes')


def main():
    try:
        payload = json.load(sys.stdin)
        # Resolve the worktree from payload.cwd; CLAUDE_PROJECT_DIR can refer to
        # the original checkout after Claude enters a worktree.
        cwd = Path(payload.get('cwd', ROOT)).resolve()
        result = subprocess.run(['git', '-C', str(cwd), 'rev-parse', '--show-toplevel'],
                                capture_output=True, text=True)
        if result.returncode:
            raise ValueError('cannot identify the hook working tree')
        guard(payload, Path(result.stdout.strip()).resolve())
        return 0
    except (OSError, ValueError, TypeError, KeyError, AttributeError) as error:
        print(f'Judge edit refused: {error}. See docs/JUDGE.md.', file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
