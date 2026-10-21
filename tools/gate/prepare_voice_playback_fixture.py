#!/usr/bin/env python3
"""Preserve the existing two-clip playback fixture from the expanded production catalog."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile


def publish(path, write, output):
    if output not in path.resolve().parents:
        raise ValueError(f'voice fixture destination escapes its output: {path}')
    for parent in path.parents:
        if parent == output:
            break
        if parent.is_symlink():
            raise ValueError(f'voice fixture destination parent is a symlink: {parent}')
    if path.is_symlink():
        raise ValueError(f'voice fixture destination is a symlink: {path}')
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, name = tempfile.mkstemp(prefix='.voice-fixture-', dir=path.parent)
    os.close(descriptor)
    temporary = Path(name)
    try:
        write(temporary)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def prepare(assets, output):
    assets, output = assets.resolve(), output.resolve()
    if output == assets or assets in output.parents:
        raise ValueError('voice fixture must be outside the production assets')
    source = json.loads((assets / 'voice-assets.json').read_text())
    if source.get('version') != 1 or not isinstance(source.get('clips'), list):
        raise ValueError('invalid production voice catalog')
    selected = [clip for clip in source['clips'] if clip['voice'] == 1]
    if len(selected) != 2 or {clip['language'] for clip in selected} != {0, 1}:
        raise ValueError('required original playback fixture needs actual JP/US voice1 clips')
    for clip in selected:
        relative = Path(clip['pcm_path'])
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('voice fixture PCM path escapes its assets')
        original = (assets / relative).resolve()
        if assets not in original.parents or not original.is_file():
            raise ValueError('required original playback PCM is absent or escapes its assets')
        if hashlib.sha256(original.read_bytes()).hexdigest() != clip['pcm_sha256']:
            raise ValueError('required original playback PCM hash mismatch')
        publish(output / relative, lambda temporary: shutil.copyfile(original, temporary), output)
    publish(output / 'voice-assets.json', lambda temporary: temporary.write_text(
        json.dumps({'version': 1, 'clips': selected}, indent=2) + '\n'), output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    arguments = parser.parse_args()
    prepare(arguments.assets, arguments.output)
