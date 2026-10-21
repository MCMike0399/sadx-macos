"""Snapshot owned save files without decoding, correcting or modifying them.

The native app seeds a NEW user saves directory from these imported copies; this
module never decides whether an existing user's progress should be overwritten.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile

FILETIME_EPOCH_NS = 11644473600 * 1000000000


def filetime(mtime_ns):
    ticks = (mtime_ns + FILETIME_EPOCH_NS) // 100
    if not 0 <= ticks <= 2**64 - 1:
        raise ValueError('save modification time is outside Windows FILETIME')
    return ticks


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def import_save_files(game, output):
    game, output = Path(game).resolve(), Path(output).resolve()
    if output == game or game in output.parents:
        raise ValueError('save snapshot output must be outside the owned install')
    directories = [entry for entry in game.iterdir() if entry.name.casefold() == 'savedata']
    if len(directories) > 1 or (directories and (not directories[0].is_dir() or directories[0].is_symlink())):
        raise ValueError('owned SAVEDATA directory is malformed, ambiguous or a symlink')
    source = directories[0] if directories else None
    files, names = [], set()
    # Preserve observed provider order in evidence only; native runtime enumerates
    # its real user directory. This is not a claim about NTFS enumeration order.
    selected = []
    if source is not None:
        with os.scandir(source) as entries:
            selected = [Path(entry.path) for entry in entries if entry.name.casefold().endswith('.snc')]
    for path in selected:
        if path.is_symlink() or not path.is_file():
            raise ValueError('save snapshot requires an ordinary file: ' + path.name)
        if path.name.casefold() in names:
            raise ValueError('ambiguous case-insensitive owned save filename: ' + path.name)
        names.add(path.name.casefold())
    if (output / 'saves').is_symlink() or (output / 'save-files.json').is_symlink():
        raise ValueError('save snapshot output cannot redirect through a symlink')
    for path in selected:
        before = path.stat()
        source_hash = digest(path)
        relative = Path('saves') / path.name
        destination = output / relative
        if destination.is_symlink():
            raise ValueError('save snapshot destination cannot be a symlink')
        destination.parent.mkdir(parents=True, exist_ok=True)
        # A previous output may hardlink ANY owned file. Never truncate it: copy
        # into a fresh inode, validate, then replace the directory entry atomically.
        descriptor, temporary_name = tempfile.mkstemp(prefix='.save-copy-', dir=destination.parent)
        os.close(descriptor)
        temporary = Path(temporary_name)
        try:
            shutil.copy2(path, temporary)
            after = path.stat()
            copied = temporary.stat()
            copied_hash = digest(temporary)
            if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns) or digest(path) != source_hash:
                raise ValueError('owned save changed during snapshot: ' + path.name)
            if copied_hash != source_hash or copied.st_mtime_ns != before.st_mtime_ns:
                raise ValueError('save snapshot did not preserve bytes and modification time: ' + path.name)
            os.replace(temporary, destination)
        finally:
            temporary.unlink(missing_ok=True)
        files.append({'name': path.name, 'path': relative.as_posix(), 'size': before.st_size,
                      'sha256': source_hash, 'mtime_ns': before.st_mtime_ns,
                      'last_write_time': filetime(before.st_mtime_ns)})
    metadata = {'version': 1, 'files': files}
    output.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix='.save-manifest-', dir=output)
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, 'w') as stream:
            stream.write(json.dumps(metadata, indent=2) + '\n')
        os.replace(temporary, output / 'save-files.json')
    finally:
        temporary.unlink(missing_ok=True)
    return metadata
