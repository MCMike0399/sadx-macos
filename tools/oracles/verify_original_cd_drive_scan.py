"""WinMain's CD-drive scan 401a20, with the real 401990 disc check and the real
njMalloc 780f60 / njFree 780fb0, against original instructions.

Win32 imports are IAT fixtures with the same rules the native host owners use:
GetLogicalDriveStringsA returns the required size (with the final NUL) when the
buffer is too small and otherwise writes the NUL-separated list and returns its
length without the final NUL; GetDriveTypeA, FindFirstFileA/FindClose answer
from the case's drives; wsprintfA formats "%s"; lstrcpyA copies; HeapCreate
returns the GuestHeap handle token and HeapAlloc follows GuestHeap's identity
scheme (first 0x50000000, 8-byte rounded spans plus an 8-byte gap) or fails on
request. Compared: completion, 389d628, the 0x104 bytes of 3a8af78, the ordered
drive-type and file queries, and the ordered heap calls (create, alloc size and
result, free address). Not covered: the reference machine's actual drives.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
IAT={0x7db048:8,0x7db04c:4,0x7db050:8,0x7db228:0,0x7db06c:8,0x7db1d4:4,0x7db078:0xc,0x7db074:0xc,0x7db0f8:0xc}
STUB={slot:CODE+0x400+0x10*i for i,slot in enumerate(IAT)};HEAP=0x50000000;HANDLE=0x00510000
def cstr(m,a):
    out=b''
    while True:
        c=bytes(m.mem_read(a+len(out),1))
        if c==b'\0':return out.decode('latin-1')
        out+=c
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x27f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    m.mem_map(HEAP,0x100000)
    for slot,stub in STUB.items():
        pop=IAT[slot];m.mem_write(stub,b'\xc3' if pop==0 else b'\xc2'+struct.pack('<H',pop));m.mem_write(slot,struct.pack('<I',stub))
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP);slot=[s_ for s_,v in STUB.items() if v==address][0];c=st['case'];drives=st['drives'];eax=0
        if slot==0x7db048:
            size_,buf=word(mu,esp+4),word(mu,esp+8);blob=b''.join(d.encode()+b'\0' for d in drives)+b'\0'
            if size_<len(blob):eax=len(blob)
            else:mu.mem_write(buf,blob);eax=len(blob)-1
        elif slot==0x7db04c:
            d=cstr(mu,word(mu,esp+4));st['text'].append('D|'+d);eax=c[3+drives.index(d)] if d in drives else 1
        elif slot==0x7db050:dst,src=word(mu,esp+4),word(mu,esp+8);v=cstr(mu,src);mu.mem_write(dst,v.encode('latin-1')+b'\0');eax=dst
        elif slot==0x7db228:buf,fmt,arg=word(mu,esp+4),cstr(mu,word(mu,esp+8)),cstr(mu,word(mu,esp+12));v=fmt.replace('%s',arg);mu.mem_write(buf,v.encode('latin-1')+b'\0');eax=len(v)
        elif slot==0x7db06c:
            pat=cstr(mu,word(mu,esp+4));st['text'].append('F|'+pat)
            hit=[i for i,d in enumerate(drives) if pat.startswith(d)];eax=0x1234 if hit and c[11+hit[0]] else 0xffffffff
        elif slot==0x7db1d4:eax=1
        elif slot==0x7db078:st['heap'].append(f'H|C|{HANDLE}|0');eax=HANDLE
        elif slot==0x7db074:
            size_=word(mu,esp+12)
            if c[1]:st['heap'].append(f'H|A|{size_}|0');eax=0
            else:addr=st['next'];span=(max(size_,1)+7)&~7;st['next']+=span+8;st['heap'].append(f'H|A|{size_}|{addr}');eax=addr
        elif slot==0x7db0f8:st['heap'].append(f'H|F|{word(mu,esp+12)}|0');eax=1
        mu.reg_write(UC_X86_REG_EAX,eax&0xffffffff)
    for stub in STUB.values():m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=stub,end=stub)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_cd_drive_scan
    fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_char_p,ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.c_char_p,ctypes.c_size_t];fn.restype=ctypes.c_int
    rng=random.Random(0x401a20);bad=[];outcomes={};cases=0
    letters='CDEFGHIJ'
    for n in range(1500):
        k=rng.choice([0,1,1,2,3,5,8]);drives=[f'{x}:\\' for x in rng.sample(letters,k)]
        if rng.random()<0.1 and drives:drives[0]='\\\\server\\share\\'
        c=[0]*20;c[0]=rng.choice([0xffffffff,0xffffffff,HANDLE]);c[1]=int(rng.random()<0.1);c[2]=len(drives)
        for i in range(8):c[3+i]=rng.choice([3,3,5,5,2,4,1,0]);c[11+i]=int(rng.random()<0.5)
        root=bytes(rng.getrandbits(8) for _ in range(0x104))
        out=(ctypes.c_uint32*16)();root_out=(ctypes.c_uint8*0x104)();text=ctypes.create_string_buffer(8192)
        blob=b''.join(d.encode()+b'\0' for d in drives)+b'\0'
        if fn((ctypes.c_uint32*20)(*c),blob,(ctypes.c_uint8*0x104).from_buffer_copy(root),out,root_out,text,len(text))!=1:raise RuntimeError('Native rejected')
        got_text=[x for x in text.value.decode('latin-1').split('\n') if x]
        m.context_restore(clean);st.update(case=c,drives=drives,text=[],heap=[],next=HEAP)
        m.mem_write(0x389d628,struct.pack('<I',c[0]));m.mem_write(0x3a8af78,root)
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x401a20,STOP,count=200000)
        except unicorn.UcError as err:raise RuntimeError(f'case {n}: {err} at {m.reg_read(UC_X86_REG_EIP):#x}')
        if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        source_text=st['text']+st['heap'];source_root=bytes(m.mem_read(0x3a8af78,0x104));source_heap=word(m,0x389d628)
        live=sum(1 for h in st['heap'] if h.startswith('H|A|') and not h.endswith('|0'))-sum(1 for h in st['heap'] if h.startswith('H|F|'))
        cases+=1;found=source_root[0]!=0;key=f"drives{len(drives)}:found{int(found)}:heapfail{c[1]}";outcomes[key]=outcomes.get(key,0)+1
        problems=[]
        if out[0]!=1:problems.append('native incomplete')
        if out[1]!=source_heap:problems.append(f'389d628 {out[1]:#x} vs {source_heap:#x}')
        if bytes(root_out)!=source_root:problems.append('3a8af78')
        if got_text!=source_text:problems.append('queries/heap trace')
        if out[2]!=live:problems.append(f'live blocks {out[2]} vs {live}')
        if problems:bad.append(dict(case=n,drives=drives,problems=problems,native=got_text[:8],source=source_text[:8]))
    names=['src/game/game_winmain.hpp','src/game/game_winmain.cpp','src/ninja/ninja_memory.hpp','src/ninja/ninja_memory.cpp','src/game/guest_heap.hpp','src/game/guest_heap.cpp','tests/probes/native_cd_drive_scan_probe.cpp','tools/oracles/verify_original_cd_drive_scan.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={hex(x):sha(image.read(x,y)) for x,y in((0x401a20,0x9e),(0x401990,0x8c),(0x780f60,0x2d),(0x780fb0,0x18))},
        cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
