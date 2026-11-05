"""Prove the common Ninja callback/table entry is exactly RET with no data/register effects."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys
ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles')]
from pe_image import PEImage
from verify_original_math import Original, STACK, STOP
from unicorn.x86_const import *


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    assert image.read(0x78c5a0, 1) == b'\xc3'
    source = Original(image, 0x7f)
    machine = source.machine
    regs = [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
            UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_EFLAGS,
            UC_X86_REG_FPCW, UC_X86_REG_FPSW]
    rng = random.Random(0x78c5a0)
    failures = []
    for case in range(1024):
        for reg in regs[:7]:
            machine.reg_write(reg, rng.getrandbits(32))
        machine.reg_write(UC_X86_REG_EFLAGS, rng.getrandbits(32) & 0x8d5 | 2)
        machine.reg_write(UC_X86_REG_FPCW, rng.choice([0x7f, 0x27f, 0x37f, 0x47f, 0x87f, 0xc7f]))
        machine.mem_write(STACK, struct.pack('<8I', STOP, *[rng.getrandbits(32) for _ in range(7)]))
        machine.reg_write(UC_X86_REG_ESP, STACK)
        before = [machine.reg_read(reg) for reg in regs]
        stack = bytes(machine.mem_read(STACK, 32))
        machine.emu_start(0x78c5a0, STOP, count=2)
        if before != [machine.reg_read(reg) for reg in regs] or \
                machine.reg_read(UC_X86_REG_ESP) != STACK + 4 or \
                machine.reg_read(UC_X86_REG_EIP) != STOP or \
                stack != bytes(machine.mem_read(STACK, 32)):
            failures.append(case)
    report = dict(cases=1024, passed=not failures, values_match=not failures,
                  value_failures=len(failures), flag_mismatches=0, failures=failures,
                  entry='0x78c5a0', original_byte='c3', source_body_bytes=1,
                  original_exe_sha256=image.sha256,
                  exact_body_sha256=hashlib.sha256(b'\xc3').hexdigest(),
                  scope='bare RET; GPRs/flags/FPcontrol/status and stack bytes preserved; ESP advances4')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
