"""Original E102 first-call order and input_buf state at the restored-input call.

Uses the unchanged Gamma caller judge's concrete lower-call receipts. The new
callee executes its original instructions; stops before GetMMMatrix. This is
bounded caller evidence, not independent verification of the intercepted callees.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/oracles'), str(ROOT / 'tools/lib')]
from verify_original_gamma_selection_core import Source, fixtures, EXTRAS
from pe_image import PEImage
from unicorn.x86_const import UC_X86_REG_ESP

INPUT = 0x3b0e7a0


class Caller(Source):
    def hook(self, m, address, size, user):
        if address == 0x483430:
            self.initial_input = [0x10203040 + i * 19 for i in range(16)]
            m.mem_write(INPUT, struct.pack('<16I', *self.initial_input))
            self.entered = []
        if address in (0x40b2d0, 0x4c3780, 0x4c2e10, 0x4c5420, 0x49fb10,
                       0x44a800, 0x43fa90, 0x4c3870, 0x4b82d0):
            self.entered.append(hex(address))
        if address == 0x40b2d0:
            sp = m.reg_read(UC_X86_REG_ESP)
            assert [self.get(sp + 4), self.get(sp + 8)] == [1, 580]
        if address == 0x4c3870:
            sp = m.reg_read(UC_X86_REG_ESP)
            self.extra, self.player = self.get(sp + 4), self.get(sp + 8)
            self.before = bytes(m.mem_read(self.extra, 580))
            self.current_player = self.get(0x3abdf64)
            return  # Execute the complete original callee, without its old receipt.
        if address == 0x4b82d0:
            self.after = bytes(m.mem_read(self.extra, 580))
            self.output = list(struct.unpack('<16I', m.mem_read(INPUT, 64)))
            self.boundary = address
            m.emu_stop()
            return
        super().hook(m, address, size, user)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe')
    parser.add_argument('--output', default='build/gamma-laser-targets/original-caller.json')
    args = parser.parse_args()
    image = PEImage(args.exe)
    source = Caller(image)
    cases = []
    for slot in range(8):
        values = fixtures()[0].copy()
        values[3] = values[23] = slot
        source.run(values, 127)
        expected = source.initial_input.copy()
        expected[slot * 2:slot * 2 + 2] = [0, 0]
        assert source.before[500:564] == bytes(64)
        assert source.before == source.after and source.output == expected
        assert source.current_player == 0 and source.extra == EXTRAS[0]
        assert source.player == slot and '0x4c3780' not in source.entered
        assert source.entered == ['0x40b2d0', '0x4c2e10', '0x4c5420', '0x49fb10',
                                  '0x44a800', '0x43fa90', '0x4c3870', '0x4b82d0']
        cases.append({'slot': slot, 'extra_identity': 'CAlloc result EXTRAS[0]',
                      'entry_current_player': source.current_player,
                      'input_buffer_words': list(struct.unpack('<16I', source.before[500:564])),
                      'output': source.output, 'calls': source.entered,
                      'extra_before_after_sha256': hashlib.sha256(source.before).hexdigest()})
    report = {'source_sha256': image.sha256, 'cases': cases, 'passed': True,
              'limits': ['Uses unchanged Gamma caller judge concrete lower receipts.',
                         'CAlloc receipt zeroes its actual 580-byte returned block.',
                         'Cursor/setup/effects/motion/weld arithmetic is not re-proved here.',
                         'All eight buffer entries observed at consumption; no menu-empty inference.']}
    path = Path(args.output)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('Original E102 first call: 8 slots, exact call order and all input buffer words pass')


if __name__ == '__main__':
    raise SystemExit(main())
