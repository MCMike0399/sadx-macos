"""Run the unchanged screen-setup judge plus the native outer-frame entry points.

The original judge retains all500 fixtures, failure prefixes and FP comparisons.
The additional frame entries record depth/material calls and their failure prefixes.
"""
import ctypes
import hashlib
import json
from pathlib import Path
import sys
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
import verify_original_game_view as setup_judge
from verify_original_math import Original, STACK, STOP


class FrameSource:
    def __init__(self, image):
        self.machine = machine = Original(image, 0x7f).machine
        machine.mem_map(0x3b28000, 0x1000)
        for address in (0x77ed00, 0x77ed20, 0x4128f0):
            machine.hook_add(unicorn.UC_HOOK_CODE, self.call, begin=address, end=address)

    def call(self, machine, address, size, context):
        stack = machine.reg_read(UC_X86_REG_ESP)
        argument = 0 if address == 0x4128f0 else setup_judge.get(machine, stack + 4, 4)
        self.calls.append((address, argument))
        if self.fail and len(self.calls) == self.fail:
            self.stopped = True
            machine.emu_stop()
            return
        machine.reg_write(UC_X86_REG_EIP, setup_judge.get(machine, stack, 4))
        machine.reg_write(UC_X86_REG_ESP, stack + 4)

    def run(self, pending, operation, fail):
        self.calls, self.fail, self.stopped = [], fail, False
        machine = self.machine
        setup_judge.put(machine, 0x3b280a8, 1, pending)
        setup_judge.put(machine, STACK, 4, STOP)
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.emu_start(0x420830 if operation == 0 else 0x420840, STOP, count=1000)
        if not self.stopped and (machine.reg_read(UC_X86_REG_EIP) != STOP or
                                 machine.reg_read(UC_X86_REG_ESP) != STACK + 4):
            raise RuntimeError('source frame entry did not return with its ABI')
        result = [setup_judge.get(machine, 0x3b280a8, 1), int(not self.stopped), len(self.calls)]
        for call in self.calls:
            result.extend(call)
        return result + [0] * (10 - len(result))


def main():
    # Do not replace fixtures/comparisons or reinterpret the old FP result.
    code = setup_judge.main()
    output_path = Path(sys.argv[sys.argv.index('--output') + 1])
    report = json.loads(output_path.read_text())
    source = FrameSource(PEImage(sys.argv[1]))
    library = ctypes.CDLL(str(Path(sys.argv[2]).resolve()))
    probe = library.sadx_native_screen_frame
    probe.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32,
                      ctypes.POINTER(ctypes.c_uint32)]
    probe.restype = ctypes.c_int
    mismatches, count = [], 0
    for pending in (0, 1, 127, 255):
        for operation in (0, 1):
            for fail in range(5):
                expected = source.run(pending, operation, fail)
                actual = (ctypes.c_uint32 * 10)()
                if probe(pending, operation, fail, actual) != 1:
                    raise RuntimeError('native frame probe failed')
                if list(actual) != expected:
                    mismatches.append({'input': [pending, operation, fail],
                                       'source': expected, 'native': list(actual)})
                count += 1
    report['frame_entries'] = {'cases': count, 'mismatches': len(mismatches), 'examples': mismatches}
    report['cases'] += count
    report['state_mismatch_count'] += len(mismatches)
    report['stores_trace_match'] = report['stores_trace_match'] and not mismatches
    report['reference_adapter_source_files'] = report['source_files']
    root = Path(__file__).resolve().parents[2]
    files = ('src/game/game_view_native.hpp', 'src/game/game_view_native.cpp',
             'tests/probes/native_game_view_native_probe.cpp',
             'tools/oracles/verify_original_game_view.py', 'tools/oracles/verify_original_game_view_native.py')
    report['source_files'] = {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in files}
    report['limits'] += '\n' + __doc__
    output_path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report['frame_entries']))
    return code or bool(mismatches)


if __name__ == '__main__':
    raise SystemExit(main())
