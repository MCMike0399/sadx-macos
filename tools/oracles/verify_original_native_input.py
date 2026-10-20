"""Compare native keyboard->DreamPad traces with actual pinned Input DLL instructions.

Mouse is disabled. The actual DLL six-key map and game KeyboardUpdate instructions
are exercised; the map tree is populated from original constructor setup values.
Window hook install is excluded (native host owns events). Focus uses the original reset range.
Unicorn2.1.4 does not raise SSE sticky flags: values and MXCSR are reported separately.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,subprocess,sys,tempfile
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_MXCSR,UC_X86_REG_EIP,UC_X86_REG_EBP
from pe_image import PEImage
SHA='c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5'
PAD,DATA,STACK,STOP=0x22000000,0x23000000,0x2400f000,0x25000000
BINDINGS=[0x1000e73a,0x1000e738,0x1000e728,0x1000e72a,0x1000e72c,0x1000e72e,0x1000e734,0x1000e732,0x1000e736,0x1000e730,0x1000e71c,0x1000e71e,0x1000e720,0x1000e722,0x1000e724,0x1000e726,0x1000e70c,0x1000e70e,0x1000e710,0x1000e712,0x1000e714,0x1000e716,0x1000e718,0x1000e71a]
KEYS=[160,69,88,90,65,83,66,67,86,13,81,87,104,98,100,102,38,40,37,39,73,77,74,76]
def put(m,a,n,v):m.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
class Source:
    def __init__(self,image,keys,game_image=None,key_map=None):
        m=self.m=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32)
        for s in image.sections:
            size=(max(s['raw_size'],s['virtual_size'])+4095)&~4095
            m.mem_map(image.base+s['rva'],size);m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],min(size,s['raw_size'])))
        for a in (PAD,DATA,0x24000000,STOP,0x40e000,0x3b0e000):m.mem_map(a,0x10000 if a in (PAD,DATA,0x24000000,STOP) else 4096)
        m.mem_write(0x40ee10,game_image.read(0x40ee10,0x110) if game_image else b'\xc3')
        # Empty std::map sentinel excludes the independent vanilla six-key channel.
        put(m,0x1000ec50,4,DATA);put(m,DATA+4,4,DATA);put(m,DATA+13,1,1)
        if key_map:
            entries=sorted(key_map.items());next_node=[DATA+0x1000]
            def tree(items,parent):
                if not items:return DATA
                middle=len(items)//2;node=next_node[0];next_node[0]+=24
                put(m,node+4,4,parent);put(m,node+16,4,items[middle][0]);put(m,node+20,4,items[middle][1])
                put(m,node,4,tree(items[:middle],node));put(m,node+8,4,tree(items[middle+1:],node));return node
            put(m,DATA+4,4,tree(entries,DATA))
        for i,a in enumerate(BINDINGS):put(m,a,2,keys[i])
        for i,a in enumerate((0x1000e050,0x1000e054,0x1000e058,0x1000e048,0x1000e04c)):put(m,a,4,DATA+256+i*4)
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x10006500,STOP,count=10)
        if get(m,0x3b0e340,4)!=0x1000e944:raise RuntimeError('Original keyboard creation callback failed')
        put(m,0x1000e920,4,1)
        # DreamPad constructor's zero bytes and keyboard-enabled player0 setting.
        put(m,PAD+0x4c,1,1);put(m,PAD+0x4e,2,7849);put(m,PAD+0x52,2,8689);put(m,PAD+0x50,1,1);put(m,PAD+0x55,1,30)
        m.reg_write(UC_X86_REG_MXCSR,0x1f80)
    def run(self,operations):
        m=self.m;outputs=[]
        def call(entry):
            put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
            m.emu_start(entry,STOP,count=5000)
            if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('source call did not finish')
        for kind,key,down in operations:
            if kind==0:
                m.reg_write(UC_X86_REG_ECX,key);m.reg_write(UC_X86_REG_EDX,down);call(0x10005170)
            elif kind==1:
                m.emu_start(0x10005852,0x100058fa,count=500)
                if m.reg_read(UC_X86_REG_EIP)!=0x100058fa:raise RuntimeError('focus range incomplete')
            call(0x10004db0)
            m.reg_write(UC_X86_REG_ECX,PAD);call(0x10004080)
            output=[get(m,PAD+a,4) for a in(0,4,8,12,16,20)]
            output += [get(m,PAD+a,2) for a in(24,26)]
            output += [int.from_bytes(m.mem_read(PAD+a,2),'little',signed=True)&0xffffffff for a in(28,30,32,34)]
            output += [get(m,PAD+44,4),get(m,PAD+68,4),get(m,PAD+72,4),m.reg_read(UC_X86_REG_MXCSR)]
            output += list(m.mem_read(0x3b0e3e0,768))+[get(m,0x3b0e3b8,1)]
            outputs.append(output)
        return outputs

def fixtures(seed):
    traces=[]
    for key in KEYS:
        traces.append([(0,key,1)]+[(2,0,0)]*4+[(0,key,0)]+[(2,0,0)]*5)
    traces += [[(0,39,1)]*4+[(1,0,0)]+[(2,0,0)]*5,
               [(0,160,1),(0,39,1),(1,0,0),(0,39,1),(0,160,0)],
               [(0,39,1),(0,37,1)]+[(2,0,0)]*5+[(0,39,0)]+[(2,0,0)]*5]
    rng=random.Random(seed)
    traces += [[(rng.choices([0,1,2],[7,1,2])[0],rng.choice(KEYS),rng.randrange(2)) for _ in range(60)] for _ in range(50)]
    return traces

def main():
    p=argparse.ArgumentParser();p.add_argument('input_dll',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--seed',type=int,default=20261020);p.add_argument('--game-exe',type=pathlib.Path,required=True);a=p.parse_args()
    image=PEImage(a.input_dll)
    if image.sha256!=SHA or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned identity/version mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));probe=lib.sadx_native_input_trace
    probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    constant_probe=lib.sadx_native_input_constants;constant_probe.argtypes=[ctypes.POINTER(ctypes.c_uint8)];constant_probe.restype=ctypes.c_int
    constant_bytes=(ctypes.c_uint8*40)()
    if not constant_probe(constant_bytes):raise RuntimeError('native constants failed')
    # Game coefficient source is independent of the Input DLL.
    game_image=PEImage(a.game_exe)
    if game_image.sha256!='6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74':raise RuntimeError('Pinned original identity mismatch')
    expected_constants=b''.join(game_image.read(address,size) for address,size in [(0x7dccf4,4),(0x7dccf0,4),(0x7dcce0,4),(0x7dcb10,4),(0x7dccd0,8),(0x7dccc8,8),(0x7dcce8,8)])
    if bytes(constant_bytes)!=expected_constants:raise RuntimeError('Native coefficient inverse-byte mismatch')
    # Actual constructor setup assembles all102 pairs before its first allocation.
    setup=Source(image,KEYS).m
    setup.reg_write(UC_X86_REG_EBP,STACK);setup.reg_write(UC_X86_REG_ESP,STACK-0x354)
    setup.emu_start(0x1000105e,0x1000131e,count=1000)
    if setup.reg_read(UC_X86_REG_EIP)!=0x1000131e:raise RuntimeError('Map initializer range incomplete')
    key_pairs=struct.unpack('<204I',setup.mem_read(STACK-0x340,816))
    source_map=dict(zip(key_pairs[::2],key_pairs[1::2]))
    key_probe=lib.sadx_native_input_vanilla_key_index;key_probe.argtypes=[ctypes.c_uint32];key_probe.restype=ctypes.c_uint32
    for key in range(258):
        if key_probe(key)!=source_map.get(key,0):raise RuntimeError(f'Vanilla source key map mismatch:{key}')
    reset_source=Source(image,KEYS,game_image,source_map).m
    reset_source.mem_map(0x3b18000,4096);reset_source.mem_map(0x40f000,4096);reset_source.mem_write(0x40efe0,game_image.read(0x40efe0,33))
    reset_probe=lib.sadx_native_input_soft_reset;reset_probe.argtypes=[ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];reset_probe.restype=ctypes.c_int
    for requested in range(256):
        for address,value in [(0x3b0eaa0,requested),(0x3b0eaa7,7),(0x3b18db5,9)]:put(reset_source,address,1,value)
        put(reset_source,STACK,4,STOP);reset_source.reg_write(UC_X86_REG_ESP,STACK);reset_source.emu_start(0x40efe0,STOP,count=40)
        expected=[reset_source.reg_read(UC_X86_REG_EAX)]+[get(reset_source,address,1) for address in(0x3b0eaa0,0x3b0eaa7,0x3b18db5)]
        reset_output=(ctypes.c_uint32*4)()
        if not reset_probe(requested,7,9,reset_output) or list(reset_output)!=expected:raise RuntimeError('Soft reset source mismatch')
    value_bad=[];flag_bad=0;count=0;traces=fixtures(a.seed)
    for n,trace in enumerate(traces):
        source=Source(image,KEYS,game_image,source_map).run(trace);raw=(ctypes.c_uint32*(3*len(trace)))(*[v for op in trace for v in op]);out=(ctypes.c_uint32*(785*len(trace)))()
        if probe((ctypes.c_uint32*24)(*KEYS),raw,len(trace),out)!=1:raise RuntimeError('native trace failed')
        for j,expected in enumerate(source):
            actual=list(out[j*785:(j+1)*785]);count+=1
            if actual[:15]+actual[16:]!=expected[:15]+expected[16:]:value_bad.append({'trace':n,'step':j,'expected':expected,'actual':actual})
            if actual[15]!=expected[15]:flag_bad+=1
    report={'source_sha256':SHA,'constants_inverse_match':True,'vanilla_key_map_inverse_match':True,'vanilla_key_map_count':len(source_map),'soft_reset_cases':256,'soft_reset_mismatches':0,'constants_hex':expected_constants.hex(),'seed':a.seed,'trace_count':len(traces),'poll_evaluations':count,'value_mismatches':len(value_bad),'mxcsr_mismatches':flag_bad,'first_mismatches':value_bad[:10],'values_match':not value_bad,'passed':not value_bad and not flag_bad,'scope':__doc__,'native_probe_sha256':hashlib.sha256(a.native_library.read_bytes()).hexdigest(),'cases_sha256':hashlib.sha256(json.dumps(traces).encode()).hexdigest()}
    root=pathlib.Path(__file__).resolve().parents[2]
    report['source_files']={str(path):hashlib.sha256((root/path).read_bytes()).hexdigest() for path in map(pathlib.Path,['src/runtime/native_input.hpp','src/runtime/native_input.cpp','tests/probes/native_native_input_probe.cpp','tools/oracles/verify_original_native_input.py'])}
    for name in ['src/engine/controller_values.hpp','src/engine/game_controller.hpp','src/engine/game_controller.cpp','src/input/input_initial_state.hpp','src/input/input_mod_pad.hpp','src/input/input_mod_pad.cpp','src/player/scene_input_producer.hpp','src/player/scene_input_producer.cpp','tools/oracles/verify_original_native_input_config.py','tools/oracles/verify_original_native_input_pipeline.py']:
        report['source_files'][name]=hashlib.sha256((root/name).read_bytes()).hexdigest()
    report['source_ranges_sha256']={name:hashlib.sha256(owner.read(address,size)).hexdigest() for name,owner,address,size in [('keyboard_poll',image,0x10004db0,953),('keyboard_ramp',image,0x10004cb0,256),('keyboard_events',image,0x10005170,0x410),('keyboard_update',game_image,0x40ee10,0x110),('keyboard_create',image,0x10006500,12)]}
    report['limits']=['Native SSE values match; Unicorn2.1.4 omits sticky exception flags, so MXCSR parity remains partial.','Window message dispatch/hardware delivery are separate qualification; raw event normalization is host owned.']
    # Keep the raw generator/comparison above unchanged. Additional source-owner
    # contracts are independent named cohorts under this single gate entry.
    auxiliary_failed=False
    with tempfile.TemporaryDirectory(prefix='sadx-native-input-cohorts-') as temporary:
        report['cohorts']={
            'raw_keyboard_pad':{'traces':len(traces),'polls':count,'value_mismatches':len(value_bad),'mxcsr_mismatches':flag_bad},
            'soft_reset':{'cases':256,'value_mismatches':0},
            'source_tables':{'key_pairs':len(source_map),'coefficient_bytes':len(expected_constants),'inverse_match':True}}
        for name,script,arguments in [
            ('configuration','verify_original_native_input_config.py',[str(a.input_dll),str(a.native_library)]),
            ('on_input_get_controllers','verify_original_native_input_pipeline.py',[str(a.game_exe),str(a.input_dll),str(a.native_library)])]:
            target=pathlib.Path(temporary)/(name+'.json')
            command=[sys.executable,str(pathlib.Path(__file__).with_name(script)),*arguments,'--seed',str(a.seed),'--output',str(target)]
            result=subprocess.run(command,capture_output=True,text=True,check=False)
            auxiliary_failed |= result.returncode!=0
            report['cohorts'][name]=json.loads(target.read_text()) if target.exists() else {'error':result.stderr[-6000:],'exit_status':result.returncode}
    report['values_match']=not value_bad and not auxiliary_failed
    report['passed']=report['passed'] and not auxiliary_failed and all(cohort.get('passed',True) for cohort in report['cohorts'].values())
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['trace_count','poll_evaluations','value_mismatches','mxcsr_mismatches']}));return int(bool(value_bad) or auxiliary_failed)
if __name__=='__main__':raise SystemExit(main())
