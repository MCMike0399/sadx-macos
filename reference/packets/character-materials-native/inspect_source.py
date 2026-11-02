"""Read-only pinned-PC material-table and imported identity inverse proof."""
import hashlib
import json
from pathlib import Path
import struct
import sys
ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/import')]
from pe_image import PEImage
from character_actions import export, SOURCE_SHA
from extract import SOURCE_SHA256

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    assets, exe_path, dll_path, output = map(Path, sys.argv[1:])
    exe, dll = PEImage(exe_path), PEImage(dll_path)
    assert exe.sha256 == SOURCE_SHA256 and dll.sha256 == SOURCE_SHA
    pack_path = assets / 'characters/sonic-selection.actions'
    raw = pack_path.read_bytes()
    count, actions = struct.unpack_from('<II', raw, 8)
    offset = 16 + actions * 16
    records = {}
    for identity in range(1, count + 1):
        kind, size = struct.unpack_from('<II', raw, offset)
        offset += 8
        records[identity] = (kind, raw[offset:offset + size])
        offset += size
    provenance = json.loads((assets / 'characters/sonic-selection.provenance.json').read_text())
    declarations = json.loads((assets / 'characters/gamma-actor.json').read_text())['materials']
    proofs = {}
    for record in provenance['records']:
        if record['kind'] != 'materials':
            continue
        identity = record['id']
        kind, payload = records[identity]
        address = int(record['source_address'], 16)
        image = dll if record['source_component'] == 'character_dll' else exe
        source = image.read(address, record['count'] * 20)
        assert kind == 5 and payload[4:] == source
        proofs[identity] = dict(address=address, count=record['count'], sha256=sha(source))
    table = export(dll, '___E102_MATERIALS')
    rows = []
    for row in declarations:
        base, = dll.unpack('<I', table + row['table_row'] * 4)
        proof = proofs[row['materials']]
        expected = base + row['material_index'] * 20
        actual = proof['address'] + row['record_index'] * 20
        assert expected == actual
        users = []
        for identity, (kind, payload) in records.items():
            if kind == 3 and struct.unpack_from('<I', payload, 16)[0] == row['materials']:
                users.append(identity)
        assert users
        rows.append(dict(**row, source_record=hex(actual), model_users=users,
                         record_sha256=sha(dll.read(actual, 20))))
    result = dict(exe_sha256=exe.sha256, dll_sha256=dll.sha256,
                  pack_sha256=sha(raw), metadata_sha256=sha((assets / 'characters/gamma-actor.json').read_bytes()),
                  material_arrays=len(proofs), material_words=sum(x['count'] * 5 for x in proofs.values()),
                  all_material_payloads_inverse_exact=True, table_export=hex(table), declarations=rows,
                  source_semantics={
                      'Gamma': '483430 reads the direct E102_MATERIALS table in order 1/6,5/2,4/2,3/2,2/2. No object/model traversal. Row2 publishes its retained material pointer.',
                      'renderer': '4034b0 rereads node.model for each draw. 77eda0 captures model.points/normals/meshsets/materials/count before its mesh loop. 77ee24 reloads the retained material-array base from stack, not model+16; 784850 reads each selected record at the transition.',
                      'limits': 'Only the five declared Gamma table accesses are bound. Other indices remain unavailable. No new mutable model-descriptor behavior is claimed.'})
    output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k:result[k] for k in ['material_arrays','material_words','all_material_payloads_inverse_exact']}))

if __name__ == '__main__':
    main()
