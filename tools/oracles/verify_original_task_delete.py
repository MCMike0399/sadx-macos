"""DeleteAllObjects40b730 original-instruction comparison; old task judges are unchanged."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_ESI
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
from verify_original_task_display import POOL,CODE,RESOURCE,CONTINUE,CALLBACKS,CALLBACK_IDS
class Source:
    def __init__(self, image):
        self.m = m = Original(image, 127).machine
        for section in image.sections:
            if section['name'] in ('.text', '.rdata'):
                continue
            size = (max(section['virtual_size'], section['raw_size']) + 4095) & ~4095
            m.mem_map(image.base + section['rva'], size)
            m.mem_write(image.base + section['rva'], image.slice(section['file_offset'], section['raw_size']))
        m.mem_map(POOL, 0x40000)
        m.mem_map(CODE, 0x1000)
        for address in CALLBACKS.values():
            if address >= CODE:
                m.mem_write(address, b'\xc3')
        self.clean = m.context_save()
        for address in (CODE + 0x10, CODE + 0x20, CODE + 0x30, CONTINUE, 0x40b750,
                        0x40b510, 0x40b140, 0x401000):
            m.hook_add(unicorn.UC_HOOK_CODE, self.hook, begin=address, end=address)

    def get(self, address):
        return int.from_bytes(self.m.mem_read(address, 4), 'little')

    def put(self, address, value):
        self.m.mem_write(address, struct.pack('<I', value & 0xffffffff))

    @staticmethod
    def slot(address):
        return (address - POOL) // 52 + 1 if address else 0

    @staticmethod
    def resource(value):
        return (value - RESOURCE) // 4 if value else 0

    def active(self):
        found = set()
        def visit(address):
            while address:
                if address in found:
                    raise RuntimeError('Fixture graph contains a cycle')
                found.add(address)
                visit(self.get(address + 12))
                address = self.get(address)
        for index in range(8):
            visit(self.get(0x3abdbc4 + index * 4))
        return found

    def event(self, task, code):
        self.trace.append([self.slot(task), code, CALLBACK_IDS.get(self.get(0x3abdbf4), 0),
                           self.resource(self.get(0x3abdbf8))])

    def return_from_receipt(self):
        esp = self.m.reg_read(UC_X86_REG_ESP)
        self.m.reg_write(UC_X86_REG_EIP, self.get(esp))
        self.m.reg_write(UC_X86_REG_ESP, esp + 4)

    def start_helper(self, address, arguments, after):
        esp = self.m.reg_read(UC_X86_REG_ESP)
        self.helper_esp = esp
        self.helper_after = after
        frame = esp - 4 * (len(arguments) + 1)
        self.put(frame, CONTINUE)
        for index, argument in enumerate(arguments):
            self.put(frame + 4 + index * 4, argument)
        self.m.reg_write(UC_X86_REG_ESP, frame)
        self.m.reg_write(UC_X86_REG_EIP, address)

    def hook(self, m, address, size, unused):
        if address == 0x401000:
            raise RuntimeError('Unexpected inactive-task diagnostic')
        if address in (0x40b510, 0x40b140, 0x40b750):
            task = m.reg_read(UC_X86_REG_EAX) if address == 0x40b750 else m.reg_read(UC_X86_REG_ESI)
            if task not in self.active():
                self.failure = 2
                self.boundary = address
                m.emu_stop()
            return
        if address == CONTINUE:
            callback = self.helper_after
            m.reg_write(UC_X86_REG_ESP, self.helper_esp)
            callback(m.reg_read(UC_X86_REG_EAX))
            return
        if address == 0x4128f0:
            self.event(0, 0)
            if self.header[7]:
                self.failure = 1
                m.emu_stop()
            else:
                self.return_from_receipt()
            return
        esp = m.reg_read(UC_X86_REG_ESP)
        task = self.get(esp + 4)
        self.event(task, CALLBACK_IDS[address])
        self.callbacks += 1
        if self.header[3] and self.callbacks == self.header[3]:
            self.failure = 1
            m.emu_stop()
            return
        index = self.slot(task) - 1
        if index in self.consumed:
            self.return_from_receipt()
            return
        self.consumed.add(index)
        row = self.rows[index] if index < len(self.rows) else [0] * 6
        action, target_id = row[3:5]
        target = POOL + (target_id - 1) * 52 if target_id else 0
        live_target = target if target in self.active() else 0
        if action in (10,11) and target == task:
            raise RuntimeError('source reentrant destructor boundary')
        if action == 1 and live_target:
            self.put(target + 16, 0)
        elif action == 2 and live_target:
            self.put(target + 16, CALLBACKS[3])
        elif action in (3, 4):
            arguments = [0, CALLBACKS[3], task] if action == 3 else [0, 8 if target_id % 8 == 7 else target_id % 8, CALLBACKS[3]]
            def created(new_task):
                self.put(new_task + 24, CALLBACKS[2])
                self.return_from_receipt()
            self.start_helper(0x40b940 if action == 3 else 0x40b860, arguments, created)
            return
        elif action in (5, 6):
            self.start_helper(0x40b7e0 if action == 5 else 0x40b570, [task],
                              lambda result: self.return_from_receipt())
            return
        elif action == 7:
            def destroyed(result):
                if target not in self.active():
                    self.return_from_receipt()
                    return
                def recreated(replacement):
                    self.put(replacement + 20, CALLBACKS[2])
                    def child_added(child):
                        self.put(child + 20, CALLBACKS[2])
                        self.return_from_receipt()
                    self.start_helper(0x40b940, [0, CALLBACKS[3], replacement], child_added)
                self.start_helper(0x40b940, [0, CALLBACKS[3], target], recreated)
            self.start_helper(0x40b570, [task], destroyed)
            return
        elif action == 10 and live_target:
            self.start_helper(0x40b570, [target], lambda result: self.return_from_receipt())
            return
        elif action == 11:
            def replacement(unused):
                def created(new_task):
                    self.put(new_task + 24, CALLBACKS[2])
                    self.return_from_receipt()
                self.start_helper(0x40b860, [0,target_id % 7,CALLBACKS[3]], created)
            if live_target:
                self.start_helper(0x40b570,[target],replacement)
            else:
                replacement(0)
            return
        elif action == 8:
            self.put(task + 24, CALLBACKS[2])
        elif action == 9:
            self.failure = 1
            m.emu_stop()
            return
        self.return_from_receipt()

    def run(self, values):
        self.header = values[:8]
        self.rows = [values[index:index + 6] for index in range(8, len(values), 6)]
        self.trace = []
        self.callbacks = 0
        self.failure = 0
        self.boundary = 0
        self.consumed = set()
        m = self.m
        m.context_restore(self.clean)
        memory = bytearray(4096 * 52)
        for index in range(len(self.rows), 4095):
            struct.pack_into('<I', memory, index * 52, POOL + (index + 1) * 52)
        m.mem_write(POOL, bytes(memory))
        m.mem_write(0x3abdbc4, bytes(32))
        self.put(0x3abdbec, POOL + len(self.rows) * 52)
        self.put(0x38f6e08, RESOURCE + 8000 * 4)
        for index, row in enumerate(self.rows):
            task = POOL + index * 52
            level, parent, display, action, target, owner = row
            head_word = POOL + (parent - 1) * 52 + 12 if parent else 0x3abdbc4 + (7 if level == 8 else level) * 4
            head = self.get(head_word)
            self.put(task, head)
            if head:
                self.put(head + 4, task)
            self.put(head_word, task)
            self.put(task + 8, POOL + (parent - 1) * 52 if parent else 0)
            self.put(task + 16, CALLBACKS[3] if display else 0)
            self.put(task + 24, CALLBACKS[1])
            self.put(task + 48, RESOURCE + owner * 4 if owner else 0)
        self.put(0x3abdbf4, CALLBACKS[3] if values[4] else 0)
        self.put(0x3abdbf8, RESOURCE + 9000 * 4 if values[5] else 0)
        self.put(STACK, STOP)
        argument = values[1] if values[0] == 1 else POOL + values[2] * 52
        self.put(STACK + 4, argument)
        m.reg_write(UC_X86_REG_ESP, STACK)
        m.emu_start(0x40b730, STOP, count=300000)
        if not self.failure and m.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError('Original traversal did not return')
        out = [0] * 2048
        live = self.active()
        free_head = self.get(0x3abdbec)
        free_count=0
        cursor=free_head
        while cursor:
            free_count+=1
            if free_count>4096:raise RuntimeError('source free list cycle')
            cursor=self.get(cursor)
        out[:6] = [self.failure, CALLBACK_IDS.get(self.get(0x3abdbf4), 0),
                   self.resource(self.get(0x3abdbf8)), len(self.trace), free_count, self.slot(free_head)]
        out[8:16] = [self.slot(self.get(0x3abdbc4 + index * 4)) for index in range(8)]
        for index in range(64):
            task = POOL + index * 52
            if task not in live:
                continue
            out[16 + index * 9:25 + index * 9] = [1] + [self.slot(self.get(task + offset)) for offset in (0, 4, 8, 12)] + [CALLBACK_IDS.get(self.get(task + offset), 0) for offset in (16, 20, 24)] + [self.resource(self.get(task + 48))]
        out[640:640 + len(self.trace) * 4] = [word for entry in self.trace for word in entry]
        return out


def fixtures(seed,count):
    rng=random.Random(seed);cases=[]
    for action in (0,1,2,4,5,8,9,10,11):
        for enabled in (0,1):
            for fail in (0,1,3):
                rows=[[0,0,enabled,0,0,1],[8,0,1,action,1,2],
                      [0,2,0,0,0,3],[0,3,1,0,0,4],[0,0,1,0,0,5]]
                cases.append([0,0,0,fail,1,1,len(rows),0]+[v for row in rows for v in row])
    for context in range(4):
        rows=[[0,0,1,0,0,1],[0,0,1,11,1,2],[8,0,0,0,0,3]]
        cases.append([0,0,0,0,context&1,context&2,len(rows),0]+[v for row in rows for v in row])
    for _ in range(count):
        rows=[]
        for index in range(rng.randrange(1,17)):
            parent=rng.randrange(index)+1 if index and rng.randrange(3)==0 else 0
            rows.append([rng.choice([0,1,2,3,4,5,6,8]),parent,rng.randrange(2),0,0,index+1])
        index=rng.randrange(len(rows));rows[index][3]=rng.choice([0,1,2,4,5,8,9,10,11]);rows[index][4]=rng.randrange(len(rows))+1
        cases.append([0,0,0,rng.choice([0,0,1,3]),rng.randrange(2),rng.randrange(2),len(rows),0]+[v for row in rows for v in row])
    return cases

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('native_library',type=Path)
    p.add_argument('--output',required=True,type=Path);p.add_argument('--seed',type=int,default=423723);p.add_argument('--cases',type=int,default=192)
    args=p.parse_args();image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise ValueError('requires pinned source')
    source=Source(image);lib=ctypes.CDLL(str(args.native_library.resolve()));native=lib.sadx_native_task_delete
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];native.restype=ctypes.c_int
    cases=fixtures(args.seed,args.cases);bad=[];unsupported=[]
    for index,values in enumerate(cases):
        try:
            expected=source.run(values)
        except RuntimeError as error:
            if str(error) not in ('source free list cycle','source reentrant destructor boundary'): raise
            unsupported.append({'case':index,'input':values,'reason':str(error)})
            continue
        out=(ctypes.c_uint32*2048)()
        if native((ctypes.c_uint32*len(values))(*values),len(values),out,2048)!=1:raise RuntimeError('probe rejected fixture')
        differences=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
        if differences:
            bad.append({'case':index,'input':values,'diff':differences})
            if len(bad)==8:break
    root=Path(__file__).resolve().parents[2];paths=['src/engine/task.hpp','src/engine/task.cpp','tests/probes/native_task_delete_probe.cpp']
    report={'passed':not bad,'values_match':not bad,'cases':len(cases)-len(unsupported),'attempted_cases':len(cases),'mismatches':len(bad),'first_mismatches':bad,'seed':args.seed,'source_unsupported_cases':len(unsupported),'source_unsupported':unsupported,
            'source_sha256':image.sha256,'source_ranges':{'0x40b730-0x40b771':hashlib.sha256(image.read(0x40b730,0x41)).hexdigest()},
            'native_sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},
            'limits':['Source destructors that recursively destroy their own already-unlinked task are outside the lifecycle API contract; those individually listed reentrant boundaries/free-list cycles are not compared or claimed.', 'Freed saved-next source pointers without slot reuse stop at the source exec-load boundary and report unsupported; no success is claimed through source free-list memory.']}
    report['native_library_sha256']=hashlib.sha256(args.native_library.read_bytes()).hexdigest()
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
