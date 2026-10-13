"""Complete original49c330 with owned fixture paths and real CRT/Ninja calls.

No path callback or instruction overlay. Explicit runtime/control fixtures,
physical sentinels and caller work words are experiments, not running assets.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_MXCSR
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import Original,DATA,STACK,STOP
PATH=DATA+0x100;POINTS=DATA+0x400;STATUS=DATA+0x1000
SEED=0x49c33049c210
def sha(v):return hashlib.sha256(v).hexdigest()
def fword(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def path_constants(image):return image.read(0x7dcb2c,4)+image.read(0x7dcb10,4)+image.read(0x7dd57c,4)+image.read(0x840d90,80)
def source_setup(image,cw):
    original=Original(image,cw);m=original.machine
    for page in (0x3d0f000,0x2bcc000,0x3c83000):m.mem_map(page,0x1000)
    m.mem_write(0x2bcc000,image.read(0x2bcc000,0x1000));return original
def seed_path(m,type,logical,total,words):
    header=bytearray(12);struct.pack_into('<BHI',header,0,type,0,0) # write offsets explicitly below
    header[0]=type;struct.pack_into('<H',header,2,logical);struct.pack_into('<II',header,4,total,POINTS)
    m.mem_write(PATH,bytes(header));m.mem_write(POINTS,struct.pack('<'+'I'*len(words),*words))
class PathOriginal:
    def __init__(self,image,cw):self.original=source_setup(image,cw);self.machine=self.original.machine
    def run(self,w,cw):
        m=self.machine;seed_path(m,w[0],w[1],w[3],w[29:]);m.mem_write(STATUS,struct.pack('<20I',*w[9:29]));m.mem_write(0x3d0f400,struct.pack('<I',w[5]));m.mem_write(0x2bcc034,struct.pack('<H',w[8]));m.mem_write(STACK,struct.pack('<3I',STOP,PATH,STATUS));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_MXCSR,w[6]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.emu_start(0x49c330,STOP,count=30000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Source path ABI mismatch')
        return dict(returned=m.reg_read(UC_X86_REG_EAX),words=list(struct.unpack('<20I',m.mem_read(STATUS,80))),cw=m.reg_read(UC_X86_REG_FPCW),flags=m.reg_read(UC_X86_REG_FPSW)&0x3f,mxcsr=m.reg_read(UC_X86_REG_MXCSR))
def point_words(angles,length,pos):return [((angles[0]&0xffff)|((angles[1]&0xffff)<<16)),fword(length),*map(fword,pos)]
def fixtures():
    out=[];rng=random.Random(SEED)
    for type in (0,1):
        for logical in (2,3,4):
            points=[v for i in range(logical+1) for v in point_words((i*8191-16000,i*6000-12000),4,(i*4,0,i))]
            for distance in (-2,0,0.5,2,3.9999,4,8,logical*4,logical*4+1):
                for cpu in (0,1):
                    work=[0xdeadbeef]*20;work[4]=fword(distance);out.append([type,logical,logical+1,fword(logical*4),1,cpu,0x1f80,1,0x027f,*work,*points])
    for _ in range(500):
        logical=rng.randrange(2,7);type=rng.randrange(2);lengths=[rng.uniform(0.25,8) for _ in range(logical)];total=sum(lengths);points=[]
        for i in range(logical+1):points.extend(point_words((rng.randrange(-32768,32768),rng.randrange(-32768,32768)),lengths[i] if i<logical else 4,tuple(rng.uniform(-20,20) for _ in range(3))))
        work=[rng.getrandbits(32) for _ in range(20)];work[4]=fword(rng.uniform(-total,total*1.5));out.append([type,logical,logical+1,fword(total),1,rng.randrange(2),0x1f80,1,0x027f,*work,*points])
    return out
def write_blob(path,cases):
    blob=b''.join(struct.pack('<I',len(w))+struct.pack('<'+'I'*len(w),*w) for w in cases);path.write_bytes(blob);return dict(path=str(path),sha256=sha(blob),record='length-prefixed little-endian uint32 word records')
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires Unicorn2.1.4')
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source image changed')
    table=image.read(TABLE_ADDRESS,4100);constants=path_constants(image);cases=fixtures();args.output.parent.mkdir(parents=True,exist_ok=True)
    lib=ctypes.CDLL(str(args.native_library.resolve()));fn=lib.sadx_native_player_path;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t]*2+[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int;buffers=[(ctypes.c_ubyte*len(v)).from_buffer_copy(v) for v in (table,constants)]
    report=dict(profile='complete original path49c330/49c210 with real floor/Ninja/ftol; no overlays',source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),constants_sha256=sha(constants),table_sha256=sha(table),seed=hex(SEED),case_blob=write_blob(args.output.with_suffix('.cases.bin'),cases),source_functions={hex(a):dict(size=b-a,sha256=sha(image.read(a,b-a))) for a,b in ((0x49c210,0x49c329),(0x49c330,0x49c5d7),(0x4393c0,0x4393f7),(0x4383a0,0x4383ac))},control_words=[],limits=['Finite caller-supplied path/runtime/work fixtures, not actual gameplay captures','Physical storage includes explicit sentinel; source unowned/undefined reads use native boundary tests','Original x86 hardware and actual mod/control initialization remain unverified','x87/SSE flags are reported separately'])
    for cw in (p|r|0x7f for p in (0,0x200,0x300) for r in (0,0x400,0x800,0xc00)):
        original=PathOriginal(image,cw);item=dict(control_word=hex(cw),cases=len(cases),value_control_mismatches=0,flag_mismatches=0,examples=[],flag_examples=[])
        for index,w in enumerate(cases):
            expected=original.run(w,cw);out=(ctypes.c_uint32*34)();status=fn(buffers[0],4100,buffers[1],92,(ctypes.c_uint32*len(w))(*w),len(w),cw,out,34)
            if status or out[0] or not out[1]:raise RuntimeError(f'Native rejected case{index}:status{status},failure{out[0]},site{out[3]:x}')
            different=out[2]!=expected['returned'] or out[4]!=expected['cw'] or list(out[14:34])!=expected['words'];flags=out[5]!=expected['flags'] or out[6]!=expected['mxcsr'];item['value_control_mismatches']+=different;item['flag_mismatches']+=flags
            if different:item['examples'].append(dict(case=index,original=expected,native=list(out)))
            if flags and len(item['flag_examples'])<5:item['flag_examples'].append(dict(case=index,original_flags=expected['flags'],native_flags=out[5],original_mxcsr=hex(expected['mxcsr']),native_mxcsr=hex(out[6])))
        report['control_words'].append(item);print(json.dumps({k:v for k,v in item.items() if k not in ('examples','flag_examples')}),flush=True)
    report['returned_values_work_and_control_match']=all(i['value_control_mismatches']==0 for i in report['control_words']);args.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['returned_values_work_and_control_match'])
if __name__=='__main__':raise SystemExit(main())
