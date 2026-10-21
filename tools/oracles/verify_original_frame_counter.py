"""Compare frame-completion scalar stores to the unchanged reference's actual pinned DLL hook.

Stops after the pre-render stores. Rendering and limiter callbacks remain caller-owned.
The existing frame-completion Source constructor/profile mapping/hook are reused unchanged;
this caller drives its machine with a wall-clock timeout rather than its legacy run method.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_frame_completion import Source, SPEEDRUN_SHA256, get, put
from verify_original_math import STACK, STOP


class CounterSource:
    def __init__(self, image, dll):
        self.reference = Source(image, dll, True)
        self.reference.m.hook_add(unicorn.UC_HOOK_CODE, self.stop_at_boundary)

    def stop_at_boundary(self, machine, address, size, user):
        if address == 0x411dc5:
            self.stopped = True
            machine.emu_stop()

    def run(self, values):
        source = self.reference
        machine = source.m
        machine.context_restore(source.clean)
        source.case = [0] * 16
        source.trace = []
        source.failure = None
        self.stopped = False
        addresses = (0x3abdf68, 0x89295c, 0x3b11188, 0x3b11180,
                     0x3b11198, 0x3b11194, 0x3b11178, 0x3b1117c)
        for index, (address, value) in enumerate(zip(addresses, values)):
            put(machine, address, 1 if index == 0 else 4, value)
        put(machine, 0x3b28114, 4, 0)
        put(machine, STACK, 4, STOP)
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.emu_start(0x411d00, STOP, timeout=1_000_000)
        if not self.stopped or machine.reg_read(UC_X86_REG_EIP) != 0x411dc5:
            raise RuntimeError("source did not reach the frame-completion store boundary")
        return [get(machine, address, 1 if index == 0 else 4)
                for index, address in enumerate((0x3abdf68, 0x3b11180, 0x3b11198, 0x3b11194,
                                                 0x3b11188, 0x3b11178, 0x3b1117c, 0x89295c))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("native_library", type=Path)
    parser.add_argument("--speedrun-dll", type=Path, default=Path("reference/updated-pc-reference/game/mods/sadx-speedrun-mod/sadx-speedrun-mod.dll"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cases", type=int, default=1500)
    parser.add_argument("--seed", type=int, default=20261020)
    args = parser.parse_args()
    image, dll = PEImage(args.exe), PEImage(args.speedrun_dll)
    if image.sha256 != SOURCE_SHA256 or dll.sha256 != SPEEDRUN_SHA256:
        raise ValueError("unsupported original executable or effective Speedrun DLL")
    source = CounterSource(image, dll)
    library = ctypes.CDLL(str(args.native_library.resolve()))
    native = library.sadx_native_frame_counter
    native.argtypes = [ctypes.POINTER(ctypes.c_uint32)] * 2
    native.restype = None
    rng = random.Random(args.seed)
    cases = []
    for resync in (0, 1, 5, 255):
        for interval in (0, 1, 3, 0x7fffffff, 0x80000000, 0xffffffff):
            for multiplier in (0, 1, 2, 0xffffffff, 0x80000000, 0x80000001, 0x7fffffff):
                for active in (0, 1, 0xffffffff):
                    cases.append([resync, interval, multiplier, rng.getrandbits(32), active,
                                  rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32)])
    cases += [[rng.getrandbits(8)] + [rng.getrandbits(32) for _ in range(7)]
              for _ in range(args.cases)]
    bad = []
    for index, values in enumerate(cases):
        expected = source.run(values)
        output = (ctypes.c_uint32 * 8)()
        native((ctypes.c_uint32 * 8)(*values), output)
        if list(output) != expected:
            bad.append({"case": index, "input": values, "source": expected, "native": list(output)})
    root = Path(__file__).resolve().parents[2]
    paths = ["src/engine/frame_loop_native.hpp", "src/engine/frame_loop_native.cpp",
             "src/engine/speedrun.hpp", "src/engine/speedrun.cpp",
             "tests/probes/native_frame_counter_probe.cpp",
             "tools/oracles/verify_original_frame_completion.py"]
    report = {"passed": not bad, "values_match": not bad, "cases": len(cases), "seed": args.seed,
              "mismatches": len(bad), "first_mismatches": bad[:3],
              "source_sha256": image.sha256, "speedrun_dll_sha256": dll.sha256,
              "source_ranges": {"0x411d00-0x411dc5": hashlib.sha256(image.read(0x411d00, 0xc5)).hexdigest()},
              "native_sources": {path: hashlib.sha256((root / path).read_bytes()).hexdigest() for path in paths},
              "native_library_sha256": hashlib.sha256(args.native_library.read_bytes()).hexdigest(),
              "case_sha256": hashlib.sha256(b"".join(struct.pack("<8I", *row) for row in cases)).hexdigest(),
              "limits": ["Compares the scalar prefix only; rendering/limiter callbacks remain caller-owned.",
                         "Original profile without the pinned Speedrun hook is not claimed."]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"cases": len(cases), "mismatches": len(bad)}))
    return int(bool(bad))


if __name__ == "__main__":
    raise SystemExit(main())
