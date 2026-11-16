"""Append the reached Big mode-zero asset closure to sonic-selection.actions.

Reads only user-owned, hash-pinned source images. No original code is executed.
Existing record IDs and bindings are unchanged; Big declarations whose payload
is outside the reached request-0 closure keep explicit unavailable boundaries.
"""
import copy
import struct

from gamma_actor_resources import pack_bytes
from sonic_actor_resources import bss, digest
from character_actions import export
from big_declarations import (ACTION_SLOT, CONSTRUCTED_ADDRESS, DECLARATIONS,
                              OBJECT_SLOT, recipes, weld)

CONSTRUCTOR = 0x7d2310
CONSTRUCTED_MOTION = 0x9c08d0
COLLISION_ADDRESS = 0x91c920
COLLISION_BYTES = 144


def constructed_action(exe, dll, sources, object_table):
    code = (b'\xa1' + struct.pack('<I', OBJECT_SLOT) + b'\x8b\x08\x89\x0d' +
            struct.pack('<I', CONSTRUCTED_ADDRESS) + b'\xc7\x05' +
            struct.pack('<II', CONSTRUCTED_ADDRESS + 4, CONSTRUCTED_MOTION) + b'\xc3')
    if exe.read(CONSTRUCTOR, len(code)) != code:
        raise ValueError('Big constructed action initializer byte proof changed at 0x7d2310')
    root = dll.unpack('<I', object_table)[0]
    logical = struct.pack('<II', root, CONSTRUCTED_MOTION)
    sources.derived[CONSTRUCTED_ADDRESS] = logical
    proof = {'character': 7, 'ordinal': 'recipe96', 'model_export': '___BIG_OBJECTS',
             'model_index': 0, 'constructor_address': hex(CONSTRUCTOR),
             'constructor_bytes': code.hex(), 'constructor_sha256': digest(code),
             'destination_provenance': hex(CONSTRUCTED_ADDRESS),
             'model_provenance': hex(root), 'motion_provenance': hex(CONSTRUCTED_MOTION),
             'initialized_action_words': logical.hex(),
             'initial_zero_fill': bss(exe, CONSTRUCTED_ADDRESS, 8),
             'origin': 'statically reconstructed initializer; not original BSS payload bytes'}
    sources.constructions.append(proof)
    return proof


def append_big(dll, exe, importer, actions):
    importer.verify_round_trip()
    old_records = list(importer.records)
    old_bindings = list(actions)
    old_evidence = copy.deepcopy(importer.evidence)
    old_bytes = pack_bytes(importer, actions)
    sources = importer.image
    object_table = export(dll, '___BIG_OBJECTS')
    action_table = export(dll, '___BIG_ACTIONS')
    declarations = list(dll.unpack('<%dI' % DECLARATIONS, action_table))
    rows, recipe_raw, recipe_proof = recipes(exe, action_table, declarations)
    construction = constructed_action(exe, dll, sources, object_table)
    if [r['row'] for r in rows if r['declaration'] == 'constructed'] != [96]:
        raise ValueError('Big constructed recipe identity changed')
    stand_declaration = rows[0]['declaration']
    if not isinstance(stand_declaration, int):
        raise ValueError('Big request0 recipe has no declaration')
    stand_object, stand_motion = dll.unpack('<II', declarations[stand_declaration])
    _, stand_frames, stand_channels, stand_interpolation = dll.unpack('<IIHH', stand_motion)
    stand_count = importer.animated_objects(stand_object)
    initial = importer.action(declarations[stand_declaration])
    # Only the request-0 payload is appended. Every other declaration keeps an
    # explicit unavailable boundary instead of a guessed payload.
    declaration_metadata = []
    for index, address in enumerate(declarations):
        identity = importer.ids.get(('action', address, 1)) if address else None
        declaration_metadata.append({'index': index,
            'status': 'known_null' if not address else 'loaded' if identity else 'unavailable',
            'action': identity,
            'resource_boundary': None if not address or identity else
                {'component': 'CHRMODELS', 'source_address': hex(address),
                 'reason': 'payload outside reached constructor closure'}})
    constructed = {'status': 'unavailable', 'action': None,
                   'resource_boundary': {'component': 'constructed_action',
                       'source_address': hex(CONSTRUCTED_ADDRESS),
                       'reason': 'recipe96 payload outside reached request0 closure'},
                   'producer_proved': True}
    for row in rows:
        if row['declaration'] == 'constructed':
            binding = constructed
        elif row['declaration'] == 'known_null':
            binding = {'status': 'known_null', 'action': None, 'resource_boundary': None}
        else:
            binding = declaration_metadata[row['declaration']]
        row.update({key: copy.deepcopy(binding[key]) for key in ('status', 'action', 'resource_boundary')})
    restored = b''.join(struct.pack('<IBbHII',
        CONSTRUCTED_ADDRESS if r['declaration'] == 'constructed' else
        0 if r['declaration'] == 'known_null' else declarations[r['declaration']],
        r['object_count'], r['mode'], r['next'], int(r['transition_bits'], 16),
        int(r['speed_bits'], 16)) for r in rows)
    if restored != recipe_raw:
        raise ValueError('Big recipe metadata inverse proof failed')
    object_declarations = list(dll.unpack('<46I', object_table))
    weld_raw, weld_proof = weld(exe, object_table, object_declarations)
    joins = []
    for index in range(17):
        obj, src, dst, vertices, mode, srcnode, dstnode, original, pairs = \
            struct.unpack_from('<IIIBbBBII', weld_raw, 24 * index)
        if any((srcnode, dstnode, original)):
            raise ValueError(f'unsupported Big weld resource at row {index}')
        entry = {'object': importer.object(obj) if obj else 0,
                 'source_object': importer.object(src) if src else 0,
                 'destination_object': importer.object(dst) if dst else 0,
                 'vertex_count': vertices, 'input_mode': mode,
                 'source_node': srcnode, 'destination_node': dstnode,
                 'original_vertices': 0,
                 'vertex_pairs': importer.array('indices', pairs, 2 * vertices, 2) if pairs else 0}
        inverse = struct.pack('<IIIBbBBII',
            *[importer.source_addresses[entry[k]] if entry[k] else 0
              for k in ('object', 'source_object', 'destination_object')],
            vertices, mode, srcnode, dstnode, original,
            importer.source_addresses[entry['vertex_pairs']] if entry['vertex_pairs'] else 0)
        if inverse != weld_raw[24 * index:24 * (index + 1)]:
            raise ValueError('Big weld inverse proof failed')
        joins.append(entry)
    collision = exe.read(COLLISION_ADDRESS, COLLISION_BYTES)
    importer.verify_round_trip()
    if importer.records[:len(old_records)] != old_records or actions != old_bindings:
        raise ValueError('Big append changed an existing record or binding')
    if sorted(importer.evidence[:len(old_evidence)], key=lambda r: r['id']) != \
            sorted(old_evidence, key=lambda r: r['id']):
        raise ValueError('Big append changed existing record metadata')
    metadata = {'version': 1, 'pack_schema': 'SDXACT03', 'character': 7,
        'initial_animation_export_index': stand_declaration,
        'initial_animation_action': initial,
        'declarations': declaration_metadata, 'constructed_action': construction,
        'recipes': rows, 'joins': joins,
        'collision_words': list(struct.unpack('<36I', collision)),
        'texture_character': 7}
    proof = {'version': 1, 'source_dll_sha256': dll.sha256, 'source_game_sha256': exe.sha256,
        'prefix': {'records': len(old_records), 'bytes': len(old_bytes),
                   'pack_sha256': digest(old_bytes)},
        'appended_ids': list(range(len(old_records) + 1, len(importer.records) + 1)),
        'recipe_declarations': recipe_proof, 'weld_declarations': weld_proof,
        'stand': {'action': hex(declarations[stand_declaration]), 'object': hex(stand_object),
                  'motion': hex(stand_motion), 'object_count': stand_count,
                  'frames': stand_frames, 'channels': stand_channels,
                  'interpolation': stand_interpolation},
        'constructed_action': construction,
        'source_declarations': [
            {'address': hex(COLLISION_ADDRESS), 'size': len(collision),
             'sha256': digest(collision), 'bytes': collision.hex()}],
        'limits': ['Only reached request0 action payload is appended; complete105 recipes retain explicit unavailable boundaries.',
                   'Recipe96 constructor is statically proved; its payload is not claimed loaded.',
                   'Equipment objects, fishing behavior and display remain native lower dependencies.',
                   'No actor, renderer, GPU, native application, N3b or full-gate claim.']}
    return metadata, proof
