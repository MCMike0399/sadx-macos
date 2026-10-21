"""Original regular/localized traversal and pinned compiled loader wrappers.

Executes compiled LoadPVM_r, check_cache and njLoadTexture_Wrapper_r. The raw
asset fixture bypasses replacement path discovery (independently qualified by
importer selected-profile proof), executing its compiled check_cache and resync
store prefix. PVM decoding, individual uploads, CRT allocation/string/memory and
opaque cache destruction are explicit resource receipts. No pixel data judged.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, CODE, STACK, STOP

LOADER_SHA = '3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba'
BASE = 0x22000000
LISTS, NAMES, STRINGS, MEMORIES, GROUP = [BASE + n for n in (0x1000, 0x2000, 0x8000, 0xa000, 0xc000)]
NAMES_TEXT = ['', 'b32ascii', 'other', 'b32ascii_j', 'CON_REGULAR', 'OBJ_REGULAR', 'SS_MIZUGI', 'CON_REGULAR_E']
STRCMP, MIP, CACHE_RETURN = CODE + 0x300, CODE + 0x310, CODE + 0x320

class Source:
    def __init__(self, image, loader):
        assert image.sha256 == SOURCE_SHA256 and loader.sha256 == LOADER_SHA
        assert unicorn.__version__ == '2.1.4'
        self.m = m = Original(image, 127).machine
        for section in image.sections:
            if section['name'] in ('.text', '.rdata'): continue
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            m.mem_map(image.base + section['rva'], size)
            m.mem_write(image.base + section['rva'], image.slice(section['file_offset'], section['raw_size']))
        for section in loader.sections:
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            m.mem_map(loader.base + section['rva'], size)
            m.mem_write(loader.base + section['rva'], loader.slice(section['file_offset'], section['raw_size']))
        m.mem_map(BASE, 0x40000)
        # Valid Windows SEH frame fixture only; null filename cohorts excluded.
        m.mem_map(0, 4096)
        self.put(0x100cd0ec, STRCMP)
        self.put(0x100cd530, MIP)
        assert self.get(0x100f1b94) == 0x3abdf68
        for address, target in [(0x421180, 0x100a9dc0), (0x4228e0, 0x100ab2d0)]:
            m.mem_write(address, b'\xe9' + struct.pack('<i', target - address - 5))
        m.hook_add(unicorn.UC_HOOK_CODE, self.hook)
        self.context = m.context_save()

    def put(self, address, value, size=4):
        self.m.mem_write(address, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))
    def get(self, address, size=4):
        return int.from_bytes(self.m.mem_read(address, size), 'little')
    def string(self, address):
        result = bytearray()
        while self.get(address, 1):
            result.append(self.get(address, 1)); address += 1
        return result.decode()
    def ret(self, result=0, pop=0):
        m = self.m; sp = m.reg_read(UC_X86_REG_ESP)
        m.reg_write(UC_X86_REG_EAX, result & 0xffffffff)
        m.reg_write(UC_X86_REG_EIP, self.get(sp))
        m.reg_write(UC_X86_REG_ESP, sp + 4 + pop)
    def list_id(self, pointer):
        return (pointer - LISTS) // 8 + 1 if pointer else 0
    def event(self, kind, texture_list=0):
        self.events += [kind, self.get(0x3abdf68, 1), self.list_id(self.get(0x3b290b0)),
                        self.list_id(self.get(0x3b29030)), self.list_id(texture_list),
                        sum(self.get(self.get(LISTS) + i * 12) == STRINGS + 3 * 64 for i in range(self.get(LISTS + 4))),
                        self.get(0x100f95b3, 1), self.cache_count]
        calls = len(self.events) // 8
        if self.values[6] and calls == self.values[6]:
            self.failed = True; self.m.emu_stop(); return
        if self.values[7] != 255: self.put(0x3abdf68, self.values[7], 1)
    def hook(self, m, at, size, user):
        sp = m.reg_read(UC_X86_REG_ESP)
        if at == STRCMP:
            self.ret(int(self.string(self.get(sp + 4)) != self.string(self.get(sp + 8))), 8)
        elif at == MIP: self.ret(0)
        elif at == 0x100c09d1:
            dest, value, count = [self.get(sp + i) for i in (4, 8, 12)]
            m.mem_write(dest, bytes([value & 255]) * count); self.ret(dest)
        elif at == 0x100c0ead:
            dest, src, count = [self.get(sp + i) for i in (4, 8, 12)]
            m.mem_write(dest, bytes(m.mem_read(src, count))); self.ret(dest)
        elif at == 0x100bfce0:
            result = self.alloc; self.alloc += (self.get(sp + 4) + 15) & ~15; self.ret(result)
        elif at in (0x100aba90, 0x100aba50):
            self.cache_count -= 1; self.ret()
        elif at == 0x10037170: self.ret(pop=4)
        elif at == 0x100a9910:
            # Qualified raw asset path: run the actual compiled cache callee and
            # exact source resync store, bypassing replacement lookup resources.
            self.raw_fixture = True
            self.put(sp - 4, CACHE_RETURN); m.reg_write(UC_X86_REG_ESP, sp - 4)
            m.reg_write(UC_X86_REG_EIP, 0x100a75e0)
        elif at == CACHE_RETURN: m.reg_write(UC_X86_REG_EIP, 0x100a9a30)
        elif at == 0x100a9a35 and self.raw_fixture: m.reg_write(UC_X86_REG_EIP, 0x100a9a40)
        elif at == 0x100a9a43 and self.raw_fixture:
            self.raw_fixture = False; self.ret(0)
        elif at == 0x77feb0:
            name = self.string(self.get(sp + 4)); target = self.get(sp + 8)
            self.event(NAMES_TEXT.index(name) + 10)
            if self.failed: return
            self.put(target + 4, self.values[4])
            for index in range(self.values[4]): self.put(self.get(target) + index * 12 + 8, MEMORIES + (100 + index) * 4)
            self.ret(self.values[5])
        elif at == 0x100ab2f0:
            self.event(2, self.get(sp + 4))
            if not self.failed: self.ret(self.values[5])
        elif at == 0x401000:
            self.event(3 if self.string(self.get(sp + 4)) == 'SS_MIZUGI\n' else 4)
            if not self.failed: self.ret()

    def run(self, values):
        self.values = values; m = self.m; m.context_restore(self.context)
        self.events = []; self.failed = False; self.cache_count = 2
        self.alloc = BASE + 0x18000; self.raw_fixture = False
        self.put(0, 0); self.put(0x3abdf68, values[2], 1); self.put(0x3b0f0e8, values[3])
        self.put(0x100f95b3, int(bool(values[15])), 1)
        self.put(0x90ed78, 0); self.put(0x90ed7c, STRINGS + 64); self.put(0x90ed80, STRINGS + 192)
        for index, text in enumerate(NAMES_TEXT): m.mem_write(STRINGS + index * 64, text.encode() + b'\0')
        for number in range(4):
            self.put(LISTS + number * 8, NAMES + number * 0x1000); self.put(LISTS + number * 8 + 4, values[8 + number])
            for index in range(values[8 + number]):
                code = values[32 + index] if number == 0 else 2
                filename = STRINGS + code * 64 if code else 0
                # Distinct original pointer identities even for equal strings.
                if code:
                    filename = BASE + 0xd000 + number * 0x1000 + index * 32
                    m.mem_write(filename, NAMES_TEXT[code].encode() + b'\0')
                self.put(NAMES + number * 0x1000 + index * 12, filename)
                self.put(NAMES + number * 0x1000 + index * 12 + 4, values[128 + index] if number == 0 else 0)
                self.put(NAMES + number * 0x1000 + index * 12 + 8, MEMORIES + (index % 10 + 1) * 4)
        self.put(0x3b290b0, LISTS + 24)
        for index in range(32): self.put(0x3b29030 + index * 4, LISTS + 24)
        self.put(GROUP, 0 if values[13] & 2 else STRINGS + 256)
        self.put(GROUP + 4, 0 if values[13] & 1 else LISTS)
        for index in range(values[14]):
            self.put(GROUP + 8 + index * 8, STRINGS + (5 if index % 2 else 4) * 64)
            self.put(GROUP + 12 + index * 8, LISTS + (index % 3 + 1) * 8)
        self.put(GROUP + 8 + values[14] * 8, 0); self.put(GROUP + 12 + values[14] * 8, 0)
        for index, pointer in enumerate([GROUP, GROUP, 0]): self.put(0x90ec70 + index * 4, pointer)
        for index in range(300):
            self.put(0x3b28220 + index * 12 + 4, values[256 + index])
        self.put(STACK, STOP)
        op = values[0]; entry = [0x421180, 0x420d80, 0x4228e0, 0x4212e0, 0x100a75e0][op]
        if op == 0:
            self.put(STACK + 4, STRINGS + values[12] * 64); self.put(STACK + 8, 0 if values[13] & 1 else LISTS)
        elif op in (1, 2): self.put(STACK + 4, LISTS)
        elif op == 3: self.put(STACK + 4, values[1])
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.emu_start(entry, STOP, count=200000)
        if not self.failed and m.reg_read(UC_X86_REG_EIP) != STOP: raise RuntimeError('source failed to return')
        out = [0] * 2048
        out[:6] = [int(self.failed), self.get(0x3abdf68, 1), self.get(0x100f95b3, 1), self.cache_count,
                   self.list_id(self.get(0x3b290b0)), len(self.events) // 8]
        if op == 2 and not self.failed: out[7] = m.reg_read(UC_X86_REG_EAX)
        for index in range(32): out[16 + index] = self.list_id(self.get(0x3b29030 + index * 4))
        for number in range(4):
            count = self.get(LISTS + number * 8 + 4); names = self.get(LISTS + number * 8)
            out[48 + number] = count
            for index in range(count):
                filename, attr, memory = struct.unpack('<3I', m.mem_read(names + index * 12, 12))
                out[64 + number * 300 + index * 3:67 + number * 300 + index * 3] = [9000 if filename == STRINGS + 192 else index + 1 if filename else 0, attr, (memory - MEMORIES) // 4 if memory else 0]
        out[1600:1600 + len(self.events)] = self.events
        return out

def qualify_initial_palette(source):
    m = source.m
    m.context_restore(source.context)
    m.reg_write(UC_X86_REG_ESP, STACK)
    # Execute the actual initialization prefix through all128 records. Later
    # initialization concerns texture bytes and device state, not this table.
    m.emu_start(0x78d0d0, 0x78d109, count=2000)
    records = [struct.unpack('<3I', m.mem_read(0x3d11f80 + i * 16, 12)) for i in range(128)]
    assert records == [(0xffffffff,i,0) for i in range(128)]
    selector = source.get(0x3cfc000)
    assert selector == 0 and source.get(0x3d128a8) == 0
    writes = []
    handle = m.hook_add(unicorn.UC_HOOK_MEM_WRITE, lambda machine,access,address,size,value,user: writes.append(value), begin=0x3d128a8,end=0x3d128ab)
    source.put(STACK,STOP); m.reg_write(UC_X86_REG_ESP,STACK); m.reg_write(UC_X86_REG_ECX,selector)
    m.emu_start(0x78cf80,STOP,count=1000)
    m.hook_del(handle)
    assert not writes and source.get(0x3d128a8) == 0
    return {'initial_records':128,'identities':-1,'ordinal_banks':True,'initial_palette':0,
            'initial_texture_palette_selector':selector,'null_selector_writes':len(writes),
            'scope':'No indexed palette registrations in qualified16bit raw native backend.'}

def fixtures(seed, count):
    rng = random.Random(seed); result = []
    for number in range(count):
        v = [0] * 768; v[0] = number % 5; v[1] = rng.randrange(3)
        v[2:16] = [rng.choice([0,1,255]), rng.choice([0,1,2,0xffffffff]), rng.randrange(12), rng.choice([0,1,0xffffffff]), rng.choice([0,0,0,1,2,5]), rng.choice([0,1,255]), rng.randrange(1,15), rng.randrange(1,4), rng.randrange(1,4), rng.randrange(1,4), rng.choice([4,5,6]), rng.randrange(2), rng.randrange(5), rng.randrange(2)]
        for index in range(96): v[32 + index] = rng.choice([1,2,3]); v[128 + index] = rng.getrandbits(32)
        for index in range(300): v[256 + index] = rng.getrandbits(32)
        result.append(v)
    for op, amount in [(1,96),(1,64),(1,63),(0,0),(0,1),(0,12),(3,32)]:
        v = [0] * 768; v[0] = op; v[2] = 0; v[7] = 255
        v[8:12] = [amount if op in (0,1) else 4,1,1,1]
        v[4] = 5 if op == 0 else 0; v[12] = 6; v[14] = amount if op == 3 else 0; v[15] = 1
        v[32:128] = [1] * 96; result.append(v)
    # Count nonmatching entries separately from the 63 changed-name capacity.
    v = result[-7].copy(); v[32:62] = [2] * 30; result.append(v)
    # Null source filenames are valid when no localization rule matches language.
    v = result[-1].copy(); v[3] = 1; v[32:128] = [0] * 96; result.append(v)
    return result

def main():
    parser = argparse.ArgumentParser(); parser.add_argument('exe', type=Path); parser.add_argument('probe', type=Path)
    parser.add_argument('--loader-dll', type=Path, default=Path('reference/updated-pc-reference/game/mods/.modloader/SADXModLoader.dll'))
    parser.add_argument('--output', required=True, type=Path); parser.add_argument('--seed', type=int, default=20261102); parser.add_argument('--cases', type=int, default=600)
    args = parser.parse_args(); image = PEImage(args.exe); loader = PEImage(args.loader_dll); source = Source(image, loader)
    lib = ctypes.CDLL(str(args.probe.resolve())); probe = lib.sadx_native_regular_textures; U = ctypes.c_uint32; probe.argtypes = [ctypes.POINTER(U), ctypes.POINTER(U)]
    palette_proof = qualify_initial_palette(source)
    bad = []; cases = fixtures(args.seed, args.cases)
    for number, values in enumerate(cases):
        expected = source.run(values); out = (U * 2048)(); assert probe((U * 768)(*values), out) == 1
        differences = [(i, x, y) for i, (x,y) in enumerate(zip(expected, out)) if x != y]
        if differences: bad.append({'case':number, 'input':values, 'differences':differences[:20]})
    report = {'source_sha256': image.sha256, 'loader_sha256':loader.sha256, 'probe_sha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(), 'seed':args.seed, 'cases':len(cases), 'mismatch_count':len(bad), 'mismatches':bad[:8], 'scope':__doc__, 'palette_initial_miss_proof':palette_proof, 'source_ranges':{f'{hex(a)}-{hex(b)}':hashlib.sha256((loader if a >= 0x10000000 else image).read(a,b-a)).hexdigest() for a,b in [(0x420d80,0x420e81),(0x4212e0,0x4213a0),(0x421180,0x421185),(0x4228e0,0x4228e5),(0x100a75e0,0x100a7665),(0x100a9cf0,0x100a9db8),(0x100a9a30,0x100a9a35),(0x100a9a40,0x100a9a43),(0x788260,0x788282),(0x78d0d0,0x78d109),(0x78cf80,0x78cfa0),(0x100a9dc0,0x100a9f1f),(0x100ab2d0,0x100ab2e6)]}, 'native_sources':{p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in ['src/game/regular_textures.cpp','src/game/regular_textures.hpp','src/engine/native_texture_lifetime.hpp','tests/probes/native_regular_textures_probe.cpp']}}
    args.output.parent.mkdir(parents=True, exist_ok=True); args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'cases':len(cases),'mismatch_count':len(bad)})); return int(bool(bad))
if __name__ == '__main__': raise SystemExit(main())
