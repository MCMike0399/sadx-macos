"""Compare native loaded/status rows with the unchanged pinned-PC producer."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles'), str(ROOT / 'tools/import')]
from pe_image import PEImage
from character_actions import export, SOURCE_SHA
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP
from unicorn.x86_const import UC_X86_REG_ESP


def sha(data):
    return hashlib.sha256(data).hexdigest()


def verify(exe_path, dll_path, directory, observation):
    exe, dll = PEImage(exe_path), PEImage(dll_path)
    assert exe.sha256 == SOURCE_SHA256 and dll.sha256 == SOURCE_SHA
    metadata = json.loads((directory / 'sonic-animation.json').read_text())
    proof = json.loads((directory / 'sonic-animation.provenance.json').read_text())
    payloads = {row['id']: row for row in proof['payload_records']}
    addresses = dll.unpack('<149I', export(dll, '___SONIC_ACTIONS'))
    assert len(metadata['declarations']) == 149
    for index, row in enumerate(metadata['declarations']):
        address = addresses[index]
        assert row['index'] == index
        assert row['declaration_identity'] == (addresses.index(address) + 1 if address else 0)
        if row['status'] == 'loaded':
            assert payloads[row['action']]['kind'] == 'action'
            assert int(payloads[row['action']]['source_address'], 16) == address
        elif row['status'] == 'known_null':
            assert address == 0 and row['action'] is None and row['resource_boundary'] is None
        else:
            assert row['status'] == 'unavailable' and row['action'] is None and address
            assert int(row['resource_boundary']['source_address'], 16) == address
    native = bytearray()
    rows = observation.read_text().splitlines()
    assert len(rows) == 147
    for index, line in enumerate(rows):
        ordinal, status, identity, count, mode, following, transition, speed, reason = line.split('\t')
        assert int(ordinal) == index
        if status == '0':  # Loaded
            address = int(payloads[int(identity)]['source_address'], 16)
            assert not reason
        elif status == '1':  # KnownNull
            address = 0
            assert int(identity) == 0 and not reason
        else:
            assert status == '2' and int(identity) == 0
            match = re.fullmatch(r'CHRMODELS:(0x[0-9a-f]+):payload outside reached initial/preview closure:', reason)
            assert match, reason
            address = int(match[1], 16)
        native.extend(struct.pack('<IBbHII', address, int(count), int(mode), int(following),
                                  int(transition), int(speed)))
    original = Original(exe, 0x7f)
    machine = original.machine
    machine.mem_map(0x38f6000, 0x1000)
    machine.mem_map(0x3c56000, 0x2000)
    machine.mem_write(0x38f6e7c, struct.pack('<I', DATA + 0x1000))
    machine.mem_write(DATA + 0x1000, struct.pack('<149I', *addresses))
    machine.mem_write(0x3c56210, b'\xa5' * 2352)
    machine.mem_write(STACK, struct.pack('<I', STOP))
    machine.reg_write(UC_X86_REG_ESP, STACK)
    machine.emu_start(0x7cec90, STOP, count=5000)
    expected = bytes(machine.mem_read(0x3c56210, 2352))
    assert native == expected, 'native declaration/scalar bytes differ from original PC table'
    pack = (directory / 'sonic-selection.actions').read_bytes()
    assert sha(pack) == proof['pack_sha256'] == metadata['pack_sha256']
    return dict(source_game_sha256=exe.sha256, source_dll_sha256=dll.sha256,
        pack_sha256=sha(pack), export_sha256=sha(struct.pack('<149I', *addresses)),
        producer_sha256=sha(exe.read(0x7cec90, 7674)), output_sha256=sha(expected),
        source_ranges={'0x7cec90': dict(size=7674, sha256=sha(exe.read(0x7cec90, 7674)))},
        declaration_slots=149, table_rows=147, compared_bytes=2352, byte_mismatches=0,
        counts=proof['counts'], loaded_exports=[row['index'] for row in metadata['declarations']
            if row['status'] == 'loaded'], null_exports=[i for i, address in enumerate(addresses) if not address],
        source_alias_groups=[[i for i, address in enumerate(addresses) if address == value]
            for value in dict.fromkeys(addresses) if value and addresses.count(value) > 1],
        appended_records=proof['appended_records'], pack_reconstruction_exact=proof['pack_reconstruction_exact'],
        limits=['Original producer comparison is table initialization, not an actor frame or animation pose sampler.',
                'Payload decoder uses the already inverse-proved MDATA2 pack closure; MDATA3 remains unavailable.'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('dll', type=Path)
    parser.add_argument('characters', type=Path)
    parser.add_argument('observation', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = verify(args.exe, args.dll, args.characters, args.observation)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
