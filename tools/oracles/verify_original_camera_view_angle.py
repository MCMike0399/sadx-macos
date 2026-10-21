"""SetViewAngle437240 with real AdjustAngle438350; perspective is a recorded boundary."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP


def word(machine, address):
    return struct.unpack("<I", machine.mem_read(address, 4))[0]


def put(machine, address, value):
    machine.mem_write(address, struct.pack("<I", value & 0xffffffff))


class Source:
    def __init__(self, image):
        self.machine = machine = Original(image, 0x7f).machine
        machine.mem_map(0x3b2c000, 0x1000)
        machine.hook_add(unicorn.UC_HOOK_CODE, self.perspective, begin=0x402ed0, end=0x402ed0)

    def perspective(self, machine, address, size, context):
        stack = machine.reg_read(UC_X86_REG_ESP)
        self.result[2] = word(machine, stack + 4)
        self.result[3] = word(machine, 0x3b2ca98)
        self.result[4] += 1
        if self.case[5]:
            put(machine, 0x3b2ca98, self.case[6])
        if self.case[7]:
            machine.emu_stop()
        else:
            machine.reg_write(UC_X86_REG_ESP, stack + 4)
            machine.reg_write(UC_X86_REG_EIP, word(machine, stack))

    def run(self, case):
        self.case = case
        self.result = [0] * 6
        machine = self.machine
        machine.mem_write(0x3b2cade, bytes([case[0] & 255]))
        machine.mem_write(0x3b2cae1, bytes([case[1] & 255]))
        put(machine, 0x3b2ca98, case[2])
        put(machine, 0x3b2cbb4, case[3])
        put(machine, STACK, STOP)
        put(machine, STACK + 4, case[4])
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.emu_start(0x437240, STOP, count=1000)
        if not case[7]:
            if machine.reg_read(UC_X86_REG_EIP) != STOP or machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
                raise RuntimeError("source SetViewAngle did not return with its ABI")
            self.result[5] = 1
        self.result[0] = word(machine, 0x3b2ca98)
        self.result[1] = word(machine, 0x3b2cbb4)
        return self.result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("probe", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=20261020)
    args = parser.parse_args()
    image = PEImage(args.executable)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unexpected source executable identity")
    source = Source(image)
    library = ctypes.CDLL(str(args.probe.resolve()))
    probe = library.sadx_camera_view_angle
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    probe.restype = ctypes.c_int
    randomizer = random.Random(args.seed)
    cases = []
    for camera in range(256):
        for interpolate in (0, 1, 255):
            for mutation, stop in ((0, 0), (1, 0), (1, 1)):
                cases.append([camera, interpolate, randomizer.getrandbits(32),
                              randomizer.getrandbits(32), randomizer.getrandbits(32),
                              mutation, randomizer.getrandbits(32), stop])
    for camera in (27, 28, 29, 30, 31):
        for previous in (0, 1, 12724, 12725, 12726, 12742, 12743, 12744, 12760,
                         12761, 12762, 32767, 32768, 65535, 0xffffffff):
            cases.append([camera, 1, previous, 123, 456, 0, 0, 0])
    mismatches = []
    for index, case in enumerate(cases):
        expected = source.run(case)
        output = (ctypes.c_uint32 * 6)()
        if probe((ctypes.c_uint32 * 8)(*case), output) != 1:
            raise RuntimeError("native probe failed")
        if list(output) != expected:
            mismatches.append({"case": index, "input": case, "expected": expected, "actual": list(output)})
    report = {"status": "pass" if not mismatches else "fail", "cases": len(cases),
              "mismatches": len(mismatches), "examples": mismatches[:10], "seed": args.seed,
              "source_sha256": image.sha256,
              "probe_sha256": hashlib.sha256(args.probe.read_bytes()).hexdigest(),
              "scope": "SetViewAngle plus real AdjustAngle; perspective callback/store order"}
    report["source_ranges"] = {
        f"{hex(address)}-{hex(address + size)}": hashlib.sha256(image.read(address, size)).hexdigest()
        for address, size in ((0x437240, 83), (0x438350, 66))}
    root = Path(__file__).resolve().parents[2]
    report["source_files"] = {name: hashlib.sha256((root / name).read_bytes()).hexdigest()
                              for name in ("src/game/game_view_native.hpp",
                                           "src/game/game_view_native.cpp",
                                           "src/player/player_heading.cpp",
                                           "tests/probes/native_camera_view_angle_probe.cpp",
                                           "tools/oracles/verify_original_camera_view_angle.py")}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))
    return bool(mismatches)


if __name__ == "__main__":
    raise SystemExit(main())
