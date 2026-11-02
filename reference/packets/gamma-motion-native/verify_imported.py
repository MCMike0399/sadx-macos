"""Compare real imported Gamma transitions with the pinned PSetMotion instructions.

Only malloc/calloc are supplied by a deterministic fixture. No game startup,
Windows loader, original allocator policy or actor draw is claimed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/import')]
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP,
                              UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)
from pe_image import PEImage
from sonic_actor_resources import export, SOURCE_SHA, SOURCE_SHA256
from gamma_declarations import recipes, ANIMATION

STATE = 0x20000100
STACK = 0x2000f000
ALLOC = 0x20010000
STOP = 0x0f000000


def original(exe, dll, label, control):
    machine = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
    for image in (exe, dll):
        for section in image.sections:
            size = (max(section['raw_size'], section['virtual_size']) + 4095) & ~4095
            if not size:
                continue
            address = image.base + section['rva']
            machine.mem_map(address, size)
            machine.mem_write(address, image.slice(section['file_offset'], section['raw_size']))
    machine.mem_map(STOP, 4096)
    machine.mem_map(0x20000000, 0x100000)
    actions = export(dll, '___E102_ACTIONS')
    declarations = dll.unpack('<78I', actions)
    _, table, _ = recipes(exe, actions, declarations)
    machine.mem_write(ANIMATION[2], bytes(table))
    machine.mem_write(STATE, bytes(48))
    machine.mem_write(STATE + 32, struct.pack('<I', ANIMATION[2]))
    cursor = ALLOC

    def allocate(m, address, size, unused):
        nonlocal cursor
        if address not in (0x40b220, 0x40b2d0):
            return
        sp = m.reg_read(UC_X86_REG_ESP)
        ret, count = struct.unpack('<II', m.mem_read(sp, 8))
        extent = count
        if address == 0x40b2d0:
            unit = struct.unpack('<I', m.mem_read(sp + 8, 4))[0]
            extent *= unit
        if extent not in (8, 12, 111 * 24, 32):
            raise RuntimeError(f'unexpected allocator request {extent}')
        m.mem_write(cursor, bytes(extent) if address == 0x40b2d0 else bytes([0xa5]) * extent)
        m.reg_write(UC_X86_REG_EAX, cursor)
        cursor += (extent + 15) & ~15
        m.reg_write(UC_X86_REG_ESP, sp + 4)
        m.reg_write(UC_X86_REG_EIP, ret)

    machine.hook_add(unicorn.UC_HOOK_CODE, allocate)

    def call():
        machine.reg_write(UC_X86_REG_FPCW, control)
        machine.reg_write(UC_X86_REG_FPSW, 0)
        machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
        machine.mem_write(STACK, struct.pack('<II', STOP, STATE))
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.emu_start(0x44a800, STOP, count=1000000)
        if machine.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError('PSetMotion did not return')

    call()  # Real Initialize path, with the same zeroed request/current/mode.
    if label != 'stand':
        machine.mem_write(STATE + 4, struct.pack('<HH', 43, 0))
        machine.mem_write(STATE + 12, bytes(2))
        machine.mem_write(STATE + 16, struct.pack('<f', 4.5))
        root = dll.unpack('<I', declarations[0])[0]
        child = dll.unpack('<I', root + 44)[0]
        rotation = (1234, -2345, 30000) if label == 'idle_pose_a' else (-321, 4321, -12345)
        machine.mem_write(child + 20, struct.pack('<3i', *rotation))
    call()
    words = struct.unpack('<8H', machine.mem_read(STATE, 16))
    frame = struct.unpack('<I', machine.mem_read(STATE + 16, 4))[0]
    state = [words[3], words[2], words[4], words[0], words[5], words[6], words[7], frame,
             machine.reg_read(UC_X86_REG_FPCW), machine.reg_read(UC_X86_REG_FPSW) & 63]
    action = struct.unpack('<I', machine.mem_read(STATE + 40, 4))[0]
    motion = struct.unpack('<I', machine.mem_read(action + 4, 4))[0]
    data = struct.unpack('<I', machine.mem_read(motion, 4))[0]
    result = [state]
    for node in range(77):
        for channel in range(2):
            pointer, = struct.unpack('<I', machine.mem_read(data + node * 16 + channel * 4, 4))
            count, = struct.unpack('<I', machine.mem_read(data + node * 16 + 8 + channel * 4, 4))
            row = [int(bool(pointer)), count]
            if pointer:
                row.extend(struct.unpack('<8I', machine.mem_read(pointer, 32)))
            result.append(row)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=Path)
    parser.add_argument('dll', type=Path)
    parser.add_argument('observations', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    exe, dll = PEImage(args.exe), PEImage(args.dll)
    assert exe.sha256 == SOURCE_SHA256 and dll.sha256 == SOURCE_SHA
    assert unicorn.__version__ == '2.1.4'
    observations = {}
    for line in args.observations.read_text().splitlines():
        label, node, *rest = line.split()
        if node == 'state':
            assert label not in observations
        else:
            slot = len(observations[label]) - 1
            assert int(node) == slot // 2 and int(rest[0]) == slot % 2
        row = list(map(int, rest if node == 'state' else rest[1:]))
        observations.setdefault(label, []).append(row)
    assert set(observations) == {'stand', 'idle_pose_a', 'idle_pose_b'}
    failures, partials = [], []
    for label, rows in observations.items():
        assert len(rows) == 155
        expected = original(exe, dll, label, rows[0][8])
        for index, (source, native) in enumerate(zip(expected, rows)):
            for field, (a, b) in enumerate(zip(source, native)):
                if a == b:
                    continue
                item = dict(scenario=label, row=index, field=field, source=a, native=b)
                (partials if index == 0 and field == 9 else failures).append(item)
            assert len(source) == len(native)
    report = dict(scenarios=len(observations), compared_rows=sum(map(len, observations.values())),
                  failures=failures, fp_partials=partials, source_sha256=exe.sha256,
                  dll_sha256=dll.sha256, observation_sha256=hashlib.sha256(args.observations.read_bytes()).hexdigest(),
                  source_ranges={hex(address): dict(bytes=size, sha256=hashlib.sha256(exe.read(address, size)).hexdigest())
                                 for address, size in [(0x44a800, 0x641), (0x43f2a0, 0x750),
                                                       (0x446950, 0x57), (0x439fc0, 0x50)]},
                  limits=['Real imported Stand and preview actions; deterministic allocator supplier.',
                          'Controlled post-initialization transitions supplement the real Gamma caller.',
                          'No physical Windows, complete actor frame or allocator-policy claim.'])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return 1 if failures else 2 if partials else 0


if __name__ == '__main__':
    raise SystemExit(main())
