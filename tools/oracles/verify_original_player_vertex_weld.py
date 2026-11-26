#!/usr/bin/env python3
"""Original PJoinVertexes initialization/cleanup, GetNodeIndex and active welding.

The active cohort substitutes the six numeric callees PJoinVertexes reaches
(CalcMMMatrix, GetMMMatrix, GetNodeIndex, njCalcPoint, njCalcVector,
njUnitVector) with the same scripted fixture events the native probe uses.
The matrix plumbing (njSetMatrix, njInvertMatrix, njMultiMatrix, njPushMatrix,
njPopMatrixEx) is scripted as well so this oracle judges the weld wiring and
event order, not the math owned by the matrix/coordinate oracles.

The development cohort retains every earlier initialization/cleanup case. The
holdout cohort is generated from its own seed without the curated active cases
and is meant to be run once for the final report.
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
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                               UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW)

SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
JOIN, NODE, SEARCH = 0x43fa90, 0x439f60, 0x439360
ALLOC, FREE, CALCULATE, GET_MM = 0x40b220, 0x40b310, 0x4b81f0, 0x4b82d0
SET, INVERT, PUSH, POP = 0x784d00, 0x787630, 0x7850f0, 0x7816e0
MULTI, POINT, VECTOR, UNIT = 0x788ad0, 0x781670, 0x784d60, 0x418ac0
BASE, HEAP, ARENA = 0x10000000, 0x20000000, 0x1000000
PLAYER, TABLE = BASE + 0x100, BASE + 0x600
OBJECTS = [BASE + 0x400 + index * 64 for index in range(3)] + [0]
MODELS = [BASE + 0x300, BASE + 0x340]
VECTORS = [BASE + 0x2fd0 + index * 0x2000 for index in range(4)]
STACK, STOP = BASE + 0xef00, BASE + 0xff00
PAIRS = BASE + 0x10ff0
ACTION_TABLE, ACTION, ACTW = BASE + 0x700, BASE + 0x800, BASE + 0x900
MATRICES = [0x3b42f38, 0x3b42eb8, 0x3b42ef8, 0x3b42e78]

# Active fixture layout; keep in step with tests/probes/vertex_weld_active_fixture.hpp.
MODE, COUNT, JOIN_MODE, ANIM_MODE, REQUESTED, FRAME = 0, 1, 2, 3, 4, 5
ACTION_ROOT, ROW_OBJECT, ROW_SOURCE, ROW_DEST = 6, 7, 8, 9
SOURCE_DEPTH, DEST_DEPTH, SOURCE_NODE, DEST_NODE, NODE_COUNT = 10, 11, 12, 13, 14
GEOMETRY, BACKUP, ACTIVE_PAIRS, ACTIVE_WORDS = 32, 80, 104, 112
OUT_RESULT, OUT_MODE, OUT_COUNT, OUT_SOURCE_DEPTH, OUT_DEST_DEPTH, OUT_EVENTS = 0, 1, 2, 3, 4, 5
OUT_BACKUP, OUT_GEOMETRY, OUT_ATTACHMENT, OUT_MATRIX, OUT_EVENT = 8, 32, 80, 116, 180
OUT_WORDS, EVENT_WORDS = 1024, 8


def packed(words):
    return struct.pack('<' + 'I' * len(words), *[word & 0xffffffff for word in words])


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def fvalue(word):
    return struct.unpack('<f', struct.pack('<I', word & 0xffffffff))[0]


def initial_matrix():
    return [float(index + 1) for index in range(16)]


def pattern(index):
    matrix = [float((slot + 1) * 10 + index) for slot in range(16)]
    matrix[12], matrix[13], matrix[14] = float(1000 + index), float(2000 + index), float(3000 + index)
    return matrix


def object_pointer(index):
    # Native weld_fixture::object maps 0/3 to null, 1 to the source model and
    # 2 to the destination model. The original table mirrors that identity.
    return {0: OBJECTS[0], 1: OBJECTS[1], 2: OBJECTS[2], 3: 0}[index]


class Source:
    def __init__(self, image):
        assert image.sha256 == SOURCE_SHA
        assert unicorn.__version__ == '2.1.4'
        self.image = image
        self.m = m = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
        for section in image.sections:
            if section['name'] not in ('.text', '.rdata', '.data'):
                continue
            address = image.base + section['rva']
            size = (max(section['raw_size'], section['virtual_size']) + 4095) & ~4095
            try:
                m.mem_map(address, size, unicorn.UC_PROT_ALL)
            except unicorn.UcError:
                continue
            m.mem_write(address, image.slice(section['file_offset'], section['raw_size']))
        for page, size in [(BASE, 4096), (BASE + 0xa000, 0x6000), (BASE + 0x10000, 4096),
                           (HEAP, ARENA)]:
            try:
                m.mem_map(page, size)
            except unicorn.UcError:
                pass
        for vector in VECTORS:
            page = vector & ~4095
            try:
                m.mem_map(page, 4096)
            except unicorn.UcError:
                pass
        m.hook_add(unicorn.UC_HOOK_CODE, self.hook)
        self.active = False

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
        self.free_head = self.next
        self.allocations[pointer] = requested
        self.m.mem_write(pointer, b'\xff' * requested)
        return pointer

    def ret(self):
        sp = self.m.reg_read(UC_X86_REG_ESP)
        self.m.reg_write(UC_X86_REG_ESP, sp + 4)
        self.m.reg_write(UC_X86_REG_EIP, self.get(sp))

    def pop_arg(self):
        self.m.reg_write(UC_X86_REG_ESP, self.m.reg_read(UC_X86_REG_ESP) + 4)

    def matrix(self, at):
        return list(struct.unpack('<16f', self.m.mem_read(at, 64)))

    def write_matrix(self, at, matrix):
        self.m.mem_write(at, struct.pack('<16f', *matrix))

    def vector(self, at):
        return list(struct.unpack('<3f', self.m.mem_read(at, 12)))

    def write_vector(self, at, vector):
        self.m.mem_write(at, struct.pack('<3f', *vector))

    def log(self, tag, *args):
        if len(self.events) >= 64:
            return
        event = [tag & 0xffffffff] + [word & 0xffffffff for word in args]
        event = (event + [0] * EVENT_WORDS)[:EVENT_WORDS]
        self.events.append(event)

    def hook(self, m, at, size, _):
        if at == ALLOC:
            sp = m.reg_read(UC_X86_REG_ESP)
            argument = self.get(sp + 4)
            result = self.allocate(argument)
            self.receipts.append(['allocate', argument, result - HEAP if result else 0])
            m.reg_write(UC_X86_REG_EAX, result)
            self.ret()
            return
        if at == FREE:
            sp = m.reg_read(UC_X86_REG_ESP)
            argument = self.get(sp + 4)
            self.released.add(argument)
            if argument:
                self.free_head = argument - HEAP - 32
            self.receipts.append(['free', argument - HEAP if argument else 0])
            self.ret()
            return
        if not self.active:
            if at == CALCULATE:
                self.failure = 2
                m.emu_stop()
            return
        sp = m.reg_read(UC_X86_REG_ESP)
        ecx = m.reg_read(UC_X86_REG_ECX)
        edx = m.reg_read(UC_X86_REG_EDX)
        if at == CALCULATE:
            frame, nodes, action = self.get(sp + 12), self.get(sp + 16), self.get(sp + 8)
            self.log(0, frame, nodes, self.case[ACTION_ROOT])
            self.instance_count = min(nodes, 128)
            self.instance = [pattern(index) for index in range(self.instance_count)]
            self.ret()
        elif at == GET_MM:
            index, destination = self.get(sp + 4), self.get(sp + 8)
            found = index < self.instance_count
            self.log(1, index, 1 if found else 0)
            if found:
                self.write_matrix(destination, self.instance[index])
            m.reg_write(UC_X86_REG_EAX, 1 if found else 0)
            self.ret()
        elif at == NODE:
            root, target = self.get(sp + 4), self.get(sp + 8)
            source = target == self.active_source
            value = self.case[SOURCE_NODE if source else DEST_NODE]
            self.log(2, self.case[ROW_OBJECT], 1 if source else 2, value & 0xffffffff)
            m.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
            self.ret()
        elif at == SET:
            self.write_matrix(ecx, self.matrix(edx))
            self.log(3)
            self.ret()
        elif at == INVERT:
            matrix = self.matrix(ecx)
            self.write_matrix(ecx, [-value for value in matrix])
            self.log(4)
            self.ret()
        elif at == MULTI:
            source_matrix = self.matrix(edx)
            if ecx:
                matrix = self.matrix(ecx)
                self.write_matrix(ecx, [a + b for a, b in zip(matrix, source_matrix)])
            else:
                self.stack[-1] = [a + b for a, b in zip(self.stack[-1], source_matrix)]
            self.log(5)
            self.ret()
        elif at == PUSH:
            self.stack.append(self.matrix(ecx))
            self.log(6, len(self.stack))
            self.ret()
        elif at == POP:
            if len(self.stack) > 1:
                self.stack.pop()
            self.log(7, len(self.stack))
            self.ret()
        elif at in (POINT, VECTOR):
            input_ptr, output = edx, self.get(sp + 4)
            matrix = self.matrix(ecx) if ecx else self.stack[-1]
            vector = self.vector(input_ptr)
            if at == POINT:
                self.log(8, fbits(matrix[12]), fbits(matrix[13]), fbits(matrix[14]),
                         fbits(vector[0]), fbits(vector[1]), fbits(vector[2]))
            else:
                self.log(9, fbits(matrix[0]), fbits(matrix[1]), fbits(matrix[2]),
                         fbits(vector[0]), fbits(vector[1]), fbits(vector[2]))
            step = float(self.call_index)
            self.call_index += 1
            if at == POINT:
                result = [vector[0] + matrix[12] + step, vector[1] + matrix[13] + step,
                          vector[2] + matrix[14] + step]
            else:
                result = [vector[0] + matrix[0] + step, vector[1] + matrix[1] + step,
                          vector[2] + matrix[2] + step]
            self.write_vector(output, result)
            self.ret()
            self.pop_arg()
        elif at == UNIT:
            pointer = self.get(sp + 4)
            vector = self.vector(pointer)
            self.log(10, fbits(vector[0]), fbits(vector[1]), fbits(vector[2]))
            step = float(self.call_index)
            self.write_vector(pointer, [vector[0] + 7.0 + step, vector[1] + 8.0 + step,
                                        vector[2] + 9.0 + step])
            self.ret()

    # ---------------------------------------------------------------- initialization / cleanup
    def prepare(self, values):
        v = values
        self.active = False
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
            out[80:80 + size // 4] = struct.unpack('<' + 'I' * (size // 4),
                                                   self.m.mem_read(pointer, size))
        if self.previous:
            size = values[8]
            out[128:128 + size // 4] = struct.unpack('<' + 'I' * (size // 4),
                                                     self.m.mem_read(self.previous, size))
        return out

    def node(self, values):
        self.active = False
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
        self.active = False
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
            for offset, value in ((0, pointer(root)), (4, pointer(source)), (8, pointer(destination))):
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

    # -------------------------------------------------------------------------- active branch
    def prepare_active(self, values):
        self.active = True
        self.case = values
        self.failure = 0
        self.next = 0
        self.free_head = 0
        self.allocations, self.released, self.receipts = {}, set(), []
        self.events, self.call_index = [], 0
        self.stack = [initial_matrix()]
        self.instance, self.instance_count = [], 0
        self.m.mem_write(BASE, bytes(4096))
        self.m.mem_write(PAIRS, bytes(8))
        for index, address in enumerate(OBJECTS[:3]):
            self.put(address, 0)
            self.put(address + 4, MODELS[0 if index != 2 else 1])
            self.put(address + 44, 0)
            self.put(address + 48, 0)
        for index, address in enumerate(MODELS):
            self.put(address, VECTORS[index * 2])
            self.put(address + 4, VECTORS[index * 2 + 1])
        for index, address in enumerate(VECTORS):
            self.m.mem_write(address, packed(values[GEOMETRY + index * 12:GEOMETRY + 12 + index * 12]))
        self.previous = self.allocate(96)
        assert self.previous
        self.m.mem_write(self.previous, packed(values[BACKUP:BACKUP + 24]))
        for index, word in enumerate(values[ACTIVE_PAIRS:ACTIVE_PAIRS + 4]):
            self.put(PAIRS + index * 2, word, 2)
        for index, address in enumerate(MATRICES):
            self.m.mem_write(address, bytes(64))
        self.active_source = object_pointer(values[ROW_SOURCE])
        self.active_destination = object_pointer(values[ROW_DEST])
        root = object_pointer(values[ACTION_ROOT])
        self.put(PLAYER + 0x120, values[ANIM_MODE], 2)
        self.put(PLAYER + 0x122, 1, 2)
        self.put(PLAYER + 0x124, values[REQUESTED], 2)
        self.put(PLAYER + 0x130, values[FRAME])
        self.put(PLAYER + 0x140, ACTION_TABLE)
        self.put(PLAYER + 0x144, TABLE)
        self.put(PLAYER + 0x148, ACTW)
        self.m.mem_write(PLAYER + 0x14c, bytes(144))
        request = ACTION_TABLE + values[REQUESTED] * 16
        self.put(request, ACTION)
        self.put(request + 4, values[NODE_COUNT], 1)
        self.put(ACTION, root)
        self.put(ACTW, root)
        self.put(TABLE, object_pointer(values[ROW_OBJECT]))
        self.put(TABLE + 4, self.active_source)
        self.put(TABLE + 8, self.active_destination)
        self.put(TABLE + 12, values[COUNT], 1)
        self.put(TABLE + 13, values[MODE], 1)
        self.put(TABLE + 14, values[SOURCE_DEPTH], 1)
        self.put(TABLE + 15, values[DEST_DEPTH], 1)
        self.put(TABLE + 16, self.previous)
        self.put(TABLE + 20, 0 if values[30] else PAIRS)
        self.put(TABLE + 24, 0)
        self.m.mem_write(STACK, packed([STOP, 0, 0, PLAYER]))
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.reg_write(UC_X86_REG_FPCW, 0x007f)

    def run_active(self, values):
        self.prepare_active(values)
        failure = 0
        try:
            self.m.emu_start(JOIN, STOP, count=200000)
        except unicorn.UcError:
            failure = 1
        out = [0] * OUT_WORDS
        out[OUT_RESULT] = failure
        out[OUT_MODE] = self.get(TABLE + 13, 1)
        out[OUT_COUNT] = self.get(TABLE + 12, 1)
        out[OUT_SOURCE_DEPTH] = self.get(TABLE + 14, 1)
        out[OUT_DEST_DEPTH] = self.get(TABLE + 15, 1)
        out[OUT_EVENTS] = len(self.events)
        for offset in range(0, 24):
            out[OUT_BACKUP + offset] = self.get(self.previous + offset * 4)
        for index, address in enumerate(VECTORS):
            for offset in range(12):
                out[OUT_GEOMETRY + index * 12 + offset] = self.get(address + offset * 4)
        for offset in range(36):
            out[OUT_ATTACHMENT + offset] = self.get(PLAYER + 0x14c + offset * 4)
        for matrix, address in enumerate(MATRICES):
            for offset in range(16):
                out[OUT_MATRIX + matrix * 16 + offset] = self.get(address + offset * 4)
        for index, event in enumerate(self.events):
            out[OUT_EVENT + index * EVENT_WORDS:OUT_EVENT + (index + 1) * EVENT_WORDS] = event
        return out


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
    values[20:26] = rng.choice([[1, 3, 3, 2, 3, 3], [1, 2, 3, 3, 3, 3],
                               [3, 1, 2, 3, 3, 3], [3, 3, 3, 3, 3, 3]])
    values[26:29] = [rng.getrandbits(32) for _ in range(3)]
    values[32:128] = [rng.getrandbits(32) for _ in range(96)]
    if index < 80:
        values[0] = 0 if index < 40 else 2
        values[1] = index % 10
        values[2] = index % 5
        values[3:6] = [1, 2, 0]
        values[8] = 0 if index < 40 else 192
        values[9] = 0
        values[12:20] = [0, 3, 1, 2, 2, 1, 3, 0]
    return values


def active_case(rng, index, curated=True):
    values = [0] * ACTIVE_WORDS
    values[JOIN_MODE] = 1
    values[20:26] = [3, 3, 3, 3, 3, 3]
    values[26:32] = [0] * 6
    values[GEOMETRY:GEOMETRY + 48] = [fbits(float(rng.randrange(0, 21))) for _ in range(48)]
    values[BACKUP:BACKUP + 24] = [fbits(float(rng.randrange(0, 21))) for _ in range(24)]
    values[ACTIVE_PAIRS:ACTIVE_PAIRS + 4] = [rng.randrange(3) for _ in range(4)]
    values[FRAME] = fbits(rng.choice([1.0, 2.5, -3.0, 0.0]))
    values[REQUESTED] = rng.randrange(5)
    values[NODE_COUNT] = rng.choice([0, 1, 2, 3, 4])
    values[ANIM_MODE] = rng.choice([0, 0, 2])
    values[SOURCE_NODE] = rng.choice([0, 1, 2, 0xffffffff])
    values[DEST_NODE] = rng.choice([0, 1, 2, 0xffffffff])
    if curated:
        values[MODE] = index % 10
        values[COUNT] = (index // 10) % 3
        values[SOURCE_DEPTH] = rng.choice([0, 1, 2, 255])
        values[DEST_DEPTH] = rng.choice([0, 1, 255])
        values[ACTION_ROOT] = rng.choice([1, 1, 2, 2, 3])
        values[ROW_OBJECT] = values[ACTION_ROOT] if index % 5 else 2
        values[ROW_SOURCE] = rng.choice([1, 2, 0])
        values[ROW_DEST] = 3 if values[MODE] >= 4 else rng.choice([2, 1, 0])
        if index % 8 == 0:
            # Deterministic root mismatch: the row must not be processed.
            values[ACTION_ROOT], values[ROW_OBJECT] = 1, 2
        if index % 4 == 0:
            values[SOURCE_DEPTH] = 255
        if index % 5 == 0:
            values[DEST_DEPTH] = 255
    else:
        values[MODE] = rng.randrange(10)
        values[COUNT] = rng.randrange(3)
        values[SOURCE_DEPTH] = rng.choice([0, 1, 2, 255])
        values[DEST_DEPTH] = rng.choice([0, 1, 255])
        values[ACTION_ROOT] = rng.randrange(4)
        values[ROW_OBJECT] = values[ACTION_ROOT] if rng.randrange(4) else rng.randrange(4)
        # The source reads source_object->model; a null source is a source
        # memory fault, not a weld result. Keep random inputs on a real node.
        values[ROW_SOURCE] = rng.randrange(3)
        values[ROW_DEST] = 3 if values[MODE] >= 4 else rng.randrange(3)
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('probe')
    parser.add_argument('--cases', type=int, default=1024)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x47574c44)
    parser.add_argument('--active-cases', type=int, default=512)
    parser.add_argument('--active-seed', type=lambda value: int(value, 0), default=0x57454c44)
    parser.add_argument('--holdout-cases', type=int, default=0)
    parser.add_argument('--holdout-seed', type=lambda value: int(value, 0), default=0x484f4c44)
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
    active = library.sadx_native_player_vertex_weld_active
    active.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    rng = random.Random(args.seed)
    mismatches, failures, active_boundary, receipts = [], 0, 0, 0
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
        active_boundary += expected[0] == 2
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

    # Active cohort: curated development cases plus random development cases.
    active_rng = random.Random(args.active_seed)
    active_cases = [active_case(active_rng, index, curated=True) for index in range(args.active_cases)]
    active_input_hash = hashlib.sha256()
    active_original_hash, active_native_hash = hashlib.sha256(), hashlib.sha256()
    active_mismatches = []
    for index, values in enumerate(active_cases):
        argument = (ctypes.c_uint32 * ACTIVE_WORDS)(*values)
        active_input_hash.update(packed(values))
        actual = (ctypes.c_uint32 * OUT_WORDS)()
        assert active(argument, actual) == 1
        expected = source.run_active(values)
        active_original_hash.update(packed(expected))
        active_native_hash.update(packed(actual))
        different = [word for word, pair in enumerate(zip(expected, actual)) if pair[0] != pair[1]]
        if different:
            active_mismatches.append({'case': index, 'words': different[:32],
                                      'source': [expected[word] for word in different[:32]],
                                      'native': [actual[word] for word in different[:32]],
                                      'input': values})

    # Holdout: its own seed, no curated active cases.
    holdout = None
    if args.holdout_cases:
        holdout_rng = random.Random(args.holdout_seed)
        holdout_input_hash = hashlib.sha256()
        holdout_mismatches = []
        for index in range(args.holdout_cases):
            values = active_case(holdout_rng, index, curated=False)
            argument = (ctypes.c_uint32 * ACTIVE_WORDS)(*values)
            holdout_input_hash.update(packed(values))
            actual = (ctypes.c_uint32 * OUT_WORDS)()
            assert active(argument, actual) == 1
            expected = source.run_active(values)
            different = [word for word, pair in enumerate(zip(expected, actual)) if pair[0] != pair[1]]
            if different:
                holdout_mismatches.append({'case': index, 'words': different[:32],
                                           'source': [expected[word] for word in different[:32]],
                                           'native': [actual[word] for word in different[:32]],
                                           'input': values})
        holdout = {'cases': args.holdout_cases, 'seed': args.holdout_seed,
                   'inputs_sha256': holdout_input_hash.hexdigest(),
                   'mismatch_count': len(holdout_mismatches),
                   'mismatches': holdout_mismatches[:20], 'passed': not holdout_mismatches}

    imported = source.imported(args.imported_topology) if args.imported_topology else None
    matched = (not mismatches and not active_mismatches and (not holdout or holdout['passed'])
               and (not imported or imported['passed']))
    report = {
        'slug': 'player_vertex_weld', 'cases': len(cases), 'seed': args.seed,
        'original_development_cohort': {'cases': args.cases, 'stream_sha256': prefix_streams,
                                        'inputs_sha256': prefix_inputs},
        'all_inputs_sha256': input_hash.hexdigest(), 'observer_schema': 3,
        'passed': matched, 'mismatch_count': len(mismatches) + len(active_mismatches),
        'values_match': matched, 'failure_prefix_cases': failures,
        'active_matrix_boundary_cases': active_boundary, 'allocator_receipts': receipts,
        'source_stream_sha256': original_hash.hexdigest(),
        'native_stream_sha256': native_hash.hexdigest(), 'node_stream_sha256': node_hash.hexdigest(),
        'active': {'cases': len(active_cases), 'seed': args.active_seed,
                   'inputs_sha256': active_input_hash.hexdigest(),
                   'source_stream_sha256': active_original_hash.hexdigest(),
                   'native_stream_sha256': active_native_hash.hexdigest(),
                   'mismatch_count': len(active_mismatches),
                   'mismatches': active_mismatches[:20]},
        'holdout': holdout,
        'source_sha256': image.sha256,
        'exe_ranges': [[hex(JOIN), '0x440375'], [hex(NODE), '0x439fb8'],
                       [hex(SEARCH), '0x4393b2']],
        'original_ranges_sha256': {
            f'{start:x}-{end:x}': hashlib.sha256(image.read(start, end - start)).hexdigest()
            for start, end in [(JOIN, 0x440375), (NODE, 0x439fb8), (SEARCH, 0x4393b2)]},
        'probe_sha256': hashlib.sha256(Path(args.probe).read_bytes()).hexdigest(),
        'mismatches': mismatches[:20],
        'imported_gamma': imported,
        'limits': ['Initialization and backup restoration keep the earlier boundary cohort.',
                   'Active math callees are scripted fixture events; their owners have separate oracles.',
                   'Geometry words, backup bytes, row fields, attachments, matrices and event order compare.',
                   'MAlloc/FreeMemory are ordered receipts; actual native arena owns all backups.',
                   'One invocation per case; native unit separately exercises repeated cleanup.',
                   'All 48 scene words, both backup images, depths and owner identities compared.',
                   'Original x86 instructions execute; node flags and child/sibling order are varied.']}
    path = Path(args.output)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ['cases', 'mismatch_count', 'failure_prefix_cases',
                                                 'active_matrix_boundary_cases', 'passed']}
                     | ({'holdout': holdout['mismatch_count']} if holdout else {})))
    return 0 if matched else 1


if __name__ == '__main__':
    raise SystemExit(main())
