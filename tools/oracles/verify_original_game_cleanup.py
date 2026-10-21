"""Native Clear/Reset/title preparation and save restoration against pinned x86.

Reset uses the unchanged existing judge, including service-failure prefixes.
Other source callees are executed except explicit external-owner receipts for
heap/tasks, fade/stage services, texture release and list clearing. Invalid
negative MSG_Disp indices are outside the four-slot native view and excluded;
all nonnegative counts, signed character flags and full regular tables remain.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP

AUX = [0x3c49ae4, 0x3c49ae8, 0x3c49aec, 0x3c49b50, 0x3c49b40,
       0x3c49b44, 0x3c49b48, 0x3c49b4c, 0x3c49af0]
TITLE = [0x3b2a2e4, 0x3b2a2e8, 0x3b2a2ec, 0x3b2c470, 0x3b2c478,
         0x90a0a8, 0x3b0ef40, 0x3b28114, 0x3b18244]
SCRATCH = 0x20000000

class Source:
    def __init__(self, image):
        self.m = m = Original(image, 127).machine
        for section in image.sections:
            if section['name'] in ('.text', '.rdata'):
                continue
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            m.mem_map(image.base + section['rva'], size)
            m.mem_write(image.base + section['rva'],
                        image.slice(section['file_offset'], min(section['raw_size'], size)))
        m.mem_map(SCRATCH, 0x10000)
        self.clean = m.context_save()
        for address in (0x40b1a0, 0x40b460, 0x42d770, 0x42c4e0, 0x414570,
                        0x420f40, 0x420ef0, 0x77fc50, 0x504ed0, 0x403290,
                        0x401000, 0x780fb0, SCRATCH + 0x1000):
            m.hook_add(unicorn.UC_HOOK_CODE, self.hook, begin=address, end=address)

    def get(self, address, size=4):
        return int.from_bytes(self.m.mem_read(address, size), 'little')

    def put(self, address, value, size=4):
        self.m.mem_write(address, (int(value) & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))

    def hook(self, m, address, size, unused):
        op = self.op
        if op == 'load':
            return
        out = self.out
        if op == 'load' or (op != 5 and address in (0x40b1a0, 0x40b460, 0x42d770, 0x42c4e0, 0x414570)):
            return
        if op != 6 and address in (0x420f40, 0x420ef0, 0x77fc50):
            return
        if op == 8 and address == 0x401000:
            pointer = self.get(m.reg_read(UC_X86_REG_ESP) + 4)
            text = bytes(m.mem_read(pointer, 32)).split(b'\0')[0]
            if text not in (b'ReleaseSetFile\n', b'ReleaseCamFile\n'):
                raise RuntimeError('Unexpected debug formatter input')
            out[15] = 1 if text == b'ReleaseSetFile\n' else 2
        elif op == 8 and address == 0x780fb0:
            self.trace.append(self.get(m.reg_read(UC_X86_REG_ESP) + 4))
            if self.values[6] and len(self.trace) == self.values[6]:
                out[21] = 1
                m.emu_stop()
                return
        elif op == 8 and address == SCRATCH + 0x1000:
            out[20] += 1
        elif address == 0x504ed0:
            out[2] += 1
            out[3:5] = [self.get(0x3b11190), self.get(0x3b1118c)]
        elif address == 0x403290:
            self.trace.append(self.get(m.reg_read(UC_X86_REG_ESP) + 4))
        elif op == 6:
            self.trace.append({0x420f40: 1, 0x420ef0: 2, 0x77fc50: 3}[address])
        elif op == 5:
            if address == 0x40b1a0:
                self.trace.append(1)
            elif address == 0x40b460:
                self.trace.append(2)
            elif address == 0x42d770:
                self.trace.append(3)
                out[20] = self.get(0x3b29d74)
                out[21] = 0
                for at in TITLE:
                    out[21] |= self.get(at, 1 if at == 0x3b0ef40 else 4)
                self.put(0x3b2b3ac, self.values[14])
            elif address == 0x42c4e0:
                self.trace += [4, self.get(self.get(m.reg_read(UC_X86_REG_ESP) + 4))]
            elif address == 0x414570:
                self.trace += [5, self.get(m.reg_read(UC_X86_REG_ESP) + 4),
                               self.get(m.reg_read(UC_X86_REG_ESP) + 8)]
        esp = m.reg_read(UC_X86_REG_ESP)
        m.reg_write(UC_X86_REG_EIP, self.get(esp))
        m.reg_write(UC_X86_REG_ESP, esp + 4)

    def call(self, address, args=()):
        self.put(STACK, STOP)
        for index, value in enumerate(args):
            self.put(STACK + 4 + 4 * index, value)
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.emu_start(address, STOP, count=200000)
        if self.m.reg_read(UC_X86_REG_EIP) != STOP and not (self.op == 8 and self.out[21]):
            raise RuntimeError(f'Source failed to return from {address:#x}')

    def helpers(self, values):
        self.m.context_restore(self.clean)
        self.values = values
        self.op = op = values[0]
        self.out = out = [0] * 512
        self.trace = []
        if op == 0:
            for at in AUX:
                self.put(at, values[1])
            self.put(0x916b44, values[2])
            self.put(0x90a0a8, values[3])
            self.call(0x457060)
            out[0] = self.m.reg_read(UC_X86_REG_EAX)
            out[1:10] = [self.get(at) for at in AUX]
            out[10] = self.get(0x916b44)
            self.call(0x412ad0)
            out[11] = self.get(0x90a0a8)
        elif op == 1:
            locations = [0x912698, 0x91269c, 0x9126a8, 0x912694, 0x3b29cf8, 0x3b29cf4]
            for at, value in zip(locations, values[1:7]):
                self.put(at, value)
            self.call(0x4257c0)
            self.call(0x4257e0)
            out[:6] = [self.get(at) for at in locations]
        elif op == 2:
            locations = [0x3b22dcc, 0x3b22dec, 0x3b22dd8, 0x3b22dd4]
            for index, at in enumerate(locations):
                self.put(at, values[index + 1], 4 if index == 3 else 2)
            self.call(0x414570, values[5:7])
            out[:4] = [self.get(at, 4 if index == 3 else 2) for index, at in enumerate(locations)]
        elif op == 3:
            self.put(0x3b0e338, values[1])
            self.put(SCRATCH, values[2], 1)
            self.put(0x3abe134, values[3], 1)
            self.put(0x3b1117c, values[4])
            self.m.mem_write(0x3abe13c, bytes(16))
            self.call(0x40d490, [SCRATCH if values[5] else 0])
            out[:5] = [self.get(0x3b0e338)] + [int(self.get(0x3abe13c + i * 4) == SCRATCH) for i in range(4)]
        elif op == 4:
            self.put(0x3b11190, values[1])
            self.put(0x3b1118c, values[2])
            self.put(0x3c5e8b8, values[3])
            self.call(0x421fb0)
            out[:2] = [self.get(0x3b11190), self.get(0x3b1118c)]
        elif op == 5:
            initial = values[1:6] + [values[8], values[10], values[11], values[9]]
            for at, value in zip(TITLE, initial):
                self.put(at, value, 1 if at == 0x3b0ef40 else 4)
            for at in AUX:
                self.put(at, values[6])
            self.put(0x916b44, values[7])
            self.m.mem_write(0x3b2b3a8, bytes(1392))
            self.m.mem_write(0x3b29d70, bytes(1392))
            self.put(0x3b2b3ac, values[12])
            self.put(0x3b29d74, values[13])
            self.call(0x42ca10)
            out[:9] = [self.get(at, 1 if at == 0x3b0ef40 else 4) for at in TITLE]
            out[9:12] = [self.get(0x3b2b3ac), self.get(0x3b29d74), self.get(0x916b44)]
            out[24:24 + len(self.trace)] = self.trace
            out[32:41] = [self.get(at) for at in AUX]
        elif op == 6:
            self.call(0x40bf30)
            out[:len(self.trace)] = self.trace
        elif op == 7:
            self.put(0x3b290b0, 1 if values[1] else 0)
            for i in range(32):
                self.put(0x3b29030 + i * 4, i + 2 if values[2] & (1 << i) else 0)
            for i in range(9):
                descriptor = SCRATCH + i * 32
                self.put(0x90ed54 + i * 4, descriptor if values[3] & (1 << i) else 0)
                for j, token in enumerate((i + 2, i + 11, 0)):
                    self.put(descriptor + j * 8 + 4, token)
                self.put(0x3b290c4 + i, values[4 + i], 1)
            self.call(0x420f40)
            self.call(0x420ef0)
            out[:33] = [self.get(0x3b290b0)] + [self.get(0x3b29030 + i * 4) for i in range(32)]
            out[33:42] = [self.get(0x3b290c4 + i, 1) for i in range(9)]
            out[63] = len(self.trace)
            out[64:64 + len(self.trace)] = self.trace
        elif op == 8:
            self.put(0x3c4e458, int(bool(values[5])))
            self.put(0x3c4e45c, int(bool(values[5])))
            callback = 0x592370 if values[2] == 2 else (SCRATCH + 0x1000 if values[2] else 0)
            self.put(0x3b291b4, callback)
            for i in range(6):
                self.put(0x3abdf40 + i * 4, i + 1 if values[3] & (1 << i) else 0)
                self.put(0x3abdf28 + i * 4, i + 7 if values[4] & (1 << i) else 0)
            self.call(0x4228f0, [values[1]])
            out[0:3] = [int(self.get(0x3c4e458) != 0), int(self.get(0x3c4e45c) != 0),
                        2 if self.get(0x3b291b4) == 0x592370 else int(self.get(0x3b291b4) != 0)]
            out[3:9] = [self.get(0x3abdf40 + i * 4) for i in range(6)]
            out[9:15] = [self.get(0x3abdf28 + i * 4) for i in range(6)]
            out[16] = len(self.trace)
            out[32:32 + len(self.trace)] = self.trace
        elif op == 9:
            character = values[1] % 9
            self.put(0x90ed54 + character * 4, SCRATCH if values[2] else 0)
            self.put(0x3b290c4 + character, values[3], 1)
            for i in range(2):
                texlist = SCRATCH + 0x200 + i * 32
                texname = SCRATCH + 0x400 + i * 32
                self.put(SCRATCH + i * 8 + 4, texlist)
                self.put(texlist, texname)
                self.put(texname + 8, (values[4] >> i) & 1)
            self.put(SCRATCH + 20, 0)
            self.call(0x420e90, [character])
            out[0] = self.get(0x3b290c4 + character, 1)
            out[1] = len(self.trace)
            out[2:2 + len(self.trace)] = [(token - SCRATCH - 0x200) // 32 + 1 for token in self.trace]
        return out

    def load(self, encoded, values):
        self.op = 'load'
        m = self.m
        m.context_restore(self.clean)
        m.mem_write(0x3b2b3a8, encoded)
        arrays = [(0x3b18808, 512), (0x3b18608, 512), (0x3b183a8, 96),
                  (0x3b18250, 344), (0x3b2b368, 60), (0x3b18a0c, 932)]
        for at, size in arrays:
            m.mem_write(at, bytes([165]) * size)
        self.put(0x3b18db5, values[2], 1)
        self.put(0x3b22dc0, values[3], 2)
        self.call(0x42d770)
        out = list(struct.unpack('<348I', encoded))
        out += [self.get(at, size) for at, size in [(0x3b0f13c, 4), (0x3b22dc0, 2),
                 (0x3b0ef34, 1), (0x3b2c3f8, 4), (0x913b10, 1), (0x3b2c3fc, 4)]]
        for at, size in arrays:
            out += struct.unpack('<' + str(size // 4) + 'I', m.mem_read(at, size))
        return out + [0] * (2048 - len(out))


def run(image, library, seed=20261020, holdout=False):
    source = Source(image)
    rng = random.Random(seed)
    bad = []
    counts = {'load_save': 0, 'helpers': 0}
    fn = library.sadx_native_cleanup_load_save
    for character in range(256):
        for metal in (0, 1, 128, 255):
            values = [rng.getrandbits(32), character, metal, rng.randrange(65536)]
            out = (ctypes.c_uint32 * 2048)()
            fn((ctypes.c_uint32 * 4)(*values), out)
            expected = source.load(bytes(out)[:1392], values)
            counts['load_save'] += 1
            if list(out) != expected:
                bad.append({'kind': 'load_save', 'input': values,
                            'fields': [i for i, (a, b) in enumerate(zip(out, expected)) if a != b][:12]})
    fn = library.sadx_native_cleanup_helpers
    cases = []
    for op in range(10):
        for index in range(128):
            values = [op] + [rng.getrandbits(32) for _ in range(15)]
            if op == 3:
                values[1] = index % 9
                values[2] = index % 4
                values[3] = index % 3
                values[4] = index % 5 - 2
                values[5] = index % 7
            elif op == 7:
                values[2] = (1 << (index % 33)) - 1 if index % 2 else values[2]
                values[3] &= 511
                values[4:] = [rng.choice([0, 1, 127, 128, 255]) for _ in values[4:]]
            elif op == 8:
                values[1] = index % 10
                values[2] = 2 if values[1] == 5 else index % 2
                values[3] &= 63
                values[4] &= 63
                values[6] = index % 8
            elif op == 9:
                values[2] = index % 3
                values[3] = index * 2 if index < 128 else index
                if index < 8:
                    values[3] = 1
                values[4] = index % 4
            cases.append(values)
    for values in cases:
        out = (ctypes.c_uint32 * 512)()
        fn((ctypes.c_uint32 * 16)(*(v & 0xffffffff for v in values)), out)
        expected = source.helpers(values)
        counts['helpers'] += 1
        if list(out) != expected:
            bad.append({'kind': 'helpers', 'input': values,
                        'fields': [[i, a, b] for i, (a, b) in enumerate(zip(out, expected)) if a != b][:12]})
    return counts, bad


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('Pinned source/Unicorn mismatch')
    library = ctypes.CDLL(str(args.native_library.resolve()))
    counts, bad = run(image, library)
    root = Path(__file__).resolve().parents[2]
    reset_judge = root / 'tools/oracles/verify_original_game_reset.py'
    with tempfile.TemporaryDirectory() as folder:
        reset_report = Path(folder) / 'reset.json'
        environment = dict(os.environ)
        environment['PYTHONPATH'] = str(root / 'tools/lib') + os.pathsep + str(root / 'tools/oracles')
        result = subprocess.run([sys.executable, str(reset_judge), str(args.exe),
                                 str(args.native_library), '--output', str(reset_report)], check=False, env=environment)
        reset = json.loads(reset_report.read_text())
    sources = ['src/game/game_cleanup.cpp', 'src/game/game_cleanup.hpp',
               'tests/probes/native_game_cleanup_probe.cpp', 'src/game/save_data.cpp',
               'src/game/save_data.hpp', 'src/game/game_bootstrap.hpp',
               'src/game/game_bootstrap.cpp', 'src/game/sound_initial_state.hpp',
               'src/engine/task.hpp', 'src/engine/sprite_immediate.hpp']
    sha = lambda data: hashlib.sha256(data).hexdigest()
    report = dict(source_sha256=image.sha256,
                  native_library_sha256=sha(args.native_library.read_bytes()),
                  native_sources={name: sha((root / name).read_bytes()) for name in sources},
                  source_ranges={f'{hex(start)}-{hex(end)}': sha(image.read(start, end - start))
                      for start, end in [(0x40bf30, 0x40bf40), (0x40bf40, 0x40c023),
                        (0x40cd10, 0x40cd1b), (0x40d490, 0x40d4d0),
                        (0x412ad0, 0x412adb), (0x4144d0, 0x4144dc), (0x4144e0, 0x4144e8),
                        (0x414570, 0x4145a4), (0x420e90, 0x420ef0), (0x420ef0, 0x420f35),
                        (0x420f40, 0x420f8d), (0x421fb0, 0x421fcc),
                        (0x422440, 0x4224b0), (0x4224b0, 0x422520), (0x4228f0, 0x42292b),
                        (0x424460, 0x42449c), (0x4257c0, 0x4257dd), (0x4257e0, 0x4257f5),
                        (0x42ca10, 0x42ca99), (0x42d250, 0x42d29c), (0x42d2a0, 0x42d2ec),
                        (0x42d2f0, 0x42d351), (0x42d360, 0x42d3aa),
                        (0x42d770, 0x42d873), (0x457060, 0x4570a5)]},
                  source_initial_values={
                      'sequence_running': image.unpack('<i', 0x90a0a8)[0],
                      'tracks': [image.unpack('<i', at)[0] for at in (0x912694, 0x912698, 0x91269c, 0x9126a8)],
                      'auxiliary_limit': image.unpack('<I', 0x916b44)[0]},
                  unchanged_reset_judge_sha256=sha(reset_judge.read_bytes()),
                  unchanged_reset_judge=reset, cases=sum(counts.values()) + reset['cases'],
                  case_groups=counts, mismatch_count=len(bad) + reset['mismatch_count'],
                  mismatches=bad[:12], values_and_call_order_match=not bad and result.returncode == 0,
                  limits=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'counts': counts, 'mismatches': len(bad), 'reset_exit': result.returncode}))
    return int(bool(bad) or result.returncode != 0)

if __name__ == '__main__':
    raise SystemExit(main())
