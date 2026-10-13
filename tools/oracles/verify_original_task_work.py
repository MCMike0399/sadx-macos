"""Original arena allocator/free instructions against the native owner.

Explicit heap fixture starts at40b1ce after unlifted CRT/Win32 heap acquisition.
Executes full REP STOSD initialization and actual40b220/40b310, no overlays.
Native size-wrap/double-free failures are distinct unit tests, not source hangs.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
ARENA=0x20000000;SIZE=0x1000020;GLOBALS=0x3abd000
def sha(v):return hashlib.sha256(v).hexdigest()
def call(m,address,args):
    m.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(address,STOP,count=5000000)
    if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError(f'ABI failed at{address:x}')
    return m.reg_read(UC_X86_REG_EAX)
def fixtures():
    out=[[(0,1),(0,56),(0,64),(1,1),(1,0),(0,80),(1,3),(1,2)],
         [(0,0),(0,SIZE-64),(0,1),(1,1)],
         [(0,32),(0,32),(0,32),(1,0),(1,2),(1,1)],
         [(0,64),(0,64),(0,64),(1,1),(1,2),(1,0)]]
    rng=random.Random(0x40b22040b310)
    for _ in range(12):
        ops=[];live=[];next_index=0
        for __ in range(300):
            if live and rng.randrange(3)==0:idx=rng.choice(live);live.remove(idx);ops.append((1,idx))
            else:ops.append((0,rng.choice([1,16,31,32,33,56,64,80,96,512,4096,0x34000])));live.append(next_index);next_index+=1
        rng.shuffle(live);ops.extend((1,i)for i in live);out.append(ops)
    return out
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('RequiresUnicorn2.1.4')
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Sourcechanged')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_task_work;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    cases=fixtures();blob=b''.join(struct.pack('<I',len(c))+b''.join(struct.pack('<2I',*v)for v in c)for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),case_blob_sha256=sha(blob),source_ranges={hex(x):dict(size=y-x,sha256=sha(image.read(x,y-x)))for x,y in ((0x40b1ce,0x40b211),(0x40b220,0x40b2cc),(0x40b310,0x40b41e))},sequences=[],limits=['StartsafterCRTandWin32heapacquisitionwithanexplicitzerofilledheapfixture','Onlyvalidownedallocation/free sequences; sourcehang/corruption failures have native unit coverage','Original hardware/fullgame startup and mod runtime remain unverified'])
    for number,ops in enumerate(cases):
        original=Original(image,0x27f);m=original.machine;m.mem_map(ARENA,0x1001000);m.mem_map(GLOBALS,0x1000);m.reg_write(UC_X86_REG_EDX,ARENA);call(m,0x40b1ce,[])
        allocations=[];expected=[]
        for op,value in ops:
            if op==0:ret=call(m,0x40b220,[value]);allocations.append(ret)
            else:call(m,0x40b310,[allocations[value]]);ret=0
            head=struct.unpack('<I',m.mem_read(0x3abdbe8,4))[0];expected.extend((ret,0 if ret or op==1 else 1,head))
        commands=[v for pair in ops for v in pair];memory=(ctypes.c_ubyte*SIZE)();trace=(ctypes.c_uint32*len(expected))();status=fn((ctypes.c_uint32*len(commands))(*commands),len(commands),memory,SIZE,trace,len(expected))
        if status:raise RuntimeError(f'Nativeprobe status{status}')
        actual_bytes=bytes(memory);expected_bytes=bytes(m.mem_read(ARENA,SIZE));trace_diffs=[i for i,(x,y)in enumerate(zip(trace,expected))if x!=y];byte_diffs=[i for i,(x,y)in enumerate(zip(actual_bytes,expected_bytes))if x!=y]
        item=dict(sequence=number,operations=len(ops),trace_mismatches=len(trace_diffs),byte_mismatches=len(byte_diffs),first_trace_indices=trace_diffs[:12],first_byte_offsets=[hex(i)for i in byte_diffs[:24]],original_arena_sha256=sha(expected_bytes),native_arena_sha256=sha(actual_bytes));report['sequences'].append(item);print(json.dumps(item),flush=True)
    report['values_links_and_bytes_match']=all(not s['trace_mismatches']and not s['byte_mismatches']for s in report['sequences']);a.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['values_links_and_bytes_match'])
if __name__=='__main__':raise SystemExit(main())
