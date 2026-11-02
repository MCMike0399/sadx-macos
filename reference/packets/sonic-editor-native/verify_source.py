"""Pinned CheckEditMode and exact reset-word store versus native editor reads.

This is an independent scalar source oracle, not a reset collector or journey.
"""
import argparse
import ctypes
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tools/lib'))
from pe_image import PEImage
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_ESP

SOURCE = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
EDITOR, MISSION = 0x3b29d08, 0x3b29d0c
ENTRY, STOP, STACK = 0x4258f0, 0x10000, 0x21000
BODY = bytes.fromhex('0fbf050c9db2030fbf0d089db2030bc1c3')

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('native_composition', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    assert image.sha256 == SOURCE
    assert image.read(ENTRY, len(BODY)) == BODY
    source_data = []
    for address, size in ((EDITOR, 4), (MISSION, 4), (0x3b18db5, 1)):
        section = next(s for s in image.sections if
                       image.base + s['rva'] <= address and
                       address + size <= image.base + s['rva'] + s['virtual_size'])
        assert address - image.base - section['rva'] >= section['raw_size']
        source_data.append(dict(address=hex(address), size=size, initialization='PE virtual zero',
                                section=section))
    text = next(s for s in image.sections if s['name'] == '.text')
    code = image.slice(text['file_offset'], text['raw_size'])
    code_base = image.base + text['rva']
    direct_references = {}
    for address in (EDITOR, MISSION, EDITOR + 2, MISSION + 2):
        operand = struct.pack('<I', address)
        offsets = [i for i in range(len(code)) if code.startswith(operand, i)]
        direct_references[hex(address)] = [dict(operand_address=hex(code_base + i),
            prefix=code[max(0, i - 3):i].hex()) for i in offsets]
        if address == MISSION:
            assert offsets == [0x4258f3 - code_base]
        elif address == EDITOR:
            assert len(offsets) == 62
            for i in offsets:
                prefix = code[i - 3:i]
                assert prefix in (bytes.fromhex('66893d'), bytes.fromhex('0fbf0d'),
                    bytes.fromhex('66833d'), bytes.fromhex('663905'), bytes.fromhex('66391d'),
                    bytes.fromhex('66393d'), bytes.fromhex('0fbf15'), bytes.fromhex('0fbf05'))
                if prefix == bytes.fromhex('66893d'):
                    assert code_base + i == 0x40bf5f
        else:
            assert not offsets
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    for address in (0x425000, 0x40b000, 0x3b29000, STOP, STACK):
        machine.mem_map(address, 4096)
    machine.mem_write(ENTRY, BODY)
    reset_store = image.read(0x40bf5c, 7)
    assert reset_store.hex() == '66893d089db203'
    machine.mem_write(0x40bf5c, reset_store)
    trace = []
    machine.hook_add(UC_HOOK_MEM_READ,
                     lambda uc, access, address, size, value, data: trace.append((address, size)))
    lib = ctypes.CDLL(str(args.native_library.resolve()))
    native = lib.sadx_native_editor_state
    native.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
    native.restype = ctypes.c_int32
    def original(editor, mission):
        initial = struct.pack('<4H', editor, 0xa59b, mission, 0x7183)
        machine.mem_write(EDITOR, initial)
        machine.mem_write(STACK, struct.pack('<I', STOP))
        machine.reg_write(UC_X86_REG_ESP, STACK)
        trace.clear()
        machine.emu_start(ENTRY, STOP, count=5)
        assert trace == [(MISSION, 2), (EDITOR, 2), (STACK, 4)], trace
        assert bytes(machine.mem_read(EDITOR, 8)) == initial
        return ctypes.c_int32(machine.reg_read(UC_X86_REG_EAX)).value
    edges = [0, 1, 2, 256, 32767, 32768, 32769, 65534, 65535]
    cases = list(itertools.product(edges, repeat=2))
    dev = random.Random(0x4258f0)
    holdout = random.Random(0x425901)
    cases += [(dev.getrandbits(16), dev.getrandbits(16)) for _ in range(4096)]
    cases += [(holdout.getrandbits(16), holdout.getrandbits(16)) for _ in range(1024)]
    digest = hashlib.sha256()
    mismatches = []
    for editor, mission in cases:
        expected = original(editor, mission)
        actual = native(editor, mission)
        digest.update(struct.pack('<HHi', editor, mission, expected))
        if expected != actual:
            mismatches.append([editor, mission, expected, actual])
    composition = [list(map(int, row.split())) for row in args.native_composition.read_text().splitlines()]
    assert len(composition) == 81
    for editor, mission, read_mission, read_editor, result in composition:
        assert read_mission == ctypes.c_int16(mission).value
        assert read_editor == ctypes.c_int16(editor).value
        assert result == original(editor, mission)
    # A focused execution of the existing reset store, with poisoned adjacent halves.
    initial = struct.pack('<4H', 0x8001, 0xa59b, 0x42a5, 0x7183)
    machine.mem_write(EDITOR, initial)
    machine.reg_write(UC_X86_REG_EDI, 0)
    machine.emu_start(0x40bf5c, 0x40bf63, count=1)
    assert bytes(machine.mem_read(EDITOR, 8)) == b'\0\0' + initial[2:]
    report = dict(source_sha256=image.sha256,
                  source_ranges={'0x4258f0-0x425901': sha(BODY),
                                 '0x40bf5c-0x40bf63': sha(reset_store),
                                 '0x3b29d08-0x3b29d10': sha(bytes(8)),
                                 '0x3b18db5-0x3b18db6': sha(bytes(1))},
                  initialization=source_data, direct_references=direct_references, cases=len(cases), development_random=4096,
                  holdout_random=1024, ordered_source_reads_per_case=[hex(MISSION), hex(EDITOR)],
                  composition_rows=len(composition), reset_store_width=2,
                  adjacent_halves_preserved=True, mismatch_count=len(mismatches),
                  mismatches=mismatches[:10], observation_sha256=digest.hexdigest(),
                  native_library_sha256=sha(args.native_library.read_bytes()),
                  composition_sha256=sha(args.native_composition.read_bytes()),
                  passed=not mismatches,
                  limits=['Scalar original plus actual native callback composition, not a full reset oracle',
                          'No reserved Task/reset vectors, original DLL execution, app frame or N3b proof'])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('cases', 'composition_rows', 'mismatch_count', 'passed')}))
    return int(bool(mismatches))

if __name__ == '__main__':
    raise SystemExit(main())
