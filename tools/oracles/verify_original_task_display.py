"""Task display traversal against original 40b130/40b4f0/40b540 instructions.

ResetMaterial is an explicit owner receipt. Task graphs and callback mutations
are fixture inputs. Creation/destruction actions execute original task helpers;
old task lifecycle judges remain unchanged. Compare callback order, active graph,
free-slot count/head, and unchanged execution-context globals. A saved pointer to
an unreused freed slot stops at the original display-load boundary and is tagged
unsupported rather than treated as a successful walk through the free list.
No image/material output or unknown game callback implementation is claimed.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_ESI
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP

POOL = 0x22000000
CODE = 0x23000000
RESOURCE = 0x24000000
CONTINUE = CODE + 0x100
CALLBACKS = {1: CODE + 0x10, 2: CODE + 0x20, 3: CODE + 0x30, 4: 0x51f680}
CALLBACK_IDS = {value: key for key, value in CALLBACKS.items()}

class Source:
    def __init__(self, image):
        self.m = m = Original(image, 127).machine
        for section in image.sections:
            if section['name'] in ('.text', '.rdata'):
                continue
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            m.mem_map(image.base + section['rva'], size)
            m.mem_write(image.base + section['rva'], image.slice(section['file_offset'], section['raw_size']))
        m.mem_map(POOL, 0x40000)
        m.mem_map(CODE, 0x1000)
        for address in CALLBACKS.values():
            if address >= CODE:
                m.mem_write(address, b'\xc3')
        self.clean = m.context_save()
        for address in (CODE + 0x10, CODE + 0x20, CODE + 0x30, CONTINUE, 0x4128f0,
                        0x40b510, 0x40b140, 0x401000):
            m.hook_add(unicorn.UC_HOOK_CODE, self.hook, begin=address, end=address)

    def get(self, address):
        return int.from_bytes(self.m.mem_read(address, 4), 'little')

    def put(self, address, value):
        self.m.mem_write(address, struct.pack('<I', value & 0xffffffff))

    @staticmethod
    def slot(address):
        return (address - POOL) // 52 + 1 if address else 0

    @staticmethod
    def resource(value):
        return (value - RESOURCE) // 4 if value else 0

    def active(self):
        found = set()
        def visit(address):
            while address:
                if address in found:
                    raise RuntimeError('Fixture graph contains a cycle')
                found.add(address)
                visit(self.get(address + 12))
                address = self.get(address)
        for index in range(8):
            visit(self.get(0x3abdbc4 + index * 4))
        return found

    def event(self, task, code):
        self.trace.append([self.slot(task), code, CALLBACK_IDS.get(self.get(0x3abdbf4), 0),
                           self.resource(self.get(0x3abdbf8))])

    def return_from_receipt(self):
        esp = self.m.reg_read(UC_X86_REG_ESP)
        self.m.reg_write(UC_X86_REG_EIP, self.get(esp))
        self.m.reg_write(UC_X86_REG_ESP, esp + 4)

    def start_helper(self, address, arguments, after):
        esp = self.m.reg_read(UC_X86_REG_ESP)
        self.helper_esp = esp
        self.helper_after = after
        frame = esp - 4 * (len(arguments) + 1)
        self.put(frame, CONTINUE)
        for index, argument in enumerate(arguments):
            self.put(frame + 4 + index * 4, argument)
        self.m.reg_write(UC_X86_REG_ESP, frame)
        self.m.reg_write(UC_X86_REG_EIP, address)

    def hook(self, m, address, size, unused):
        if address == 0x401000:
            raise RuntimeError('Unexpected inactive-task diagnostic')
        if address in (0x40b510, 0x40b140):
            task = m.reg_read(UC_X86_REG_ESI)
            if task not in self.active():
                self.failure = 2
                self.boundary = address
                m.emu_stop()
            return
        if address == CONTINUE:
            callback = self.helper_after
            m.reg_write(UC_X86_REG_ESP, self.helper_esp)
            callback(m.reg_read(UC_X86_REG_EAX))
            return
        if address == 0x4128f0:
            self.event(0, 0)
            if self.header[7]:
                self.failure = 1
                m.emu_stop()
            else:
                self.return_from_receipt()
            return
        esp = m.reg_read(UC_X86_REG_ESP)
        task = self.get(esp + 4)
        self.event(task, CALLBACK_IDS[address])
        self.callbacks += 1
        if self.header[3] and self.callbacks == self.header[3]:
            self.failure = 1
            m.emu_stop()
            return
        index = self.slot(task) - 1
        if index in self.consumed:
            self.return_from_receipt()
            return
        self.consumed.add(index)
        row = self.rows[index] if index < len(self.rows) else [0] * 6
        action, target_id = row[3:5]
        target = POOL + (target_id - 1) * 52 if target_id else 0
        live_target = target if target in self.active() else 0
        if action == 1 and live_target:
            self.put(target + 20, 0)
        elif action == 2 and live_target:
            self.put(target + 20, CALLBACKS[2])
        elif action in (3, 4):
            arguments = [0, CALLBACKS[3], task] if action == 3 else [0, 8 if target_id % 8 == 7 else target_id % 8, CALLBACKS[3]]
            def created(new_task):
                self.put(new_task + 20, CALLBACKS[2])
                self.return_from_receipt()
            self.start_helper(0x40b940 if action == 3 else 0x40b860, arguments, created)
            return
        elif action in (5, 6):
            self.start_helper(0x40b7e0 if action == 5 else 0x40b570, [task],
                              lambda result: self.return_from_receipt())
            return
        elif action == 7:
            def destroyed(result):
                if target not in self.active():
                    self.return_from_receipt()
                    return
                def recreated(replacement):
                    self.put(replacement + 20, CALLBACKS[2])
                    def child_added(child):
                        self.put(child + 20, CALLBACKS[2])
                        self.return_from_receipt()
                    self.start_helper(0x40b940, [0, CALLBACKS[3], replacement], child_added)
                self.start_helper(0x40b940, [0, CALLBACKS[3], target], recreated)
            self.start_helper(0x40b570, [task], destroyed)
            return
        elif action == 10 and live_target:
            self.start_helper(0x40b570, [target], lambda result: self.return_from_receipt())
            return
        elif action == 8:
            self.put(task + 20, CALLBACKS[2])
        elif action == 9:
            self.failure = 1
            m.emu_stop()
            return
        self.return_from_receipt()

    def run(self, values):
        self.header = values[:8]
        self.rows = [values[index:index + 6] for index in range(8, len(values), 6)]
        self.trace = []
        self.callbacks = 0
        self.failure = 0
        self.boundary = 0
        self.consumed = set()
        m = self.m
        m.context_restore(self.clean)
        memory = bytearray(4096 * 52)
        for index in range(len(self.rows), 4095):
            struct.pack_into('<I', memory, index * 52, POOL + (index + 1) * 52)
        m.mem_write(POOL, bytes(memory))
        m.mem_write(0x3abdbc4, bytes(32))
        self.put(0x3abdbec, POOL + len(self.rows) * 52)
        self.put(0x38f6e08, RESOURCE + 8000 * 4)
        for index, row in enumerate(self.rows):
            task = POOL + index * 52
            level, parent, display, action, target, owner = row
            head_word = POOL + (parent - 1) * 52 + 12 if parent else 0x3abdbc4 + (7 if level == 8 else level) * 4
            head = self.get(head_word)
            self.put(task, head)
            if head:
                self.put(head + 4, task)
            self.put(head_word, task)
            self.put(task + 8, POOL + (parent - 1) * 52 if parent else 0)
            self.put(task + 16, CALLBACKS[3])
            self.put(task + 20, CALLBACKS.get(display, 0))
            self.put(task + 48, RESOURCE + owner * 4 if owner else 0)
        self.put(0x3abdbf4, CALLBACKS[3] if values[4] else 0)
        self.put(0x3abdbf8, RESOURCE + 9000 * 4 if values[5] else 0)
        self.put(STACK, STOP)
        argument = values[1] if values[0] == 1 else POOL + values[2] * 52
        self.put(STACK + 4, argument)
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.emu_start([0x40b540, 0x40b4f0, 0x40b130][values[0]], STOP, count=300000)
        if not self.failure and m.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError('Original traversal did not return')
        out = [0] * 2048
        live = self.active()
        free_head = self.get(0x3abdbec)
        out[:6] = [self.failure, CALLBACK_IDS.get(self.get(0x3abdbf4), 0),
                   self.resource(self.get(0x3abdbf8)), len(self.trace), 4096 - len(live), self.slot(free_head)]
        out[8:16] = [self.slot(self.get(0x3abdbc4 + index * 4)) for index in range(8)]
        for index in range(64):
            task = POOL + index * 52
            if task not in live:
                continue
            out[16 + index * 9:25 + index * 9] = [1] + [self.slot(self.get(task + offset)) for offset in (0, 4, 8, 12)] + [CALLBACK_IDS.get(self.get(task + offset), 0) for offset in (16, 20, 24)] + [self.resource(self.get(task + 48))]
        out[640:640 + len(self.trace) * 4] = [word for entry in self.trace for word in entry]
        return out


def fixtures(seed=0x40b540, repetitions=192):
    result = []
    rng = random.Random(seed)
    for index in range(repetitions):
        count = 1 + index % 16
        header = [index % 3, rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 8]), rng.randrange(count),
                  rng.choice([0, 0, 0, 1, 3, 9]), index % 2, index % 3 == 0,
                  count, index % 23 == 0]
        rows = []
        for node in range(count):
            parent = rng.randrange(node) + 1 if node and rng.randrange(3) else 0
            level = rng.choice([0, 1, 2, 3, 4, 5, 6, 8])
            callback = rng.randrange(3)
            action = rng.randrange(11)
            rows += [level, parent, callback, action, rng.randrange(count) + 1, node + 1]
        result.append(header + rows)
    # Focused callbacks add a new child, remove children, replace a sibling's
    # display and recycle the current source pool slot with new descendants.
    for action in range(11):
        for context in range(4):
            rows = [[0, 0, 1, 0, 0, 1], [8, 0, 1, action, 1, 2],
                    [0, 2, 0, 0, 0, 3], [0, 3, 2, 0, 0, 4], [0, 0, 2, 0, 0, 5]]
            result.append([0, 0, 0, 0, context & 1, context & 2, len(rows), 0] +
                          [word for row in rows for word in row])
    return result


def compare(image, library, seed=0x40b540, repetitions=192):
    source = Source(image)
    fn = library.sadx_native_task_display
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
                   ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    fn.restype = ctypes.c_int
    bad = []
    cases = fixtures(seed, repetitions)
    outcomes = {}
    for index, values in enumerate(cases):
        output = (ctypes.c_uint32 * 2048)()
        if fn((ctypes.c_uint32 * len(values))(*values), len(values), output, 2048) != 1:
            raise RuntimeError('Native fixture rejected')
        expected = source.run(values)
        key = str(source.failure)
        outcomes[key] = outcomes.get(key, 0) + 1
        if list(output) != expected:
            bad.append({'case': index, 'input': values,
                        'differences': [[i, a, b] for i, (a, b) in enumerate(zip(output, expected)) if a != b][:20]})
    compare.last_outcomes = outcomes
    return len(cases), bad


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('Pinned source/Unicorn mismatch')
    count, bad = compare(image, ctypes.CDLL(str(args.native_library.resolve())))
    sha = lambda data: hashlib.sha256(data).hexdigest()
    root = Path(__file__).resolve().parents[2]
    names = ['src/engine/task.hpp', 'src/engine/task.cpp', 'src/engine/task_work.hpp',
             'src/engine/task_work.cpp', 'tests/probes/native_task_display_probe.cpp']
    report = dict(source_sha256=image.sha256, native_library_sha256=sha(args.native_library.read_bytes()),
                  cases=count, mismatch_count=len(bad), mismatches=bad[:12],
                  callback_order_graph_and_context_match=not bad,
                  source_outcomes=compare.last_outcomes,
                  native_sources={name: sha((root / name).read_bytes()) for name in names},
                  source_ranges={f'{hex(a)}-{hex(b)}': sha(image.read(a, b - a)) for a, b in
                                 [(0x40b130, 0x40b168), (0x40b4f0, 0x40b538), (0x40b540, 0x40b563)]},
                  limits=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': count, 'mismatches': len(bad)}))
    return int(bool(bad))

if __name__ == '__main__':
    raise SystemExit(main())
