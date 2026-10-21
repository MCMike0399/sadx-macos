#!/usr/bin/env python3
"""Import pinned regular texture declarations and prove the selected raw-asset profile."""
import configparser
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage
from extract import SOURCE_SHA256

TABLE = 0x90ec70
LOCALIZATION = 0x90ed78
TEXTURE_EXTENSIONS = {'.pvm', '.gvm', '.pvr', '.gvr', '.prs', '.pvmx', '.dds',
                      '.png', '.bmp', '.tga', '.jpg', '.jpeg', '.pvrx'}
sha = lambda data: hashlib.sha256(data).hexdigest()
PINNED_LOADER_SHA = '3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba'
PINNED_MODS = {
    'sadx-input-mod': {'SDL2.dll': '25003b5bb2bda0d5b558d357cd8eb6c96fa20d776b815950b46ff3444791d1cf',
                       'sadx-input-mod.dll': 'c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5'},
    'sadx-speedrun-mod': {'sadx-speedrun-mod.dll': 'f3fc68a3058fb726142547018da860b170f137248e54b82712b52527e7b74802'}}


def initial_bytes(image, address, size):
    """Read PE initial memory, including loader-defined zero-filled section tails."""
    try:
        return image.read(address, size)
    except ValueError:
        for section in image.sections:
            offset = address - image.base - section['rva']
            if 0 <= offset and offset + size <= section['virtual_size']:
                stored = max(0, min(size, section['raw_size'] - offset))
                return (image.read(address, stored) if stored else b'') + bytes(size - stored)
        raise ValueError('regular texture pointer is outside source initial memory')


def declarations(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('regular textures require the pinned executable SHA256')
    pointers = image.unpack('<3I', TABLE)
    list_indices, lists, groups, group_indices, entries = {}, [], [], {}, []
    records, string_bytes = [], {}

    def string(pointer):
        value = image.string(pointer)
        raw = value.encode('ascii') + b'\0'
        if image.read(pointer, len(raw)) != raw:
            raise ValueError('texture string failed inverse-byte validation')
        string_bytes[hex(pointer)] = raw.hex()
        return value

    def list_index(pointer):
        if pointer in list_indices:
            return list_indices[pointer]
        raw = image.read(pointer, 8)
        names_pointer, count = struct.unpack('<2I', raw)
        if count > 4096:
            raise ValueError('regular texture declaration exceeds supported source storage')
        names_raw = initial_bytes(image, names_pointer, count * 12) if count else b''
        names, attributes = [], []
        reconstructed = bytearray()
        for item in range(count):
            filename, attr, texture = struct.unpack_from('<3I', names_raw, item * 12)
            if texture != 0:
                raise ValueError('preloaded regular texture identity is unrecovered')
            if filename == 0xffffffff:
                raise ValueError('regular sentinel filename construction is unrecovered')
            names.append(string(filename) if filename else None)
            attributes.append(attr)
            reconstructed += struct.pack('<3I', filename, attr, texture)
        if bytes(reconstructed) != names_raw:
            raise ValueError('texture-name inverse-byte validation failed')
        result = len(lists)
        list_indices[pointer] = result
        lists.append({'texture_count': count, 'filenames': names, 'attributes': attributes})
        records.append({'kind': 'texture_list', 'address': hex(pointer), 'bytes': raw.hex(),
                        'names_address': hex(names_pointer), 'names_bytes': names_raw.hex(),
                        'round_trip_exact': True})
        return result

    for pointer in pointers:
        if pointer == 0:
            entries.append(None)
            continue
        if pointer in group_indices:
            entries.append(group_indices[pointer])
            continue
        group_indices[pointer] = len(groups)
        entries.append(len(groups))
        group = []
        for index in range(34):
            address = pointer + 8 * index
            raw = image.read(address, 8)
            name, texture_list = struct.unpack('<2I', raw)
            records.append({'kind': 'binding', 'address': hex(address), 'bytes': raw.hex(),
                            'round_trip_exact': struct.pack('<2I', name, texture_list) == raw})
            if index != 0 and texture_list == 0:
                break
            if index == 33:
                raise ValueError('regular texture group lacks a supported terminator')
            group.append({'archive': string(name) if name else None,
                          'texture_list': list_index(texture_list) if texture_list else None})
        else:
            raise ValueError('regular texture group has no terminator')
        groups.append(group)
    raw = image.read(LOCALIZATION, 12)
    language, original, replacement = struct.unpack('<iII', raw)
    localizations = [{'language': language, 'source': string(original),
                      'replacement': string(replacement) if replacement else None}]
    result = {'version': 1, 'entries': entries, 'groups': groups, 'lists': lists,
              'localizations': localizations}
    reconstructed = struct.pack('<3I', *pointers)
    proof = {'version': 2, 'source_sha256': image.sha256,
             'pointer_table_address': hex(TABLE), 'pointer_table_bytes': reconstructed.hex(),
             'pointer_table_sha256': sha(reconstructed),
             'directory_round_trip_exact': reconstructed == image.read(TABLE, 12),
             'localization_bytes': raw.hex(), 'localization_round_trip_exact':
                 struct.pack('<iII', language, original, replacement) == raw,
             'records': records, 'strings': string_bytes}
    return result, proof


def selected_mods(game, mod_directories):
    if mod_directories is not None:
        return [Path(path) for path in mod_directories], {'selection': 'explicit directories'}
    folder = game / 'mods/.modloader/profiles'
    registry = folder / 'Profiles.json'
    if not registry.is_file():
        raise ValueError('regular texture profile proof requires the selected loader profile')
    registry_data = json.loads(registry.read_text())
    index = registry_data['ProfileIndex']
    profiles = registry_data['ProfilesList']
    if type(index) is not int or not 0 <= index < len(profiles):
        raise ValueError('invalid selected texture profile index')
    name = profiles[index]['Filename']
    if Path(name).name != name:
        raise ValueError('selected texture profile filename is not local')
    path = folder / name
    data = json.loads(path.read_text())
    enabled = data['EnabledMods']
    if not isinstance(enabled, list) or any(not isinstance(name, str) or Path(name).name != name for name in enabled):
        raise ValueError('invalid enabled texture mod list')
    return [game / 'mods' / name for name in enabled], {
        'selection': 'selected loader profile', 'registry_sha256': sha(registry.read_bytes()),
        'profile_name': name, 'profile_sha256': sha(path.read_bytes()), 'enabled_mods': enabled}


def profile_evidence(game, metadata, mod_directories=None):
    game = Path(game)
    directories, proof = selected_mods(game, mod_directories)
    archives = {binding['archive'] for group in metadata['groups'] for binding in group if binding['archive'] is not None}
    if 'CON_REGULAR' in archives:
        archives.add('CON_REGULAR_E')
    individual = set()
    for group in metadata['groups']:
        for binding in group:
            if binding['archive'] is None and binding['texture_list'] is not None:
                for name in metadata['lists'][binding['texture_list']]['filenames']:
                    if name is None:
                        raise ValueError('anonymous regular texture lacks its actual filename')
                    individual.add(name)
    for rule in metadata['localizations']:
        individual.add(rule['source'])
        if rule['replacement'] is not None:
            individual.add(rule['replacement'])
    if proof['selection'] == 'selected loader profile':
        if [directory.name for directory in directories] != list(PINNED_MODS):
            raise ValueError('regular textures require the pinned Input/Speedrun profile')
        loader = game / 'mods/.modloader/SADXModLoader.dll'
        if not loader.is_file() or sha(loader.read_bytes()) != PINNED_LOADER_SHA:
            raise ValueError('regular textures require the pinned loader DLL identity')
        proof['loader_sha256'] = PINNED_LOADER_SHA
    proof['mods'] = []
    for directory in directories:
        if not directory.is_dir():
            raise ValueError('selected texture mod directory is missing: ' + str(directory))
        files = sorted(path for path in directory.rglob('*') if path.is_file())
        texture_files = [path.relative_to(directory).as_posix() for path in files if path.suffix.casefold() in TEXTURE_EXTENSIONS]
        # Current selected mods contain no loose replacement texture content.
        # Reject any such content rather than silently applying vanilla assets.
        if texture_files:
            raise ValueError('texture replacement overlay requires explicit import mapping: ' + str(directory))
        ini = directory / 'mod.ini'
        if ini.is_file():
            config = configparser.ConfigParser(interpolation=None, strict=False)
            config.read_string('[__root__]\n' + ini.read_text(encoding='utf-8-sig'))
            if any(config.items(section) for section in config.sections()
                   if section.casefold() in ('replacefiles', 'swapfiles')):
                raise ValueError('texture profile file replacement mappings require explicit import support')
        dlls = {path.name: sha(path.read_bytes()) for path in files if path.suffix.casefold() == '.dll'}
        if dlls != PINNED_MODS.get(directory.name, {}):
            raise ValueError('texture mod code identity is not the qualified pinned profile: ' + directory.name)
        proof['mods'].append({'name': directory.name, 'texture_files': texture_files,
                              'inventory_sha256': sha('\n'.join(path.relative_to(directory).as_posix() for path in files).encode()),
                              'mod_ini_sha256': sha(ini.read_bytes()) if ini.is_file() else None,
                              'dlls': dlls})
    system = game / 'system'
    children = list(system.iterdir())
    proof['assets'] = []
    for names, extension, alternatives in [(archives, '.PVM', ('.GVM', '.PRS', '.PVM.PRS', '.GVM.PRS')),
                                            (individual, '.PVR', ('.GVR', '.PRS', '.PVR.PRS', '.GVR.PRS'))]:
        for name in sorted(names):
            if not name or '/' in name or '\\' in name or Path(name).suffix:
                raise ValueError('unrecovered regular texture source path: ' + name)
            wanted = (name + extension).casefold()
            matches = [path for path in children if path.name.casefold() == wanted]
            if len(matches) != 1 or not matches[0].is_file():
                raise ValueError('regular texture source is missing or ambiguous: ' + name + extension)
            unexpected = [path.name for path in children if path.name.casefold() in
                          {(name + suffix).casefold() for suffix in alternatives} or
                          (path.is_dir() and path.name.casefold() in {name.casefold(), wanted})]
            if unexpected:
                raise ValueError('regular texture alternate/replacement is unsupported: ' + ','.join(unexpected))
            path = matches[0]
            raw = path.read_bytes()
            valid = raw[:4] == b'PVMH' if extension == '.PVM' else raw[:4] in (b'GBIX', b'PVRT')
            if not valid:
                raise ValueError('regular texture uses an unsupported packed/replacement format: ' + path.name)
            proof['assets'].append({'filename': path.name, 'sha256': sha(raw), 'size': len(raw),
                                    'gvm_gvr_prs_or_directory_alternate_present': False})
    proof['raw_texture_profile_proved'] = True
    return proof


def import_regular_textures(game, output, mod_directories=None):
    image = PEImage(Path(game) / 'sonic.exe')
    result, proof = declarations(image)
    proof['profile'] = profile_evidence(game, result, mod_directories)
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    (output / 'regular-textures.json').write_text(json.dumps(result, indent=2) + '\n')
    (output / 'regular-textures-evidence.json').write_text(json.dumps(proof, indent=2) + '\n')
    return result
