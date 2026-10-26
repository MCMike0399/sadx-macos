"""Append the reached E102 mode-zero asset closure to sonic-selection.actions.

Reads only user-owned, hash-pinned source images. No original code is executed.
SDXACT03, its strict decoder, existing record IDs and 22 bindings are unchanged.
Complete recipe metadata distinguishes known NULL from unavailable payloads.
"""
import argparse
import copy
import json
import os
from pathlib import Path
import struct
import tempfile

from sonic_actor_resources import (PEImage, SOURCE_SHA, SOURCE_SHA256, atomic_json,
                                   build_sonic_actor_resources, bss, digest, export)
from gamma_declarations import WELD, fold, recipes

OLD_RECORDS = 3302
OLD_BYTES = 760168
OLD_PACK_SHA = 'df53cf2eb5427491cd444350b231f9b168060602b65a8cf93371f6ad0f38d423'
OBJECT_ROWS = (0, 24, 25, 26, 27)
MATERIAL_ROWS = ((1, 6), (5, 2), (4, 2), (3, 2), (2, 2))
CONSTRUCTED_ADDRESS = 0x3c5d530


def atomic_pack(importer, actions, output):
    output.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix='.gamma-actions-', dir=output.parent)
    os.close(fd)
    try:
        result = importer.write(Path(name), actions, selection=True)
        os.replace(name, output)
        return result
    finally:
        if os.path.exists(name):
            os.unlink(name)


def pack_bytes(importer, actions):
    return (b'SDXACT03' + struct.pack('<II', len(importer.records), len(actions)) +
            b''.join(struct.pack('<4I', *row) for row in actions) +
            b''.join(struct.pack('<II', kind, len(payload)) + payload
                     for kind, payload in importer.records))


def constructed_action(exe, dll, sources, object_table):
    code = (b'\xa1' + struct.pack('<I', 0x3a8af6c) + b'\x8b\x08\x89\x0d' +
            struct.pack('<I', CONSTRUCTED_ADDRESS) + b'\xc7\x05' +
            struct.pack('<II', CONSTRUCTED_ADDRESS + 4, 0x9c1050) + b'\xc3')
    if exe.read(0x7d2330, len(code)) != code:
        raise ValueError('Gamma constructed action initializer byte proof changed at 0x7d2330')
    root = dll.unpack('<I', object_table)[0]
    logical = struct.pack('<II', root, 0x9c1050)
    sources.derived[CONSTRUCTED_ADDRESS] = logical
    proof = {'character': 6, 'ordinal': 'recipe72', 'model_export': '___E102_OBJECTS',
             'model_index': 0, 'constructor_address': '0x7d2330',
             'constructor_bytes': code.hex(), 'constructor_sha256': digest(code),
             'destination_provenance': hex(CONSTRUCTED_ADDRESS),
             'model_provenance': hex(root), 'motion_provenance': '0x9c1050',
             'initialized_action_words': logical.hex(),
             'initial_zero_fill': bss(exe, CONSTRUCTED_ADDRESS, 8),
             'origin': 'statically reconstructed initializer; not original BSS payload bytes'}
    sources.constructions.append(proof)
    return proof


def material_binding(importer, dll, table, row, index):
    base = dll.unpack('<I', table + 4 * row)[0]
    if not base:
        raise ValueError(f'Gamma material table row {row} is null')
    address = base + 20 * index
    # Borrow an already imported material array whenever it contains this record.
    # This includes source aliases between material table rows and model arrays.
    candidates = sorted((identity, start, count) for (kind, start, count), identity
                        in importer.ids.items() if kind == 'materials' and
                        start <= address and address + 20 <= start + 20 * count and
                        (address - start) % 20 == 0)
    if not candidates:
        identity = importer.array('materials', base, index + 1, 20)
        candidates = [(identity, base, index + 1)]
    identity, start, _ = candidates[0]
    return {'table_row': row, 'material_index': index, 'materials': identity,
            'record_index': (address - start) // 20}


def cursor_template(importer, exe, address):
    raw = exe.read(address, 32)
    points, colors, indices, lines, padding, p3points, p3colors, texture, count = \
        struct.unpack('<IIIhHIIII', raw)
    if lines <= 0 or padding or any((p3points, p3colors, texture, count)):
        raise ValueError(f'unsupported Gamma WIREMODEL template at {address:#x}')
    indices_raw = exe.read(indices, 2 * lines * 2)
    values = struct.unpack('<' + 'h' * (2 * lines), indices_raw)
    if min(values) < 0:
        raise ValueError('negative Gamma cursor vertex index')
    vertices = max(values) + 1
    result = {'points': importer.array('vectors', points, vertices, 12),
              'colors': importer.array('colors', colors, vertices, 4),
              'indices': importer.array('indices', indices, 2 * lines, 2),
              'line_count': lines, 'padding': padding,
              'initial_point_colors': {'points': 0, 'colors': 0, 'texture': 0, 'count': 0}}
    restored = struct.pack('<IIIhHIIII',
                           *[importer.source_addresses[result[key]]
                             for key in ('points', 'colors', 'indices')],
                           lines, padding, 0, 0, 0, 0)
    if restored != raw:
        raise ValueError('Gamma cursor template inverse proof failed')
    return result, {'address': hex(address), 'bytes': raw.hex(), 'sha256': digest(raw)}


def matrix_node_bindings(importer, dll, root):
    # Asset identity only: ScanMotionModel visits each object before its children
    # and resumes its siblings. Matrix values remain an animated runtime result.
    traversal = []
    active = set()
    def visit(address):
        if not address:
            return
        if address in active:
            raise ValueError('cyclic Gamma matrix-node traversal')
        active.add(address)
        flags = dll.unpack('<I', address)[0]
        child, sibling = dll.unpack('<II', address + 44)
        traversal.append({'node': len(traversal), 'source_address': hex(address),
                          'evaluation_flags': flags,
                          'child': hex(child), 'sibling': hex(sibling)})
        visit(child)
        visit(sibling)
        active.remove(address)
    visit(root)
    if len(traversal) != 77 or any(row['evaluation_flags'] & 64 for row in traversal):
        raise ValueError('Gamma matrix-node traversal no longer matches the proved77-node graph')
    expected = {54: 0x10201ae4, 30: 0x1020491c, 39: 0x10202c9c,
                2: 0x10207228, 37: 0x1020323c}
    bindings = []
    for node, address in expected.items():
        if int(traversal[node]['source_address'], 16) != address:
            raise ValueError(f'Gamma matrix node {node} object identity changed')
        bindings.append({'node': node, 'object': importer.object(address)})
    return bindings, traversal


def append_gamma(dll, exe, importer, actions):
    importer.verify_round_trip()
    old_records = list(importer.records)
    old_bindings = list(actions)
    old_evidence = copy.deepcopy(importer.evidence)
    old_bytes = pack_bytes(importer, actions)
    if (len(old_records) != OLD_RECORDS or len(actions) != 22 or
            len(old_bytes) != OLD_BYTES or digest(old_bytes) != OLD_PACK_SHA):
        raise ValueError('Gamma requires the exact existing Sonic/selection namespace prefix')
    sources = importer.image
    object_table = export(dll, '___E102_OBJECTS')
    action_table = export(dll, '___E102_ACTIONS')
    material_table = export(dll, '___E102_MATERIALS')
    declarations = list(dll.unpack('<78I', action_table))
    if [i for i, value in enumerate(declarations) if not value] != [61]:
        raise ValueError('Gamma action declaration NULL identity changed')
    rows, recipe_raw, recipe_proof = recipes(exe, action_table, declarations)
    construction = constructed_action(exe, dll, sources, object_table)
    if [r['row'] for r in rows if r['declaration'] == 'constructed'] != [72]:
        raise ValueError('Gamma constructed recipe identity changed')
    stand_object, stand_motion = dll.unpack('<II', declarations[0])
    _, stand_frames, stand_channels, stand_interpolation = dll.unpack('<IIHH', stand_motion)
    stand_count = importer.animated_objects(stand_object)
    if (declarations[0], stand_object, stand_motion, stand_count, stand_frames,
            stand_channels, stand_interpolation) != (0x1020e344, 0x10207290,
                0x1020e338, 77, 16, 3, 2):
        raise ValueError('Gamma request0 Stand resource identity changed')
    initial = importer.action(declarations[0])
    # Request 0 is the only new action payload required by this constructor.
    # Preserve every other declaration, including existing selection aliases.
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
                       'reason': 'recipe72 payload outside reached request0 closure'},
                   'producer_proved': True}
    for row in rows:
        binding = constructed if row['declaration'] == 'constructed' else declaration_metadata[row['declaration']]
        row.update({key: copy.deepcopy(binding[key]) for key in ('status', 'action', 'resource_boundary')})
    restored = b''.join(struct.pack('<IBbHII',
        CONSTRUCTED_ADDRESS if r['declaration'] == 'constructed' else declarations[r['declaration']],
        r['object_count'], r['mode'], r['next'], int(r['transition_bits'], 16),
        int(r['speed_bits'], 16)) for r in rows)
    if restored != recipe_raw:
        raise ValueError('Gamma recipe metadata inverse proof failed')
    objects = [{'index': index, 'object': importer.object(dll.unpack('<I', object_table + 4 * index)[0])}
               for index in OBJECT_ROWS]
    matrix_nodes, matrix_traversal = matrix_node_bindings(importer, dll, stand_object)
    # Producer only consumes rows 0..27; do not invent the rest of the object catalogue.
    weld_raw, weld_proof = fold(exe, WELD, {0x3a8af6c: object_table},
                              {object_table: dll.read(object_table, 28 * 4)})
    joins = []
    for index in range(5):
        obj, src, dst, vertices, mode, srcnode, dstnode, original, pairs = \
            struct.unpack_from('<IIIBbBBII', weld_raw, 24 * index)
        if any((dst, srcnode, dstnode, original, pairs)):
            raise ValueError(f'unsupported Gamma weld resource at row {index}')
        expected = (2, 2, 0, 0, 0)[index], (4, 5, 6, 7, 0)[index]
        if (vertices, mode) != expected or (index == 4 and (obj or src)):
            raise ValueError('Gamma weld source declaration changed')
        entry = {'object': importer.object(obj), 'source_object': importer.object(src),
                 'destination_object': 0, 'vertex_count': vertices, 'input_mode': mode,
                 'source_node': srcnode, 'destination_node': dstnode,
                 'original_vertices': 0, 'vertex_pairs': 0}
        inverse = struct.pack('<IIIBbBBII',
            *[importer.source_addresses[entry[k]] if entry[k] else 0
              for k in ('object', 'source_object', 'destination_object')],
            vertices, mode, srcnode, dstnode, original, pairs)
        if inverse != weld_raw[24 * index:24 * (index + 1)]:
            raise ValueError('Gamma weld inverse proof failed')
        joins.append(entry)
    materials = [material_binding(importer, dll, material_table, row, index)
                 for row, index in MATERIAL_ROWS]
    cursors, cursor_proofs = [], []
    for address in (0x981ea0, 0x981ec0):
        cursor, proof = cursor_template(importer, exe, address)
        cursors.append(cursor)
        cursor_proofs.append(proof)
    textures = []
    texture_proofs = []
    for index in range(6):
        raw = exe.read(0x90ed04 + index * 8, 8)
        name, texture_list = struct.unpack('<II', raw)
        textures.append({'row': index, 'archive': exe.string(name) if name else None,
                         'texture_list_provenance': hex(texture_list) if texture_list else None})
        texture_proofs.append({'address': hex(0x90ed04 + index * 8), 'bytes': raw.hex()})
    expected_textures = [('E102', '0x91c560'), ('E102BEAM', '0x91c568'),
                         ('E102EFFECT', '0x91c570'), ('E102TIME', '0x91d5e0'),
                         ('E102HIT', '0x91d15c'), (None, None)]
    if [(r['archive'], r['texture_list_provenance']) for r in textures] != expected_textures:
        raise ValueError('Gamma character6 texture declaration changed')
    collision = exe.read(0x91c5b8, 144)
    colors = exe.read(0x91c648, 8)
    metadata = {'version': 1, 'pack_schema': 'SDXACT03', 'character': 6,
        'initial_animation_export_index': 0, 'initial_animation_action': initial,
        'declarations': declaration_metadata, 'constructed_action': constructed,
        'recipes': rows, 'objects': objects, 'joins': joins, 'materials': materials,
        'matrix_nodes': matrix_nodes,
        'matrix_cache_dependency': {
            'status': 'animated_runtime_result_required',
            'producer': 'CalcMMMatrix/ScanMotionModel',
            'consumer': 'GetMMMatrix', 'cache_provenance': '0x3c58278',
            'valid_count_provenance': '0x3c58270',
            'static_object_pose_is_not_matrix_value': True},
        'collision_words': list(struct.unpack('<36I', collision)),
        'line_color_words': list(struct.unpack('<2I', colors)),
        'line_point_count': 2, 'texture_character': 6, 'texture_declaration': textures,
        'cursor_templates': cursors,
        'setup_dependencies': [
            {'name': 'cursor_allocate_and_expand', 'source_entry': '0x4c2d40',
             'status': 'native_behavior_required', 'allocator': '0x40b220',
             'allocation_bytes_per_template': [288, 96],
             'point_count_per_template': 24, 'texture': None},
            {'name': 'load_setup_resources', 'source_entry': '0x49fb10',
             'status': 'native_behavior_required', 'texture_list_provenance': '0x91d15c',
             'resource_boundary': {'source_address': '0x91d168',
                                   'callee': '0x4cc330', 'final_callee': '0x49f9f0'}},
            {'name': 'initialize_effects', 'source_entry': '0x4c5420',
             'status': 'native_behavior_required',
             'resource_boundary': {'source_address': '0x982148',
                                   'callee': '0x456c50', 'result_owner': '0x982154'}}]}
    importer.verify_round_trip()
    if importer.records[:OLD_RECORDS] != old_records or actions != old_bindings:
        raise ValueError('Gamma append changed an existing record or binding')
    if sorted(importer.evidence[:len(old_evidence)], key=lambda r: r['id']) != \
            sorted(old_evidence, key=lambda r: r['id']):
        raise ValueError('Gamma append changed existing record metadata')
    proof = {'version': 1, 'source_dll_sha256': dll.sha256, 'source_game_sha256': exe.sha256,
        'records': importer.evidence, 'round_trip_exact': True,
        'prefix': {'records': OLD_RECORDS, 'bytes': OLD_BYTES, 'pack_sha256': OLD_PACK_SHA,
                   'bindings_sha256': digest(old_bytes[16:368]),
                   'record_bytes_sha256': digest(old_bytes[368:]),
                   'metadata_preserved': True,
                   'record_byte_range': [368, OLD_BYTES], 'binding_byte_range': [16, 368],
                   'header_change': 'record count at bytes8..11 increases; whole-file prefix is not identical'},
        'appended_ids': list(range(OLD_RECORDS + 1, len(importer.records) + 1)),
        'recipe_declarations': recipe_proof, 'weld_declarations': weld_proof,
        'stand': {'action': hex(declarations[0]), 'object': hex(stand_object),
                  'motion': hex(stand_motion), 'object_count': stand_count,
                  'frames': stand_frames, 'channels': stand_channels,
                  'interpolation': stand_interpolation},
        'constructed_action': construction, 'cursor_template_sources': cursor_proofs,
        'matrix_node_traversal': {'root': hex(stand_object),
            'order': 'object, child subtree, sibling subtree', 'nodes': matrix_traversal,
            'source_ranges': {'0x4b7d00-0x4b81e6': digest(exe.read(0x4b7d00, 0x4e6)),
                              '0x4b81f0-0x4b82c2': digest(exe.read(0x4b81f0, 0xd2)),
                              '0x4b82d0-0x4b82f7': digest(exe.read(0x4b82d0, 0x27))}},
        'source_declarations': [
            {'address': hex(address), 'size': len(raw), 'sha256': digest(raw), 'bytes': raw.hex()}
            for address, raw in [(action_table, dll.read(action_table, 78 * 4)),
                                 (object_table, dll.read(object_table, 28 * 4)),
                                 (0x91c5b8, collision), (0x91c648, colors)]],
        'material_table_sources': [
            {'table_row': row, 'address': hex(material_table + 4 * row),
             'bytes': dll.read(material_table + 4 * row, 4).hex()}
            for row, _ in MATERIAL_ROWS],
        'setup_source_ranges': [
            {'address': hex(start), 'size': end - start,
             'sha256': digest(exe.read(start, end - start))}
            for start, end in [(0x4c2d40, 0x4c2de4), (0x4c2e10, 0x4c2e48),
                               (0x49fb10, 0x49fb29), (0x4c5420, 0x4c5436)]],
        'texture_declaration_sources': texture_proofs,
        'limits': ['Only reached request0 action payload is appended; complete91 recipes retain explicit unavailable boundaries and known NULL export61.',
                   'Recipe72 constructor is statically proved; its payload is not claimed loaded.',
                   'Cursor allocator/expansion and setup/effects behavior remain native lower dependencies.',
                   'No actor, renderer, GPU, native application, N3b or full-gate claim.']}
    return metadata, proof


def import_gamma_actor_resources(model_dll, game_exe, output):
    model_dll, game_exe = Path(model_dll).resolve(), Path(game_exe).resolve()
    # Resolve the directory, preserving the output basename: replace a symlink inode
    # instead of following it into the user's install.
    output = Path(output).absolute()
    output = output.parent.resolve() / output.name
    if output.is_relative_to(model_dll.parent) or output.is_relative_to(game_exe.parent):
        raise ValueError('Gamma actor output must be outside owned source directories')
    dll, exe = PEImage(model_dll), PEImage(game_exe)
    if dll.sha256 != SOURCE_SHA or exe.sha256 != SOURCE_SHA256:
        raise ValueError('unsupported Gamma actor source hash')
    importer, actions, *_ = build_sonic_actor_resources(dll, exe)
    metadata, proof = append_gamma(dll, exe, importer, actions)
    proof['pack_sha256'] = atomic_pack(importer, actions, output)
    atomic_json(output.with_suffix('.provenance.json'), proof)
    atomic_json(output.parent / 'gamma-actor.json', metadata)
    return {'metadata': metadata, 'proof': proof, 'records': len(importer.records),
            'bytes': output.stat().st_size, 'sha256': proof['pack_sha256']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('model_dll', type=Path)
    parser.add_argument('game_exe', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = import_gamma_actor_resources(args.model_dll, args.game_exe, args.output)
    print(json.dumps({key: result[key] for key in ('records', 'bytes', 'sha256')}))
