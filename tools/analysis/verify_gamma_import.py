"""Compare a static Gamma import with the independently executed producer receipt.

This verifier reads JSON and pack bytes only; neither it nor the importer executes
original code. The producer receipt comes from the Gamma core development judge.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def verify(pack_path, producer_path):
    pack_path, producer_path = Path(pack_path), Path(producer_path)
    proof = json.loads(pack_path.with_suffix('.provenance.json').read_text())
    metadata = json.loads((pack_path.parent / 'gamma-actor.json').read_text())
    source = json.loads(producer_path.read_text())
    for importer_key, producer_key in [('source_dll_sha256', 'dll_sha256'),
                                       ('source_game_sha256', 'exe_sha256')]:
        if proof[importer_key] != source[producer_key]:
            raise ValueError('independent Gamma producer used a different source image')
    recipe_fields = ('row', 'declaration', 'object_count', 'mode', 'next',
                     'transition_bits', 'speed_bits')
    rows = [{key: row[key] for key in recipe_fields} for row in metadata['recipes']]
    if rows != source['recipes']:
        raise ValueError('Gamma recipes differ from independent original producer')
    if proof['recipe_declarations']['sentinel_recipe_bytes_sha256'] != source['sentinel_recipe_bytes_sha256']:
        raise ValueError('Gamma sentinel recipe bytes differ from original producer')
    if [row['index'] for row in metadata['declarations'] if row['status'] == 'known_null'] != [61]:
        raise ValueError('Gamma genuine NULL declaration set changed')
    if metadata['declarations'][61]['status'] != 'known_null':
        raise ValueError('Gamma export61 lost genuine NULL identity')
    if source['null_declarations'] != [61] or source['constructed_rows'] != [72]:
        raise ValueError('Gamma independent source declarations differ')
    records = {row['id']: row for row in proof['records']}
    for imported, original, child in zip(metadata['joins'][:4], source['weld']['rows'],
                                         source['weld']['source_object_pointers'][1:]):
        if records[imported['object']]['source_address'] != source['weld']['source_object_pointers'][0]:
            raise ValueError('Gamma weld root differs from original producer')
        if (records[imported['source_object']]['source_address'] != child or
                imported['vertex_count'] != original['vertex_count'] or
                imported['input_mode'] != original['input_mode']):
            raise ValueError('Gamma weld differs from original producer')
    sentinel = metadata['joins'][4]
    if any(sentinel.values()) or len(metadata['joins']) != 5:
        raise ValueError('Gamma weld full NULL sentinel changed')
    for key in ('action', 'object', 'motion', 'frames', 'object_count', 'channels', 'interpolation'):
        if proof['stand'][key] != source['stand'][key]:
            raise ValueError('Gamma Stand differs from independent source receipt')
    for words, fmt, source_key in [('collision_words', '<36I', 'collision'),
                                    ('line_color_words', '<2I', 'line_colors')]:
        if digest(struct.pack(fmt, *metadata[words])) != source[source_key]['sha256']:
            raise ValueError('Gamma source content inverse differs')
    for binding, original in zip(metadata['materials'], source['materials']):
        record = records[binding['materials']]
        address = int(record['source_address'], 16) + binding['record_index'] * 20
        if address != int(original['record'], 16):
            raise ValueError('Gamma material alias identity differs')
    raw = pack_path.read_bytes()
    if digest(raw) != proof['pack_sha256']:
        raise ValueError('Gamma output pack hash differs from provenance')
    if raw[:8] != b'SDXACT03' or struct.unpack_from('<I', raw, 12)[0] != 22:
        raise ValueError('Gamma output schema or bindings changed')
    prefix = proof['prefix']
    if digest(raw[16:368]) != prefix['bindings_sha256']:
        raise ValueError('Gamma original binding bytes changed')
    if digest(raw[368:prefix['bytes']]) != prefix['record_bytes_sha256']:
        raise ValueError('Gamma original encoded record bytes changed')
    old = bytearray(raw[:prefix['bytes']])
    struct.pack_into('<I', old, 8, prefix['records'])
    if digest(old) != prefix['pack_sha256']:
        raise ValueError('Gamma reconstructed original pack differs')
    return {'recipes_equal_original_producer': len(rows), 'weld_rows_equal_original_producer': 5,
            'producer_receipt_sha256': digest(producer_path.read_bytes()),
            'pack_sha256': digest(raw), 'records': struct.unpack_from('<I', raw, 8)[0],
            'preserved_record_prefix': prefix['records'], 'preserved_bindings': 22,
            'all_record_inverse_proofs': all(row['round_trip_exact'] for row in records.values())}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('pack', type=Path)
    parser.add_argument('original_producer_receipt', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = verify(args.pack, args.original_producer_receipt)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
