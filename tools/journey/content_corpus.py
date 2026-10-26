"""Resolve qualified replay inputs without trusting mutable build output names."""
import hashlib
import os
from pathlib import Path
import re
import tempfile


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def artifact_paths(root, relative, expected):
    root = Path(root).resolve()
    name = Path(relative)
    if name.is_absolute() or '..' in name.parts or not re.fullmatch('[0-9a-f]{64}', expected):
        raise ValueError('invalid qualified artifact path or SHA-256')
    source = root / name
    blob = root / 'reference/content-corpus/sha256' / expected
    if not source.resolve().is_relative_to(root) or not blob.resolve().is_relative_to(root):
        raise ValueError('qualified artifact escapes corpus root')
    return source, blob


def resolve_artifact(root, relative, expected):
    source, blob = artifact_paths(root, relative, expected)
    chosen = blob if blob.exists() else source
    if digest(chosen) != expected:
        raise ValueError(f'qualified artifact hash differs: {chosen}')
    return chosen


def archive_artifact(root, relative, expected):
    source, blob = artifact_paths(root, relative, expected)
    if blob.exists():
        return resolve_artifact(root, relative, expected)
    data = source.read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise ValueError(f'cannot archive changed qualified artifact: {source}')
    blob.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=blob.parent, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
        try:
            os.link(temporary, blob)  # no replacement of an existing immutable artifact
            blob.chmod(0o444)
        except FileExistsError:
            pass
    finally:
        if temporary is not None:
            temporary.unlink()
    return resolve_artifact(root, relative, expected)
