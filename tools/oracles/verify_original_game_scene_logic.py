"""413d40 scene orchestration against original instructions.

Input/context/collision/registry/query callees are explicit owner fixtures. The
source40b170 task body executes with an explicitly empty eight-list fixture;
native task-order/mutation checks cover nonempty lists separately. This does
not establish full scene execution, physical input or hardware FP equivalence.
"""
import argparse,ctypes,hashlib,json,pathlib,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
CALLEES={0x40fdc0,0x4014b0,0x5919c0,0x420700,0x456ec0,0x4511d0,0x42fb00,0x426070}
def sha(b):return hashlib.sha256(b).hexdigest()
def source_run(image,case):
    original=Original(image,0x7f);m=original.machine
    for page in [0x3abd000,0x3b22000,0x3b0e000,0x3b0f000]:m.mem_map(page,0x1000)
    put=lambda address,value:m.mem_write(address,struct.pack('<I',value&0xffffffff))
    put(0x3abdc7c,case[0]);m.mem_write(0x3b22dcc,struct.pack('<H',case[1]&0xffff))
    m.mem_write(0x3b0ef40,bytes([case[2]&255]));m.mem_write(0x3b0f150,bytes([case[3]&255]))
    calls=[];failure=boundary=nested=0;tasks=0
    def hook(machine,address,size,user):
        nonlocal failure,boundary,nested,tasks
        if address==0x413d5d:tasks=1
        if address not in CALLEES:return
        calls.append(address);esp=machine.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',machine.mem_read(esp,4))[0]
        if len(calls)==case[5]:failure=4;boundary=ret-5;nested=address;machine.emu_stop();return
        if address==0x42fb00:machine.reg_write(UC_X86_REG_EAX,case[4])
        if address==0x426070 and case[6]:machine.mem_write(0x3b22dcc,struct.pack('<H',case[7]&0xffff))
        machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,ret)
    m.hook_add(unicorn.UC_HOOK_CODE,hook);put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x413d40,STOP,count=10000)
    if not failure and (m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+4):raise RuntimeError('Source return ABI')
    out=[failure,boundary,nested,tasks,int(not failure),m.mem_read(0x3b0ef40,1)[0],m.mem_read(0x3b0f150,1)[0],int.from_bytes(m.mem_read(0x3b22dcc,2),'little'),len(calls)]+calls
    return out+[0]*(32-len(out))
def fixtures():
    cases=[]
    for mode in [0,9,10,0xffffffff]:
      for stage in [0,14,15,25,26,34,35,38,39,0xffff,0x8000]:
       for query in [0,1,0xffffffff]:
        for change in [0,1]:
         for late_stage in [0,20,35]:cases.append([mode,stage,7,8,query,0,change,late_stage])
    for stop in range(1,9):cases.append([10,15,7,8,0,stop,1,35])
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_scene_logic;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    cases=fixtures();bad=[]
    for number,case in enumerate(cases):
        expected=source_run(image,case);out=(ctypes.c_uint32*32)()
        if fn((ctypes.c_uint32*8)(*case),8,out,32)!=1:raise RuntimeError('Native fixture rejected')
        if list(out)!=expected:bad.append({'case':number,'fields':[i for i,(x,y) in enumerate(zip(out,expected)) if x!=y],'source':expected,'native':list(out)})
    blob=b''.join(struct.pack('<8I',*c) for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/game/game_scene_logic.hpp','src/game/game_scene_logic.cpp','src/game/task.hpp','src/game/task.cpp','tests/probes/native_game_scene_logic_probe.cpp','tools/oracles/verify_original_game_scene_logic.py']
    report={'source_sha256':image.sha256,'native_library_sha256':sha(a.native_library.read_bytes()),'source_files':{n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},'original_range_sha256':sha(image.read(0x413d40,142)),'cases':len(cases),'case_sha256':sha(blob),'mismatches':bad,'ordering_stores_and_stopped_boundaries_match':not bad,'limits':__doc__};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':len(cases),'mismatches':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
