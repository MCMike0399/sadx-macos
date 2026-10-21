#!/usr/bin/env python3
"""Import typed sound-bank lists and unmodified PCM WAV entries from owned assets.

Source executable is read only as data and validated once. Runtime uses neither
that image nor mod DLLs. DAT parsing reuses the pinned sa_tools-backed importer.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/lib'))
from pe_image import PEImage
from extract import SOURCE_SHA256
from import_audio import dat_entries, validate_wave

TABLE = 0x90fcb8
COUNT = 123

def read_catalog(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('Sound-bank tables require the pinned executable identity')
    lists = []
    evidence = []
    for index in range(COUNT):
        address = TABLE + index * 8
        raw = image.read(address, 8)
        count, pointer = struct.unpack('<iI', raw)
        if count < 0:
            raise ValueError('Negative effective loader sound-list count is unrecovered')
        entries = []
        entry_bytes = image.read(pointer, count * 8) if count else b''
        for offset in range(count):
            slot, name_pointer = struct.unpack_from('<iI', entry_bytes, offset * 8)
            name = image.string(name_pointer)
            if slot < 0 or slot >= 16 or any(c not in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_' for c in name):
                raise ValueError('Unsupported slot/name in original sound-bank table')
            entries.append({'slot': slot, 'name': name})
        lists.append(entries)
        evidence.append({'index': index, 'table_address': address, 'table_bytes': raw.hex(),
                         'entry_address': pointer, 'entry_bytes': entry_bytes.hex(),
                         'names': [entry['name'] for entry in entries]})
    return lists, evidence

def import_banks(executable, sounddata, destination, selected=(42,), cd_sounddata=None, mod_directories=()):
    if any(index < 0 or index >= COUNT for index in selected):
        raise ValueError('Selected sound-bank list is outside source table')
    image = PEImage(executable)
    lists, table_evidence = read_catalog(image)
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    banks, missing, missing_packs = [], [], []
    # This pinned profile has no audio replacement content. Reject an overlay
    # rather than silently importing vanilla assets when a real replacement exists.
    overlays = []
    for directory in mod_directories:
        directory = Path(directory)
        if not directory.is_dir():
            raise ValueError('Audio mod directory is missing: ' + str(directory))
        audio = [path.relative_to(directory).as_posix() for path in directory.rglob('*')
                 if path.is_file() and 'sounddata/se/' in path.relative_to(directory).as_posix().lower()]
        if audio:
            raise ValueError('Audio replacement overlay import requires an explicit mapping: ' + str(directory))
        overlays.append({'name': directory.name, 'audio_files': audio})
    names = sorted({entry['name'] for index in selected for entry in lists[index]})
    sources = [('system', Path(sounddata))]
    if cd_sounddata is not None:
        sources.append(('cd', Path(cd_sounddata)))
    for origin, root in sources:
        if origin == 'system':
            for name in names:
                if (root / name / 'index.txt').exists():
                    raise ValueError('Loose sound-pack index import is explicitly unsupported: ' + name)
                missing_packs.append({'origin': origin, 'name': name, 'reason': 'source index.txt absent'})
        for name in names:
            matches = [path for path in root.iterdir() if path.name.casefold() == (name + '.dat').casefold()]
            if len(matches) > 1:
                raise ValueError('Ambiguous case-insensitive DAT path')
            if not matches:
                missing.append({'origin': origin, 'name': name, 'reason': 'source file absent'})
                continue
            source = matches[0]
            raw = source.read_bytes()
            samples = []
            for ordinal, filename, payload in dat_entries(raw):
                relative = Path('banks') / origin / (name.lower() or 'unnamed') / f'{ordinal:04d}_{filename.lower()}'
                output = destination / relative
                output.parent.mkdir(parents=True, exist_ok=True)
                if output.exists() and output.read_bytes() != payload:
                    raise ValueError('Conflicting imported bank sample')
                output.write_bytes(payload)
                facts = validate_wave(output)
                samples.append({'ordinal': ordinal, 'name': filename, 'path': relative.as_posix(),
                                'sha256': hashlib.sha256(payload).hexdigest(), **facts})
            banks.append({'name': name, 'origin': origin, 'source_filename': source.name, 'archive_tag': raw[14],
                          'source_sha256': hashlib.sha256(raw).hexdigest(), 'samples': samples})
    manifest = {'version': 2, 'profile': 'pinned loader sound-bank replacement',
                'missing_packs': missing_packs, 'mod_indices': {}, 'audio_overlays': overlays, 'source_exe_sha256': image.sha256, 'lists': lists,
                'selected_lists': list(selected), 'banks': banks, 'missing': missing,
                'cd_source_imported': cd_sounddata is not None}
    (destination / 'sound-banks.json').write_text(json.dumps(manifest, indent=2) + '\n')
    evidence = {'version': 1, 'source_exe_sha256': image.sha256,
                'table_sha256': hashlib.sha256(image.read(TABLE, COUNT * 8)).hexdigest(),
                'tables': table_evidence}
    (destination / 'sound-bank-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    return manifest

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('sounddata', type=Path, help='original system/SoundData/SE directory')
    parser.add_argument('destination', type=Path)
    parser.add_argument('--list', dest='selected', type=int, action='append')
    parser.add_argument('--cd-sounddata', type=Path)
    parser.add_argument('--mod-directory', type=Path, action='append', default=[])
    args = parser.parse_args()
    selected = args.selected or [42]
    if any(index < 0 or index >= COUNT for index in selected):
        parser.error('sound-bank list index outside original table')
    manifest = import_banks(args.executable, args.sounddata, args.destination, selected, args.cd_sounddata, args.mod_directory)
    print(json.dumps({'lists': len(manifest['lists']), 'banks': len(manifest['banks']),
                      'samples': sum(len(bank['samples']) for bank in manifest['banks']),
                      'missing': manifest['missing']}))
if __name__ == '__main__':
    main()
