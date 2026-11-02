"""Append the Gamma display object declarations without changing existing resource IDs."""
import argparse
import copy
import json
from pathlib import Path
from gamma_actor_resources import (PEImage, SOURCE_SHA, SOURCE_SHA256, append_gamma,
    atomic_json, atomic_pack, build_sonic_actor_resources, digest, export, pack_bytes)

# Only the table rows consumed by E102Display. Existing constructor rows stay intact.
DISPLAY_ROWS = (2, 13, 14, 16, 18, 20, 22, 23)


def append_display(dll, importer, actions, metadata):
    before = pack_bytes(importer, actions)
    records = list(importer.records)
    evidence = copy.deepcopy(importer.evidence)
    table = export(dll, '___E102_OBJECTS')
    rows = []
    for index in DISPLAY_ROWS:
        address = dll.unpack('<I', table + 4 * index)[0]
        rows.append({'index': index, 'object': importer.object(address)})
    importer.verify_round_trip()
    if importer.records[:len(records)] != records or importer.evidence[:len(evidence)] != evidence:
        raise ValueError('Gamma display append changed existing resources')
    metadata['display_objects'] = rows
    after = pack_bytes(importer, actions)
    if before[:8] != after[:8] or before[12:] != after[12:len(before)]:
        raise ValueError('Gamma display append changed old pack bytes outside record count')
    return {'prefix_records': len(records), 'prefix_bytes': len(before),
            'prefix_sha256': digest(before), 'preserved_record_bytes_sha256': digest(before[12:]),
            'new_records': len(importer.records) - len(records),
            'source_table': hex(table),
            'source_rows': [{'index': i, 'address': hex(table + 4*i),
                             'bytes': dll.read(table + 4*i, 4).hex()} for i in DISPLAY_ROWS],
            'declarations': rows}


def import_display(model_dll, game_exe, output):
    model_dll, game_exe, output = Path(model_dll).resolve(), Path(game_exe).resolve(), Path(output).absolute()
    output = output.parent.resolve() / output.name
    if output.is_relative_to(model_dll.parent) or output.is_relative_to(game_exe.parent):
        raise ValueError('Gamma display output must be outside source directories')
    dll, exe = PEImage(model_dll), PEImage(game_exe)
    if dll.sha256 != SOURCE_SHA or exe.sha256 != SOURCE_SHA256:
        raise ValueError('unsupported Gamma display input hash')
    importer, actions, *_ = build_sonic_actor_resources(dll, exe)
    metadata, proof = append_gamma(dll, exe, importer, actions)
    proof['display_append'] = append_display(dll, importer, actions, metadata)
    proof['records'] = importer.evidence
    proof['pack_sha256'] = atomic_pack(importer, actions, output)
    atomic_json(output.with_suffix('.provenance.json'), proof)
    atomic_json(output.parent / 'gamma-actor.json', metadata)
    return {'records': len(importer.records), 'bytes': output.stat().st_size,
            'sha256': proof['pack_sha256'], 'append': proof['display_append']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('model_dll', type=Path)
    parser.add_argument('game_exe', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    print(json.dumps(import_display(args.model_dll, args.game_exe, args.output), indent=2))
