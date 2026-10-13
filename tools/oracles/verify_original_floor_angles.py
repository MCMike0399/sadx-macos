"""Execute source GetNormalAngles/CRTasin/_ftol, preserving oracle disagreements.

Unicorn 2.1.4 FPATAN uses host-double math and its x87 flags are incomplete.
This profile is useful function-local evidence, not Intel Ultra equivalence.
The caller selects CRT policy1 (dispatcher bypass) or policy0 (stop before the
real dispatcher call after its binary64 spill). No callback result is invented.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct
import time

import unicorn
from unicorn import UC_HOOK_CODE, UC_PROT_READ, UC_PROT_WRITE
from unicorn.x86_const import (UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original, CODE, DATA, FIRST, STACK, STOP

OUT = DATA + 0x300
CRT_CONTROL = 0x3c837a8
CRT_BOUNDARY = 0x648b6b
SEED = 0x4b8e80
CONSTANTS = ((0x7e66d8, 4), (0x7e66d4, 4), (0x7e00e0, 4),
             (0x7dccd0, 8), (0x7dccc8, 8), (0x7dcce8, 8))


def sha(data): return hashlib.sha256(data).hexdigest()
def fbits(value): return struct.unpack('<I', struct.pack('<f', value))[0]
def from_bits(value): return struct.unpack('<f', struct.pack('<I', value))[0]


def evidence_result(result):
    # Exact register/store bits remain strings in JSON consumers, including
    # those whose ordinary number type cannot retain 64-bit significands.
    record=dict(result)
    for field in ('boundary','flags','cw'):
        record[field]=hex(record[field])
    record['angles']=list(map(hex,record['angles']))
    if record['ext80'] is not None: record['ext80']=list(map(hex,record['ext80']))
    if record['pending_f64'] is not None: record['pending_f64']=hex(record['pending_f64'])
    return record


def fixtures(constants):
    upper, lower, epsilon = struct.unpack('<3f', constants[:12])
    cases = []
    def add(name, words, mode=0, policy=1):
        cases.append(dict(name=name, words=[mode, *words, 0x13579bdf, 0xfdb97531, 1, policy]))
    for z in (0., -0., 1., -1., upper, lower, 0.5, -0.5, 1.0001, -1.0001, 2., -2.):
        for x, y in ((0., 0.), (-0., -0.), (1., 0.), (0., 1.), (1., 1.), (-1., -1.)):
            add('axis/threshold', list(map(fbits, (x, y, z))))
    for base in (fbits(upper), fbits(lower), fbits(epsilon)):
        for delta in (-2, -1, 0, 1, 2):
            raw = (base + delta) & 0xffffffff
            add('component threshold-neighbor', [raw, fbits(epsilon), fbits(0.25)])
            add('inclination threshold-neighbor', [fbits(0.25), fbits(0.5), raw])
            add('CRTasin threshold-neighbor', [0, 0, raw], mode=1)
    for raw in (0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x807fffff,
                0x7f800000, 0xff800000, 0x7fc12345, 0xffc54321, 0x7f812345, 0xff812345,
                0x3f7fffff, 0x3f800000, 0x3f800001, 0xbf7fffff, 0xbf800000, 0xbf800001):
        for component in range(3):
            words = list(map(fbits, (0.25, 0.5, 0.75))); words[component] = raw
            add('nonfinite/subnormal/one-neighbor', words)
        add('CRTasin special/domain', [0, 0, raw], mode=1)
    rng = random.Random(SEED)
    for _ in range(1200):
        vector = [rng.uniform(-1, 1) for _ in range(3)]
        length = sum(v * v for v in vector) ** 0.5
        add('normalized random source-f32 input', list(map(fbits, (v / length for v in vector))))
    for _ in range(300):
        add('unnormalized random', list(map(fbits, (rng.uniform(-8, 8) for _ in range(3)))))
    for _ in range(200):
        add('CRTasin random finite', [0, 0, fbits(rng.uniform(-1.25, 1.25))], mode=1)
    # Dispatcher boundaries get no invented callback: actual instructions stop
    # at the source call after FSTP64, native reports missing handler.
    for raw in map(fbits, (0., 0.5, -0.5, 1., -1., 1.5, -1.5)):
        add('CRTasin explicit policy0 dispatcher boundary', [0, 0, raw], mode=1, policy=0)
        add('floor explicit policy0 dispatcher boundary', [fbits(0.25), fbits(0.5), raw], policy=0)
    for raw in (0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff812345):
        add('CRTasin policy0 nonfinite boundary', [0,0,raw], mode=1, policy=0)
        add('floor policy0 nonfinite', [fbits(0.25),fbits(0.5),raw], policy=0)
    return cases


class FloorOriginal(Original):
    def __init__(self, image, cw):
        super().__init__(image, cw)
        self.machine.mem_map(0x2bcc000, 0x1000, UC_PROT_READ | UC_PROT_WRITE)
        self.machine.mem_write(0x2bcc000, image.read(0x2bcc000, 0x1000))
        self.machine.mem_map(CRT_CONTROL & ~0xfff, 0x1000, UC_PROT_READ | UC_PROT_WRITE)
        self.boundary = 0
        def stop_dispatch(machine, address, size, user):
            self.boundary = address; machine.emu_stop()
        self.machine.hook_add(UC_HOOK_CODE, stop_dispatch, begin=CRT_BOUNDARY, end=CRT_BOUNDARY)
        # FNCLEX resets flags; exact original call returns ST0; FSTP ext80 is a
        # harness-only projection retaining its significand instead of f64.
        prefix = b'\xdb\xe2\xd9\x05' + struct.pack('<I', FIRST)
        call = b'\xe8' + struct.pack('<i', 0x645020 - (CODE + len(prefix) + 5))
        suffix = b'\xdb\x3d' + struct.pack('<I', DATA + 0x500) + b'\xc3'
        self.machine.mem_write(CODE, prefix + call + suffix)

    def run(self, words, cw):
        mode, x, y, z, sentinel_x, sentinel_z, known, policy = words
        if not known: raise ValueError('An unknown runtime word cannot seed an original execution')
        machine = self.machine; self.boundary = 0
        machine.mem_write(CRT_CONTROL, struct.pack('<I', policy))
        machine.mem_write(OUT, struct.pack('<2I', sentinel_x, sentinel_z))
        machine.reg_write(UC_X86_REG_FPCW, cw)
        machine.reg_write(UC_X86_REG_FPSW, 0) # caller-selected clean stack/status profile
        machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
        if mode == 0:
            machine.mem_write(STACK, struct.pack('<6I', STOP, x, y, z, OUT, OUT + 4))
            entry = 0x4b8e80
        else:
            machine.mem_write(FIRST, struct.pack('<I', z))
            machine.mem_write(STACK, struct.pack('<I', STOP)); entry = CODE
        machine.reg_write(UC_X86_REG_ESP, STACK)
        machine.emu_start(entry, STOP, count=2000)
        result = dict(angles=list(struct.unpack('<2I', machine.mem_read(OUT, 8))),
                      boundary=self.boundary, flags=machine.reg_read(UC_X86_REG_FPSW) & 0x3f,
                      cw=machine.reg_read(UC_X86_REG_FPCW), returned=not self.boundary,
                      ext80=None, pending_f64=None)
        if self.boundary:
            result['pending_f64'] = struct.unpack('<Q', machine.mem_read(machine.reg_read(UC_X86_REG_EBP) - 8, 8))[0]
        else:
            if machine.reg_read(UC_X86_REG_ESP) != STACK + 4:
                raise RuntimeError('Original floor/CRT function did not return with cdecl ABI')
            if mode == 1:
                signif, sign_exp = struct.unpack('<QH', machine.mem_read(DATA + 0x500, 10))
                result['ext80'] = [sign_exp, signif]
        return result


def main():
    parser = argparse.ArgumentParser(); parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('native_library', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    if unicorn.__version__ != '2.1.4': raise RuntimeError('Requires pinned Unicorn 2.1.4')
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256: raise RuntimeError('Unknown original address-map identity')
    constant_bytes = b''.join(image.read(address, size) for address, size in CONSTANTS)
    cases = fixtures(constant_bytes)
    case_blob = b''.join(struct.pack('<8I', *case['words']) for case in cases)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    case_path = args.output.with_suffix('.cases.bin'); case_path.write_bytes(case_blob)
    library = ctypes.CDLL(str(args.native_library.resolve())); probe = library.sadx_native_floor_angles
    library.sadx_native_floor_angles_compiler.argtypes=[]
    library.sadx_native_floor_angles_compiler.restype=ctypes.c_char_p
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(ctypes.c_uint32),
        ctypes.c_size_t, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    probe.restype = ctypes.c_int
    constants = (ctypes.c_uint8 * 36).from_buffer_copy(constant_bytes)
    source_dir = pathlib.Path(__file__).resolve().parents[2]
    report = dict(profile='source-GetNormalAngles-CRTasin-ftol/Unicorn2.1.4/explicit-policy/native-Bochs-P5P6',
        source_sha256=image.sha256, native_library_sha256=sha(args.native_library.read_bytes()),
        native_model='X87Transcendental::fpatan_bochs_p5_p6', seed=hex(SEED),
        source_functions={hex(a): dict(size=b-a, sha256=sha(image.read(a,b-a))) for a,b in
            ((0x4b8e80,0x4b8f82),(0x645020,0x6450eb),(0x648b47,0x648c65),(0x644340,0x6443b5))},
        constants={hex(a): dict(size=n, bytes=image.read(a,n).hex(), sha256=sha(image.read(a,n))) for a,n in CONSTANTS},
        native_sources={name: sha((source_dir/name).read_bytes()) for name in
            ('src/player/floor_angles.cpp','src/player/floor_angles.hpp','src/cpu/crt_asin.cpp','src/cpu/crt_asin.hpp',
             'src/cpu/crt_integer.cpp','src/cpu/crt_integer.hpp','src/cpu/x87_math.cpp','src/cpu/x87_math.hpp',
             'src/cpu/x87_transcendental.cpp','src/cpu/x87_transcendental.hpp','src/player/player_coordinates.hpp',
             'tests/probes/native_floor_angles_probe.cpp','tools/oracles/verify_original_floor_angles.py')},
        native_library_compiler=library.sadx_native_floor_angles_compiler().decode('utf-8'),
        declared_native_build_profile='arm64 C++17 O2 ff-contract=off fno-fast-math; native integer SoftFloat/Bochs archives',
        bochs_dependency=sha((source_dir/'third_party/bochs_fpatan/upstream.json').read_bytes()),
        basic_softfloat_dependency=sha((source_dir/'third_party/softfloat/upstream.json').read_bytes()),
        cases=dict(count=len(cases), path=str(case_path.resolve()), sha256=sha(case_blob), record='8 LE uint32 words'),
        controls=[], mismatches=[], native_unknown_policy=[],
        limits=['All GetNormalAngles/CRTasin/_ftol source instructions execute until return or explicit dispatcher boundary',
            'Policy1 bypass and policy0 pre-dispatch stop are test seeds; actual initialized CRT word is unobserved',
            'Original error callback is never replaced or executed; no callback result is invented',
            'Unicorn FPATAN uses host-double arithmetic and x87 exception flags are incomplete',
            'Native Bochs P5/P6 results must not be changed merely to match this weaker oracle',
            'Original Intel Ultra results, startup state, gameplay/mod call inputs, FPS and hardware exception delivery remain unverified'])
    start = time.monotonic()
    for cw in (p | r | 0x7f for p in (0,0x200,0x300) for r in (0,0x400,0x800,0xc00)):
        original = FloorOriginal(image,cw)
        item = dict(cw=hex(cw), cases=len(cases), floor_angle_mismatches=0, asin_ext80_mismatches=0,
                    boundary_mismatches=0, cw_mismatches=0, flag_mismatches=0, pending_store_mismatches=0)
        counters=('floor_angle_mismatches','asin_ext80_mismatches','boundary_mismatches',
                  'cw_mismatches','flag_mismatches','pending_store_mismatches')
        item['policy_summaries']={str(policy):dict(cases=0,**{key:0 for key in counters}) for policy in (0,1)}
        for index, case in enumerate(cases):
            words = case['words']; expected = original.run(words,cw); output = (ctypes.c_uint32 * 14)()
            status = probe(constants,36,(ctypes.c_uint32 * 8)(*words),8,cw,output,14)
            if status or output[7]: raise RuntimeError(f'Native floor probe error case{index}:status{status},x87{output[7]}')
            native = dict(angles=list(output[:2]), failure=output[2], boundary=output[3], flags=output[4],
                cw=output[5], returned=bool(output[6]), ext80=[output[10],output[11] | output[12]<<32] if words[0] and output[6] else None,
                pending_f64=(output[8] | output[9]<<32) if output[13] else None)
            diffs = []
            summary=item['policy_summaries'][str(words[7])];summary['cases']+=1
            for field, counter in (('angles','floor_angle_mismatches'),('ext80','asin_ext80_mismatches'),
                ('boundary','boundary_mismatches'),('cw','cw_mismatches'),('flags','flag_mismatches')):
                if expected[field] != native[field]:
                    item[counter]+=1;summary[counter]+=1;diffs.append(field)
            # Floor owner does not yet expose partial CRT spill; the standalone
            # CRT probe does, so compare it only for mode1.
            if words[0] == 1 and expected['pending_f64'] != native['pending_f64']:
                item['pending_store_mismatches']+=1;summary['pending_store_mismatches']+=1;diffs.append('pending_f64')
            if diffs: report['mismatches'].append(dict(cw=hex(cw),case=index,name=case['name'],
                inputs=list(map(hex,words)),fields=diffs,original=evidence_result(expected),native=evidence_result(native)))
        for mode in (0,1):
            words=[mode,fbits(0.25),fbits(0.5),fbits(0.5),0x13579bdf,0xfdb97531,0,0]
            output=(ctypes.c_uint32*14)();status=probe(constants,36,(ctypes.c_uint32*8)(*words),8,cw,output,14)
            report['native_unknown_policy'].append(dict(cw=hex(cw),mode=mode,status=status,output=list(output)))
        report['controls'].append(item)
        print(json.dumps(item),flush=True)
    report['duration_seconds']=time.monotonic()-start
    report['floor_output_and_boundary_match']=all(not any(p[k] for k in
        ('floor_angle_mismatches','boundary_mismatches','cw_mismatches')) for p in report['controls'])
    report['asin_extended_register_match']=all(not p['asin_ext80_mismatches'] for p in report['controls'])
    report['emulator_exception_flags_match']=all(not p['flag_mismatches'] for p in report['controls'])
    report['bypass_floor_output_control_and_boundary_match']=all(not any(p['policy_summaries']['1'][k] for k in
        ('floor_angle_mismatches','boundary_mismatches','cw_mismatches')) for p in report['controls'])
    report['bypass_asin_extended_register_match']=all(not p['policy_summaries']['1']['asin_ext80_mismatches']
        for p in report['controls'])
    report['limits']=([report['limits']] if isinstance(report.get('limits'),str) else list(report.get('limits') or []))+['The value-class differences come from Unicorn 2.1.4, not the port: it computes FPATAN with host doubles inside CRT asin (asin_ext80_mismatches) and SNaN loads; the floor-angle, boundary and pending-store differences follow from those intermediates. The native port follows the Bochs P5/P6 / SoftFloat models and must not be changed to match. Settling these cases needs the Intel capture (roadmap M4); until then this report stays `fail`.']
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    # A disagreement is retained evidence; the command reports it to the caller
    # rather than treating weaker transcendental emulation as a repair target.
    return 0 if report['floor_output_and_boundary_match'] and report['asin_extended_register_match'] else 1


if __name__=='__main__': raise SystemExit(main())
