"""Execute actual game Start initialization and compare native scalar/table effects.

Character texture release and pinned Input vibration-stop are explicit service
fixtures. Actual Sonic action pointer identities come from a supplied table; the
source copies entries100/101 without dereferencing their object/motion payloads.
CRT srand is a fixture receipt for the separately recovered CRT store.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct

import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP, DATA


class Source:
    def __init__(self, image):
        self.m = Original(image, 127).machine
        for section in image.sections:
            if section['name'] in ['.text', '.rdata']:
                continue
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            address = image.base + section['rva']
            self.m.mem_map(address, size)
            self.m.mem_write(address, image.slice(section['file_offset'], min(size, section['raw_size'])))
        self.m.hook_add(unicorn.UC_HOOK_CODE, self.hook)

    def read(self, address, size=4):
        return int.from_bytes(self.m.mem_read(address, size), 'little')

    def put(self, address, value, size=4):
        self.m.mem_write(address, int(value).to_bytes(size, 'little'))

    def hook(self, m, address, size, user):
        if address not in [0x420e90, 0x785330, 0x6443b5]:
            return
        stack = m.reg_read(UC_X86_REG_ESP)
        if address == 0x6443b5:
            self.seed = self.read(stack + 4)
        else:
            self.calls.append([address, self.read(stack + 4)])
        m.reg_write(UC_X86_REG_EIP, self.read(stack))
        m.reg_write(UC_X86_REG_ESP, stack + 4)

    def run(self, values):
        self.seed, self.calls = 0, []
        for address, size in [(0x3b183a8, 96), (0x3b18808, 512), (0x3b18408, 512),
                              (0x3b18608, 512), (0x3b18250, 344), (0x3b18a0c, 932),
                              (0x3c4b040, 13312)]:
            self.m.mem_write(address, bytes([165]) * size)
        self.put(0x3b22dc0, 0, 2)
        self.put(0x3b2c578, 1)
        self.put(0x3b22dcc, values[0] & 65535, 2)
        self.put(0x3b2b5f9, values[1], 1)
        self.m.mem_write(0x3b2b5fa, bytes([values[2]]) * 7)
        self.put(0x38f6e7c, DATA + 4096)
        self.put(DATA + 4096 + 400, values[3])
        self.put(DATA + 4096 + 404, values[4])
        self.put(STACK, STOP)
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.emu_start(0x415000, STOP, timeout=1000000)
        if self.calls != [[0x420e90, 0], [0x785330, 0]]:
            raise RuntimeError(f'Unexpected source service calls: {self.calls}')
        scalar_fields = [
            (0x3b290d8, 1), (0x3b290e0, 1), (0x3b290e4, 4), (0x3b290e8, 1),
            (0x3b290dc, 4), (0x3b22dc0, 2), (0x3b0ef34, 1),
            (0x3c566c0, 4), (0x3c566d0, 4), (0x3b0f13c, 4), (0x3b0f108, 4),
            (0x3b0ef35, 1), (0x3b0f128, 1), (0x3b0ef48, 1), (0x3b0f0e4, 2),
            (0x3b0ef44, 4), (0x3abe134, 1), (0x3b2c560, 4), (0x3b2c578, 4),
            (0x3b2c565, 1), (0x3b2c574, 1), (0x914788, 4), (0x3b18244, 4),
            (0x3b22de4, 2), (0x3b22e1c, 2), (0x3b2c45c, 4), (0x3b18db8, 2),
            (0x3b22e20, 4), (0x3b0f0f8, 4), (0x3b0f0fc, 4), (0x3b0f100, 4),
            (0x3b0f10c, 4), (0x3b0f0f4, 4), (0x3b0f0e0, 1), (0x3d08420, 4)]
        out = [self.read(address, size) for address, size in scalar_fields]
        for index in [4, 18, 27]:
            out[index] = int(out[index] == 0)
        out[34] = int(out[34] == 0x40a340)
        out += [self.seed, int(self.read(0x3b0ef4c) == 0), int(self.read(0x3b18db0) == 0)]
        out += [self.read(a, n) for a, n in [
            (0x3c4b00c, 4), (0x3c4b010, 4), (0x3c4b014, 2), (0x3c4b008, 1),
            (0x3c4b034, 1), (0x3c4ae80, 1), (0x3c4b02c, 1), (0x3c4b030, 4), (0x3c4b024, 4)]]
        for index in range(10):
            out += [self.read(0x3c4ae08 + index * 12, 1), self.read(0x3c4ae0c + index * 12)]
        for address, size in [(0x3b183a8, 96), (0x3b18808, 512), (0x3b18408, 512),
                              (0x3b18608, 512), (0x3b18250, 344), (0x3b18a0c, 932),
                              (0x3c4b040, 13312)]:
            out += struct.unpack('<' + str(size // 4) + 'I', self.m.mem_read(address, size))
        return out + [0] * (8192 - len(out))

    def game_result(self, values):
        self.put(0x3b22dcc, values[0] & 65535, 2)
        self.put(0x3b2b5f9, values[1], 1)
        self.put(0x3b0ef34, values[2], 1)
        self.put(STACK, STOP)
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.emu_start(0x413c00, STOP, timeout=1000000)
        return self.m.reg_read(UC_X86_REG_EAX) & 255

    def menu_defaults(self):
        self.m.mem_write(0x3b2a2f0, bytes([165]) * 18)
        self.put(0x3b2b358, 1)
        self.put(STACK, STOP)
        self.m.reg_write(UC_X86_REG_ESP, STACK)
        self.m.emu_start(0x42cea0, STOP, timeout=1000000)
        out = list(self.m.mem_read(0x3b2a2f0, 16))
        out += [self.read(0x3b2a300, 2), int(self.read(0x3b2b358) == 0)]
        for entry in [0x421f70, 0x421f80]:
            self.put(STACK, STOP)
            self.m.reg_write(UC_X86_REG_ESP, STACK)
            self.m.emu_start(entry, STOP, timeout=1000000)
            out.append(self.m.reg_read(UC_X86_REG_EAX))
        return out


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe', type=pathlib.Path)
    p.add_argument('native_library', type=pathlib.Path)
    p.add_argument('--output', type=pathlib.Path, required=True)
    p.add_argument('--seed', type=int, default=20261020)
    p.add_argument('--cases', type=int, default=256)
    a = p.parse_args()
    if hashlib.sha256(a.exe.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise RuntimeError('Unexpected source executable')
    source = Source(PEImage(a.exe))
    lib = ctypes.CDLL(str(a.native_library.resolve()))
    rng = random.Random(a.seed)
    cases = [[stage, flags, lives, 0, 123] for stage in [-32768, 0, 25, 26, 34, 35, 38, 39, 32767]
             for flags in [0, 2, 255] for lives in [0, 3, 4, 127, 128, 255]]
    cases += [[rng.randrange(65536), rng.randrange(256), rng.randrange(256),
               rng.getrandbits(32), rng.getrandbits(32)] for _ in range(a.cases)]
    bad = []
    for index, values in enumerate(cases):
        output = (ctypes.c_uint32 * 8192)()
        lib.sadx_native_game_bootstrap((ctypes.c_uint32 * 5)(*values), output)
        expected = source.run(values)
        differences = [[offset, native, original] for offset, (native, original)
                       in enumerate(zip(output, expected)) if native != original]
        if differences:
            bad.append({'case': index, 'input': values, 'differences': differences[:12]})
        native_result = lib.sadx_native_game_bootstrap_result((ctypes.c_uint32 * 5)(*values))
        original_result = source.game_result(values)
        if native_result != original_result:
            bad.append({'game_result': index, 'input': values, 'native': native_result,
                        'original': original_result})
    root = pathlib.Path(__file__).resolve().parents[2]
    menu = (ctypes.c_uint32 * 20)()
    lib.sadx_native_game_bootstrap_menu(menu)
    original_menu = source.menu_defaults()
    if list(menu) != original_menu:
        bad.append({'menu_defaults': True, 'native': list(menu), 'original': original_menu})
    files = ['src/engine/character_resources.hpp', 'src/game/game_bootstrap.hpp',
             'src/game/game_bootstrap.cpp', 'tests/probes/native_game_bootstrap_probe.cpp']
    ranges = [(0x415000, 0x415041), (0x414470, 0x4144c8), (0x46b230, 0x46b2c0),
              (0x413300, 0x413379), (0x413c00, 0x413c90), (0x494820, 0x49483e),
              (0x421f90, 0x421fae), (0x427f10, 0x427f4a), (0x427ee0, 0x427eee),
              (0x42f5d0, 0x42f606), (0x43b490, 0x43b4bb), (0x42cea0, 0x42cec2),
              (0x421f70, 0x421f76), (0x421f80, 0x421f86)]
    report = {'cases': len(cases), 'seed': a.seed, 'mismatches': len(bad), 'first_mismatches': bad[:3],
              'passed': not bad, 'native_library_sha256': hashlib.sha256(a.native_library.read_bytes()).hexdigest(),
              'native_sources': {f: hashlib.sha256((root / f).read_bytes()).hexdigest() for f in files},
              'source_ranges': {f'{hex(start)}-{hex(end)}': hashlib.sha256(PEImage(a.exe).read(start, end - start)).hexdigest() for start, end in ranges},
              'initialization_result_checks': len(cases), 'menu_state_checked': True,
              'shared_owners': {
                  'card_result': {'source': '0x3b290e4', 'borrowed_from': 'Session.card_result_ (distinct from GameFlowState.save_phase)'},
                  'global_timer': {'source': '0x3b0f13c', 'borrowed_from': 'session frame counter'},
                  'rings': {'source': '0x3b0f0e4', 'borrowed_from': 'LogoScene/save rings'},
                  'movie_number': {'source': '0x3b2c45c', 'borrowed_from': 'GameFlowState.movie_number'},
                  'game_mode': {'source': '0x3b22de4', 'borrowed_from': 'loop game mode'},
                  'menu_mode': {'source': '0x3b2a2fa', 'borrowed_from': 'GameFlowState.tutorial_variant'}},
              'limits': __doc__}
    a.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ['cases', 'mismatches']}))
    return int(bool(bad))


if __name__ == '__main__':
    raise SystemExit(main())
