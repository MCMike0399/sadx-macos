"""792100 and 791df0 (the 77f110/77efc0 callback targets) against their original
instructions: with a zero joystick count (3d08814) / null mouse device (3d08834)
each returns through its stack-cookie check; otherwise the source reaches the
first body instruction (792122 / 791e0b), where the native body owner stops.
Compared: completion and the body reached. The bodies themselves are not covered.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    clean=m.context_save();st={}
    def hook(mu,address,size,user):st['body']=address;mu.emu_stop()
    for b in(0x792122,0x791e0b):m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=b,end=b)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_input_devices;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x792100);bad=[];cases=0;outcomes={}
    for n in range(400):
        which=n%2;gate=rng.choice([0,0,1,0x1234,0xffffffff]);out=(ctypes.c_uint32*2)()
        fn((ctypes.c_uint32*2)(which,gate),out)
        m.context_restore(clean);st['body']=0
        m.mem_write(0x3d08834 if which else 0x3d08814,struct.pack('<I',gate))
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x791df0 if which else 0x792100,STOP,count=10000)
        if not st['body'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        e=[int(st['body']==0),st['body']];cases+=1;key=f"{['792100','791df0'][which]}:body{int(bool(st['body']))}";outcomes[key]=outcomes.get(key,0)+1
        if list(out)!=e:bad.append(dict(case=n,which=which,gate=gate,source=e,native=list(out)))
    names=['src/ninja/ninja_input_devices.hpp','src/ninja/ninja_input_devices.cpp','tests/probes/native_input_devices_probe.cpp','tools/oracles/verify_original_input_devices.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x792100-0x792122':sha(image.read(0x792100,0x22)),'0x791df0-0x791e0b':sha(image.read(0x791df0,0x1b))},cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,mismatches=len(bad))))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
