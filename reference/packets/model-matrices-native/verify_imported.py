"""Compare actual Gamma Stand/generated action matrices with original instructions.

Only allocation suppliers are intercepted. PSetMotion, transition construction,
CalcMMMatrix, ScanMotionModel and reached Ninja primitives execute original code.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/import')]
import unicorn as uc
from unicorn.x86_const import *
from pe_image import PEImage
from sonic_actor_resources import export, SOURCE_SHA, SOURCE_SHA256
from gamma_declarations import recipes, ANIMATION

STATE, STACK, ALLOC, STOP = 0x20000100, 0x2000f000, 0x20010000, 0xf000000
FRAMES, DEST = 0x20002000, 0x20005000
COUNT, BUFFER, LIMIT, UNIT = 0x3c58270, 0x3c58278, 0x3c5a278, 0x389d650

def pack(words): return struct.pack('<' + 'I' * len(words), *words)
def bits(f): return struct.unpack('<I', struct.pack('<f', f))[0]
def sha(data): return hashlib.sha256(data).hexdigest()

def original(exe, dll, label):
    m = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
    for image in (exe, dll):
        for section in image.sections:
            size = (max(section['raw_size'], section['virtual_size']) + 4095) & ~4095
            if size:
                at = image.base + section['rva']
                m.mem_map(at, size)
                m.mem_write(at, image.slice(section['file_offset'], section['raw_size']))
    m.mem_map(STOP, 4096); m.mem_map(0x20000000, 0x100000)
    actions = export(dll, '___E102_ACTIONS')
    declarations = dll.unpack('<78I', actions)
    _, table, _ = recipes(exe, actions, declarations)
    m.mem_write(ANIMATION[2], bytes(table))
    m.mem_write(STATE, bytes(48)); m.mem_write(STATE + 32, pack([ANIMATION[2]]))
    cursor = ALLOC
    def allocation(machine, at, size, user):
        nonlocal cursor
        if at not in (0x40b220, 0x40b2d0): return
        sp = machine.reg_read(UC_X86_REG_ESP)
        ret, count = struct.unpack('<II', machine.mem_read(sp, 8))
        extent = count if at == 0x40b220 else count * struct.unpack('<I', machine.mem_read(sp + 8, 4))[0]
        assert extent in (8, 12, 111 * 24, 32)
        machine.mem_write(cursor, bytes(extent) if at == 0x40b2d0 else bytes([0xa5]) * extent)
        machine.reg_write(UC_X86_REG_EAX, cursor); cursor += (extent + 15) & ~15
        machine.reg_write(UC_X86_REG_ESP, sp + 4); machine.reg_write(UC_X86_REG_EIP, ret)
    m.hook_add(uc.UC_HOOK_CODE, allocation)
    def call(entry, args):
        m.reg_write(UC_X86_REG_FPCW, 127); m.reg_write(UC_X86_REG_FPSW, 0)
        m.reg_write(UC_X86_REG_FPTAG, 0xffff)
        m.mem_write(STACK, pack([STOP, *args])); m.reg_write(UC_X86_REG_ESP, STACK)
        m.emu_start(entry, STOP, count=1000000)
        assert m.reg_read(UC_X86_REG_EIP) == STOP
    def read(at, count): return list(struct.unpack('<' + 'I' * count, m.mem_read(at, count * 4)))
    call(0x44a800, [STATE])
    action, frame = declarations[0], 0
    if label != 'stand':
        m.mem_write(STATE + 4, struct.pack('<HH', 43, 0))
        m.mem_write(STATE + 12, bytes(2)); m.mem_write(STATE + 16, struct.pack('<f', 4.5))
        root, = dll.unpack('<I', declarations[0]); child, = dll.unpack('<I', root + 44)
        rotation = (1234, -2345, 30000) if label == 'idle_pose_a' else (-321, 4321, -12345)
        m.mem_write(child + 20, struct.pack('<3i', *rotation))
        call(0x44a800, [STATE])
        action, frame = read(STATE + 40, 1)[0], read(STATE + 16, 1)[0]
    m.mem_write(BUFFER, pack([bits((n * 16 + c + 17) / 32) for n in range(128) for c in range(16)]))
    m.mem_write(FRAMES, pack([bits((s * 16 + c + 1) / 64) for s in range(128) for c in range(16)]))
    m.mem_write(DEST, pack([bits((c + 39) / 8) for c in range(16)]))
    for at, val in [(COUNT, 19), (LIMIT, 23), (0x3d0f818, FRAMES), (0x3d0fa28, 128), (0x3d0fa2c, 1)]:
        m.mem_write(at, pack([val]))
    call(0x4b81f0, [UNIT, action, frame, 76, DEST])
    flags = m.reg_read(UC_X86_REG_FPSW) & 63
    if label == 'stand': call(0x4b82d0, [54, DEST])
    rows = [[read(COUNT, 1)[0], read(LIMIT, 1)[0], read(0x3d0fa2c, 1)[0], 127, flags]]
    rows += [read(BUFFER + i * 64, 16) for i in range(128)]
    rows += [read(FRAMES + i * 64, 16) for i in range(128)]
    rows += [read(DEST, 16)]
    return rows

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe'); p.add_argument('dll'); p.add_argument('observations'); p.add_argument('output')
    a = p.parse_args(); exe, dll = PEImage(a.exe), PEImage(a.dll)
    assert exe.sha256 == SOURCE_SHA256 and dll.sha256 == SOURCE_SHA and uc.__version__ == '2.1.4'
    observations = {}
    for line in Path(a.observations).read_text().splitlines():
        label, kind, *rest = line.split()
        row = list(map(int, rest if kind == 'state' else rest[1:]))
        observations.setdefault(label, []).append(row)
    failures, partials = [], []
    for label, rows in observations.items():
        expected = original(exe, dll, label)
        assert len(rows) == len(expected) == 258
        for index, (source, native) in enumerate(zip(expected, rows)):
            assert len(source) == len(native)
            for field, (x, y) in enumerate(zip(source, native)):
                if x == y: continue
                item = {'scenario': label, 'row': index, 'field': field, 'source': x, 'native': y}
                (partials if index == 0 and field == 4 else failures).append(item)
    report = {'scenarios': len(observations), 'compared_rows': sum(map(len, observations.values())),
              'failures': failures, 'fp_partials': partials, 'source_sha256': exe.sha256,
              'values_match': not failures, 'flag_mismatches': len(partials),
              'passed': not failures and not partials,
              'dll_sha256': dll.sha256, 'observation_sha256': sha(Path(a.observations).read_bytes()),
              'limits': ['Original allocator policy supplied by deterministic fixture.',
                         'Actual actions, generated keys and live pose edits; controlled post-initialization callers.',
                         'No first full-route GetMMMatrix state, active welding, attachments, actor frame or N3b claim.']}
    Path(a.output).write_text(json.dumps(report, indent=2) + '\n')
    print(f'{report["compared_rows"]} real imported rows, {len(failures)} value mismatches, {len(partials)} individual FP partials')
    return 1 if failures else 2 if partials else 0
if __name__ == '__main__': raise SystemExit(main())
