"""Execute original creation rollback paths; lower allocation/destruction are controlled."""
import argparse
import hashlib
import json
from pathlib import Path
from verify_source import (ROOT, Original, DATA, STACK, STOP, PEImage, put, word, ret,
                           UC_HOOK_CODE, UC_X86_REG_EAX,
                           UC_X86_REG_ESP)
from unicorn.x86_const import UC_X86_REG_EDI


def observe(image, flags, failing_allocation, setup):
    machine = Original(image, 127).machine
    task, primary, models = DATA + 0x1000, DATA + 0x2000, DATA + 0x4000
    put(machine, STACK, STOP)
    put(machine, STACK + 4, 7 if setup else flags)
    put(machine, STACK + 8, 2 if setup else 3)
    put(machine, STACK + 12, 0x493730 if flags == 2 else 0x4937b0)
    machine.reg_write(UC_X86_REG_ESP, STACK)
    machine.reg_write(UC_X86_REG_EDI, models)
    calls = []
    allocations = 0

    def hook(vm, address, size, context):
        nonlocal allocations
        sp = machine.reg_read(UC_X86_REG_ESP)
        if address == 0x40b030:
            calls.append(['AllocateTask'])
            ret(machine, task)
        elif address == 0x40b220:
            allocations += 1
            amount = word(machine, sp + 4)
            assert amount == 64
            calls.append(['MAlloc', amount])
            ret(machine, 0 if allocations == failing_allocation else primary)
        elif address == 0x40b570:
            assert word(machine, sp + 4) == task
            calls.append(['DestroyTask', task])
            ret(machine)
        elif address == 0x40b6c0:
            raise AssertionError('Deferred FreeTask is not creation rollback')

    machine.hook_add(UC_HOOK_CODE, hook)
    machine.emu_start(0x495860 if setup else 0x40b860, STOP, count=1000)
    result = machine.reg_read(UC_X86_REG_EAX)
    assert result == 0
    assert calls == [['AllocateTask']] + [['MAlloc', 64]] * failing_allocation + [
        ['DestroyTask', task]]
    assert word(machine, task + 32) == (primary if failing_allocation == 2 else 0)
    if failing_allocation == 2:
        assert bytes(machine.mem_read(primary, 64)) == bytes(64)
    return dict(flags=flags, failing_allocation=failing_allocation, setup=setup,
                result=result, calls=calls, published_primary=word(machine, task + 32))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = PEImage(ROOT / 'reference/updated-pc-reference/game/sonic.exe')
    assert image.sha256 == '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
    observations = [observe(image, 2, 1, True), observe(image, 2, 1, False),
                    observe(image, 3, 1, False), observe(image, 3, 2, False)]
    report = dict(source_sha256=image.sha256, observations=observations,
                  creation_body_sha256=hashlib.sha256(image.read(0x40b860, 213)).hexdigest(),
                  helper_body_sha256=hashlib.sha256(image.read(0x495860, 91)).hexdigest(),
                  limits=['Original AllocateTask, MAlloc and DestroyTask are controlled.',
                          'Native scheduler rollback and later Sonic publications are checked separately.'])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('Four original work-exhaustion paths destroy the task and return zero')


if __name__ == '__main__':
    main()
