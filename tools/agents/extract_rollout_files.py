#!/usr/bin/env python3
"""Conservative authored-text replay. Never runs transcript code or reads copy sources."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shlex


class Unsupported(ValueError):
    pass


# Only JSON-compatible double-quoted JS strings are accepted; no template evaluation.
STRING = r'"(?:[^"\\\r\n]|\\.)*"'
VALUE = rf'(?:{STRING}|-?\d+|true|false|null)'
OBJECT = rf'\{{\s*(?:["\w]+\s*:\s*{VALUE}\s*,?\s*)*\}}'
WRAPPER = re.compile(
    rf'\s*text\(await tools\.(exec_command|apply_patch)\(\s*({OBJECT}|{STRING})\s*\)\);?')


def wrapped_calls(source):
    """Recognize the entire wrapper, so strings/comments cannot impersonate tool calls."""
    calls = []
    pos = 0
    while source[pos:].strip():
        match = WRAPPER.match(source, pos)
        if not match:
            raise Unsupported('uninspectable JS wrapper (only literal text(await tools...))')
        name, argument = match.groups()
        if argument.startswith('{'):
            pairs = re.finditer(rf'([\"\w]+)\s*:\s*({VALUE})\s*,?\s*', argument[1:-1])
            argument = '{' + ','.join(
                json.dumps(m[1].strip('\"')) + ':' + m[2] for m in pairs) + '}'
        try:
            calls.append((name, json.loads(argument)))
        except json.JSONDecodeError as exc:
            raise Unsupported('non-JSON static tool arguments') from exc
        pos = match.end()
    return calls


def relative_path(path, prefix, workdir=None):
    if '\x00' in path or any(c in path for c in '$`\\*?[]~'):
        raise Unsupported('dynamic or invalid path')
    value = PurePosixPath(path)
    if not value.is_absolute():
        if not workdir:
            raise Unsupported('relative path has no explicit selected workdir')
        value = PurePosixPath(workdir) / value
    if '..' in value.parts:
        raise Unsupported('path traversal')
    try:
        result = value.relative_to(prefix).as_posix()
    except ValueError as exc:
        raise Unsupported('outside selected prefix') from exc
    if result == '.':
        raise Unsupported('file path is the prefix directory')
    return result


def patch_tree(tree, patch, prefix, workdir):
    lines = patch.splitlines(keepends=True)
    if not lines or lines[0].strip() != '*** Begin Patch' or lines[-1].strip() != '*** End Patch':
        raise Unsupported('incomplete patch')
    trial = tree.copy()
    i = 1
    while i < len(lines) - 1:
        header = re.fullmatch(r'\*\*\* (Add|Update|Delete) File: (.+)\n?', lines[i])
        if not header:
            raise Unsupported('unsupported patch header (moves are not supported)')
        kind, path = header.groups()
        path = relative_path(path.strip('"'), prefix, workdir)
        i += 1
        end = i
        while end < len(lines) - 1 and not lines[end].startswith('*** '):
            end += 1
        body = lines[i:end]
        if kind == 'Add':
            if path in trial or any(not line.startswith('+') for line in body):
                raise Unsupported('patch add requires absent file and literal added lines')
            trial[path] = ''.join(line[1:] for line in body)
        elif kind == 'Delete':
            if body or path not in trial:
                raise Unsupported('patch delete requires existing file and no body')
            del trial[path]
        else:
            if path not in trial:
                raise Unsupported('patch update prerequisite: missing authored base file')
            text = trial[path]
            cursor = 0
            j = 0
            while j < len(body):
                if body[j].rstrip('\n') != '@@':
                    raise Unsupported('only bare @@ exact patch hunks are supported')
                j += 1
                old, new = '', ''
                while j < len(body) and not body[j].startswith('@@'):
                    line = body[j]
                    if not line or line[0] not in ' +-':
                        raise Unsupported('unsupported patch hunk marker')
                    if line[0] in ' -':
                        old += line[1:]
                    if line[0] in ' +':
                        new += line[1:]
                    j += 1
                # Require one exact occurrence in the entire file, at a line boundary.
                starts = []
                search = 0
                while old:
                    found = text.find(old, search)
                    if found < 0:
                        break
                    starts.append(found)
                    search = found + 1
                starts = [p for p in starts if p == 0 or text[p - 1] == '\n']
                if len(starts) != 1 or starts[0] < cursor:
                    raise Unsupported('patch context missing, ambiguous or out of order')
                at = starts[0]
                text = text[:at] + new + text[at + len(old):]
                cursor = at + len(new)
            if not body:
                raise Unsupported('empty update patch')
            trial[path] = text
        i = end
    return trial


HEREDOC = re.compile(r"^cat\s*(>>|>)\s*(.+?)\s*<<\s*(['\"])([A-Za-z_][\w]*)\3\s*$")
ANY_HEREDOC = re.compile(r"<<-?\s*(['\"]?)([A-Za-z_]\w*)\1")
READ_ONLY = {'rg', 'grep', 'ls', 'cat', 'head', 'tail', 'wc', 'pwd', 'stat', 'sha256sum', 'shasum'}


class Replay:
    def __init__(self, prefix):
        self.prefix = prefix
        self.tree = {}
        self.operations = []
        self.unsupported = []

    def problem(self, identity, reason, snippet):
        self.unsupported.append({**identity, 'reason': reason, 'snippet': snippet})

    def command(self, cmd, workdir, identity):
        lines = cmd.splitlines(keepends=True)
        i = 0
        selected_context = bool(workdir and self.prefix in workdir)
        while i < len(lines):
            start = i
            line = lines[i].rstrip('\n')
            i += 1
            match = HEREDOC.fullmatch(line)
            heredoc = ANY_HEREDOC.search(line)
            if heredoc:
                delimiter = heredoc.group(2)
                body_start = i
                while i < len(lines) and lines[i].rstrip('\n') != delimiter:
                    i += 1
                closed = i < len(lines)
                body = ''.join(lines[body_start:i])
                i += int(closed)
            snippet = ''.join(lines[start:i])
            selected = self.prefix in snippet or selected_context
            if self.prefix in line and re.search(r'\bcd\b|=', line):
                selected_context = True
            if not selected:
                if self.prefix in cmd and (
                        re.search(r'[;&|\\]', line) or
                        re.match(r'\s*(if|then|else|fi|for|while|until|case|done|function)\b', line)):
                    self.problem({**identity, 'line': start + 1},
                                 'shell control flow may affect later selected writes', snippet)
                # Dynamic destinations cannot be proven unrelated by a literal-prefix filter.
                dynamic_write = (('$' in snippet or '`' in snippet) and
                                 re.search(r'>|\b(cp|mv|sed|tee|rm)\b', snippet))
                opaque_write = heredoc and re.search(
                    r'write_text|write_bytes|\.write\(|\bopen\(|\bos\.|\bsubprocess\b', body)
                if dynamic_write or opaque_write:
                    self.problem({**identity, 'line': start + 1},
                                 'unresolved write target may affect selected prefix', snippet)
                continue
            try:
                if match and closed:
                    tokens = shlex.split(match.group(2))
                    if len(tokens) != 1:
                        raise Unsupported('nonliteral heredoc destination')
                    path = relative_path(tokens[0], self.prefix, workdir)
                    if '\x00' in body:
                        raise Unsupported('binary content')
                    if match.group(1) == '>>' and path not in self.tree:
                        raise Unsupported('append prerequisite: missing authored base file')
                    self.tree[path] = (self.tree[path] if match.group(1) == '>>' else '') + body
                    self.operations.append({**identity, 'line': start + 1, 'kind': 'cat', 'path': path})
                elif heredoc:
                    raise Unsupported('unsupported heredoc edit or incomplete heredoc; Python is never evaluated')
                else:
                    tokens = shlex.split(line)
                    if not tokens:
                        continue
                    # Shell syntax invalidates the read-only classification.
                    safe = not re.search(r'[;&|<>`$\\\n]', line)
                    if safe and tokens[0] in READ_ONLY and not any(
                            token.startswith(('--pre', '--hostname-bin')) for token in tokens[1:]):
                        continue
                    if safe and tokens[0] == 'mkdir' and tokens[1:2] == ['-p']:
                        continue  # File parent directories are created only on output.
                    if tokens[0] == 'cp':
                        raise Unsupported('copy prerequisite: explicit historical source provenance required')
                    raise Unsupported('unsupported selected-prefix command')
            except (Unsupported, ValueError) as exc:
                self.problem({**identity, 'line': start + 1}, str(exc), snippet)

    def call(self, name, argument, identity):
        try:
            if name in ('exec', 'functions.exec'):
                if isinstance(argument, dict) and set(argument) == {'code'}:
                    argument = argument['code']
                calls = wrapped_calls(argument)
                for index, (tool, args) in enumerate(calls):
                    self.call(tool, args, {**identity, 'nested_call': index})
            elif name in ('exec_command', 'functions.exec_command'):
                if not isinstance(argument, dict) or not isinstance(argument.get('cmd'), str):
                    raise Unsupported('nonliteral command arguments')
                self.command(argument['cmd'], argument.get('workdir'), identity)
            elif name in ('apply_patch', 'functions.apply_patch'):
                if not isinstance(argument, str):
                    raise Unsupported('nonliteral patch')
                if self.prefix in argument:
                    updated = patch_tree(self.tree, argument, self.prefix, None)
                    changed = sorted(path for path in self.tree.keys() | updated.keys()
                                     if self.tree.get(path) != updated.get(path))
                    self.tree = updated
                    self.operations.append({**identity, 'kind': 'apply_patch', 'paths': changed})
                elif re.search(r'\*\*\* (?:Add|Update|Delete) File: (?!/)', argument):
                    raise Unsupported('relative patch targets have unknown cwd')
            elif self.prefix in str(argument):
                raise Unsupported('unknown selected-prefix tool')
        except (Unsupported, TypeError, ValueError) as exc:
            self.problem(identity, str(exc), str(argument))


def read_session(session, prefix):
    replay = Replay(prefix)
    digest = hashlib.sha256()
    with session.open('rb') as stream:
        for number, raw in enumerate(stream, 1):
            digest.update(raw)
            try:
                record = json.loads(raw)
                payload = record.get('payload', {})
                if record.get('type') != 'response_item' or payload.get('type') not in (
                        'custom_tool_call', 'function_call'):
                    continue
                identity = {'record': number, 'call_id': payload.get('call_id'), 'id': payload.get('id')}
                argument = payload.get('input') if payload['type'] == 'custom_tool_call' else payload.get('arguments')
                if payload['type'] == 'function_call':
                    try:
                        argument = json.loads(argument)
                    except (TypeError, ValueError):
                        replay.problem(identity, 'invalid function-call JSON', str(argument))
                        continue
                replay.call(payload.get('name'), argument, identity)
            except (ValueError, AttributeError) as exc:
                replay.problem({'record': number}, f'invalid JSONL record: {exc}', raw.decode('utf-8', 'replace'))
    return replay, digest.hexdigest()


def directory(parent_fd, name, create=False):
    if create:
        try:
            os.mkdir(name, dir_fd=parent_fd)
        except FileExistsError:
            pass
    return os.open(name, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=parent_fd)


def write_file(root_fd, path, content):
    parts = PurePosixPath(path).parts
    fd = os.dup(root_fd)
    try:
        for part in parts[:-1]:
            child = directory(fd, part, create=True)
            os.close(fd)
            fd = child
        out = os.open(parts[-1], os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600, dir_fd=fd)
        with os.fdopen(out, 'wb') as stream:
            stream.write(content)
    finally:
        os.close(fd)


def extract(session, prefix, repo):
    if not re.fullmatch(r'/private/tmp/sadx-[A-Za-z0-9][A-Za-z0-9_-]*', prefix):
        raise ValueError('prefix must be one literal /private/tmp/sadx-<name> directory')
    replay, source_hash = read_session(session, prefix)
    name = prefix.removeprefix('/private/tmp/sadx-')
    manifest = {
        'schema': 1, 'source_session': str(session.absolute()), 'source_sha256': source_hash,
        'prefix': prefix, 'verdict': 'incomplete' if replay.unsupported else 'complete',
        'meaning': 'authored static text only; tool success and game behavior are not verified',
        'operations': replay.operations, 'unsupported': [], 'files': [],
    }
    # dir_fd + NOFOLLOW on every component avoids output symlinks, including ancestor races.
    fd = os.open('/', os.O_RDONLY | os.O_DIRECTORY)
    try:
        for part in repo.absolute().parts[1:]:
            child = directory(fd, part)
            os.close(fd)
            fd = child
        for part in ('build', 'salvage'):
            child = directory(fd, part, create=True)
            os.close(fd)
            fd = child
        os.mkdir(name, dir_fd=fd)  # Existing output, even empty, is a hard error.
        child = directory(fd, name)
        os.close(fd)
        fd = child
        for index, problem in enumerate(replay.unsupported):
            problem = problem.copy()
            snippet = problem.pop('snippet').encode('utf-8')
            evidence = f'evidence/unsupported-{index:04d}.txt'
            write_file(fd, evidence, snippet)
            manifest['unsupported'].append({**problem, 'evidence': evidence,
                                             'snippet_sha256': hashlib.sha256(snippet).hexdigest()})
        for path, text in sorted(replay.tree.items()):
            data = text.encode('utf-8')
            write_file(fd, 'files/' + path, data)
            manifest['files'].append({'path': path, 'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data)})
        write_file(fd, 'manifest.json', (json.dumps(manifest, indent=2) + '\n').encode())
    finally:
        os.close(fd)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('session', type=Path)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--repo', type=Path, default=Path.cwd(), help='output repo, defaults to cwd')
    args = parser.parse_args()
    try:
        manifest = extract(args.session, args.prefix, args.repo)
    except (OSError, ValueError) as exc:
        parser.exit(2, f'extraction refused: {exc}\n')
    print(json.dumps({'verdict': manifest['verdict'], 'files': len(manifest['files']),
                      'unsupported': len(manifest['unsupported'])}))
    return 1 if manifest['verdict'] == 'incomplete' else 0


if __name__ == '__main__':
    raise SystemExit(main())
