#!/usr/bin/env python3
"""Original PJoinVertexes initialization/cleanup and complete GetNodeIndex.

The active matrix branch stops at its real CalcMMMatrix call. It is not judged
as native welding. MAlloc/FreeMemory are bounded ordered allocation receipts;
the unchanged TaskWorkArena implementation is independently qualified.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
JOIN, NODE, SEARCH = 0x43fa90, 0x439f60, 0x439360
ALLOC, FREE, CALCULATE = 0x40b220, 0x40b310, 0x4b81f0
BASE, HEAP, ARENA = 0x10000000, 0x20000000, 0x1000000
PLAYER, TABLE = BASE + 0x100, BASE + 0x600
OBJECTS = [BASE + 0x400 + index * 64 for index in range(3)] + [0]
MODELS = [BASE + 0x300, BASE + 0x340]
VECTORS = [BASE + 0x2fd0 + index * 0x2000 for index in range(4)]
STACK, STOP = BASE + 0xef00, BASE + 0xff00
PAIRS = BASE + 0x10ff0


def packed(words):
    return struct.pack('<' + 'I' * len(words), *words)


class Source:
    def __init__(self, image):
        assert image.sha256 == SOURCE_SHA
        assert unicorn.__version__ == '2.1.4'
        self.m = m = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
        for page, size in [(0x43f000, 8192), (0x439000, 4096), (0x40b000, 4096),
                           (0x4b8000, 4096), (BASE, 4096), (BASE + 0xa000, 0x6000),
                           (BASE + 0x10000, 4096),
                           (HEAP, ARENA)]:
            m.mem_map(page, size)
        for vector in VECTORS:
            m.mem_map(vector & ~4095, 4096)
        for first, last in [(JOIN, 0x440375), (NODE, 0x439fb8), (SEARCH, 0x4393b2)]:
            m.mem_write(first, image.read(first, last - first))
        m.hook_add(unicorn.UC_HOOK_CODE, self.hook)

    def put(self, at, value, size=4):
        self.m.mem_write(at, (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'))

    def get(self, at, size=4):
        return int.from_bytes(self.m.mem_read(at, size), 'little')

    def allocate(self, requested):
        total = (requested + 63) & ~31
        if not requested or self.next + total > ARENA:
            return 0
        pointer = HEAP + self.next + 32
        self.next += total
        # The independently qualified arena retains its terminal sentinel in
        # the free list after an exact-full allocation (task_work unit 31–33).
        self.free_head = self.next
        self.allocations[pointer] = requested
        self.m.mem_write(pointer, b'\xff' * requested)
        return pointer

    def hook(self, m, at, size, _):
        if at == CALCULATE:
            self.failure = 2
            m.emu_stop()
            return
        if at not in (ALLOC, FREE):
            return
        sp = m.reg_read(UC_X86_REG_ESP)
        argument = self.get(sp + 4)
        if at == ALLOC:
            result = self.allocate(argument)
            self.receipts.append(['allocate', argument, result - HEAP if result else 0])
            m.reg_write(UC_X86_REG_EAX, result)
        else:
            self.released.add(argument)
            if argument:
                self.free_head = argument - HEAP - 32
            self.receipts.append(['free', argument - HEAP if argument else 0])
        m.reg_write(UC_X86_REG_EIP, self.get(sp))
        m.reg_write(UC_X86_REG_ESP, sp + 4)

    def prepare(self, values):
        v = values
        self.next = ARENA - v[9] if v[9] else 0
        self.free_head = self.next
        self.allocations, self.released, self.receipts = {}, set(), []
        self.failure = 0
        self.m.mem_write(BASE, bytes(4096))
        for index, address in enumerate(OBJECTS[:3]):
            self.put(address, v[26 + index])
            self.put(address + 4, 0 if v[29] & (1 << index) else MODELS[int(index == 2)])
            self.put(address + 44, OBJECTS[v[20 + index * 2]])
            self.put(address + 48, OBJECTS[v[21 + index * 2]])
        for index, address in enumerate(MODELS):
            self.put(address, VECTORS[index * 2])
            self.put(address + 4, 0 if v[31] & (1 << index) else VECTORS[index * 2 + 1])
        for index, address in enumerate(VECTORS):
            self.m.mem_write(address, packed(v[32 + index * 12:44 + index * 12]))
        self.previous = self.allocate(v[8]) if v[8] else 0
        if v[8]:
            assert self.previous
            self.m.mem_write(self.previous, packed(v[80:128])[:v[8]])
        self.put(PLAYER + 0x122, v[0], 2)
        self.put(PLAYER + 0x144, TABLE)
        self.put(TABLE, OBJECTS[v[5]])
        self.put(TABLE + 4, OBJECTS[v[3]])
        self.put(TABLE + 8, OBJECTS[v[4]])
        for offset, word in zip(range(12, 16), v[2:3] + v[1:2] + v[6:8]):
            self.put(TABLE + offset, word, 1)
        self.put(TABLE + 16, self.previous)
        self.put(TABLE + 20, 0 if v[30] else PAIRS)
        for index, word in enumerate(v[12:20]):
            self.put(PAIRS + index * 2, word, 2)
        # Real source arguments reach CalcMMMatrix; no matrix value is fabricated.
        self.put(PLAYER + 0x140, BASE + 0x700)
        self.put(BASE + 0x700, BASE + 0x800)
        self.put(BASE + 0x704, 3, 1)
        self.put(BASE + 0x800, OBJECTS[0])
        self.m.mem_write(STACK, packed([STOP, 0, 0, PLAYER]))
        self.m.reg_write(UC_X86_REG_ESP, STACK)

    def run(self, values):
        self.prepare(values)
        try:
            self.m.emu_start(JOIN, STOP, count=100000)
        except unicorn.UcError:
            self.failure = 1
        out = [0] * 192
        out[0] = self.failure
        out[1] = self.get(PLAYER + 0x122, 2)
        pointer = self.get(TABLE + 16)
        out[2] = pointer - HEAP if pointer else 0
        out[3] = self.previous - HEAP if self.previous else 0
        out[4] = int(bool(self.previous) and self.previous in self.released)
        out[5:9] = [self.get(TABLE + 14, 1), self.get(TABLE + 15, 1),
                     self.get(TABLE + 13, 1), self.get(TABLE + 12, 1)]
        out[9] = self.allocations[pointer] if pointer else 0
        out[10] = self.free_head + 1 if self.free_head is not None else 0
        out[11] = self.allocations[self.previous] if self.previous else 0
        out[16:20] = [int(self.get(TABLE + offset) == expected) for offset, expected in
                       [(0, OBJECTS[values[5]]), (4, OBJECTS[values[3]]),
                        (8, OBJECTS[values[4]]), (20, 0 if values[30] else PAIRS)]]
        for index, address in enumerate(VECTORS):
            out[32 + index * 12:44 + index * 12] = struct.unpack('<12I', self.m.mem_read(address, 48))
        if pointer:
            size = min(192, self.allocations[pointer])
            out[80:80 + size // 4] = struct.unpack('<' + 'I' * (size // 4), self.m.mem_read(pointer, size))
        if self.previous:
            size = values[8]
            out[128:128 + size // 4] = struct.unpack('<' + 'I' * (size // 4), self.m.mem_read(self.previous, size))
        return out

    def node(self, values):
        self.prepare(values)
        self.m.mem_write(STACK, packed([STOP, OBJECTS[values[5]], OBJECTS[values[3]]]))
        try:
            self.m.emu_start(NODE, STOP, count=10000)
        except unicorn.UcError:
            return [0, 1]
        return [ctypes.c_int32(self.m.reg_read(UC_X86_REG_EAX)).value, 0]

    def imported(self, path):
        objects, rows, native_mode = {}, [], None
        for line in Path(path).read_text().splitlines():
            kind, *values = line.split()
            values = list(map(int, values))
            if kind == 'object':
                objects[values[0]] = values[1:]
            elif kind == 'row':
                rows.append(values)
            elif kind == 'mode':
                native_mode = values[0]
        assert objects and len(rows) == 5 and native_mode is not None
        object_base = 0x30000000
        extent = ((max(objects) + 1) * 64 + 4095) & ~4095
        self.m.mem_map(object_base, extent)
        def pointer(identity):
            return object_base + identity * 64 if identity else 0
        for identity, (flags, child, sibling) in objects.items():
            assert not child or child in objects
            assert not sibling or sibling in objects
            self.put(pointer(identity), flags)
            self.put(pointer(identity) + 44, pointer(child))
            self.put(pointer(identity) + 48, pointer(sibling))
        self.prepare(case(random.Random(0), 0))
        self.put(PLAYER + 0x122, 0, 2)
        self.m.mem_write(TABLE, bytes(24 * len(rows)))
        for index, row in enumerate(rows):
            root, source, destination, count, mode, _, _ = row
            address = TABLE + index * 24
            for offset, value in [(0, pointer(root)), (4, pointer(source)), (8, pointer(destination))]:
                self.put(address + offset, value)
            self.put(address + 12, count, 1)
            self.put(address + 13, mode, 1)
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.emu_start(JOIN, STOP, count=1000000)
        actual = [[self.get(TABLE + index * 24 + 14, 1), self.get(TABLE + index * 24 + 15, 1)]
                  for index in range(len(rows))]
        expected = [row[5:] for row in rows]
        source_mode = self.get(PLAYER + 0x122, 2)
        return {'objects': len(objects), 'rows': len(rows), 'source_depths': actual,
                'native_depths': expected, 'source_mode': source_mode,
                'native_mode': native_mode,
                'passed': actual == expected and source_mode == native_mode == 1,
                'topology_sha256': hashlib.sha256(Path(path).read_bytes()).hexdigest()}


def case(rng, index):
    values = [0] * 128
    values[0] = rng.choice([0, 0, 0, 2, 65535, 1])
    values[1] = rng.choice(list(range(11)) + [127, 128, 255])
    values[2] = rng.randrange(5)
    values[3:6] = [rng.randrange(4) for _ in range(3)]
    values[6:8] = [rng.randrange(256), rng.randrange(256)]
    values[8] = rng.choice([0, 192])
    values[9] = rng.choice([0, 0, 0, 32, 64, 128, 256, 512]) if not values[8] else 0
    values[12:20] = [rng.randrange(5) for _ in range(8)]
    # Acyclic topology: chains, child/sibling ordering and disconnected targets.
    values[20:26] = rng.choice([[1, 3, 3, 2, 3, 3], [1, 2, 3, 3, 3, 3],
                               [3, 1, 2, 3, 3, 3], [3, 3, 3, 3, 3, 3]])
    values[26:29] = [rng.getrandbits(32) for _ in range(3)]
    values[32:128] = [rng.getrandbits(32) for _ in range(96)]
    if index < 80:
        # Complete successful backups/restoration, alias cases and all source modes.
        values[0] = 0 if index < 40 else 2
        values[1] = index % 10
        values[2] = index % 5
        values[3:6] = [1, 2, 0]
        values[8] = 0 if index < 40 else 192
        values[9] = 0
        values[12:20] = [0, 3, 1, 2, 2, 1, 3, 0]
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('probe')
    parser.add_argument('--cases', type=int, default=1024)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x47574c44)
    parser.add_argument('--output', default='build/oracles/player_vertex_weld.json')
    parser.add_argument('--imported-topology')
    args = parser.parse_args()
    image = PEImage(args.exe)
    source = Source(image)
    library = ctypes.CDLL(str(Path(args.probe).resolve()))
    native = library.sadx_native_player_vertex_weld
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    node = library.sadx_native_player_node_index
    node.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_int32)]
    rng = random.Random(args.seed)
    mismatches, failures, active, receipts = [], 0, 0, 0
    original_hash, native_hash = hashlib.sha256(), hashlib.sha256()
    node_hash = hashlib.sha256()
    cases = [case(rng, index) for index in range(args.cases)]
    # Preserve every existing development input. Append width/null-pointer cases
    # instead of replacing the earlier cohort or reducing its random coverage.
    extra_rng = random.Random(args.seed ^ 0x50414952)
    for index in range(64):
        values = case(extra_rng, index)
        values[0] = 0 if index < 32 else 2
        values[2] = [0, 4, 5, 127, 128, 255][index % 6]
        values[3:6] = [1, 2, 0]
        values[8] = 0 if index < 32 else 192
        values[9] = 0
        values[30] = int(index % 4 == 0)
        values[12:20] = [0, 3, 1, 2, 2, 1, 3, 0]
        cases.append(values)
    for index in range(64):
        values = case(extra_rng, index)
        values[0] = 0 if index < 32 else 2
        values[1] = 1 + index % 3
        values[2] = index % 5
        values[3:6] = [1, 2, 0]
        values[8] = 0 if index < 32 else 192
        values[9] = 0
        values[29] = index % 8
        values[12:20] = [0, 3, 1, 2, 2, 1, 3, 0]
        cases.append(values)
    for index in range(48):
        values = case(extra_rng, index)
        values[0] = 0 if index < 24 else 2
        values[1] = 1 + index % 3
        values[2] = index % 5
        values[3:6] = [1, 2, 0]
        values[8] = 0 if index < 24 else 192
        values[9] = 0
        values[31] = index % 4
        values[12:20] = [0, 3, 1, 2, 2, 1, 3, 0]
        cases.append(values)
    input_hash = hashlib.sha256()
    prefix_streams = None
    prefix_inputs = None
    for index, values in enumerate(cases):
        argument = (ctypes.c_uint32 * 128)(*values)
        input_hash.update(packed(values))
        actual = (ctypes.c_uint32 * 192)()
        assert native(argument, actual) == 1
        expected = source.run(values)
        original_hash.update(packed(expected))
        native_hash.update(packed(actual))
        failures += expected[0] == 1
        active += expected[0] == 2
        receipts += len(source.receipts)
        different = [word for word, pair in enumerate(zip(expected, actual)) if pair[0] != pair[1]]
        node_result = (ctypes.c_int32 * 2)()
        assert node(argument, node_result) == 1
        expected_node = source.node(values)
        node_hash.update(packed([value & 0xffffffff for value in expected_node]))
        if index == args.cases - 1:
            prefix_streams = [original_hash.hexdigest(), native_hash.hexdigest()]
            prefix_inputs = input_hash.hexdigest()
        if different or list(node_result) != expected_node:
            mismatches.append({'case': index, 'words': different,
                               'source': [expected[word] for word in different],
                               'native': [actual[word] for word in different],
                               'source_node': expected_node, 'native_node': list(node_result),
                               'input': values})
    imported = source.imported(args.imported_topology) if args.imported_topology else None
    matched = not mismatches and (not imported or imported['passed'])
    report = {
        'slug': 'player_vertex_weld', 'cases': len(cases), 'seed': args.seed,
        'original_development_cohort': {'cases': args.cases, 'stream_sha256': prefix_streams,
                                        'inputs_sha256': prefix_inputs},
        'all_inputs_sha256': input_hash.hexdigest(), 'observer_schema': 2,
        'passed': matched, 'mismatch_count': len(mismatches),
        'values_match': matched, 'failure_prefix_cases': failures,
        'active_matrix_boundary_cases': active, 'allocator_receipts': receipts,
        'source_stream_sha256': original_hash.hexdigest(),
        'native_stream_sha256': native_hash.hexdigest(), 'node_stream_sha256': node_hash.hexdigest(),
        'source_sha256': image.sha256,
        'exe_ranges': [[hex(JOIN), '0x440375'], [hex(NODE), '0x439fb8'],
                       [hex(SEARCH), '0x4393b2']],
        'original_ranges_sha256': {
            f'{start:x}-{end:x}': hashlib.sha256(image.read(start, end - start)).hexdigest()
            for start, end in [(JOIN, 0x440375), (NODE, 0x439fb8), (SEARCH, 0x4393b2)]},
        'probe_sha256': hashlib.sha256(Path(args.probe).read_bytes()).hexdigest(),
        'mismatches': mismatches[:20],
        'imported_gamma': imported,
        'limits': ['Initialization and backup restoration only; active branch stops at CalcMMMatrix.',
                   'No active deformation or attachment-vector or matrix-buffer claim.',
                   'MAlloc/FreeMemory are ordered receipts; actual native arena owns all backups.',
                   'Receipt counts are source diagnostics; native handles, bytes and release lifetime compare.',
                   'One invocation per case; native unit separately exercises repeated cleanup.',
                   'All 48 scene words, both backup images, depths and owner identities compared.',
                   'Original x86 instructions execute; node flags and child/sibling order are varied.']}
    path = Path(args.output)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ['cases', 'mismatch_count', 'failure_prefix_cases',
                                                 'active_matrix_boundary_cases', 'passed']}))
    return 0 if matched else 1


if __name__ == '__main__':
    raise SystemExit(main())
