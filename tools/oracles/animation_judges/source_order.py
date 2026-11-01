"""Record actual malloc/publication/free ordering with explicit allocator receipts.

Allocator policy is not emulated: only actual caller instructions and their
stores are evidence. Failed malloc returns null; page zero stays unmapped.
"""
import argparse
import json
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / "tools/oracles"), str(ROOT / "tools/lib")]
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_player_animation import AnimationOriginal, fixtures, STATE, TABLE, BUFFER, BMOTION
from verify_original_math import DATA

ALLOC = DATA + 0x3000
MDATA = DATA + 0x4000


def experiment(image, fail_allocation=None, release=False):
    original = AnimationOriginal(image)
    machine = original.machine
    events = []
    allocations = []

    def get(address):
        return struct.unpack("<I", machine.mem_read(address, 4))[0]

    def call(m, address, size, user):
        if release and address == 0x43F9F0:
            # Reach one requested row; its current-row neighbour stays count 0.
            m.mem_write(TABLE + 4, b"\x01")
            m.mem_write(BMOTION, struct.pack("<I", MDATA))
            m.mem_write(MDATA, struct.pack("<4I", ALLOC, 0, 2, 77))
        if address in (0x40B220, 0x40B2D0):
            stack = m.reg_read(UC_X86_REG_ESP)
            ret, first, second = struct.unpack("<III", m.mem_read(stack, 12))
            count, width = (1, first) if address == 0x40B220 else (first, second)
            target = ALLOC + len(allocations) * 0x100
            if len(allocations) == fail_allocation:
                target = 0
            allocations.append(target)
            events.append(dict(kind="allocation", caller=ret - 5, entry=address,
                               count=count, width=width, returned=target))
            if target:
                # Explicit fixture fill, never a source malloc default.
                m.mem_write(target, bytes([0xA5 if address == 0x40B220 else 0]) * (count * width))
            m.reg_write(UC_X86_REG_EAX, target)
            m.reg_write(UC_X86_REG_ESP, stack + 4)
            m.reg_write(UC_X86_REG_EIP, ret)
        elif address == 0x40B310:
            stack = m.reg_read(UC_X86_REG_ESP)
            ret, pointer = struct.unpack("<II", m.mem_read(stack, 8))
            events.append(dict(kind="free", caller=ret - 5, pointer=pointer,
                               channels=list(struct.unpack("<4I", m.mem_read(MDATA, 16)))))
            m.reg_write(UC_X86_REG_ESP, stack + 4)
            m.reg_write(UC_X86_REG_EIP, ret)

    def write(m, access, address, size, value, user):
        if STATE <= address < STATE + 48 or ALLOC <= address < ALLOC + 0x1000 or MDATA <= address < MDATA + 16:
            events.append(dict(kind="write", pc=m.reg_read(UC_X86_REG_EIP), address=address,
                               size=size, bits=value & ((1 << (size * 8)) - 1)))

    machine.hook_add(unicorn.UC_HOOK_CODE, call)
    machine.hook_add(unicorn.UC_HOOK_MEM_WRITE, write)
    fixture = next(fixtures())
    fixture[0] = 14 if release else 0
    fault = None
    try:
        original.run(fixture)
    except unicorn.UcError as error:
        fault = dict(pc=machine.reg_read(UC_X86_REG_EIP), error=str(error))
    expected_fault = (0x44A880 if fail_allocation == 0 else 0x44A898 if fail_allocation == 1 else None)
    assert (fault["pc"] if fault else None) == expected_fault
    if release:
        frees = [event for event in events if event["kind"] == "free"]
        assert [event["pointer"] for event in frees] == [ALLOC, MDATA, BMOTION, BUFFER]
        assert frees[0]["channels"] == [ALLOC, 0, 2, 77]
        assert frees[1]["channels"] == [0, 0, 0, 77]
    else:
        allocs = [event for event in events if event["kind"] == "allocation"]
        assert [(event["count"], event["width"]) for event in allocs] == [(1, 8), (1, 12), (111, 24)][:len(allocs)]
        # Concrete order: current/mode/start/frame precede malloc; publish action
        # precedes object read/store; publish motion precedes header stores.
        pcs = [event["pc"] for event in events if event["kind"] == "write"]
        assert pcs.index(0x44A85B) < pcs.index(0x44A873)
        if fail_allocation != 0:
            assert pcs.index(0x44A873) < pcs.index(0x44A880) < pcs.index(0x44A88A)
        if fail_allocation not in (0, 1):
            assert pcs.index(0x44A88A) < pcs.index(0x44A898) < pcs.index(0x44A8C6)
    return dict(failed_allocation=fail_allocation, release=release, fault=fault, events=events,
                state_hex=bytes(machine.mem_read(STATE, 48)).hex(),
                limits="Allocator return values and A5 fill are explicit receipts; actual caller instruction ordering only")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    image = PEImage(args.exe)
    assert image.sha256 == SOURCE_SHA256 and unicorn.__version__ == "2.1.4"
    rows = [experiment(image, failure) for failure in (None, 0, 1, 2)]
    rows.append(experiment(image, release=True))
    args.output.write_text(json.dumps(rows, indent=2) + "\n")
    print("5 original ordering experiments passed; null allocation PCs and free order retained")


if __name__ == "__main__":
    main()
