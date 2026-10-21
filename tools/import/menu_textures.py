#!/usr/bin/env python3
"""Import all five PC menu texture tables with source list identity and inverse proof."""
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from regular_textures import declarations as regular_declarations, initial_bytes, profile_evidence
from pe_image import PEImage
from extract import SOURCE_SHA256

TABLE = 0x10d7cb0
LANGUAGE_COUNT = 5
TEXTURE_COUNT = 30
JAPANESE_TITLE_NAME = 0x7efa80
JAPANESE_TITLE_LIST = 0x10d9fb4


def declarations(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('menu textures require the pinned executable SHA256')
    _, regular_proof = regular_declarations(image)
    regular_lists = {int(record['address'], 16): index for index, record in enumerate(
        row for row in regular_proof['records'] if row['kind'] == 'texture_list')}
    list_indices, lists, records, strings = {}, [], [], {}

    def string(address):
        text = image.string(address)
        raw = text.encode('ascii') + b'\0'
        if image.read(address, len(raw)) != raw:
            raise ValueError('menu texture string inverse mismatch')
        strings[hex(address)] = raw.hex()
        return text

    def list_index(address):
        if address in list_indices:
            return list_indices[address]
        if address == 0:
            raise ValueError('menu binding has a null texture list')
        raw = image.read(address, 8)
        names_address, count = struct.unpack('<2I', raw)
        if count > 4096:
            raise ValueError('menu texture list exceeds supported source storage')
        names_raw = initial_bytes(image, names_address, count * 12) if count else b''
        filenames, attributes, inverse = [], [], bytearray()
        for item in range(count):
            filename, attr, memory = struct.unpack_from('<3I', names_raw, item * 12)
            if filename == 0xffffffff or memory != 0:
                raise ValueError('preloaded or sentinel menu texture construction is unrecovered')
            filenames.append(string(filename) if filename else None)
            attributes.append(attr)
            inverse += struct.pack('<3I', filename, attr, memory)
        if bytes(inverse) != names_raw:
            raise ValueError('menu texture names inverse mismatch')
        index = len(lists)
        list_indices[address] = index
        lists.append({'declaration': {'texture_count': count, 'filenames': filenames,
                                      'attributes': attributes},
                      'regular_texture_list': regular_lists.get(address)})
        records.append({'kind': 'texture_list', 'address': hex(address), 'bytes': raw.hex(),
                        'names_address': hex(names_address), 'names_bytes': names_raw.hex(),
                        'round_trip_exact': struct.pack('<2I', names_address, count) == raw})
        return index

    raw_pointers = image.read(TABLE, LANGUAGE_COUNT * 4)
    pointers = struct.unpack('<5I', raw_pointers)
    languages = []
    for pointer in pointers:
        raw = image.read(pointer, TEXTURE_COUNT * 8)
        bindings, inverse = [], bytearray()
        for item in range(TEXTURE_COUNT):
            archive, texture_list = struct.unpack_from('<2I', raw, item * 8)
            if archive == 0:
                raise ValueError('anonymous menu texture loading is unrecovered')
            bindings.append({'archive': string(archive), 'texture_list': list_index(texture_list)})
            inverse += struct.pack('<2I', archive, texture_list)
        if bytes(inverse) != raw:
            raise ValueError('menu language table inverse mismatch')
        languages.append(bindings)
        records.append({'kind': 'language', 'address': hex(pointer), 'bytes': raw.hex(),
                        'round_trip_exact': True})
    japanese = {'archive': string(JAPANESE_TITLE_NAME),
                'texture_list': list_index(JAPANESE_TITLE_LIST)}
    metadata = {'version': 1, 'languages': languages, 'lists': lists, 'japanese_title': japanese}
    proof = {'version': 1, 'source_sha256': image.sha256,
             'pointer_table_address': hex(TABLE), 'pointer_table_bytes': raw_pointers.hex(),
             'directory_round_trip_exact': struct.pack('<5I', *pointers) == raw_pointers,
             'records': records, 'strings': strings,
             'japanese_call_bytes': image.read(0x51014e, 15).hex(),
             'japanese_name_address': hex(JAPANESE_TITLE_NAME),
             'japanese_list_address': hex(JAPANESE_TITLE_LIST)}
    # Ensure the imported extra binding is still the actual call operand, not a
    # similarly named SDK declaration or an assumed language-table alias.
    expected = b'\x68' + struct.pack('<I', JAPANESE_TITLE_LIST) + b'\x68' + struct.pack('<I', JAPANESE_TITLE_NAME)
    expected += b'\xe8' + struct.pack('<i', 0x421180 - 0x51015d)
    if image.read(0x51014e, 15) != expected:
        raise ValueError('Japanese title texture call operands changed')
    return metadata, proof


def import_menu_textures(game, output, mod_directories=None):
    metadata, proof = declarations(PEImage(Path(game) / 'sonic.exe'))
    # Preserve declarations even when an unused source archive is absent. The
    # current title journey requires its four mode bindings and Japanese extra;
    # other absent assets remain explicit unsupported import boundaries.
    children = list((Path(game) / 'system').iterdir())
    names = {binding['archive'] for row in metadata['languages'] for binding in row}
    names.add(metadata['japanese_title']['archive'])
    unavailable = sorted(name for name in names if not any(
        path.is_file() and path.name.casefold() == (name + '.PVM').casefold() for path in children))
    required = {row[index]['archive'] for row in metadata['languages'] for index in (18,17,1,29)}
    required.add(metadata['japanese_title']['archive'])
    if required.intersection(unavailable):
        raise ValueError('required title texture archive is missing: ' + ','.join(sorted(required.intersection(unavailable))))
    metadata['unavailable_archives'] = unavailable
    profile = {'groups': [[binding for binding in row if binding['archive'] not in unavailable]
                           for row in metadata['languages']] + [[metadata['japanese_title']]],
               'lists': [], 'localizations': []}
    proof['profile'] = profile_evidence(game, profile, mod_directories)
    proof['profile']['scope'] = 'All present menu PVM assets; required title bindings verified present.'
    proof['unavailable_archives'] = [{'archive':name, 'reason':'raw PVM absent; native load remains an explicit asset boundary'}
                                       for name in unavailable]
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    (output / 'menu-textures.json').write_text(json.dumps(metadata, indent=2) + '\n')
    (output / 'menu-textures-evidence.json').write_text(json.dumps(proof, indent=2) + '\n')
    return metadata
