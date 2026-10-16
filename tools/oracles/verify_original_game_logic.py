"""413dd0 instructions versus native orchestration, with explicit owner fixtures.

Called input/scene/timing operations are fixture boundaries in both runs. This
does not establish a complete scene, physical input, keyboard-edge owner or
native game loop. Every call, argument, partial store and press union is kept.
"""
import argparse, ctypes, hashlib, json, pathlib, random, struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP

BASE=0x22000000
CALLEES={0x40ec80:1,0x413920:0,0x40f460:0,0x413d40:0,0x411e30:2}
WORDS=[0x3abdf70,0x3b11198,0x3b11194,0x3b1118c,0x3b1117c,0x3b11178]
def sha(b):return hashlib.sha256(b).hexdigest()
def run_source(image,case):
    original=Original(image,0x7f);m=original.machine
    for page in [0x3abd000,0x3b0e000,0x3b11000]:m.mem_map(page,0x1000)
    m.mem_map(BASE,0x1000)
    put=lambda at,value:m.mem_write(at,struct.pack('<I',value&0xffffffff))
    get=lambda at:struct.unpack('<I',m.mem_read(at,4))[0]
    for address,value in zip(WORDS,[case[5],case[0],case[1],case[6],case[2],case[3]]):put(address,value)
    pointers=[BASE+i*64 for i in range(4)]
    for i,pointer in enumerate(pointers):put(0x3b0e77c+i*4,pointer);put(pointer+16,case[8+i])
    calls=[];polls=0;ticks=0;failure=boundary=nested=0
    def hook(machine,address,size,user):
        nonlocal polls,ticks,failure,boundary,nested
        if address not in CALLEES:return
        esp=machine.reg_read(UC_X86_REG_ESP);argc=CALLEES[address]
        arguments=[get(esp+4+i*4) for i in range(argc)]
        calls.extend([address]+arguments+[0xffffffff]*(2-argc))
        if len(calls)//3==case[7]:
            failure=6;boundary=get(esp)-5;nested=address;machine.emu_stop();return
        if address==0x413920:put(0x3b1117c,case[4]);put(0x3b1118c,case[4])
        elif address==0x40f460:
            polls+=1
            for i in range(4):put(get(0x3b0e77c+i*4)+16,case[12+i])
        elif address==0x413d40:
            ticks+=1
            for i in range(4):put(get(0x3b0e77c+i*4)+16,0)
        elif address==0x411e30 and case[16]:put(0x3b0e77c,BASE+512);put(BASE+528,case[17])
        machine.reg_write(UC_X86_REG_EIP,get(esp));machine.reg_write(UC_X86_REG_ESP,esp+4)
    m.hook_add(unicorn.UC_HOOK_CODE,hook)
    put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x413dd0,STOP,count=10000)
    if not failure and (m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+4):raise RuntimeError('Source routine did not return')
    out=[failure,boundary,nested,ticks]+[get(at) for at in WORDS]+[get(get(0x3b0e77c+i*4)+16) for i in range(4)]+[len(calls)//3,polls]+calls
    return out+[0]*(128-len(out))
def fixtures():
    rng=random.Random(0x413dd0);cases=[]
    for active in [0,1,0xffffffff]:
      for previous in [0,1]:
       for mode in [0,1,2,0xffffffff]:
        for ticks in [0,1,2,3,6,8]:
         cases.append([active,previous,0xabcdef01,mode,ticks,0x87654321,0x76543210,0]+[rng.getrandbits(32) for _ in range(8)]+[rng.randrange(2),rng.getrandbits(32)])
    # Stop at every required callee in the longest fixture. This checks stores
    # and pressed fields preceding a later unavailable native owner.
    for ordinal in range(1,27):cases.append([1,0,9,2,8,77,55,ordinal]+[rng.getrandbits(32) for _ in range(8)]+[1,0x80000000])
    return cases
def main():
    parser=argparse.ArgumentParser();parser.add_argument('exe',type=pathlib.Path);parser.add_argument('native_library',type=pathlib.Path);parser.add_argument('--output',type=pathlib.Path,required=True);args=parser.parse_args()
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    library=ctypes.CDLL(str(args.native_library.resolve()));native=library.sadx_native_game_logic;native.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];native.restype=ctypes.c_int
    cases=fixtures();bad=[]
    for number,case in enumerate(cases):
        expected=run_source(image,case);output=(ctypes.c_uint32*128)()
        if native((ctypes.c_uint32*18)(*case),18,output,128)!=1:raise RuntimeError('Native fixture rejected')
        if list(output)!=expected:bad.append({'case':number,'fields':[i for i,(a,b) in enumerate(zip(output,expected)) if a!=b],'native':list(output),'source':expected})
    blob=b''.join(struct.pack('<18I',*case) for case in cases);args.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parents[2];sources=[root/'src/engine/game_logic_support.hpp',root/'src/engine/game_logic.hpp',root/'src/guest/game_logic.cpp',root/'tests/probes/native_game_logic_probe.cpp',pathlib.Path(__file__)]
    report={'source_sha256':image.sha256,'native_library_sha256':sha(args.native_library.read_bytes()),'native_sources':{str(p):sha(p.read_bytes()) for p in sources},'source_range':{'address':'0x413dd0','bytes':301,'sha256':sha(image.read(0x413dd0,301))},'cases':len(cases),'case_sha256':sha(blob),'mismatches':bad,'orchestration_stores_press_union_calls_match':not bad,'limits':__doc__}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':len(cases),'mismatches':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
