"""Independent 77ed00/77ed20 depth-state comparison before their readability rewrite.

The only external execution fixture is COM SetRenderState. Missing reference
facts stop before their source read; unavailable backends stop at the attempted
call with its actual stack arguments. No game callee or expected value is replaced.
This setup checker has an explicit private probe rather than changing the inherited
blend oracle/probe ABI used by both native and legacy blend judges.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct

import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import CODE, DATA, STACK, STOP, Original

DEVICE = DATA + 256
VTABLE = DATA + 4096
METHOD = CODE + 512
SITES = ((0x77ED00, 0x77ED04, 0x77ED09, 0x77ED19),
         (0x77ED20, 0x77ED24, 0x77ED29, 0x77ED2F))


def sha(data):
    return hashlib.sha256(data).hexdigest()


class DepthOriginal:
    def __init__(self, image):
        self.machine = machine = Original(image, 127).machine
        machine.mem_map(0x3D12000, 4096)
        machine.mem_write(DEVICE, struct.pack('<I', VTABLE))
        machine.mem_write(VTABLE + 200, struct.pack('<I', METHOD))
        machine.mem_write(METHOD, b'\xc2\x0c\x00')
        self.clean = machine.context_save()
        machine.hook_add(unicorn.UC_HOOK_CODE, self.instruction)

    def instruction(self, machine, address, size, user):
        if address == self.stop:
            self.boundary = address
            if address == self.call:
                stack = machine.reg_read(UC_X86_REG_ESP)
                self.command = struct.unpack('<3I', machine.mem_read(stack, 12))
            machine.emu_stop()
        elif address == METHOD:
            stack = machine.reg_read(UC_X86_REG_ESP)
            self.command = struct.unpack('<3I', machine.mem_read(stack + 4, 12))
            return_address = struct.unpack('<I', machine.mem_read(stack, 4))[0]
            if return_address - 6 != self.call:
                raise RuntimeError('Unexpected original method call site')
            self.callback_count += 1
            if self.words[4] == 2:
                # The explicit external fixture supplies no completed result.
                # The game CALL was entered; report its caller boundary, not RET.
                self.boundary = self.call
                machine.emu_stop()
                return
            self.receipt = self.words[5]
            machine.reg_write(UC_X86_REG_EAX, self.receipt)

    def run(self, words):
        machine = self.machine
        machine.context_restore(self.clean)
        self.words = words
        entry, load, dereference, self.call = SITES[words[0]]
        self.boundary = self.stop = 0
        self.command = self.receipt = None
        self.callback_count = 0
        if words[2] == 0:
            self.stop = load
        elif words[2] == 2:
            self.stop = dereference
        elif words[0] == 0 and not words[3]:
            self.stop = 0x77ED0E
        elif words[4] == 0:
            self.stop = self.call
        machine.mem_write(0x3D128B0, struct.pack('<I', DEVICE if words[2] == 1 else 0))
        machine.mem_write(STACK, struct.pack('<2I', STOP, words[1]))
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.emu_start(entry, STOP, count=100)
        if not self.boundary and machine.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError('Original did not return or stop at its declared fact boundary')
        output = [0] * 20
        output[0] = int(not self.boundary)
        output[1] = self.boundary
        output[2] = int(self.command is not None)
        output[3] = int(self.receipt is not None)
        output[4] = int(self.boundary == self.call)
        if self.command is not None:
            device, state, value = self.command
            if device != DEVICE:
                raise RuntimeError('Unexpected original COM identity')
            output[5:9] = [1, state, value, self.call]
        if self.receipt is not None:
            output[9:11] = [1, self.receipt]
        output[11] = self.callback_count
        if self.callback_count:
            output[12:16] = output[5:9]
        if output[4]:
            output[16:20] = output[5:9]
        return output


def fixtures(holdout=False):
    if holdout:
        rng = random.Random(0x77ED20)
        return [[kind, rng.getrandbits(32), rng.choice((0, 1, 2)), rng.randrange(2),
                 rng.choice((0, 1, 2)), rng.getrandbits(32)]
                for kind in (0, 1) for _ in range(512)]
    rng = random.Random(0x77ED00)
    values = list(range(256)) + [256, 0x80000000, 0xFFFFFFFF]
    values += [rng.getrandbits(32) for _ in range(32)]
    result = []
    for kind in (0, 1):
        for value in values:
            for hresult in (0, 0x80004005, 0xFFFFFFFF):
                result.append([kind, value, 1, 1, 1, hresult])
        for value in (0, 1, 7, 8, 42, 0x80000000, 0xFFFFFFFF):
            for device in (0, 1, 2):
                for table in (0, 1):
                    for backend in (0, 1, 2):
                        result.append([kind, value, device, table, backend, 0x80004005])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--holdout', action='store_true', help='reserved final cases; not development')
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('Pinned source/Unicorn identity differs')
    table = image.read(0x88AD9C, 32)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    probe = library.sadx_native_ninja_render_depth
    word = ctypes.c_uint32
    probe.argtypes = [ctypes.POINTER(word)] * 3
    probe.restype = ctypes.c_int
    original = DepthOriginal(image)
    cases = fixtures(args.holdout)
    mismatches = []
    rows = []
    for index, words in enumerate(cases):
        expected = original.run(words)
        output = (word * 20)()
        status = probe((word * 8)(*struct.unpack('<8I', table)), (word * 6)(*words), output)
        if status:
            raise RuntimeError(f'Probe rejected case {index}')
        actual = list(output)
        rows.append({'case': index, 'input': words, 'source': expected, 'native': actual})
        if actual != expected:
            mismatches.append({'case': index, 'input': words, 'source': expected, 'native': actual})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    corpus = args.output.with_suffix('.cases.jsonl')
    corpus.write_text(''.join(json.dumps(row, separators=(',', ':')) + '\n' for row in rows))
    root = Path(__file__).resolve().parents[2]
    names = ('src/engine/ninja_render.cpp', 'src/engine/ninja_render.hpp',
             'tests/probes/ninja_render_depth_probe.cpp', 'tools/oracles/check_ninja_render_depth.py')
    report = {'cases': len(cases), 'holdout': args.holdout,
              'values_match': not mismatches, 'passed': not mismatches,
              'mismatch_count': len(mismatches), 'mismatches': mismatches,
              'source_sha256': image.sha256,
              'source_files': {name: sha((root / name).read_bytes()) for name in names},
              'native_library_sha256': sha(args.native_library.read_bytes()),
              'source_ranges': {hex(start): {'bytes': size, 'sha256': sha(image.read(start, size))}
                                for start, size in ((0x77ED00, 32), (0x77ED20, 22))},
              'case_outputs_sha256': sha(corpus.read_bytes()), 'case_outputs': str(corpus),
              'limits': [__doc__, 'No floating-point arithmetic in these two source leaves.',
                         'Incomplete external backend is entered once without completion; its logical boundary is the actual caller CALL site. Missing backend is stopped before entry.',
                         'Callback count/payload and pending payload are compared independently from the implementation ledger.',
                         'Native failure enum names are adapter diagnostics; compared facts are control, source boundary, attempts, receipts and full command values.',
                         'GPU/Windows device behavior and texture-stage methods remain separate qualification.']}
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'cases': len(cases), 'mismatches': len(mismatches)}), flush=True)
    return int(bool(mismatches))


if __name__ == '__main__':
    raise SystemExit(main())
