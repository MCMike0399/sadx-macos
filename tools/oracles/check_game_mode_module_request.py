#!/usr/bin/env python3
"""Record the pinned FCLoadFileModule body and its disabled printer directly."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original, STACK, STOP


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("Refusing to overwrite source evidence")
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != "2.1.4":
        raise RuntimeError("Pinned executable or Unicorn mismatch")
    body = image.read(0x4221e0, 21)
    printer = image.read(0x51f680, 1)
    # This is a bounded byte proof, not an address-based implementation or a
    # heuristic function finder. The existing port already names these entries.
    if body.hex() != "8b4424045068c0de7d00e891d40f0083c40833c0c3" or printer != b"\xc3":
        raise RuntimeError("Module request/printer instruction identity changed")
    profile_path = Path("tools/gate/profile_patches.json")
    profile = json.loads(profile_path.read_text())
    overlaps = []
    for row in profile["patches"]:
        start, size = int(row["va"], 0), row["size"]
        if any(start < end and begin < start + size
               for begin, end in ((0x4221e0, 0x4221f5), (0x51f680, 0x51f681))):
            overlaps.append(row)
    if any(row["active"] for row in overlaps):
        raise RuntimeError("Active pinned profile patch overlaps the bounded instruction proof")
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    instructions = [{"address": item.address, "bytes": bytes(item.bytes).hex(),
                     "mnemonic": item.mnemonic, "operands": item.op_str}
                    for item in decoder.disasm(body, 0x4221e0)]
    machine = Original(image, 0x7f).machine
    rows = []
    for first in (0, 1, 2, 2147483648, 4294967295):
        for second in (0, 1, 2, 2147483648, 4294967295):
            stack = struct.pack("<III", STOP, first, second)
            machine.mem_write(STACK, stack)
            machine.reg_write(UC_X86_REG_ESP, STACK)
            machine.reg_write(UC_X86_REG_EAX, 3456789012)
            machine.emu_start(0x4221e0, STOP, count=100)
            eax, esp = machine.reg_read(UC_X86_REG_EAX), machine.reg_read(UC_X86_REG_ESP)
            if eax != 0 or esp != STACK + 4 or bytes(machine.mem_read(STACK + 4, 8)) != stack[4:]:
                raise RuntimeError("Original module request contract changed")
            rows.append({"arguments": [first, second], "eax": eax, "stack_delta": esp - STACK})
    result = {"source_sha256": image.sha256, "entry": "0x4221e0", "body_hex": body.hex(),
              "body_sha256": hashlib.sha256(body).hexdigest(), "instructions": instructions,
              "printer": {"entry": "0x51f680", "body_hex": printer.hex(), "instruction": "ret"},
              "profile_patch_census_sha256": hashlib.sha256(profile_path.read_bytes()).hexdigest(),
              "profile_patch_overlaps": overlaps,
              "cases": rows, "limits": [
                  "Direct original instruction evidence. The printer returns before dereferencing its arguments.",
                  "No resource/module load occurs on this complete bounded path; the game-mode caller ignores EAX.",
                  "This is not filesystem, general diagnostics, downstream service or new gameplay qualification.",
                  "Pinned profile patch census is separately bound by the packet; no live mod execution here."]}
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print("25 original module requests exact; complete21-byte body calls1-byte RET printer")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
