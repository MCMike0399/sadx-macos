"""Execute SonicTheHedgehog's actual inline source prefix against real declarations.

Normal splice cases stop before PJoinVertexes; equal-sibling/Metal cases execute
real CCL_Init and its allocator/range callees, then stop before LoadCharTextures.
This is a source-site fixture, not a recovered standalone function ABI.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/import'),
                str(ROOT / 'tools/oracles')]
from pe_image import PEImage
from sonic_actor_resources import declarations, CONSTRUCTORS, SOURCE_SHA, SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP
from verify_original_collision_component import call, u32
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UcError
from unicorn.x86_const import (UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ESI,
                               UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_FPSW, UC_X86_REG_EIP)

ARENA, TASK, PLAYER, WORLD, TABLE = 0x20000000, DATA + 0x1000, DATA + 0x2000, DATA + 0x3000, DATA + 0x5000
JOINS, CACHE, METAL, OBJECTS = 0x3c55e28, 0x3c55d40, 0x3b18db5, 0x38f6e80

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def words(raw):
    return list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))

def put(machine, address, value):
    machine.mem_write(address, struct.pack('<I', value))

def original(exe, dll, metadata, addresses, pairs, join_bytes, inputs):
    original = Original(exe, 127)
    m = original.machine
    for base, size in ((ARENA, 0x1001000), (0x3abd000, 4096), (0x3c55000, 8192),
                       (0x38f6000, 4096), (0x3b18000, 4096), (0x91c000, 4096)):
        m.mem_map(base, size)
    # Materialize the pinned pair arrays for observation; the prefix never writes them.
    for address, identity in pairs.items():
        extent = 4 * len(metadata['vertex_pairs'][identity - 1]['values'])
        assert 0x91c000 <= address and address + extent <= 0x91d000
        m.mem_write(address, exe.read(address, extent))
    root, attach = (addresses[metadata['objects'][index]['object']] for index in (1, 0))
    m.mem_map(root & ~4095, 8192)
    m.mem_write(root, dll.read(root, 52))
    m.mem_write(0x91cbf0, exe.read(0x91cbf0, 96))
    m.mem_write(JOINS, join_bytes)
    m.mem_write(TABLE, bytes(79 * 4))
    put(m, TABLE + 5 * 4, attach)
    put(m, TABLE + 6 * 4, root)
    put(m, OBJECTS, TABLE)
    m.mem_write(METAL, bytes((inputs[0],)))
    put(m, root + 48, addresses[inputs[1]] if inputs[1] else 0)
    put(m, root + 8, inputs[9])
    put(m, CACHE, addresses[inputs[2]] if inputs[2] else 0)
    put(m, JOINS + 11 * 24 + 8, addresses[inputs[3]] if inputs[3] else 0)
    m.mem_write(JOINS + 10 * 24 + 14, bytes((inputs[10],)))
    m.mem_write(JOINS + 12 * 24 + 15, bytes((inputs[11],)))
    # Exact source allocator/scheduler reservation and primary allocation.
    from unicorn.x86_const import UC_X86_REG_EDX
    m.reg_write(UC_X86_REG_EDX, ARENA)
    call(m, 0x40b1ce, [])
    call(m, 0x40b220, [0x34000])
    primary = call(m, 0x40b2d0, [1, 64])
    put(m, TASK + 32, primary)
    m.mem_write(primary, bytes((inputs[6],)))
    m.mem_write(primary + 4, struct.pack('<H', inputs[7]))
    put(m, PLAYER + 0x144, 0)
    m.mem_write(PLAYER + 0x122, struct.pack('<H', inputs[4]))
    m.mem_write(PLAYER + 0xe, struct.pack('<H', inputs[5]))
    put(m, PLAYER + 0x10c, inputs[12])
    if inputs[8] != 0xffffffff:
        free_start = u32(m, 0x3abdbe8)
        free_bytes = ARENA + 0x1000000 - free_start
        if free_bytes > inputs[8] + 32:
            assert call(m, 0x40b220, [free_bytes - inputs[8] - 32])
    put(m, STACK + 0x2c, TASK)
    for register, value in ((UC_X86_REG_EBP, PLAYER), (UC_X86_REG_EBX, primary),
                            (UC_X86_REG_ESI, WORLD), (UC_X86_REG_EDI, 0),
                            (UC_X86_REG_ESP, STACK - 8)):
        m.reg_write(register, value)
    boundary = [0]
    calls = []
    def hook(machine, address, size, _):
        if address in (0x43fa90, 0x41caf0, 0x421280):
            calls.append(hex(address))
        if address == 0x43fa90:
            boundary[0] = 1
            machine.emu_stop()
        elif address == 0x421280:
            boundary[0] = 2
            machine.emu_stop()
    fault = []
    def invalid(machine, access, address, size, value, _):
        fault.append(dict(pc=hex(machine.reg_read(UC_X86_REG_EIP)), address=hex(address), size=size))
        boundary[0] = 3
        return False
    m.hook_add(UC_HOOK_CODE, hook)
    m.hook_add(UC_HOOK_MEM_INVALID, invalid)
    try:
        m.emu_start(0x49ab58, STOP, count=100000)
    except UcError:
        if not fault:
            raise
    assert boundary[0]
    if fault:
        assert fault[0]['address'] == '0xc', fault
    reverse = {address: identity for identity, address in addresses.items()}
    def identity(address):
        return reverse[address] if address else 0
    out = [0] * 320
    out[:8] = [boundary[0], identity(u32(m, root + 48)), identity(u32(m, CACHE)),
               int(u32(m, PLAYER + 0x144) == JOINS),
               struct.unpack('<H', m.mem_read(PLAYER + 0x122, 2))[0],
               struct.unpack('<H', m.mem_read(PLAYER + 0xe, 2))[0],
               m.mem_read(primary, 1)[0], struct.unpack('<H', m.mem_read(primary + 4, 2))[0]]
    work = u32(m, primary + 56)
    if work:
        info = u32(m, work + 12)
        out[8:10] = [work - ARENA, info - ARENA]
        out[11] = u32(m, work + 8)
        out[12] = struct.unpack('<H', m.mem_read(work + 6, 2))[0]
        out[13] = struct.unpack('<H', m.mem_read(work + 4, 2))[0]
        out[251:275] = words(m.mem_read(info, 96))
    out[10] = u32(m, 0x3abdbe8) - ARENA
    out[15] = m.reg_read(UC_X86_REG_FPSW) & 63
    obj = words(m.mem_read(root, 52))
    for index in (1, 11, 12):
        obj[index] = identity(obj[index])
    out[16:29] = obj
    pair_observations = []
    for index in range(37):
        row = words(m.mem_read(JOINS + index * 24, 24))
        for column in range(3):
            row[column] = identity(row[column])
        assert row[4] == 0  # no PJoin execution is claimed
        pointer = row[5]
        row[5] = pairs[pointer] if pointer else 0
        count = 2 * (row[3] & 255) if pointer else 0
        pair_observations.append(count)
        if count:
            pair_observations.extend(struct.unpack('<' + 'H' * count, m.mem_read(pointer, count * 2)))
        out[29 + index * 6:35 + index * 6] = row
    out.extend(pair_observations)
    assert len(out) <= 1024
    out.extend([0] * (1024 - len(out)))
    return out, calls, fault

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('library', type=Path)
    parser.add_argument('--assets', type=Path, default=ROOT / 'game/imported')
    parser.add_argument('--exe', type=Path, default=ROOT / 'game/Sonic Adventure DX/sonic.exe')
    parser.add_argument('--dll', type=Path, default=ROOT / 'game/Sonic Adventure DX/system/CHRMODELS_orig.dll')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--holdout', action='store_true')
    args = parser.parse_args()
    exe, dll = PEImage(args.exe), PEImage(args.dll)
    assert exe.sha256 == SOURCE_SHA256 and dll.sha256 == SOURCE_SHA
    meta = json.loads((args.assets / 'characters/sonic-actor.json').read_text())
    proof = json.loads((args.assets / 'characters/sonic-selection.provenance.json').read_text())
    assert proof['pack_sha256'] == sha((args.assets / 'characters/sonic-selection.actions').read_bytes())
    addresses = {row['id']: int(row['source_address'], 16) for row in proof['records']
                 if row.get('direct_image_bytes') and row.get('count') == 1}
    join_bytes, construction = declarations(dll, exe, CONSTRUCTORS[0])
    pairs = {}
    for index, row in enumerate(meta['joins']['sonic']):
        raw = words(join_bytes[index * 24:(index + 1) * 24])
        assert raw[:3] == [addresses[row[key]] if row[key] else 0
                           for key in ('object', 'source_object', 'destination_object')]
        assert raw[3] == row['vertex_count'] | ((row['input_mode'] & 255) << 8) | (row['source_node'] << 16) | (row['destination_node'] << 24)
        if raw[5]:
            pairs[raw[5]] = row['vertex_pairs']
            values = meta['vertex_pairs'][row['vertex_pairs'] - 1]['values']
            assert exe.read(raw[5], 4 * row['vertex_count']) == b''.join(struct.pack('<HH', *pair) for pair in values)
    library = ctypes.CDLL(str(args.library.resolve()))
    function = library.sadx_sonic_model_observed
    word = ctypes.c_uint32
    function.argtypes = [ctypes.c_char_p, ctypes.POINTER(word), ctypes.POINTER(word), ctypes.c_size_t]
    rng = random.Random(0x534f4e + args.holdout)
    attach, root, other = [row['object'] for row in meta['objects']]
    cases = []
    for alternate in (0, 1, 2, 128, 255):
        for sibling in (0, attach, root, other):
            for remaining in (0xffffffff, 0, 224, 352):
                cases.append([alternate, sibling, rng.choice([0, attach, other]),
                              rng.choice([0, root, other]), rng.randrange(65536),
                              rng.randrange(65536), rng.randrange(256), rng.randrange(65536),
                              remaining, rng.getrandbits(32), rng.randrange(256), rng.randrange(256),
                              rng.getrandbits(32)])
    if args.holdout:
        rng.shuffle(cases)
    failures, partials, observations = [], [], []
    for index, case in enumerate(cases):
        expected, calls, fault = original(exe, dll, meta, addresses, pairs, join_bytes, case)
        output = (word * 1024)()
        assert function(str(args.assets).encode(), (word * len(case))(*case), output, len(output)) == 0
        actual = list(output)
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        values = [i for i in differences if i != 15]
        if values:
            failures.append(dict(case=index, input=case, differences=[(i, expected[i], actual[i]) for i in values]))
        elif differences:
            partials.append(dict(case=index, source_flags=expected[15], native_flags=actual[15]))
        observations.append(dict(case=index, boundary=expected[0], calls=calls, fault=fault,
                                 source_sha256=sha(struct.pack('<320I', *expected[:320])),
                                 native_sha256=sha(struct.pack('<320I', *actual[:320])),
                                 extended_source_sha256=sha(struct.pack('<1024I', *expected)),
                                 extended_native_sha256=sha(struct.pack('<1024I', *actual))))
    report = dict(cases=len(cases), holdout=args.holdout, observation_schema=2, output_words=1024,
                  failures=failures, partials=partials,
                  values_match=not failures, passed=not failures and not partials,
                  original_ranges_sha256={'0x49ab58-0x49abf0':sha(exe.read(0x49ab58,0x98)),
                                          '0x41caf0-0x41cbc0':sha(exe.read(0x41caf0,0xd0))},
                  source_game_sha256=exe.sha256, source_dll_sha256=dll.sha256,
                  construction=construction, observations=observations,
                  limits=['Inline source-site fixture, not a function entry.',
                          'Normal splice stops before real PJoinVertexes; no helper or actor-frame completion.',
                          'Only CCL allocation/range callees execute; original textures/common tail do not.',
                          'All 37 join records, complete slot6 object, collision records/range/flags and allocation prefixes compared.',
                          'FP status differences are individual partials, never passes.'])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('cases','holdout','values_match','passed')}))
    return 1 if failures else 2 if partials else 0

if __name__ == '__main__':
    raise SystemExit(main())
