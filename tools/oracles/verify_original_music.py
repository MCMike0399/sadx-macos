"""Title music tasks, ADX manager and WMP game wrappers against original instructions.

Task allocation/destruction, reader open/start/stop/reset/volume, clock and child
traversal are declared native-resource receipts. Original task publication, work
stores, catalog reads, path construction, media flags, fallback/loop decisions and
live state reloads execute. Media group executes actual reader-state getter; it
never maps page0, and checks null-player/allocation faults at their real boundary.
Reads outside the imported voice-duration catalog stop at a tagged reference
missing-data boundary and are reported separately as bounded-prefix cases; their
adjacent source memory is not fabricated as a voice duration. No WMA codec/sample
equivalence or Windows thread latency claim.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, CODE, STACK, STOP

BASE=0x22000000
TASKS=BASE+0x1000
WORK=BASE+0x2000
MUSIC_PLAYER=BASE+0x3000
VOICE_PLAYER=BASE+0x3100
BUFFER=BASE+0x3200
VTABLE=BASE+0x3300
STRINGS=BASE+0x4000
FORMAT,CLOCK,WIDE,SLEEP,VOLUME,EVENT=[CODE+value for value in (0x300,0x310,0x320,0x330,0x340,0x350)]
TRACKS=[0x912698,0x91269c,0x9126a8,0x912694,0x3b29cf8,0x3b29cf4]
PREFIXES=['','D:\\','X:\\owned\\']

class Source:
    def __init__(self,image):
        assert image.sha256==SOURCE_SHA256 and unicorn.__version__=='2.1.4'
        self.m=m=Original(image,127).machine
        for section in image.sections:
            if section['name'] in ('.text','.rdata'):continue
            size=(max(section['raw_size'],section['virtual_size'])+4095)&~4095
            m.mem_map(image.base+section['rva'],size)
            m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
        m.mem_map(BASE,0x20000)
        for at,value in [(0x7db228,FORMAT),(0x7db270,CLOCK),(0x7db080,WIDE),(0x7db07c,SLEEP),(0x7db0a8,EVENT)]:self.put(at,value)
        self.put(VTABLE+0x3c,VOLUME)
        self.put(VTABLE+0x48,EVENT)
        for index in range(125):
            pointer=STRINGS+index*32
            m.mem_write(pointer,('song'+str(index)).encode()+b'\0')
            self.put(0x910298+index*8,pointer);self.put(0x91029c+index*8,index%3-1)
        for index in range(2049):
            self.put(0x910680+index*2,(index*17+7)&65535,2)
            self.put(0x911688+index*2,(index*23+11)&65535,2)
        for index,song in enumerate([102,101,72,24,103]):self.put(0x7ee8f8+index*4,song)
        m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
        self.context=m.context_save()
    def put(self,at,value,size=4):self.m.mem_write(at,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
    def get(self,at,size=4):return int.from_bytes(self.m.mem_read(at,size),'little')
    def string(self,at,wide=False):
        result=[];step=2 if wide else 1
        while self.get(at,step):result.append(chr(self.get(at,step)));at+=step
        return ''.join(result)
    def ret(self,value=0,pop=0):
        m=self.m;sp=m.reg_read(UC_X86_REG_ESP)
        m.reg_write(UC_X86_REG_EAX,value&0xffffffff);m.reg_write(UC_X86_REG_ESP,sp+4+pop);m.reg_write(UC_X86_REG_EIP,self.get(sp))
    def track_words(self):return [self.get(at) for at in TRACKS]
    def manager_event(self,kind,text='',argument=0):
        words=self.track_words()+[self.get(0x3b29cec),self.get(0x3b29cf0),int(self.get(0x3a7cf3c)!=0),int(self.get(0x38f6ec0)!=0),self.get(0x3b0ef38)]
        self.events.append('|'.join(map(str,[kind,text,argument]+words))+'\n')
        calls=len(self.events);resource=kind not in (1,4)
        if resource and self.values[21] and calls==self.values[21]:self.failed=1;self.m.emu_stop();return
        if resource and self.values[22] and calls==self.values[22]:
            for at,value in zip([TRACKS[0],TRACKS[3],0x3b0ef38,0x3a7cf3c,0x38f6ec0,0x3b29cec,0x3b29cf0],self.values[23:30]):self.put(at,value)
    def media_event(self,kind,text='',argument=0):
        words=[self.get(at) for at in [0x3abdfa0,0x3abdf98,0x3abdfa8,0x3abdfa4,0x909f28]]
        words += [int(self.get(MUSIC_PLAYER+4)!=0),self.get(MUSIC_PLAYER+0x44)]
        self.events.append('|'.join(map(str,[kind,text,argument]+words))+'\n')
        calls=len(self.events)
        if self.values[13] and calls==self.values[13]:self.failed=1;self.m.emu_stop();return
        if self.values[14] and calls==self.values[14]:
            for at,value in zip([0x3abdfa4,0x3abdfa0,0x3abdf98,0x909f28],self.values[15:19]):self.put(at,value)
    def tid(self,pointer):return (pointer-TASKS)//64 if pointer else 0
    def task_event(self,kind,a=0,b=0,c=0):
        words=[self.tid(self.get(at)) for at in [0x3c5e8c8,0x3c5e8cc,0x3b29d00]]+self.track_words()
        self.events.append('|'.join(map(str,[kind,a,b,c]+words))+'\n')
        calls=len(self.events)
        if self.values[14] and calls==self.values[14]:self.failed=1;self.m.emu_stop();return
        if self.values[15] and calls==self.values[15]:
            for at,value in zip([0x3c5e8c8,0x3c5e8cc,0x3b29d00],self.values[16:19]):self.put(at,TASKS+value*64 if value else 0)
    def hook(self,m,at,size,user):
        sp=m.reg_read(UC_X86_REG_ESP)
        if at==EVENT:self.ret(1,4);return
        if at==FORMAT:
            dest=self.get(sp+4);fmt=self.string(self.get(sp+8));prefix=self.string(self.get(sp+12));second=self.get(sp+16)
            if '%04d' in fmt:second=ctypes.c_int32(second).value
            else:second=self.string(second)
            text=fmt%(prefix,second);m.mem_write(dest,text.encode()+b'\0');self.ret(len(text));return
        if at==WIDE:
            text=self.string(self.get(sp+12));dest=self.get(sp+20)
            m.mem_write(dest,text.encode('utf-16-le')+b'\0\0');self.ret(len(text)+1,24);return
        if self.group=='manager':
            if at in (0x42552b,0x42559e):
                register=UC_X86_REG_ECX if at==0x42552b else UC_X86_REG_EAX
                voice=m.reg_read(register)
                if voice>=2049:
                    self.boundary={'instruction':hex(at),'voice':ctypes.c_int32(voice).value,
                                   'fact':'voice duration outside imported2049-entry table'}
                    self.failed=4;m.emu_stop();return
            if at==0x40ff10:self.manager_event(1)
            elif at==0x40cd20:
                self.manager_event(2,self.string(self.get(sp+4)),self.get(sp+8))
                if not self.failed:self.ret(self.values[13 if self.music_calls else 12]);self.music_calls+=1
            elif at==0x40cf50:
                self.manager_event(3)
                if not self.failed:self.ret()
            elif at==0x40cfe0:self.manager_event(4)
            elif at==0x40ce30:
                self.manager_event(5,self.string(self.get(sp+4)))
                if not self.failed:self.ret(self.values[15 if self.voice_calls else 14]);self.voice_calls+=1
            elif at==0x40cf20:
                self.manager_event(6)
                if not self.failed:self.ret(self.values[17])
            elif at==CLOCK:
                self.manager_event(7)
                if not self.failed:self.ret(self.values[18]+self.clocks*self.values[19]);self.clocks+=1
            elif at==0x40b420:
                self.manager_event(8)
                if not self.failed:self.ret()
        elif self.group=='media':
            if at in (0x411760,0x411720):
                self.media_event(1 if at==0x411760 else 2)
                if not self.failed:self.put(MUSIC_PLAYER+4,0);self.ret()
            elif at==0x411830:
                self.media_event(3,self.string(self.get(sp+4),True))
                if not self.failed:self.ret(self.values[8],4)
            elif at==0x410ff0:
                assert self.get(sp+4)==0 and self.get(sp+8)==0
                self.media_event(4,'',self.get(sp+12))
                if not self.failed:
                    self.put(MUSIC_PLAYER+4,BUFFER);self.put(MUSIC_PLAYER+0x44,2 if self.pending else 1);self.ret(pop=12)
            elif at==SLEEP:
                assert self.get(sp+4)==0
                self.media_event(6)
                if not self.failed:
                    if self.pending:self.pending-=1
                    if self.pending==0:self.put(MUSIC_PLAYER+0x44,1)
                    self.ret(pop=4)
            elif at==VOLUME:
                self.media_event(5,'',self.get(sp+8))
                if not self.failed:self.ret(pop=8)
        elif self.group=='tasks':
            if at==0x40b570:
                self.task_event(1,self.tid(self.get(sp+4)))
                if not self.failed:self.ret()
            elif at==0x40b860:
                flags,level,callback=[self.get(sp+i) for i in (4,8,12)]
                manager=callback==0x425830
                assert callback in (0x425830,0x5058c0)
                self.task_event(2 if manager else 4,flags,level)
                if not self.failed:self.ret(0 if self.values[19 if manager else 20] else TASKS+(5 if manager else 6)*64)
            elif at==0x40b2d0:
                assert self.get(sp+4)==1 and self.get(sp+8)==8
                self.task_event(5,self.tid(m.reg_read(UC_X86_REG_ESI)),1,8)
                if not self.failed:self.ret(0 if self.values[21] else WORK+0x100)
            elif at==0x425670:
                self.task_event(6)
                if not self.failed:self.ret()
            elif at==0x425842:
                self.task_event(3,self.tid(m.reg_read(UC_X86_REG_EAX)))
                if not self.failed:self.installed=self.tid(m.reg_read(UC_X86_REG_EAX))
    def prepare(self,group,values):
        self.group=group;self.values=values;self.events=[];self.failed=0;self.boundary=None
        self.m.context_restore(self.context);self.put(STACK,STOP);self.m.reg_write(UC_X86_REG_ESP,STACK)
    def execute(self,entry):
        try:self.m.emu_start(entry,STOP)
        except unicorn.UcError:
            if self.group=='media' and self.m.reg_read(UC_X86_REG_EIP)==0x4113e0 and self.m.reg_read(UC_X86_REG_ECX)==0:self.failed=7
            elif self.group=='tasks' and self.m.reg_read(UC_X86_REG_EIP)==0x505981:self.failed=6
            else:raise
        if not self.failed and self.m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original did not return')
    def manager(self,v):
        self.prepare('manager',v);self.clocks=self.music_calls=self.voice_calls=0
        for at,value in zip(TRACKS,v[:6]):self.put(at,value)
        for at,value in zip([0x3b29cec,0x3b29cf0,0x3a7cf3c,0x38f6ec0,0x3b0ef38,0x3b0ef2c],v[6:12]):self.put(at,value)
        self.put(0x3abdf98,v[16]);self.m.mem_write(0x3a8af78,PREFIXES[v[20]].encode()+b'\0')
        self.put(STACK+4,TASKS+7*64);self.execute(0x4253d0)
        out=[0]*32;out[:13]=[self.failed]+self.track_words()+[self.get(0x3b29cec),self.get(0x3b29cf0),int(self.get(0x3a7cf3c)!=0),int(self.get(0x38f6ec0)!=0),self.get(0x3b0ef38),len(self.events)]
        return out,''.join(self.events)
    def media(self,v):
        self.prepare('media',v);self.pending=v[11]
        for at,value in [(0x3abdf9c,MUSIC_PLAYER if v[1] else 0),(0x3abdf94,VOICE_PLAYER if v[2] else 0),
                          (0x3abdfa0,v[3]),(0x3abdf98,v[4]),(0x3abdfa8,v[5]),(0x3abdfa4,v[6]),(0x909f28,v[7]),
                          (MUSIC_PLAYER+4,BUFFER if v[9] else 0),(MUSIC_PLAYER+0x44,v[10]),(VOICE_PLAYER+0x44,v[12]),(BUFFER,VTABLE)]:self.put(at,value)
        self.m.mem_write(STRINGS+0x3000,b'SYSTEM\\SoundData\\BGM\\WMA\\title2.wma\0')
        self.put(STACK+4,STRINGS+0x3000);self.put(STACK+8,v[6]);self.execute([0x40cd20,0x40cf50,0x40cfe0,0x40cf20][v[0]])
        out=[0]*32;out[0]=self.failed
        if not self.failed:out[1]=self.m.reg_read(UC_X86_REG_EAX)
        out[2:10]=[self.get(at) for at in [0x3abdfa0,0x3abdf98,0x3abdfa8,0x3abdfa4,0x909f28]]+[int(self.get(MUSIC_PLAYER+4)!=0),self.get(MUSIC_PLAYER+0x44),len(self.events)]
        return out,''.join(self.events)
    def tasks(self,v):
        self.prepare('tasks',v);self.installed=0
        self.m.mem_write(TASKS,bytes(8*64));self.m.mem_write(WORK,bytes(512))
        for at,value in zip([0x3c5e8c8,0x3c5e8cc,0x3b29d00],v[1:4]):self.put(at,TASKS+value*64 if value else 0)
        for at,value in zip(TRACKS,v[4:10]):self.put(at,value)
        self.put(0x91268c,v[10]);self.put(0x3c5e8e0,v[11]);self.put(TASKS+7*64+0x2c,WORK)
        self.put(WORK,v[12]);self.put(WORK+4,v[13])
        op=v[0]
        self.put(STACK+4,TASKS+7*64 if op in (1,4) else v[22] if op==3 else v[12])
        self.execute([0x4258b0,0x425830,0x505900,0x505990,0x5058c0,0x425690,0x4256d0,0x505870][op])
        out=[0]*32;out[0]=self.failed
        if op==6 and not self.failed:out[1]=self.m.reg_read(UC_X86_REG_EAX)
        out[2:8]=self.track_words();out[8:11]=[self.tid(self.get(at)) for at in [0x3c5e8c8,0x3c5e8cc,0x3b29d00]]
        out[11:15]=[len(self.events),self.installed,self.get(WORK),self.get(WORK+4)]
        pointer=self.get(TASKS+6*64+0x2c)
        if pointer:out[15:18]=[1,self.get(pointer),self.get(pointer+4)]
        return out,''.join(self.events)

def qualify_reader_contract(source):
    source.prepare('contract',[])
    m=source.m
    readiness=[]
    for initialized in [0,1,17,0xffffffff]:
        source.put(0x3b0ef2c,initialized);source.put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x40ff10,STOP)
        readiness.append(m.reg_read(UC_X86_REG_EAX))
    assert readiness==[0,1,17,0xffffffff]
    # Successful reader initialization's actual final state store, followed by
    # its actual getter. This does not invent Windows handles or claim native
    # device initialization; that owner is independently qualified by runtime.
    m.reg_write(UC_X86_REG_ESI,MUSIC_PLAYER)
    m.emu_start(0x4116ee,0x4116f5)
    source.put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_ECX,MUSIC_PLAYER)
    m.emu_start(0x4113e0,STOP)
    initial=m.reg_read(UC_X86_REG_EAX)
    assert initial==2
    states={}
    for status,expected in [(2,0),(3,1),(11,1),(12,2)]:
        source.put(MUSIC_PLAYER+0x44,99);source.put(MUSIC_PLAYER+4,BUFFER);source.put(BUFFER,VTABLE);source.put(STACK,STOP)
        for index,value in enumerate([MUSIC_PLAYER,status,0,0,0,0]):source.put(STACK+4+index*4,value)
        m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x411490,STOP)
        states[str(status)]=source.get(MUSIC_PLAYER+0x44)
        assert states[str(status)]==expected
    return {'readiness_values_preserved':readiness,'initial_reader_state':initial,
            'original_status_to_reader_state':states,
            'scope':'Actual getter/init store/status callback instructions; DSBuffer.Stop/SetEvent are opaque notification receipts.'}

def fixtures(seed,count):
    rng=random.Random(seed);groups={name:[] for name in ['manager','media','tasks']}
    for _ in range(count):
        song=rng.choice([0,1,102,124,0xffffffff]);real=rng.choice([song,0xffffffff]);voice=rng.choice([0,1,2048,0xffffffff])
        groups['manager'].append([song,rng.getrandbits(32),real,voice,rng.getrandbits(32),rng.getrandbits(32),rng.randrange(65536),rng.getrandbits(32),rng.randrange(2),rng.randrange(2),rng.choice([0,1,0xffffffff]),rng.choice([0,1,7]),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.choice([0,3,5]),rng.choice([0,3,5]),rng.getrandbits(32),rng.randrange(100),rng.randrange(3),rng.choice([0,0,rng.randrange(1,12)]),rng.choice([0,0,rng.randrange(1,12)]),rng.choice([0,102,124]),rng.choice([0,1,2048,0xffffffff]),rng.choice([0,1]),rng.randrange(2),rng.randrange(2),rng.randrange(65536),rng.getrandbits(32)])
        groups['media'].append([rng.randrange(4),rng.randrange(2),rng.randrange(2),rng.choice([0,1,7]),rng.getrandbits(32),rng.getrandbits(32),rng.choice([0,1,0xffffffff]),rng.choice([0,0xffffd8f0,0xffffe318]),rng.choice([0,1,0x80070002]),rng.randrange(2),rng.choice([0,1,2]),rng.randrange(4),rng.choice([0,1,2]),rng.choice([0,0,rng.randrange(1,9)]),rng.choice([0,0,rng.randrange(1,6)]),rng.choice([0,1,0xffffffff]),rng.choice([0,1,7]),rng.getrandbits(32),rng.choice([0,0xffffd8f0])])
        groups['tasks'].append([rng.randrange(8),rng.randrange(4),rng.randrange(4),rng.randrange(4),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.choice([0,1,7]),rng.choice([0,1,7]),rng.choice([0,102,0xffffffff]),rng.choice([0,1,2,0x7fffffff,0xffffffff]),rng.choice([0,0,rng.randrange(1,8)]),rng.choice([0,0,rng.randrange(1,8)]),rng.randrange(4),rng.randrange(4),rng.randrange(4),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(5)])
    # Target the title request, delayed startup, unsigned voice clock and media
    # completion branches independently of random cohort coverage.
    base=[102,0xffffffff,0xffffffff,0xffffffff,0,0,0,0,1,1,0,1,1,1,1,1,3,5,10,1,1,0,0,102,0,0,1,1,0,0]
    groups['manager'].append(base.copy())
    for duration,started,now in [(0,10,10),(30,0xfffffff0,5),(30,10,40)]:
        v=base.copy();v[0]=0xffffffff;v[6]=duration;v[7]=started;v[18]=now;groups['manager'].append(v)
    v=base.copy();v[0]=0xffffffff;v[3]=1;v[14]=0;v[22]=4;v[24]=2;v[25]=1;groups['manager'].append(v)
    for operation in (0,1):
        for player in (0,1):
            for opened in (0,1):
                for loop in (0,1):
                    for reader in (1,2):
                        groups['media'].append([operation,player,1,opened,3,0,loop,0xffffe318,0,1,reader,2,2,0,0,0,0,0,0])
    for operation in range(8):
        for fail in range(8):
            groups['tasks'].append([operation,1,2,3,8,9,10,11,12,13,1,1,102,1,fail,0,0,0,0,0,0,0,0])
    return groups

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--seed',type=int,default=20261108);p.add_argument('--cases',type=int,default=400)
    a=p.parse_args();image=PEImage(a.exe);source=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()));U=ctypes.c_uint32
    contract=qualify_reader_contract(source)
    groups=fixtures(a.seed,a.cases);bad=[];bounded=[]
    for group,cases in groups.items():
        probe=getattr(lib,'sadx_native_music_'+group);probe.argtypes=[ctypes.POINTER(U),ctypes.POINTER(U),ctypes.c_char_p,ctypes.c_size_t]
        for number,values in enumerate(cases):
            expected,expected_trace=getattr(source,group)(values);out=(U*32)();trace=ctypes.create_string_buffer(16384)
            if source.boundary:bounded.append({'group':group,'case':number,'input':values,**source.boundary})
            assert probe((U*len(values))(*values),out,trace,len(trace))==1
            differences=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
            if differences or expected_trace!=trace.value.decode():bad.append({'group':group,'case':number,'input':values,'differences':differences,'expected_trace':expected_trace,'actual_trace':trace.value.decode()})
    paths=['src/game/music.hpp','src/game/music.cpp','tests/probes/native_music_probe.cpp']
    ranges=[(0x505870,0x5058ad),(0x505900,0x50598d),(0x505990,0x5059a4),(0x5058c0,0x5058fc),(0x4258b0,0x4258e6),(0x425830,0x425856),(0x425690,0x4256a8),(0x4256d0,0x4256d6),(0x4253d0,0x425620),(0x40cd20,0x40ce2e),(0x40cf50,0x40cfe0),(0x40cfe0,0x40cfe6),(0x40cf20,0x40cf50),(0x40ff10,0x40ff16),(0x4113e0,0x4113e4),(0x4116ee,0x4116f5),(0x411490,0x4116a0)]
    report={'source_sha256':image.sha256,'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'seed':a.seed,'cases':sum(map(len,groups.values())),'group_cases':{k:len(v) for k,v in groups.items()},'mismatch_count':len(bad),'mismatches':bad[:12],'stores_calls_live_reads_match':not bad,'limits':__doc__,'reader_contract_proof':contract,'fully_executed_cases':sum(map(len,groups.values()))-len(bounded),'bounded_prefix_cases':len(bounded),'unsupported_data_boundaries':bounded,'native_sources':{path:hashlib.sha256(Path(path).read_bytes()).hexdigest() for path in paths},'source_ranges':{f'{hex(start)}-{hex(end)}':hashlib.sha256(image.read(start,end-start)).hexdigest() for start,end in ranges}}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':report['cases'],'mismatch_count':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
