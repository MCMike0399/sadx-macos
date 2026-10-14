"""Actual creation/destruction/dispatch instructions and native task graph.

Only the retained default51f680 RET is used as a callback. Heap and field30
resources are explicit fixture owners. No source cleanup callbacks substituted.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EDI
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
from verify_original_task_work import ARENA,SIZE,GLOBALS,call,sha
POOL=ARENA+32
def fixtures():
    out=[[(1,2,0,0),(9,0,0,0),(3,0,0,0)],[(1,15,0,0),(1,2,8,0),(0,7,0,0),(2,2,0,0),(2,1,3,0),(2,8,0,0),(8,0,32,0x7f812345),(2,2,0,0),(4,0,0,0),(5,0,0,0),(3,1,0,0),(0,0,0,0)],
         [(1,0,0,0),(2,0,0,0),(2,0,0,0),(4,1,0,0),(7,0,0,0),(5,0,0,0),(4,2,0,0),(5,0,0,0),(5,0,0,0)],
         [(1,15,0,0),(2,15,0,0),(2,15,1,0),(2,15,2,0),(3,0,0,0)],
         [(0,0,0,0),(0,1,0,0),(0,8,0,0),(3,0,0,0),(0,7,0,0)]]
    out.append([(1,2,0,0),(1,2,0,0),(1,2,1,0),(5,0,0,0),(5,1,0,0),(2,2,0,0),(7,0,0,0),(5,0,0,0),(5,0,0,0)])
    out.append([(1,15,0,0),(10,0,0,0),(2,15,0,0),(10,1,0,0),(3,0,0,0)])
    rng=random.Random(0x40b86040b570)
    for _ in range(12):
        ops=[];live=[];allocation=0
        for __ in range(150):
            if live and rng.randrange(4)==0:
                selected=rng.choice(live);live.remove(selected);ops.append((3,selected,0,0))
            else:
                if live and rng.randrange(3)==0:
                    parent=rng.choice(live);ops.append((2,rng.randrange(16),parent,0))
                    # Child lifetime follows its parent, so random later direct
                    # destruction would need descendant bookkeeping. Keep only
                    # top-level candidates in the native/source live selection.
                else:ops.append((1,rng.randrange(16),rng.choice([0,1,2,3,4,5,6,8]),0));live.append(allocation)
                allocation+=1
        ops.extend((3,i,0,0)for i in live);out.append(ops)
    return out
def original_run(image,ops):
    original=Original(image,0x27f);m=original.machine;m.mem_map(ARENA,0x1001000);m.mem_map(GLOBALS,0x1000);m.mem_map(0x38f6000,0x1000);m.reg_write(UC_X86_REG_EDX,ARENA);call(m,0x40b1ce,[])
    # 40b460 already pushed ESI before the unlifted51a740 call. Reproduce its
    # caller frame, then execute actual post-initializer pool allocation/body.
    m.mem_write(STACK,struct.pack('<2I',0,STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40b466,STOP,count=100000)
    if m.reg_read(UC_X86_REG_ESP)!=STACK+8:raise RuntimeError('Pool initializer ABI')
    m.mem_write(0x38f6e08,struct.pack('<I',0x30000000));allocations=[];trace=[];callbacks=[]
    def observe(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP);callbacks.extend(struct.unpack('<I',mu.mem_read(esp+4,4)));callbacks.extend(struct.unpack('<2I',mu.mem_read(0x3abdbf4,8)));task=struct.unpack('<I',mu.mem_read(esp+4,4))[0];parent=struct.unpack('<I',mu.mem_read(task+8,4))[0];callbacks.append(struct.unpack('<I',mu.mem_read(parent+16,4))[0] if parent else 0)
    m.hook_add(unicorn.UC_HOOK_CODE,observe,begin=0x51f680,end=0x51f680)
    for step,(op,a,b,c) in enumerate(ops):
        ret=0
        if op==0:
            m.reg_write(UC_X86_REG_EDX,a);m.reg_write(UC_X86_REG_EDI,0);ret=call(m,0x40b030,[])
        elif op==1:ret=call(m,0x40b860,[a,b,0])
        elif op==2:ret=call(m,0x40b940,[a,0,allocations[b]])
        elif op==3:
            try:call(m,0x40b570,[allocations[a]])
            except Exception as e:
                from unicorn.x86_const import UC_X86_REG_EIP
                raise RuntimeError(f"Destroy failed step{step} address{allocations[a]:x} pc{m.reg_read(UC_X86_REG_EIP):x} taskwords{struct.unpack(chr(60)+chr(49)+chr(51)+chr(73),m.mem_read(allocations[a],52))}") from e
        elif op==4:call(m,0x40b6c0,[allocations[a]])
        elif op==5:call(m,0x40b0c0,[a])
        elif op==6:call(m,0x40b7e0,[allocations[a]])
        elif op==7:m.mem_write(allocations[a]+16,struct.pack('<I',0x40b7e0))
        elif op==8:
            work=struct.unpack('<I',m.mem_read(allocations[a]+32,4))[0];m.mem_write(work+b,struct.pack('<I',c))
        elif op==10:m.mem_write(allocations[a]+24,struct.pack('<I',0x51f680))
        elif op==9:
            work=struct.unpack('<I',m.mem_read(allocations[a]+32,4))[0]
            blocks=[call(m,0x40b220,[n])for n in (32,16,80,20,20,8,16)]
            collision,nested,obj,first,second,extra,last=blocks
            for pointer,length in ((collision,32),(obj,80),(first,20),(second,20),(last,16)):m.mem_write(pointer,bytes(length))
            m.mem_write(collision+5,b'\x80')
            for at,pointer in ((collision+12,nested),(work+56,collision),(work+60,obj),(first+16,second),(obj+64,first),(obj+68,extra),(obj+76,last)):m.mem_write(at,struct.pack('<I',pointer))
        if op<=2:allocations.append(ret)
        head=struct.unpack('<I',m.mem_read(0x3abdbec,4))[0]
        trace.extend((ret,0,head))
    trace.extend(struct.unpack('<8I',m.mem_read(0x3abdbc4,32)));trace.extend(struct.unpack('<2I',m.mem_read(0x3abdbf4,8)));trace.extend(struct.unpack('<I',m.mem_read(0x3abdbe8,4)))
    return bytes(m.mem_read(ARENA,SIZE)),trace,callbacks
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('RequiresUnicorn2.1.4')
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Sourcechanged')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_task_lifecycle;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    cases=fixtures();blob=b''.join(struct.pack('<I',len(c))+b''.join(struct.pack('<4I',*v)for v in c)for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),case_blob_sha256=sha(blob),source_ranges={hex(x):dict(size=y-x,sha256=sha(image.read(x,y-x)))for x,y in ((0x40b030,0x40b0bf),(0x40b466,0x40b4e7),(0x40b570,0x40b6db),(0x40b7e0,0x40b85c),(0x40b860,0x40b9c4),(0x51f680,0x51f681),(0x41c4e0,0x41c521),(0x4187a0,0x4187cd),(0x417ea0,0x417ee2),(0x418330,0x418369))},sequences=[],limits=['Heap acquisition and51a740 initializer excluded; explicit runtimefield30 owner supplied','Known51f680 RET callback only; arbitrary game callbacks/mods not captured','One sourcecleanup fixture seeds owned collision/object chains explicitly; initialization producers not claimed','Pool native graph is encoded only by the probe canonical test identity scheme; functional native code never casts guest pointers','Inactive membership diagnostic401000/CRT formatting and corruption/hang paths excluded from originalgoldens, retained as native failures','Original hardware/fullgame startup remain unverified'])
    for number,ops in enumerate(cases):
        original_bytes,expected,callback_expected=original_run(image,ops);commands=[v for record in ops for v in record];memory=(ctypes.c_ubyte*SIZE)();trace=(ctypes.c_uint32*len(expected))();status=fn((ctypes.c_uint32*len(commands))(*commands),len(commands),memory,SIZE,trace,len(expected))
        if status:raise RuntimeError(f'Nativeprobe status{status}')
        failure_sites=[]
        for i,op in enumerate(ops):
            if trace[i*3+1]:failure_sites.append(dict(operation=i,opcode=op[0],native_failure=trace[i*3+1]))
        callback_fn=lib.sadx_native_task_callback_trace;callback_fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];callback_fn.restype=ctypes.c_size_t
        callback_out=(ctypes.c_uint32*(len(callback_expected)+16))();callback_count=callback_fn(callback_out,len(callback_out));callback_match=callback_count==len(callback_expected) and list(callback_out)[:callback_count]==callback_expected
        actual_bytes=bytes(memory);trace_diffs=[i for i,(x,y)in enumerate(zip(trace,expected))if x!=y];byte_diffs=[i for i,(x,y)in enumerate(zip(actual_bytes,original_bytes))if x!=y]
        item=dict(callback_contexts_match=callback_match,callback_count=len(callback_expected)//4,sequence=number,operations=len(ops),trace_mismatches=len(trace_diffs),byte_mismatches=len(byte_diffs),expected_boundary_failures=failure_sites,trace_examples=[dict(index=i,original=hex(expected[i]),native=hex(trace[i]))for i in trace_diffs[:12]],byte_examples=[dict(offset=hex(i),original=original_bytes[i],native=actual_bytes[i])for i in byte_diffs[:24]],original_arena_sha256=sha(original_bytes),native_arena_sha256=sha(actual_bytes));report['sequences'].append(item);print(json.dumps(item),flush=True)
    # Keep inactive membership evidence as a strict boundary experiment. Stop
    # on entry to the real401000 logger; its CRT side effects remain unexecuted.
    original=Original(image,0x27f);m=original.machine;m.mem_map(ARENA,0x1001000);m.mem_map(GLOBALS,0x1000);m.mem_map(0x38f6000,0x1000);m.reg_write(UC_X86_REG_EDX,ARENA);call(m,0x40b1ce,[])
    m.mem_write(STACK,struct.pack('<2I',0,STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40b466,STOP,count=100000);m.mem_write(0x38f6e08,struct.pack('<I',0x30000000))
    task=call(m,0x40b860,[2,7,0]);before=bytes(m.mem_read(ARENA,SIZE));m.mem_write(STACK,struct.pack('<2I',STOP,task));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40b570,0x401000,count=100000)
    from unicorn.x86_const import UC_X86_REG_EIP
    ip=m.reg_read(UC_X86_REG_EIP);after=bytes(m.mem_read(ARENA,SIZE));memory=(ctypes.c_ubyte*SIZE)();trace=(ctypes.c_uint32*17)();cmd=(ctypes.c_uint32*8)(1,2,7,0,3,0,0,0)
    if fn(cmd,8,memory,SIZE,trace,17):raise RuntimeError('Inactive native structural probe rejected')
    leaf=lib.sadx_native_task_inactive_boundary;leaf.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];leaf.restype=ctypes.c_int;pending=(ctypes.c_uint32*4)()
    if leaf(pending,4):raise RuntimeError('Inactive native boundary probe rejected')
    boundary_match=ip==0x401000 and before==after and bytes(memory)==after and pending[0]==10 and pending[1]==ip and pending[2]==0 and pending[3]==1
    report['inactive_destroy_boundary']=dict(original_stop=hex(ip),source_arena_unchanged=before==after,native_pending_failure=pending[0],native_source_boundary=hex(pending[1]),native_task_retired=bool(pending[2]),native_work_retained=bool(pending[3]),source_arena_sha256=sha(after),native_arena_sha256=sha(bytes(memory)),state_and_explicit_boundary_match=boundary_match,original_return_observed=False,limits=['Real401000/CRT formatting not executed; no source cleanup or diagnostic success claimed'])
    report['values_links_and_bytes_match']=all(s['callback_contexts_match'] and not s['trace_mismatches']and not s['byte_mismatches']for s in report['sequences']);a.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['values_links_and_bytes_match']or not boundary_match)
if __name__=='__main__':raise SystemExit(main())
