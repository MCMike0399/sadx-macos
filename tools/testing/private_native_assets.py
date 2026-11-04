#!/usr/bin/env python3
"""Create a new, symlink-free private native asset tree; never update an existing tree."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess

CANONICAL_SHA256 = 'c414e173bf4bf9f803f24d61642e233ff9650fe9276d70da2ac3939fee2b5d7f'
PACK = Path('characters/sonic-selection.actions')
ROWS = [2, 13, 14, 16, 18, 20, 22, 23]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def reject_links(root):
    root = Path(root)
    if root.is_symlink():
        raise ValueError(f'symlink rejected: {root}')
    for path in root.rglob('*'):
        if path.is_symlink() or not (path.is_file() or path.is_dir()):
            raise ValueError(f'symlink or special file rejected: {path}')


def validate_append(canonical, appended, proof, actor):
    if len(canonical) < 16 or canonical[:8] != b'SDXACT03':
        raise ValueError('invalid canonical pack header')
    count = struct.unpack_from('<I', canonical, 8)[0]
    added_count = struct.unpack_from('<I', appended, 8)[0] if len(appended) >= 16 else 0
    prefix = proof['display_append']
    if (len(appended) <= len(canonical) or appended[:8] != canonical[:8] or
            appended[12:len(canonical)] != canonical[12:] or
            prefix['prefix_records'] != count or prefix['prefix_bytes'] != len(canonical) or
            prefix['prefix_sha256'] != digest(canonical) or
            prefix['preserved_record_bytes_sha256'] != digest(canonical[12:]) or
            prefix['new_records'] != added_count - count or added_count <= count or
            proof['pack_sha256'] != digest(appended) or proof['round_trip_exact'] is not True):
        raise ValueError('append provenance or canonical bytes differ')
    declarations = prefix['declarations']
    if (actor['pack_schema'] != 'SDXACT03' or actor['character'] != 6 or
            actor['display_objects'] != declarations or
            [row['index'] for row in declarations] != ROWS or
            any(type(row['object']) is not int or not 1 <= row['object'] <= added_count
                for row in declarations)):
        raise ValueError('Gamma append metadata differs')


def clone_or_copy(source, target):
    # clonefile gives independent copy-on-write files, never hard links or symlinks.
    result = subprocess.run(['/bin/cp', '-c', str(source), str(target)], capture_output=True)
    if result.returncode:
        shutil.copy2(source, target)
    return str(target)


def build_tree(source, output, append=None, expected_sha256=CANONICAL_SHA256):
    source, output = Path(source), Path(output).absolute()
    reject_links(source)
    source = source.resolve(strict=True)
    if output.exists() or output.is_symlink():
        raise ValueError('output must be a new private directory; refusing overwrite')
    output = output.parent.resolve(strict=True) / output.name
    if output.is_relative_to(source) or source.is_relative_to(output):
        raise ValueError('private output must be outside the source asset tree')
    canonical = (source / PACK).read_bytes()
    if digest(canonical) != expected_sha256:
        raise ValueError('canonical selection pack hash differs')
    replacements = []
    if append is not None:
        append = Path(append)
        paths = [append, append.with_suffix('.provenance.json'), append.parent / 'gamma-actor.json']
        for path in paths:
            if path.is_symlink() or not path.is_file():
                raise ValueError(f'append requires ordinary files: {path}')
        validate_append(canonical, paths[0].read_bytes(), json.loads(paths[1].read_text()),
                        json.loads(paths[2].read_text()))
        replacements = list(zip(paths, [PACK, PACK.with_suffix('.provenance.json'),
                                        Path('characters/gamma-actor.json')]))
    shutil.copytree(source, output, copy_function=clone_or_copy)
    for path, relative in replacements:
        # Unlink before cp: do not depend on cp's overwrite/clone behavior.
        target = output / relative
        target.unlink(missing_ok=True)
        clone_or_copy(path, target)
    reject_links(output)
    if (source / PACK).read_bytes() != canonical:
        raise ValueError('source canonical bytes changed during copy')
    copied = (output / PACK).read_bytes()
    reconstructed = copied[:8] + canonical[8:12] + copied[12:len(canonical)]
    if digest(reconstructed) != expected_sha256:
        raise ValueError('private canonical prefix hash differs after copy')
    if replacements:
        validate_append(canonical, copied,
                        json.loads((output / PACK.with_suffix('.provenance.json')).read_text()),
                        json.loads((output / 'characters/gamma-actor.json').read_text()))
    report = {'source': str(source), 'output': str(output), 'symlinks': 0,
              'canonical_before_sha256': digest(canonical),
              'canonical_after_sha256': digest((source / PACK).read_bytes()),
              'private_prefix_sha256': digest(reconstructed), 'pack_sha256': digest(copied),
              'append_metadata_validated': bool(replacements)}
    (output / '.private-native-assets.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--append', type=Path)
    args = parser.parse_args()
    print(json.dumps(build_tree(args.source, args.output, args.append), indent=2))


if __name__ == '__main__':
    main()
