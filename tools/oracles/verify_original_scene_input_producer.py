"""40fdc0 scene input producer (with actual 4146b0 and 40f070) against original instructions.

The 40f170 analog producer is an explicit owner fixture here; WriteAnalogs and
the Input2.6.1 hook have their own reports. Raw controller records hold
deterministic fixture bytes; observed null ControllerPointers fault in the
source copy and return early in 40f070. Unknown native facts and physical
controller delivery are outside this comparison.
"""
import argparse,ctypes,hashlib,json,pathlib,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
RAW=0x20000000
def sha(b):return hashlib.sha256(b).hexdigest()
def pattern(seed,tag,index):return (seed*31+tag*17+index*7)&0xff
def pattern32(seed,tag,index):return sum(pattern(seed,tag,index*4+b)<<(8*b) for b in range(4))
def source_run(image,case):
    m=Original(image,0x7f).machine
    for page in [0x909000,0x3b0e000,0x3b22000,RAW]:m.mem_map(page,0x1000)
    put=lambda a,v:m.mem_write(a,struct.pack('<I',v&0xffffffff))
    get=lambda a:struct.unpack('<I',m.mem_read(a,4))[0]
    seed=case[6]
    m.mem_write(0x3b22dcc,struct.pack('<H',case[0]&0xffff));m.mem_write(0x909fb0,bytes([case[1]&255]));m.mem_write(0x909fb4,bytes([case[2]&255,case[3]&255,0,0]))
    for i in range(4):
        if case[4]>>i&1:put(0x3b0e77c+4*i,0);continue
        base=RAW+0x40*i;put(0x3b0e77c+4*i,base);m.mem_write(base,bytes(pattern(seed,1+i,b) for b in range(52)))
        put(base+0x1c,case[7+i]);put(base+8,case[11+i]);put(base+0x10,case[15+i])
    for slot in range(8):m.mem_write(0x3b0e7f0+0x34*slot,bytes(pattern(seed,8+slot,b) for b in range(52)))
    put(0x909f94,pattern32(seed,20,0));put(0x909fa4,pattern32(seed,20,1))
    for i in range(4):
        put(0x3b0e3a8+4*i,case[19+i]);put(0x3b0e3bc+4*i,case[23+i]);put(0x3b0e354+4*i,case[27+i])
        put(0x3b0e7e0+4*i,pattern32(seed,21,i));put(0x3b0e3cc+4*i,pattern32(seed,22,i))
    failure=boundary=nested=calls=0
    def hook(machine,address,size,user):
        nonlocal failure,boundary,nested,calls
        if address!=0x40f170:return
        calls+=1;esp=machine.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',machine.mem_read(esp,4))[0]
        if case[5]:failure,boundary,nested=11,ret-5,0x40f170;machine.emu_stop();return
        machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,ret)
    m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=0x40f170,end=0x40f170);put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
    try:m.emu_start(0x40fdc0,STOP,count=20000)
    except unicorn.UcError as error:
        if error.errno!=unicorn.UC_ERR_READ_UNMAPPED:raise
        failure,boundary=2,m.reg_read(UC_X86_REG_EIP)
    if not failure and (m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+4):raise RuntimeError('Source return ABI')
    out=[failure,boundary,nested,calls,get(0x909f94),get(0x909fa4)]
    for base in [0x3b0e3a8,0x3b0e354,0x3b0e3bc,0x3b0e7e0,0x3b0e3cc]:out+=[get(base+4*i) for i in range(4)]
    for slot in range(8):out+=[get(0x3b0e7f0+0x34*slot+4*w) for w in range(13)]
    return out
def stick(x,y):return (x&0xffff)|((y&0xffff)<<16)
def fixtures():
    cases=[];seed=1
    sticks=[stick(0,0),stick(78,-78),stick(79,-79),stick(-79,79),stick(0x7fff,-0x8000),stick(-78,78)]
    for stage in [0,0x26,0x27,0x28,0x29,0x2a,0x2b,0x127]:
     for control in [0,1,2,0xff]:
      for enabled in [(1,1),(0,1),(1,0),(2,1),(1,0xff)]:
       for nulls in [0,1,2,4,0xc]:
        for analog in [0,1]:
         seed+=1;s=sticks[seed%len(sticks)]
         cases.append([stage,control,enabled[0],enabled[1],nulls,analog,seed,s,sticks[(seed+1)%6],sticks[(seed+2)%6],sticks[(seed+3)%6],
                       0,0x400,seed*0x101,0xffffffff,0,0x20,0x80,seed,0,0x10,0xf0,0xffffffff,0,30,31,0x7fffffff,seed*3,0,0x55,0xffffffff,0])
    for repeat in [-1,0,29,30,31,36,0x7fffffff,-0x80000000]:
     for held in [0,0x10,0xf0,0x30]:
      for prior in [0,0x20,0xf0]:
       for pressed in [0,0x10,0x100]:
        seed+=1;r=repeat&0xffffffff
        cases.append([0x1a,1,1,1,0,0,seed,stick(0,79),stick(-79,0),stick(0,0),stick(79,-79),held,held,0,held,pressed,0,pressed,0,prior,prior,prior,prior,r,r,r,r,0,0,0,0,0])
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_scene_input_producer;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    cases=fixtures();bad=[];outcomes={}
    for number,case in enumerate(cases):
        expected=source_run(image,case);out=(ctypes.c_uint32*130)()
        if fn((ctypes.c_uint32*32)(*case),32,out,130)!=1:raise RuntimeError('Native fixture rejected')
        outcomes[expected[0]]=outcomes.get(expected[0],0)+1
        if list(out)!=expected:bad.append({'case':number,'fields':[i for i,(x,y) in enumerate(zip(out,expected)) if x!=y],'source':expected,'native':list(out)})
    blob=b''.join(struct.pack('<32I',*c) for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/game/game_controller.hpp','src/game/game_controller.cpp','src/input/scene_input_producer.hpp','src/input/scene_input_producer.cpp','tests/probes/native_scene_input_producer_probe.cpp','tools/oracles/verify_original_scene_input_producer.py']
    report={'source_sha256':image.sha256,'native_library_sha256':sha(a.native_library.read_bytes()),'source_files':{n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},
            'original_ranges_sha256':{'40fdc0':sha(image.read(0x40fdc0,0x141)),'40f070':sha(image.read(0x40f070,0xfb)),'4146b0':sha(image.read(0x4146b0,0x27))},
            'cases':len(cases),'case_sha256':sha(blob),'source_outcomes':{str(k):v for k,v in sorted(outcomes.items())},'mismatches':bad[:50],'mismatch_count':len(bad),
            'stores_order_and_boundaries_match':not bad,'limits':__doc__}
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':len(cases),'mismatches':len(bad),'outcomes':report['source_outcomes']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
