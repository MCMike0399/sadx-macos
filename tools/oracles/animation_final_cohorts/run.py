"""Execute once, after freezing definitions; stop an affected cohort on first mismatch.

Every generated input is persisted before any library or instruction execution.
No expected tolerance, fixture vector or old harness is modified by this program.
"""
import ctypes as C
import gzip, hashlib, importlib, json, os, pathlib, random, shutil, struct, subprocess, sys, traceback
from prepare import ROOT, OUT, HERE, OLD, CURRENT, FOUNDATION, sha
PACKET=ROOT/'docs/refactor-inventory/packets/animation-final-cohorts'
EXE=pathlib.Path('/Users/burbujamc/Developer/sadx-macos/reference/updated-pc-reference/game/sonic.exe')
IMPORTS=pathlib.Path('/Users/burbujamc/Developer/sadx-macos/game/imported')
COHORTS=[('table-retry',2026121701,2048),('motion-state',2026121702,4096),('blend-backing',2026121703,2048),('consumer-attachment',2026121704,1024)]
CW=[p|r|0x7f for p in (0,0x200,0x300) for r in (0,0x400,0x800,0xc00)]
U32=C.POINTER(C.c_uint32);U8=C.POINTER(C.c_ubyte)
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def dumps(x):return json.dumps(x,separators=(',',':'),default=lambda x:{'bytes_hex':x.hex()} if isinstance(x,bytes) else str(x))
def modules():
    tree=OUT/'current'
    sys.path[:0]=[str(tree/'tools/lib'),str(tree/'tools/oracles'),str(tree/'tools/oracles/animation_judges')]
    return [importlib.import_module('verify_original_'+slug) for slug in ('player_animation','player_animation_blend','sonic_player','player_startup')]
def fixtures(name,seed,count):
    m,b,s,p=modules();rng=random.Random(seed)
    motion=next(m.fixtures());blend=next(b.fixtures());sonic=next(s.fixtures());startup=p.fixtures()[0]['words']
    for case in range(count):
        cw=CW[case%12]
        if name=='table-retry':
            exports=[0x25000000+rng.randrange(1,100000)*32 for i in range(149)]
            # Repeated identities remain independent of ordinal magnitude.
            for i in range(0,149,11):exports[i]=exports[(i+7)%149]
            old=[]
            for row in range(147):old += [(case+row)%65,rng.getrandbits(31)+1,rng.randrange(256),rng.randrange(256),rng.randrange(65536),rng.getrandbits(32),rng.getrandbits(32)]
            passes=[]
            for k in range(2):
                variant=(case+k)%8
                passes += [rng.randrange(149) if variant in (0,1,2) else 0xffffffff,rng.randrange(150) if variant==3 else 149,rng.randrange(149) if variant in (1,4,5) else 0xffffffff,1 if variant==6 else 2 if variant==7 else 0]
            yield dict(case=case,table=exports+old+passes);continue
        f=[0]*102
        f[:28]=[int(name=='blend-backing'),cw,0,1+case%12,rng.randrange(2),rng.randrange(2),rng.randrange(65536),rng.randrange(65536),bits(rng.uniform(-2,9)),bits(rng.uniform(0,1)),rng.randrange(65536),rng.randrange(65536),bits(rng.uniform(-.75,.75)),bits(rng.choice((.125,.5,1))),rng.randrange(2,15),bits(rng.uniform(.1,4)),bits(rng.uniform(.1,4)),bits(rng.uniform(.1,4)),bits(rng.uniform(.1,4)),case%35,(0,64,128)[case%3],rng.choice((3,7)),rng.choice((3,7)),case%4,3 if case%9==0 else 1,(0,4,8,12,16,32,48)[case%7],(4,8,12,16,32,48)[case%6],0]
        f[100]=rng.randrange(1,256)<<8
        if case%13==0:f[3]=rng.choice((0x103,0xff03,14,0x7fff,0x8001))
        f[2]=case%5 if name=='motion-state' else (3 if case%3==0 else 5)
        if name=='motion-state' and f[19]>=32:f[2]=0
        for i in range(6):
            ch=i%3
            for key in range(3):f[28+i*12+key*4:32+i*12+key*4]=[key*4]+[rng.getrandbits(32) if ch==1 else bits(rng.uniform(-12,12)) for axis in range(3)]
        if name=='motion-state':
            n=motion.copy();n[0]=1+case%12;n[1]=rng.randrange(2);n[2]=rng.randrange(2);n[3]=rng.randrange(65536);n[4]=rng.randrange(65536);n[5]=rng.randrange(65536);n[6]=rng.randrange(65536);n[7]=bits(rng.uniform(-20,20));n[8]=bits(rng.uniform(0,1));n[9]=bits(rng.uniform(.1,4));n[10]=bits(rng.uniform(.1,4));n[11]=cw;n[12]=case%3
            for i in range(4):n[17+i]=(rng.randrange(1,256)<<8)|rng.randrange(4);n[21+i]=bits(rng.choice((.125,.5,1)));n[25+i]=bits(rng.uniform(-.75,.75));n[29+i]=rng.randrange(1,32)
            yield dict(case=case,numeric=n,interface=f)
        elif name=='blend-backing':
            n=blend.copy();n[0]=cw;n[1]=bits(rng.choice((-1,0,.5,4,5,8,9,12)));n[2]=rng.choice((3,7));n[3]=rng.choice((3,7));n[4]=10;n[5]=rng.randrange(8);n[6]=rng.randrange(8);n[7]=rng.randrange(1,4)
            for ch in range(3):
                for k in range(3):
                    for a in range(3):n[9+ch*12+k*4+a]=rng.getrandbits(32) if ch==1 else bits(rng.uniform(-50,50))
            yield dict(case=case,numeric=n,interface=f)
        else:
            n=sonic.copy();n[0]=1+case%2;n[1]=cw;n[2]=rng.choice((0,4,0x8000,0xffff));n[3]=rng.randrange(256);n[4]=rng.choice((0,2,255));n[5]=rng.choice((0,1,65535));n[6]=rng.choice((0,0x8000));n[8]=rng.randrange(65536);n[28]=rng.randrange(2);n[35]=rng.randrange(2)
            w=startup.copy();w[0]=rng.randrange(8);w[1]=rng.randrange(8);w[2]=rng.randrange(256);w[11]=rng.randrange(65536)
            for i in range(12,15):w[i]=rng.getrandbits(32)
            for i in range(15,31):w[i]=bits(rng.uniform(-8,8))
            w[31]=1;w[32]=bits(rng.uniform(.25,8))
            events=bytes(rng.choice((0,0,0,2,255)) for i in range(512))
            c=[255 if case%3 else rng.randrange(88),cw,rng.randrange(65536),rng.randrange(65536),rng.randrange(2),case%3]
            yield dict(case=case,sonic=n,startup=w,events_hex=events.hex(),attachment=c,gamma=[rng.randrange(91),rng.randrange(91)])
def pin():
    paths={}
    current=pathlib.Path('/Users/burbujamc/.t3/worktrees/sadx-macos/t3-9b97f112/build/animation-contract')
    # Existing qualified libraries only; copy into this checkout, never load from another build.
    for label,base in [('old',FOUNDATION/'baseline'),('current',current/'root-requalified')]:
        for slug in ('player_animation','player_animation_blend','sonic_player','player_startup'):
            source=(FOUNDATION/'root-frozen/sonic_player.dylib') if label=='old' and slug=='sonic_player' else base/(slug+'.dylib')
            target=OUT/'libraries'/label/(slug+'.dylib');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
            paths[str(target.relative_to(ROOT))]=dict(origin=str(source),sha256=sha(target))
    for slug in ('player_animation','player_animation_blend'):
        source=current/'root-plain'/(slug+'.dylib');target=OUT/'libraries/native'/(slug+'.dylib');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target);paths[str(target.relative_to(ROOT))]=dict(origin=str(source),sha256=sha(target))
    old_ledger=subprocess.check_output(['git','show',OLD+':docs/refactor-inventory/packets/animation-judges/local-artifacts.json'],cwd=ROOT,text=True)
    current_ledger=subprocess.check_output(['git','show',CURRENT+':docs/refactor-inventory/packets/animation-contract/ROOT-REQUALIFIED.json'],cwd=ROOT,text=True)
    for path,record in paths.items():
        ledger=old_ledger if '/libraries/old/' in path else current_ledger
        assert record['sha256'] in ledger, ('Unqualified library',record)
    return paths
def freeze():
    assert not (PACKET/'freeze.json').exists()
    libraries=pin()
    paths=list(HERE.glob('*'));paths=[p for p in paths if p.is_file()]
    definitions={str(p.relative_to(ROOT)):sha(p) for p in paths}
    definitions[str((PACKET/'DEFINITIONS.md').relative_to(ROOT))]=sha(PACKET/'DEFINITIONS.md')
    builds={p.name:json.loads(p.read_text()) for p in OUT.glob('*builds.json')}
    required=[OUT/label/(slug+'.dylib') for label in ('old','current') for slug in ('interface','table','consumer','gamma')]
    assert all(p.exists() for p in required),required
    binary_hashes={str(p.relative_to(ROOT)):sha(p) for p in required}
    inputs={str(EXE):sha(EXE)}
    for name in ('sonic-selection.actions','gamma-actor.json','sonic-actor.json'):inputs[str(IMPORTS/'characters'/name)]=sha(IMPORTS/'characters'/name)
    compiler=subprocess.check_output(['xcrun','clang++','--version'],text=True)
    source_tree=subprocess.check_output(['git','rev-parse',CURRENT+':src'],cwd=ROOT,text=True).strip()
    before_tree=subprocess.check_output(['git','rev-parse',OLD+':src'],cwd=ROOT,text=True).strip()
    receipt=dict(source_tree=source_tree,before_tree=before_tree,source=CURRENT,before=OLD,cohorts=COHORTS,definitions=definitions,libraries=libraries,custom_libraries=binary_hashes,builds=builds,inputs=inputs,compiler=compiler,python=sys.executable,python_sha256=sha(pathlib.Path(sys.executable).resolve()),generated=False,executed=False,policy='Execute each cohort once. Stop affected cohort at first value/interface mismatch. FP-only differences remain partial; no inputs or expectations may change after freeze.')
    (PACKET/'freeze.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Frozen; commit freeze.json and definitions before running execute.')
def array(v):return (C.c_uint32*len(v))(*v)
def function(path,name,args,ret=C.c_int):
    lib=C.CDLL(str(path));fn=getattr(lib,name);fn.argtypes=args;fn.restype=ret;return fn
def numeric(slug,n,constants,oracles):
    index={'player_animation':0,'player_animation_blend':1,'sonic_player':2}[slug]
    source=oracles[index].run(n)
    count={'player_animation':15,'player_animation_blend':35,'sonic_player':40}[slug]
    outputs={}
    for label in ('old','current')+(() if slug=='sonic_player' else ('native',)):
        fn=function(OUT/'libraries'/label/(slug+'.dylib'),'sadx_native_'+slug,([U8,C.c_size_t,U32,U32] if slug=='sonic_player' else [C.POINTER(C.c_float),U32,U32]))
        out=(C.c_uint32*count)()
        status=fn(constants[1],len(constants[0]),array(n),out) if slug=='sonic_player' else fn(constants,array(n),out)
        outputs[label]=dict(return_code=status,output=list(out))
    a=outputs['current']['output'];different=[];partial=False
    if slug=='player_animation':
        expected,boundary,returned=source
        different=[i for i,v in enumerate(expected) if i!=9 and a[i]!=v]
        if returned!=bool(a[13]) or boundary and a[12]!=boundary:different.append('boundary')
        partial=a[9]!=expected[9]
    elif slug=='player_animation_blend':different=[i for i,v in enumerate(source) if i!=30 and a[i]!=v];partial=a[30]!=source[30]
    else:
        expected,boundary=source;different=[i for i,v in expected.items() if i!=17 and a[i]!=v];partial=17 in expected and a[17]!=expected[17]
    equal=len({dumps(o) for o in outputs.values()})==1
    return dict(source=source,observations=outputs,defined_value_differences=different,fp_partial=partial,old_current_native_equal=equal),bool(different or not equal or any(o['return_code'] for o in outputs.values()))
def startup(n,events,cw,parameters,oracle):
    source=oracle.run(n,events,parameters,cw);outputs={}
    for label in ('old','current'):
        fn=function(OUT/'libraries'/label/'player_startup.dylib','sadx_native_player_startup',[U8,C.c_size_t,U8,C.c_size_t,U32,C.c_size_t,C.c_uint16,U32,C.c_size_t,U8,C.c_size_t])
        out=(C.c_uint32*24)();snap=(C.c_ubyte*1376)()
        rc=fn((C.c_ubyte*1056).from_buffer_copy(parameters),1056,(C.c_ubyte*512).from_buffer_copy(events),512,array(n),33,cw,out,24,snap,1376)
        outputs[label]=dict(return_code=rc,output=list(out),snapshot=bytes(snap))
    a=outputs['current'];out=a['output'];boundary=out[2] if out[0]==4 and out[2] in (0x417e85,0x417e95,0x417e9b) else out[1]
    differences=[]
    checks={'state':(source['snapshot'],a['snapshot']),'boundary':(source['boundary'],boundary),'resource':([source[k] for k in ('mask','player','contact','extra','motion_player','object','free_slots','free_head')],[out[6],*out[9:14],out[15],out[16]]),'producer_fact':([(1|(source['snapshot'][0]<<8)) if out[19] else 0,15 if source['player'] else 0,0,3 if source['player'] else 0],out[20:24]),'control':(source['cw'],out[8])}
    differences=[key for key,(x,y) in checks.items() if x!=y]
    equal=outputs['old']==outputs['current']
    return dict(source=source,observations=outputs,defined_value_differences=differences,fp_partial=source['flags']!=out[7],old_current_equal=equal),bool(differences or not equal or any(o['return_code'] for o in outputs.values()))
def table_oracle(image):
    from verify_original_math import Original,DATA,STACK,STOP
    import unicorn
    from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
    original=Original(image,0x7f);m=original.machine;TABLE=0x3c56210;EXPORT=DATA+0x1000;events=[]
    m.mem_map(0x38f6000,0x1000);m.mem_map(0x3c56000,0x2000);m.mem_write(0x38f6e7c,struct.pack('<I',EXPORT))
    def read(machine,access,address,size,value,user):
        if EXPORT<=address<EXPORT+149*4:events.append(['read',machine.reg_read(UC_X86_REG_EIP),(address-EXPORT)//4])
    def write(machine,access,address,size,value,user):
        if TABLE<=address<TABLE+2352:
            row,offset=divmod(address-TABLE,16);field=(0,4,5,6,8,12).index(offset);events.append(['write',machine.reg_read(UC_X86_REG_EIP),row,field,value&((1<<(size*8))-1)])
    m.hook_add(unicorn.UC_HOOK_MEM_READ,read);m.hook_add(unicorn.UC_HOOK_MEM_WRITE,write)
    def run(f):
        rows=[]
        for i in range(147):
            mask,*values=f[149+i*7:156+i*7];rows.append([int(mask!=64)]+[part for field,v in enumerate(values) for part in (((2 if field==0 else 1) if mask!=64 and mask&(1<<field) else 0),v if mask!=64 and mask&(1<<field) else 0)])
        output=[];source=[]
        for phase in range(2):
            missing,size,null,state=f[1178+phase*4:1182+phase*4];exports=f[:149].copy()
            if null<149:exports[null]=0
            m.mem_write(EXPORT,struct.pack('<149I',*exports));m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);events.clear();m.emu_start(0x7cec90,STOP,count=5000)
            assert m.reg_read(UC_X86_REG_EIP)==STOP and len(events)==1027
            full=[e.copy() for e in events];source.append(dict(events=full,raw_table_hex=bytes(m.mem_read(TABLE,2352)).hex(),returned=True))
            failure,boundary,touched=0,0,[0]*147
            if state:failure,boundary=(1,0x7cec90) if state==1 else (2,next(e[1] for e in full if e[0]=='read'))
            else:
                for e in full:
                    if e[0]=='read':
                        if e[2]==missing or e[2]>=size:failure,boundary=1,e[1];break
                    else:
                        _,pc,row,field,value=e;rows[row][0]=1;rows[row][1+field*2:3+field*2]=[2 if field==0 and value else 1,value];touched[row]|=1<<field
            output += [failure,boundary,touched.count(63)]+[v for row in rows for v in row]
        return output,source
    return run

def execute():
    freeze=json.loads((PACKET/'freeze.json').read_text())
    for p,h in freeze['definitions'].items():assert sha(ROOT/p)==h,p
    for p,h in freeze['custom_libraries'].items():assert sha(ROOT/p)==h,p
    for p,v in freeze['libraries'].items():assert sha(ROOT/p)==v['sha256'],p
    for p,h in freeze['inputs'].items():assert sha(pathlib.Path(p))==h,p
    import unicorn
    assert unicorn.__version__=='2.1.4'
    # Global marker forbids accidental regeneration or rerun of the reserved cases.
    fd=os.open(OUT/'EXECUTED_ONCE',os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600);os.write(fd,b'Final cohort authorization consumed. Never rerun or alter inputs.\n');os.close(fd)
    input_receipts={}
    for name,seed,count in COHORTS:
        path=OUT/(name+'.inputs.jsonl')
        with path.open('x') as stream:
            for row in fixtures(name,seed,count):stream.write(dumps(row)+'\n')
        input_receipts[name]=dict(seed=seed,cases=count,path=str(path),sha256=sha(path))
    (OUT/'generated-inputs.json').write_text(json.dumps(input_receipts,indent=2)+'\n')
    m,b,s,p=modules();from pe_image import PEImage
    from extract import SOURCE_SHA256
    image=PEImage(EXE);assert image.sha256==SOURCE_SHA256
    constants=(C.c_float*3).from_buffer_copy(b''.join(image.read(a,4) for a in m.CONSTANTS))
    sonic_constants=b''.join(image.read(a,4) for a in s.CONSTANTS);sonic_buffer=(C.c_ubyte*len(sonic_constants)).from_buffer_copy(sonic_constants)
    parameters=image.read(p.PARAMETERS,1056)
    oracles=[m.AnimationOriginal(image),b.BlendOriginal(image),s.SonicOriginal(image)];start=p.StartupOriginal(image);table=table_oracle(image)
    custom={label:{slug:function(OUT/label/(slug+'.dylib'),'final_'+slug,([U32,U32] if slug=='table' else [U8,C.c_size_t,U32] if slug=='consumer' else [C.c_char_p,U32] if slug=='gamma' else [U32]),C.c_int if slug=='table' else C.c_char_p) for slug in ('table','interface','consumer','gamma')} for label in ('old','current')}
    summaries=[]
    for name,seed,count in COHORTS:
        report=dict(name=name,seed=seed,requested=count,generated=count,executed=0,mismatches=[],fp_partial_ids=[],input=input_receipts[name],source_contract='original instructions where recorded; lazy availability/aliases/allocation identity contrasted old/current separately',native_contract='plain current numeric probe for motion/blend; actual initialized-view canonical identity for consumers',gaps=[])
        if name=='consumer-attachment':report['gaps']=['Gamma binding uses the actual imported table and the same initialized-view assignment as native_gamma_binding; the complete runtime Context function is not linked. PSetMotion remains UNRECOVERED. Sonic initialized action-0 table injection is not executed by this attachment probe; actual updater routing and startup are executed separately.']
        path=OUT/(name+'.outputs.jsonl.gz')
        with gzip.open(path,'xt') as stream:
            for line in (OUT/(name+'.inputs.jsonl')).open():
                f=json.loads(line);case=f['case'];record=dict(case=case,input=f);bad=False;partials=[]
                try:
                    if name=='table-retry':
                        expected,source=table(f['table']);outputs={}
                        for label in ('old','current'):
                            out=(C.c_uint32*(2*(3+147*13)))();rc=custom[label]['table'](array(f['table']),out);outputs[label]=dict(return_code=rc,output=list(out))
                        record.update(original=source,availability_projection=expected,observations=outputs)
                        bad=any(o['return_code'] or o['output']!=expected for o in outputs.values())
                    elif name in ('motion-state','blend-backing'):
                        slug='player_animation' if name=='motion-state' else 'player_animation_blend'
                        result,bad=numeric(slug,f['numeric'],constants,oracles);record['numeric']=result
                        if result['fp_partial']:partials.append('numeric')
                        outputs={label:json.loads(custom[label]['interface'](array(f['interface']))) for label in ('old','current')}
                        record['interface']=outputs;bad=bad or outputs['old']!=outputs['current']
                    else:
                        result,bad=numeric('sonic_player',f['sonic'],(sonic_constants,sonic_buffer),oracles);record['sonic']=result
                        if result['fp_partial']:partials.append('sonic')
                        result,failed=startup(f['startup'],bytes.fromhex(f['events_hex']),f['attachment'][1],parameters,start);record['startup']=result;bad=bad or failed
                        if result['fp_partial']:partials.append('startup')
                        for slug in ('consumer','gamma'):
                            outputs={label:json.loads(custom[label][slug](sonic_buffer,len(sonic_constants),array(f['attachment'])) if slug=='consumer' else custom[label][slug](os.fsencode(IMPORTS),array(f['gamma']))) for label in ('old','current')}
                            record[slug]=outputs;bad=bad or outputs['old']!=outputs['current']
                    record['mismatch']=bad
                except Exception:
                    record['exception']=traceback.format_exc();bad=True
                stream.write(dumps(record)+'\n');stream.flush();report['executed']+=1
                if partials:report['fp_partial_ids'].append(dict(case=case,observations=partials,input_sha256=hashlib.sha256(dumps(f).encode()).hexdigest()))
                if bad:
                    report['mismatches'].append(dict(case=case,output_record=case,reason='See complete retained output record; no Source, fixture or tolerance edits made.'))
                    report['stopped_at_first_mismatch']=True;break
                if case%128==0:print(name,case,flush=True)
        report['output_sha256']=sha(path);report['original_exit_status']=1 if report['mismatches'] else 2 if report['fp_partial_ids'] else 0
        report['verdict']='MISMATCH_STOPPED' if report['mismatches'] else 'PARTIAL' if report['fp_partial_ids'] else 'VALUE_EXACT'
        if report['gaps']:report['coverage_verdict']='INCOMPLETE_RESERVED_DESCRIPTION'
        else:report['coverage_verdict']='BOUNDED_DESCRIPTION_EXECUTED'
        (PACKET/(name+'.json')).write_text(json.dumps(report,indent=2)+'\n');summaries.append(report)
        print(name,report['verdict'],report['executed'],len(report['fp_partial_ids']),flush=True)
    (PACKET/'results.json').write_text(json.dumps(summaries,indent=2)+'\n')
    return int(any(r['mismatches'] for r in summaries))
if __name__=='__main__':
    if sys.argv[1:] == ['freeze']:freeze()
    elif sys.argv[1:] == ['execute']:raise SystemExit(execute())
    else:raise SystemExit('Choose freeze or execute')
