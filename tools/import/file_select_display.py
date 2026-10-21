"""Import the source file-select drawing tables, with inverse-byte proof."""
import json
from pathlib import Path
import struct
import os
import tempfile
import math
from pe_image import PEImage
from extract import SOURCE_SHA256


def declarations(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('file-select display requires the pinned executable')
    records = []

    def table(name, address, fmt):
        raw = image.read(address, struct.calcsize(fmt))
        values = struct.unpack(fmt, raw)
        if any(isinstance(value,float) and not math.isfinite(value) for value in values):
            raise ValueError('nonfinite file-select display table: '+name)
        if struct.pack(fmt, *values) != raw:
            raise ValueError('file-select display inverse mismatch: ' + name)
        records.append({'name': name, 'address': hex(address), 'bytes': raw.hex(), 'round_trip_exact': True})
        return list(values)

    rows = []
    for index in range(3):
        values = table('frame_row_' + str(index), 0x7eca38 + 24 * index, '<3fII4B')
        rows.append(dict(zip(['right_x', 'bottom_y', 'depth', 'top_color', 'bottom_color',
                              'corner_texture', 'horizontal_texture', 'vertical_texture', 'padding'], values)))
    tile_count = table('sky_tile_count', 0x10d7cfc, '<h')[0]
    if tile_count != 8:
        raise ValueError('unqualified sky tile count')
    tiles = [table('sky_tile_' + str(i), 0x10d7cfe + i * 6, '<3h') for i in range(tile_count)]
    result = {'version': 1, 'frame_rows': rows, 'sky_tiles': tiles}
    for name, address, fmt in [('detail_labels', 0x7eca20, '<4B'), ('unlocked_icons', 0x7eca24, '<7B'),
                                ('locked_icons', 0x7eca2c, '<7B'), ('character_names', 0x7ef6f0, '<7B'),
                                ('stage_names', 0x7ef65c, '<44B'), ('sequence_indices', 0x7ef6b4, '<8H'),
                                ('clear_flags', 0x7ef6c4, '<7I'), ('digits', 0x7ef704, '<10B')]:
        result[name] = table(name, address, fmt)
    # Drawing operands select these existing imported menu bindings; import no
    # duplicate list or texture owners. Confirm all five languages retain identity.
    expected = {0: 0x10d7c90, 1: 0x915088, 10: 0x915068, 3: 0x10d7c08, 7: 0x10d7c28, 8: 0x10d7c30,
                11: 0x10d7c18, 13: 0x10d7c38, 15: 0x10d7be8, 22: 0x10d7c98,
                24: 0x10d7ca8, 28: 0x10d7bf0}
    for language, pointer in enumerate(struct.unpack('<5I', image.read(0x10d7cb0, 20))):
        for index, address in expected.items():
            if struct.unpack('<I', image.read(pointer + index * 8 + 4, 4))[0] != address:
                raise ValueError('file-select draw binding differs from imported menu identity')
    proof = {'source_sha256': image.sha256, 'records': records,
             'menu_list_bindings': {str(index): hex(address) for index, address in expected.items()},
             'limits': ['Stage texture prefix qualified for source IDs0..43; no fabricated high-index entries.']}
    return result, proof


def atomic_json(path, value):
    if path.is_symlink():
        raise ValueError('display metadata destination cannot be a symlink')
    descriptor, name = tempfile.mkstemp(prefix='.display-metadata-', dir=path.parent)
    temporary = Path(name)
    try:
        with os.fdopen(descriptor, 'w') as stream:
            stream.write(json.dumps(value, indent=2) + '\n')
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def import_file_select_display(game, output):
    game, output = Path(game).resolve(), Path(output).resolve()
    if output == game or game in output.parents:
        raise ValueError('display metadata must be outside the owned install')
    metadata, proof = declarations(PEImage(game / 'sonic.exe'))
    output.mkdir(parents=True, exist_ok=True)
    atomic_json(output / 'file-select-display.json', metadata)
    atomic_json(output / 'file-select-display-evidence.json', proof)
    return metadata
