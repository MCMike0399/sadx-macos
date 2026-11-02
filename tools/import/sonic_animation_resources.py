"""Bind the complete Sonic export declaration map to the existing reached pack.

No payload or identity is added. Reconstruct the maintained Sonic/Gamma import
and require byte equality before publishing this separate metadata document.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

from sonic_actor_resources import build_sonic_actor_resources, PEImage, SOURCE_SHA, SOURCE_SHA256
from gamma_actor_resources import append_gamma
from character_actions import export
from file_select_display import atomic_json


def import_sonic_animation_resources(model_dll, game_exe, pack_path):
    model_dll, game_exe = Path(model_dll).resolve(), Path(game_exe).resolve()
    pack_path = Path(pack_path).resolve()
    if pack_path.is_relative_to(model_dll.parent) or pack_path.is_relative_to(game_exe.parent):
        raise ValueError('Sonic animation metadata must be outside owned source directories')
    dll, exe = PEImage(model_dll), PEImage(game_exe)
    if dll.sha256 != SOURCE_SHA or exe.sha256 != SOURCE_SHA256:
        raise ValueError('unsupported Sonic animation source hashes')
    importer, actions, *_ = build_sonic_actor_resources(dll, exe)
    append_gamma(dll, exe, importer, actions)
    existing = pack_path.read_bytes()
    with tempfile.TemporaryDirectory(dir=pack_path.parent) as temporary:
        reconstructed = Path(temporary) / 'reconstructed.actions'
        pack_hash = importer.write(reconstructed, actions, selection=True)
        if reconstructed.read_bytes() != existing:
            raise ValueError('existing combined pack differs; never renumber or replace it')
    table = export(dll, '___SONIC_ACTIONS')
    raw = dll.read(table, 149 * 4)
    addresses = struct.unpack('<149I', raw)
    aliases, declarations = {}, []
    for index, address in enumerate(addresses):
        identity = importer.ids.get(('action', address, 1)) if address else None
        declaration = aliases.setdefault(address, index + 1) if address else 0
        declarations.append(dict(index=index, declaration_identity=declaration,
            status='known_null' if not address else 'loaded' if identity else 'unavailable',
            action=identity, resource_boundary=None if not address or identity else dict(
                component='CHRMODELS', source_address=hex(address),
                reason='payload outside reached initial/preview closure')))
    # Repeated source addresses must remain repeated declaration identities,
    # independently of whether their action payload is currently imported.
    inverse = {row['declaration_identity']: addresses[row['index']] for row in declarations}
    if b''.join(struct.pack('<I', inverse[row['declaration_identity']])
                for row in declarations) != raw:
        raise ValueError('Sonic export declaration inverse mismatch')
    metadata = dict(version=1, pack_schema='SDXACT03', pack_sha256=pack_hash,
        initial_animation_export_index=1,
        initial_animation_action=importer.ids[('action', addresses[1], 1)],
        declarations=declarations)
    proof = dict(version=1, source_dll_sha256=dll.sha256, source_game_sha256=exe.sha256,
        pack_sha256=pack_hash, pack_bytes=len(existing), pack_records=len(importer.records),
        pack_reconstruction_exact=True, appended_records=0, export_slots=149,
        export_address=hex(table), export_bytes=raw.hex(),
        export_sha256=hashlib.sha256(raw).hexdigest(), declaration_inverse_exact=True,
        payload_records=importer.evidence,
        counts={status: sum(row['status'] == status for row in declarations)
                for status in ('loaded', 'known_null', 'unavailable')},
        limits=['147 table rows are canonical code content, not 147 loaded animations.',
                'No editor, model, geometry, event stepping or complete actor frame is supplied.'])
    atomic_json(pack_path.parent / 'sonic-animation.json', metadata)
    atomic_json(pack_path.parent / 'sonic-animation.provenance.json', proof)
    return proof


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('model_dll', type=Path)
    parser.add_argument('game_exe', type=Path)
    parser.add_argument('pack', type=Path)
    args = parser.parse_args()
    result = import_sonic_animation_resources(args.model_dll, args.game_exe, args.pack)
    print(json.dumps({key: result[key] for key in
        ('pack_sha256', 'pack_records', 'appended_records', 'counts', 'export_sha256')}))
