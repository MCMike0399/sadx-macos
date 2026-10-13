"""Retained Input2.6.1 helper and keyboard scalar instruction comparison.

Executes isolated DLL instructions, not startup/imports/Windows input polling.
All masks are explicit; captured production MXCSR remains unavailable.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_MXCSR,UC_X86_REG_EIP
from pe_image import PEImage
INPUT_SHA='c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5'
DATA,STACK,STOP=0x20000000,0x21008000,0x22000000
CONSTANTS=[0x1000bb6c,0x1000bbb0,0x1000bba0,0x1000bba4,0x1000bb7c]
class Original:
    def __init__(self,image):
        self.m=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32)
        for s in image.sections:
            if s['name'] not in ('.text','.rdata','.data'):continue
            size=(max(s['raw_size'],s['virtual_size'])+4095)&~4095
            self.m.mem_map(image.base+s['rva'],size)
            self.m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
        self.m.mem_map(DATA,4096);self.m.mem_map(STACK-0x8000,0x10000);self.m.mem_map(STOP,4096)
    def run(self,values,mode,mxcsr):
        m=self.m;m.reg_write(UC_X86_REG_MXCSR,mxcsr);m.reg_write(UC_X86_REG_ESP,STACK)
        if mode==0:
            x,y,deadzone,radial=values
            m.mem_write(DATA,struct.pack('<hh',x,y));m.mem_write(DATA+16,b'\xef\xbe\xad\xde')
            m.reg_write(UC_X86_REG_ECX,DATA+16);m.reg_write(UC_X86_REG_EDX,DATA)
            m.mem_write(STACK,struct.pack('<III',STOP,deadzone&0xffff,radial))
            m.emu_start(0x10004400,STOP,count=1000)
            if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('helper did not return')
            x,y=struct.unpack('<hh',m.mem_read(DATA+16,4));mag=m.reg_read(unicorn.x86_const.UC_X86_REG_XMM0)&0xffffffff
            output=[x&0xffffffff,y&0xffffffff,mag]
        else:
            x,y,rx,ry=values;m.reg_write(UC_X86_REG_EAX,(x&65535)|((y&65535)<<16))
            m.mem_write(0x1000e938,struct.pack('<hh',rx,ry));m.mem_write(0x1000e926,b'\x00')
            m.emu_start(0x10004e59,0x10005007,count=1000)
            if m.reg_read(UC_X86_REG_EIP)!=0x10005007:raise RuntimeError('keyboard range did not complete')
            lx,ly,rx,ry=struct.unpack('<hhhh',m.mem_read(0x1000e908,8))
            output=[lx&0xffffffff,ly&0xffffffff,struct.unpack('<I',m.mem_read(0x1000e8e8,4))[0],rx&0xffffffff,ry&0xffffffff,struct.unpack('<I',m.mem_read(0x1000e744,4))[0]]
        return output,m.reg_read(UC_X86_REG_MXCSR)
def fixtures():
    rng=random.Random(0x10004400)
    edges=[-32768,-32767,-24576,-16384,-8192,-7849,-7848,-1,0,1,7848,7849,8192,16384,24576,32767]
    result={0:[],1:[]}
    for x in edges:
        for y in edges:
            for dz in [0,7849,8689,32767,-1]:
                for radial in [0,1]:result[0].append([x,y,dz,radial])
            result[1].append([x,y,y,x])
    for _ in range(2000):
        result[0].append([rng.randrange(-32768,32768),rng.randrange(-32768,32768),rng.choice([0,7849,8689,32767,-32768]),rng.randrange(2)])
        result[1].append([rng.randrange(-32768,32768) for _ in range(4)])
    return result
def main():
    p=argparse.ArgumentParser();p.add_argument('input_mod',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires pinned Unicorn 2.1.4')
    image=PEImage(a.input_mod)
    if image.sha256!=INPUT_SHA:raise RuntimeError('DLL identity mismatch')
    constants=b''.join(image.read(address,4) for address in CONSTANTS);buf=(ctypes.c_uint8*20).from_buffer_copy(constants)
    library=ctypes.CDLL(str(a.native_library.resolve()));native=library.sadx_native_input_mod_axes
    native.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];native.restype=ctypes.c_int
    data=fixtures();original=Original(image);start=time.monotonic();reports=[]
    controls=[0x1f80|(r<<13)|(d<<6)|(f<<15) for r in range(4) for d in range(2) for f in range(2)]
    for mode,cases in data.items():
        report={'mode':'controller helper' if mode==0 else 'keyboard scalar range','cases':len(cases),'cases_sha256':hashlib.sha256(json.dumps(cases,separators=(',',':')).encode()).hexdigest(),'evaluations':0,'value_mismatches':0,'mxcsr_mismatches':0,'first_value_mismatches':[],'first_mxcsr_mismatches':[]}
        for mxcsr in controls:
            for values in cases:
                expected,flags=original.run(values,mode,mxcsr);out=(ctypes.c_uint32*8)();args=(ctypes.c_uint32*4)(*[x&0xffffffff for x in values])
                status=native(args,mode,buf,mxcsr,out)
                if status or out[7]:raise RuntimeError('native failed a masked fixture')
                actual=list(out)[:len(expected)];report['evaluations']+=1
                if actual!=expected:
                    report['value_mismatches']+=1
                    if len(report['first_value_mismatches'])<10:report['first_value_mismatches'].append({'input':values,'mxcsr':hex(mxcsr),'original':expected,'native':actual})
                if flags!=out[6]:
                    report['mxcsr_mismatches']+=1
                    if len(report['first_mxcsr_mismatches'])<10:report['first_mxcsr_mismatches'].append({'input':values,'mxcsr':hex(mxcsr),'original':hex(flags),'native':hex(out[6])})
        reports.append(report);print(json.dumps(report),flush=True)
    result={'input_mod_sha256':image.sha256,'native_probe_sha256':hashlib.sha256(a.native_library.read_bytes()).hexdigest(),'constants_addresses':[hex(x) for x in CONSTANTS],'constants_hex':constants.hex(),'constants_sha256':hashlib.sha256(constants).hexdigest(),'controller_instructions_sha256':hashlib.sha256(image.read(0x10004400,0x1e0)).hexdigest(),'keyboard_instructions_sha256':hashlib.sha256(image.read(0x10004e59,0x1ae)).hexdigest(),'unicorn_version':unicorn.__version__,'mxcsr_controls':[hex(x) for x in controls],'results':reports,'seconds':time.monotonic()-start,'passed':all(not(r['value_mismatches'] or r['mxcsr_mismatches']) for r in reports),'scope':'isolated actual DLL instructions, all exceptions masked; not Windows runtime or physical hardware'}
    a.output.write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
