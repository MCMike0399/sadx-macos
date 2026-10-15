"""Actual411e30/7899a0/40f010/411e20 control instructions vs native owners.

Counter words and input bytes are explicit fixtures; no live host clock or
physical keys are inferred. Missing facts stop before their actual source read
as a declared availability fixture. The unused EAX return of the input callback
is excluded: its actual780074 caller immediately overwrites that register.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP

ENTRIES=[0x7899a0,0x411e30,0x40f010,0x411e20]
FIELDS=[(0x3d0853c,4),(0x3d08538,4),(0x389d7dc,4),(0x89295c,4),
        (0x3d084a8,4),(0x3d084ac,4),(0x3d0fa34,4),(0x3b0eaa0,1),
        (0x3b0e3b8,1),(0x3b291a4,4),(0x3b0e493,1),(0x3b11188,4),
        (0x3b11178,4),(0x3b1117c,4)]
READS={0x7899a0:(0,1),0x7899ab:(1,2),0x411e53:(3,3),0x411e72:(3,3),
       0x40f015:(7,4),0x40f01e:(9,5),0x40f037:(10,6),0x40f040:(8,7)}
WRITES={0x7899b1:2,0x7899ca:4,0x7899cf:5,0x77ff54:6,
        0x411e38:11,0x411e3e:12,0x411e44:13}
def sha(data):return hashlib.sha256(data).hexdigest()
def source_run(image,case):
    original=Original(image,0x7f);m=original.machine
    pages={at&~0xfff for at,_ in FIELDS}|{0x3b0e760&~0xfff}
    for page in sorted(pages):m.mem_map(page,0x1000)
    present=case[1];failure=boundary=nested=0
    values=case[4:10]+[0x411e20 if case[17] else 0]+case[10:14]+case[14:17]
    for (at,length),value in zip(FIELDS,values):m.mem_write(at,struct.pack('<I',value)[:length])
    m.mem_write(0x3b0e760,struct.pack('<I',case[18]))
    def hook(machine,address,size,data):
        nonlocal present,failure,boundary,nested
        if address in READS:
            bit,tag=READS[address]
            if not present&(1<<bit):
                failure=tag;boundary=address
                if case[0]==1 and address in [0x7899a0,0x7899ab]:
                    #7899aa pushes the frequency high word before the low
                    #read. The caller return remains one word above that push.
                    nested=address;esp=machine.reg_read(UC_X86_REG_ESP)
                    return_offset=4 if address==0x7899ab else 0
                    boundary=struct.unpack('<I',machine.mem_read(esp+return_offset,4))[0]-5
                machine.emu_stop();return
        if address in WRITES:present|=1<<WRITES[address]
    m.hook_add(unicorn.UC_HOOK_CODE,hook)
    args=[case[2]] if case[0]==0 else case[2:4] if case[0]==1 else []
    m.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));m.reg_write(UC_X86_REG_ESP,STACK)
    m.emu_start(ENTRIES[case[0]],STOP,count=10000)
    if not failure and (m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+4):raise RuntimeError('Source return ABI')
    values=[]
    for bit,(at,length) in enumerate(FIELDS):values.append(int.from_bytes(m.mem_read(at,length),'little') if present&(1<<bit) else 0)
    return [failure,boundary,nested,int(not failure),present]+[values[i] for i in [2,4,5,6,7,11,12,13,9,10,8]]
def fixtures():
    rng=random.Random(0x411e307899a0);out=[]
    for entry in range(4):
      for n in range(500):
        values=[rng.getrandbits(32) for _ in range(19)];values[0]=entry;values[1]=0x3fff
        values[2]=rng.choice([0,1,2,3,0xffffffff,0x80000000,rng.getrandbits(32)])
        values[7]=rng.choice([0,1,2,0xffffffff]);values[10]=rng.choice([0,0,1,2,4,0xff]);values[11]&=255
        values[12]=rng.choice([0,1,0xffffffff]);values[13]=rng.choice([0,1,0xff]);values[17]&=1;out.append(values)
    for entry in range(4):
      for missing in range(14):
       for mode in [0,1,2]:
        for flags in [0,2]:
         for factor in [1,2]:
          case=[entry,0x3fff&~(1<<missing),mode,2,0,10000000,7,factor,0x11111111,0x22222222,flags,0x40,0,1,9,9,9,1,0x12345678];out.append(case)
    return out
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args()
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(args.native_library.resolve()));fn=lib.sadx_native_game_frame_control;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    cases=fixtures();bad=[]
    for number,case in enumerate(cases):
        expected=source_run(image,case);actual=(ctypes.c_uint32*16)()
        if fn((ctypes.c_uint32*19)(*case),19,actual,16)!=1:raise RuntimeError('Native fixture rejected')
        if list(actual)!=expected:bad.append({'case':number,'input':case,'source':expected,'native':list(actual),'fields':[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]})
    blob=b''.join(struct.pack('<19I',*case) for case in cases);args.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/engine/game_frame_control.hpp','src/engine/game_frame_control.cpp','src/engine/game_logic_support.hpp','tests/probes/native_game_frame_control_probe.cpp','tools/oracles/verify_original_game_frame_control.py']
    ranges=[(0x77ff50,10),(0x411e20,10),(0x411e30,93),(0x7899a0,54),(0x40f010,86),(0x792f60,3),(0x780069,30)]
    report={'source_sha256':image.sha256,'native_library_sha256':sha(args.native_library.read_bytes()),'source_files':{n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},'original_ranges_sha256':{f'{at:x}-{at+length:x}':sha(image.read(at,length)) for at,length in ranges},'cases':len(cases),'case_sha256':sha(blob),'mismatches':bad,'stores_callback_identity_and_boundaries_match':not bad,'limits':__doc__}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':len(cases),'mismatches':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
