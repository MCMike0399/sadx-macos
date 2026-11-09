"""EntryColliList against the pinned sonic.exe.

Unicorn runs 0x41c280, including CCL_ClearInfo. The ten list bases and their
counts are the source globals. A DestroyTask exec pointer returns before the
stage word is stored.
"""
import ctypes
import hashlib
import json
import random
import struct
import sys
from pathlib import Path

import unicorn as uc
from unicorn import UcError
from unicorn.unicorn_const import UC_PROT_ALL

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage
from verify_original_math import Original, STACK, STOP
from verify_original_task_work import call

ENTRY = 0x41c280
SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
TASKWK = 0x20000000
WORK = 0x21000000
OWNER = 0x22000000
DESTROY = 0x40b570
SENTINEL = 0x11111111
# id, stage, destroy, status, count, work_present, hit_count
WORDS, OUT_WORDS = 7, 6
LISTS = (
    (0x3b27468, 0x3b25980, None),
    (0x3b23290, 0x3b27050, None),
    (0x3b2746c, 0x3b23ea8, 0x3b240f8),
    (0x3b27c88, 0x3b25780, 0x3b26e48),
    (0x3b240f4, 0x3b23aa8, None),
    (0x3b27c80, 0x3b27268, None),
    (0x3b27260, 0x3b27c90, None),
    (0x3b23294, 0x3b27e90, None),
    (0x3b240f0, 0x3b22e88, None),
    (0x3b27c78, 0x3b240b0, None),
)


def blank(values):
    row = [0, 0, 0, 0x14, 0, 1, 4]
    row[:len(values)] = values
    return [value & 0xffffffff for value in row]


def fixtures(count, seed):
    cases = [
        blank([0, 0, 0, 0x14, 0, 1, 4]),
        blank([0, 0x27, 0, 0x14, 0, 1, 4]),
        blank([0, 0, 0, 0x14, 16, 1, 4]),
        blank([10, 0, 0, 0x14, 0, 1, 4]),
        blank([2, 0, 0, 0x14, 0, 1, 4]),
        blank([3, 0x28, 0, 0xff, 0, 1, 9]),
        blank([0, 0x27, 1, 0x14, 0, 1, 4]),
        blank([0, 0, 0, 0x14, 0, 0, 4]),
        blank([9, 0, 0, 0x14, 15, 1, 1]),
        blank([9, 1, 0, 0x14, 16, 1, 1]),
        blank([1, 0x127, 0, 0x14, 127, 1, 2]),
        blank([4, -1, 0, 0, 0, 1, 0]),
    ]
    rng = random.Random(seed)
    while len(cases) < count:
        cases.append(blank([
            rng.choice([0, 1, 2, 3, 8, 9, 10, 40]),
            rng.choice([0, 0x26, 0x27, 0x2a, 0x2b, 0x127, -1, 300]),
            rng.choice([0, 0, 1]),
            rng.choice([0, 0x14, 0xff, 0xc3]),
            rng.choice([0, 1, 15, 16, 127, 128, 255, 256]),
            rng.choice([1, 1, 0]),
            rng.choice([0, 4, 9]),
        ]))
    return cases


class Source(Original):
    def __init__(self, image):
        super().__init__(image, 0x7f)
        self.image_bytes = image.read(ENTRY, 0x41c4e0 - ENTRY)
        for page in (TASKWK, WORK, OWNER):
            self.machine.mem_map(page, 0x1000, UC_PROT_ALL)
        self.machine.mem_map(0x3b20000, 0x30000, UC_PROT_ALL)

    def run(self, case):
        machine = self.machine
        machine.mem_write(TASKWK, bytes(0x80))
        machine.mem_write(WORK, bytes(0x100))
        machine.mem_write(OWNER, bytes(0x20))
        ident, stage, destroy, status, count, present, hit = case
        machine.mem_write(TASKWK + 0x38, struct.pack('<I', WORK if present else 0))
        machine.mem_write(TASKWK + 4, struct.pack('<H', status & 0xffff))
        machine.mem_write(WORK, struct.pack('<H', ident & 0xffff))
        machine.mem_write(WORK + 2, struct.pack('<H', hit & 0xffff))
        machine.mem_write(WORK + 0x9c, struct.pack('<I', OWNER))
        machine.mem_write(OWNER + 0x10, struct.pack('<I', DESTROY if destroy else 0))
        machine.mem_write(0x3b22dcc, struct.pack('<H', stage & 0xffff))
        machine.mem_write(0x3b22dec, struct.pack('<H', 0))
        machine.mem_write(0x3b22e80, struct.pack('<I', SENTINEL))
        for count_at, _, _ in LISTS:
            machine.mem_write(count_at, struct.pack('<H', 0))
        if ident < 10:
            count_at, slot_at, work_at = LISTS[ident]
            machine.mem_write(count_at, struct.pack('<H', count & 0xffff))
            machine.mem_write(slot_at + (count & 0xffff) * 4, struct.pack('<I', 0))
            if work_at is not None:
                machine.mem_write(work_at + (count & 0xffff) * 4, struct.pack('<I', 0))
        try:
            eax = call(machine, ENTRY, [TASKWK])
        except (UcError, RuntimeError):
            return None
        special = struct.unpack('<I', machine.mem_read(0x3b22e80, 4))[0]
        stored_count = struct.unpack('<H', machine.mem_read(LISTS[ident][0], 2))[0] if ident < 10 else 0
        stored_work = 0
        if not present:
            route = 0 if eax == 0 else 99
        elif destroy:
            route = 1 if eax == 0 and special == SENTINEL else 99
        elif eax == 1 and ident < 10:
            route = 3
            _, slot_at, work_at = LISTS[ident]
            stored = struct.unpack('<I', machine.mem_read(slot_at + (count & 0xffff) * 4, 4))[0]
            if stored != TASKWK:
                route = 99
            if work_at is not None:
                stored_work = int(struct.unpack('<I', machine.mem_read(
                    work_at + (count & 0xffff) * 4, 4))[0] == WORK)
        else:
            route = 2 if eax == 0 else 99
        observed_status = struct.unpack('<H', machine.mem_read(TASKWK + 4, 2))[0]
        observed_hit = struct.unpack('<H', machine.mem_read(WORK + 2, 2))[0]
        return [route, special, stored_count, observed_status, observed_hit, stored_work]


def main():
    image = PEImage('game/Sonic Adventure DX/sonic.exe')
    if image.sha256 != SOURCE_SHA or uc.__version__ != '2.1.4':
        raise SystemExit('source or unicorn mismatch')
    probe = Path('build/native-collision-entry-probe.dylib')
    library = ctypes.CDLL(str(probe.resolve()))
    library.sadx_native_collision_entry.argtypes = [
        ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    library.sadx_native_collision_entry.restype = ctypes.c_int
    source = Source(image)
    cases = fixtures(48, ENTRY)
    mismatches = faults = 0
    samples = []
    for index, case in enumerate(cases):
        native = (ctypes.c_uint32 * OUT_WORDS)()
        ok = library.sadx_native_collision_entry((ctypes.c_uint32 * WORDS)(*case), native)
        original = source.run(case)
        match = ok == 0 and original is not None and list(native) == original
        mismatches += not match
        faults += original is None
        if not match and len(samples) < 8:
            samples.append({'index': index, 'case': case, 'native': list(native),
                            'original': original, 'ok': ok})
    report = {
        'passed': mismatches == 0 and faults == 0,
        'values_match': mismatches == 0,
        'cases': len(cases),
        'mismatches': mismatches,
        'faults': faults,
        'holdout_index': len(cases) // 2,
        'source_range_sha256': hashlib.sha256(source.image_bytes).hexdigest(),
        'source': SOURCE_SHA,
        'entry': hex(ENTRY),
        'samples': samples,
        'limits': [
            'CCL_ClearInfo runs. The info walk is held at count 0 so only the status byte and hit count are compared.',
            'The source stores the taskwk. The native slot stores the task that owns that primary, so the pointer word itself is not compared.',
            'Ids 2 and 3 also store the collision work. The other ids do not.',
            'A DestroyTask exec returns before the special-stage word is written.',
        ],
    }
    out = Path('build/oracles/collision_entry.json')
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'cases', 'mismatches', 'faults')}))
    if samples:
        print(json.dumps(samples[:3]))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
