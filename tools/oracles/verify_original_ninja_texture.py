"""Actual packed16 texture converters and source header/global-index readers.
RGBA8 is checked against an independently expressed pinned PuyoTools policy;
no D3D/Metal sampling, GPU upload, modified archive or runtime binding is claimed.
"""
import argparse,ctypes,hashlib,json,pathlib,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
ASSETS=['EFF_REGULAR','BEACH01','BEACH02','BEACH03','OBJ_BEACH','BEACH_SEA','BG_BEACH','EC_CLOUDS']
BASE=0x11000000;DEST=BASE+0x20000;SOURCE=BASE+0x100000;STACK=BASE+0x1ff000;STOP=0x10000100
sha=lambda b:hashlib.sha256(b).hexdigest()
class Original:
    def __init__(self,image):
        self.m=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32)
        for s in image.sections:
            length=max(s['virtual_size'],s['raw_size']);length=(length+4095)&~4095
            if not length:continue
            start=image.base+s['rva'];self.m.mem_map(start,length);self.m.mem_write(start,image.slice(s['file_offset'],s['raw_size']))
        self.m.mem_map(BASE,0x200000);self.m.mem_map(0x10000000,4096);self.m.mem_write(STOP,b'\x90')
    def call(self,address,args):
        self.m.mem_write(STACK,struct.pack('<'+str(len(args)+1)+'I',STOP,*args));self.m.reg_write(UC_X86_REG_ESP,STACK)
        self.m.emu_start(address,STOP,count=12000000)
        if self.m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError(f'Original{address:x} did not complete')
        return self.m.reg_read(UC_X86_REG_EAX)
    def pixels(self,fmt,width,height,payload,level_width):
        self.m.mem_write(SOURCE,payload);count=level_width*level_width*2 if fmt==2 else width*height*2
        self.m.mem_write(DEST,b'\xa5'*(count+16))
        if fmt==2:self.call(0x78bd10,[DEST,SOURCE,width,2,level_width.bit_length()-1])
        elif fmt==9:self.call(0x78c570,[DEST,SOURCE,width,height,2])
        else:self.call({1:0x78bc30,3:0x78bf50}[fmt],[DEST,SOURCE,width,height,2])
        if bytes(self.m.mem_read(DEST+count,16))!=b'\xa5'*16:raise RuntimeError('Converter overran its target texels')
        return bytes(self.m.mem_read(DEST,count))
def chunks(b):
    p=0;entries=[];table=None;stride=0
    if b[:4]==b'PVMH':
        flags,count=struct.unpack_from('<HH',b,8);p=8+struct.unpack_from('<I',b,4)[0]
        stride=2+(28 if flags&1 else 0)+(2 if flags&2 else 0)+(2 if flags&4 else 0)+(4 if flags&8 else 0);table=(flags,count)
    while p<len(b):
        n=struct.unpack_from('<I',b,p+4)[0]+8
        if b[p:p+4]==b'PVRT':
            pf,df,res,w,h=struct.unpack_from('<BBHHH',b,p+8);record=dict(offset=p,size=n,pixel=pf,data=df,reserved=res,width=w,height=h)
            if table:
                q=12+len(entries)*stride;record['table_index']=struct.unpack_from('<H',b,q)[0];q+=2
                if table[0]&1:record['name_raw_hex']=b[q:q+28].hex();q+=28
                if table[0]&2:record['formats_raw_hex']=b[q:q+2].hex();q+=2
                if table[0]&4:record['dimensions_raw_hex']=b[q:q+2].hex();q+=2
                if table[0]&8:record['global_index']=struct.unpack_from('<I',b,q)[0]
            entries.append(record)
        p+=n
    return entries
def expected_rgba(words,pf):
    output=bytearray()
    for (v,) in struct.iter_unpack('<H',words):
        if pf==0:rgba=((v>>10&31)*255//31,(v>>5&31)*255//31,(v&31)*255//31,255*(v>>15))
        elif pf==1:rgba=((v>>11&31)*255//31,(v>>5&63)*255//63,(v&31)*255//31,255)
        else:rgba=((v>>8&15)*17,(v>>4&15)*17,(v&15)*17,(v>>12)*17)
        output.extend(rgba)
    return bytes(output)
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--asset-dir',type=pathlib.Path,default=pathlib.Path('Sonic Adventure DX/system'));p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--limit',type=int);a=p.parse_args()
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Original source/oracle identity changed')
    native=ctypes.CDLL(str(a.library.resolve()));fn=native.sadx_native_ninja_texture;fn.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.c_size_t,ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint16),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint64)];fn.restype=ctypes.c_int;native.sadx_native_ninja_texture_compiler.restype=ctypes.c_char_p
    original=Original(image);root=pathlib.Path(__file__).resolve().parents[2]
    report=dict(source_sha256=image.sha256,library_sha256=sha(a.library.read_bytes()),compiler=native.sadx_native_ninja_texture_compiler().decode(),unicorn=unicorn.__version__,source_sha256_by_file={v:sha((root/v).read_bytes()) for v in ['src/engine/ninja_texture.hpp','src/engine/ninja_texture.cpp','tests/probes/native_ninja_texture_probe.cpp','tools/oracles/verify_original_ninja_texture.py']},original_ranges_sha256={hex(at):sha(image.read(at,n)) for at,n in [(0x77f2f0,0xc1),(0x77f3f0,0x49),(0x78bc30,0xcd),(0x78bd10,0x113),(0x78bf50,0xe7),(0x78c570,0x29)]},archives=[],mismatches=[],limits=['Packed16 values compared with actual77f2f0/77f3f0/78bc30/78bd10/78bf50/78c570 instructions, no overlays','RGBA8 compared with integer expansion derived from pinned MIT PuyoTools; original GPU consumes packed16','No D3D/Metal upload/filter/normalized-channel/hardware/runtime texture binding equivalence','Supplied source assets only; other pixel/data formats explicitly unsupported'])
    decoded_dir=a.output.parent/'texture-decoded';decoded_dir.mkdir(exist_ok=True)
    for name in ASSETS:
        path=a.asset_dir/(name+'.PVM');data=path.read_bytes();records=chunks(data);buffer=(ctypes.c_uint8*len(data)).from_buffer_copy(data);item=dict(name=name,sha256=sha(data),bytes=len(data),records=len(records),levels=0,packed_mismatches=0,rgba_mismatches=0,metadata_mismatches=0,decoded=[])
        if name=='EFF_REGULAR' and item['sha256']!='e7972e401ef9744cecc01740d47d23343c82add0044b74b13fce77bcfc64a530':raise RuntimeError('Pinned bubble archive changed')
        original.m.mem_write(SOURCE,data[:struct.unpack_from('<I',data,4)[0]+8])
        for index,r in enumerate(records[:a.limit] if a.limit else records):
            count=r['width']*r['height'];pixels=(ctypes.c_uint16*count)();rgba=(ctypes.c_uint8*(count*4))();metadata=(ctypes.c_uint64*23)();status=fn(buffer,len(data),index,0,pixels,count,rgba,count*4,metadata)
            if status:raise RuntimeError(f'Native{name}/{index} status{status} metadata{list(metadata)}')
            expected=[r['offset'],r['size'],r['offset']+16,r['size']-16,r['pixel'],r['data'],r['width'],r['height'],r['reserved'],r.get('global_index',0x100000000),r.get('table_index',0x100000000)]
            if list(metadata[5:16])!=expected:item['metadata_mismatches']+=1;report['mismatches'].append(dict(archive=name,index=index,kind='metadata'))
            # Source global-index reader uses the actual PVM header and ordinal.
            original.m.mem_write(SOURCE,data[:struct.unpack_from('<I',data,4)[0]+8]);observed=original.call(0x77f3f0,[SOURCE,index])
            if observed!=r.get('global_index',0xffffffff):raise RuntimeError('PVM table/global-index association disagrees with original')
            # The raw pointer is compared as a source-relative offset; no guest
            # address enters the native resource owner.
            original.m.mem_write(SOURCE,data[r['offset']:r['offset']+r['size']]);original.m.mem_write(DEST,b'\xa5'*64);original.call(0x77f2f0,[DEST,SOURCE]);descriptor=bytes(original.m.mem_read(DEST,64))
            if struct.unpack_from('<I',descriptor,0x18)[0]!=((r['pixel']|(r['data']<<8))<<16) or struct.unpack_from('<I',descriptor,0x20)[0]!=(r['pixel']<<27) or struct.unpack_from('<II',descriptor,0x24)!=(r['width'],r['height']) or struct.unpack_from('<II',descriptor,0x2c)!=(r['size']-16,0) or struct.unpack_from('<I',descriptor,0x34)[0]!=SOURCE+16:raise RuntimeError('Source PVRT dimensions/payload descriptor disagreement')
            levels=int(metadata[16]);entry=dict(**r,chunk_sha256=sha(data[r['offset']:r['offset']+r['size']]),levels=[])
            for level in range(levels):
                if level:status=fn(buffer,len(data),index,level,pixels,count,rgba,count*4,metadata)
                if status:raise RuntimeError('Native mipmap failure')
                w,h=int(metadata[17]),int(metadata[18]);size=w*h*2;actual=bytes(pixels)[:size];colors=bytes(rgba)[:w*h*4]
                wanted=original.pixels(r['data'],r['width'],r['height'],data[r['offset']+16:r['offset']+r['size']],w)
                if actual!=wanted:item['packed_mismatches']+=1;report['mismatches'].append(dict(archive=name,index=index,level=level,kind='packed',first_byte=next(i for i,(x,y) in enumerate(zip(actual,wanted)) if x!=y)))
                if colors!=expected_rgba(wanted,r['pixel']):item['rgba_mismatches']+=1;report['mismatches'].append(dict(archive=name,index=index,level=level,kind='rgba'))
                entry['levels'].append(dict(width=w,height=h,source_offset=metadata[19],encoded_bytes=metadata[20],packed16_sha256=sha(actual),rgba8_sha256=sha(colors),tail_offset=metadata[21],tail_bytes=metadata[22]));item['levels']+=1
                if name=='EFF_REGULAR' and index==130 and level==0:
                    (decoded_dir/'EFF_REGULAR-130.rgba8').write_bytes(colors);(decoded_dir/'EFF_REGULAR-130.packed16').write_bytes(actual)
            item['decoded'].append(entry)
        report['archives'].append(item);print(json.dumps({k:v for k,v in item.items() if k!='decoded'}),flush=True)
    # Independent synthetic metadata profiles disambiguate the low PVM flag
    # masks; the real archives all happen to use0x0f. Their PVRT remains the
    # observed bubble chunk; synthetic names/indices are clearly not game facts.
    bubble=(a.asset_dir/'EFF_REGULAR.PVM').read_bytes()[0xda730:0xda730+8224]
    report['synthetic_metadata_flags']=[]
    for flags in range(16):
        entry=bytearray(struct.pack('<H',17))
        if flags&1:entry+=b'synthetic'+bytes(19)
        if flags&2:entry+=bytes([0,9])
        if flags&4:entry+=bytes([0x44,0])
        if flags&8:entry+=struct.pack('<I',0x76543210)
        header=b'PVMH'+struct.pack('<IHH',4+len(entry),flags,1)+entry
        resource=header+bubble;buffer=(ctypes.c_uint8*len(resource)).from_buffer_copy(resource)
        pixels=(ctypes.c_uint16*4096)();rgba=(ctypes.c_uint8*16384)();metadata=(ctypes.c_uint64*23)()
        if fn(buffer,len(resource),0,0,pixels,4096,rgba,16384,metadata):raise RuntimeError('Synthetic flags native parse failed')
        original.m.mem_write(SOURCE,header);want=0x76543210 if flags&8 else 0xffffffff
        actual=original.call(0x77f3f0,[SOURCE,0]);outside=original.call(0x77f3f0,[SOURCE,1])
        if actual!=want or outside!=0xffffffff or metadata[14]!=(want if flags&8 else 0x100000000):raise RuntimeError('Synthetic flags disagree with source')
        report['synthetic_metadata_flags'].append(dict(flags=flags,source_global_index=hex(actual),source_outside=hex(outside),native_global_index=int(metadata[14])))
    report['packed_and_metadata_match']=not any(x['packed_mismatches'] or x['metadata_mismatches'] for x in report['archives']);report['rgba_primary_policy_match']=not any(x['rgba_mismatches'] for x in report['archives']);report['passed']=report['packed_and_metadata_match'] and report['rgba_primary_policy_match'];a.output.write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
