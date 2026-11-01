"""Additional instruction-backed writer callback characterization, not an old judge edit.

Original CRT functions remain receipts. Known scalar/data values and published
pointer identities are compared at service entry, after callback mutation, and
at exit/stop. Allocator bytes before formatting remain UNKNOWN and excluded.
Native exceptions correspond to stopping at that reached service, not a claim
that Windows CRT throws. Original filename allocation and pointer are distinct.
"""
from pathlib import Path
import json
import hashlib
import struct
import sys
sys.path[:0]=['tools/lib','tools/oracles']
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_save_write import Source as Base,FIELDS,SCRATCH
from verify_original_math import CODE,STACK,STOP
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX

SAVE=0x3b2b3a8
class Source(Base):
    def snapshot(self,event):
        values=[int.from_bytes(self.m.mem_read(a,n),'little') for a,n in FIELDS]
        pointer=self.get(0x3b290dc)
        identity={0:0,SCRATCH:1,self.name_allocation:2,SCRATCH+0x800:3}.get(pointer,4)
        self.snapshots.append((event,values+[identity],bytes(self.m.mem_read(SAVE,1392)).hex()))
    def hook(self,m,address,size,user):
        sp=m.reg_read(UC_X86_REG_ESP)
        if (address==0x644df9 and self.get(sp+4)==0) or (address==0x644e89 and self.null_open):
            self.stopped=address;m.emu_stop();return
        event={CODE+0x600:1,0x42d630:2,0x644e51:3,0x645002:4,0x644de6:6,0x644e89:7,0x644c7d:8,0x40f350:9}.get(address,0)
        if address==0x645002:
            if self.get(sp+4)==14:self.name_allocation=self.allocate
            else:event=0
        if address==0x644e51 and self.get(sp+4)!=SCRATCH:event=0
        if address==0x644df9 and self.get(sp+4)==self.name_allocation:event=5
        if event:
            if event==7:self.entry_written=bytes(m.mem_read(SAVE,1392)).hex()
            self.snapshot(event)
            if self.fail==event and not self.after:
                self.stopped=address;m.emu_stop();return
            if self.mutation==event:
                values=[None,31,32,33,34,35,36,37,None,None,39,40,38]
                for (a,n),v in zip(FIELDS,values):
                    if v is not None:self.put(a,v,n)
                self.put(SAVE+4,0x98760000+event)
                if event==1:self.put(FIELDS[8][0],1,1)
                if event==2:self.put(FIELDS[9][0],0,1)
                if event in (3,8):self.put(0x3b290dc,SCRATCH+0x800)
                self.snapshot(event+100)
            if self.fail==event and self.after:
                self.stopped=address;m.emu_stop();return
        super().hook(m,address,size,user)
        if address==0x645002 and self.get(sp+4)==14 and self.null_allocation:
            m.reg_write(UC_X86_REG_EAX,0)
        if address==0x644de6 and self.null_open:m.reg_write(UC_X86_REG_EAX,0)
    def characterize(self,mutation,fail,after,mode=-1):
        self.mutation,self.fail,self.after=mutation,fail,after
        self.snapshots=[];self.stopped=0;self.entry_written=None;self.name_allocation=-1;self.null_allocation=mode==3;self.null_open=mode==5
        m=self.m;m.context_restore(self.clean);self.calls=[];self.path='';self.written=bytes(1392);self.allocate=SCRATCH+0x2000;self.items=int(mode==7)
        for (a,n),value in zip(FIELDS,[0,9,8,5,0xffffffff,3,4,5,0,1,1,7,9]):self.put(a,value,n)
        if mode==0:self.put(FIELDS[0][0],0xffffffff)
        if mode==1:self.put(FIELDS[8][0],1,1)
        if mode==2:self.put(FIELDS[10][0],99,1)
        if mode==4:self.put(FIELDS[9][0],0,1)
        if mode==6:self.put(FIELDS[3][0],4,1)
        self.put(0x3b290dc,SCRATCH);m.mem_write(SCRATCH,b'SonicDX42.snc\0');m.mem_write(SCRATCH+0x800,b'SonicDX73.snc\0');m.mem_write(SAVE,bytes(1392))
        self.put(0x3c5e8b8,SCRATCH+0x100);self.put(SCRATCH+0x10c,SCRATCH+0x200)
        self.put(SCRATCH+0x200,SCRATCH+0x1000);self.put(SCRATCH+0x20c,0);m.mem_write(SCRATCH+0x1000,b'sonicdx01.snc\0')
        if mode==8:
            for slot in range(1,100):
                node=SCRATCH+0x3000+slot*16;name=SCRATCH+0x4000+slot*16
                self.put(node,name);self.put(node+12,node+16 if slot<99 else 0)
                m.mem_write(name,('SonicDX%02d.snc'%slot).encode()+b'\0')
            self.put(SCRATCH+0x10c,SCRATCH+0x3010)
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x421fd0,STOP,timeout=1000000)
        assert self.stopped or m.reg_read(UC_X86_REG_EIP)==STOP
        self.snapshot(99)
        return {'snapshots':self.snapshots,'written':self.entry_written,'stopped_at':self.stopped,'path':self.path}

def native_cases(text):
    result=[]
    for group in text.split('writer:')[1:40]:
        lines=group.splitlines();snapshots=[];written=None
        for i,line in enumerate(lines):
            prefix,sep,rest=line.partition(':')
            if sep and prefix.isdigit() and ',' in rest:
                values=[int(x)&0xffffffff for x in rest.split(',')]
                snapshots.append((int(prefix),values,lines[i+1]))
            if prefix=='written':written=rest
        result.append({'snapshots':snapshots,'written':written})
    return result

if __name__=='__main__':
    exe,native,folder=sys.argv[1:];folder=Path(folder);folder.mkdir(parents=True,exist_ok=True)
    image=PEImage(exe);assert image.sha256==SOURCE_SHA256
    source=Source(image);rows=[];bad=[]
    for case,native in enumerate(native_cases(Path(native).read_text())):
        event=case//3 if case<30 else 0;mode=case%3 if case<30 else 0
        expected=source.characterize(event,event if mode else 0,mode==2,case-30)
        compared={key:expected[key] for key in ('snapshots','written')}
        if compared!=native:bad.append(case)
        rows.append({'case':case,'mutation':event,'failure_mode':mode,'source':expected,'native':native})
    raw=folder/'writer-source-contract.json';raw.write_text(json.dumps(rows,separators=(',',':'))+'\n')
    report={'cases':len(rows),'mismatch_cases':bad,'full_outputs_sha256':hashlib.sha256(raw.read_bytes()).hexdigest(),'full_outputs_bytes':raw.stat().st_size,'source_sha256':image.sha256,'limits':__doc__}
    (folder/'writer-source-receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));raise SystemExit(bool(bad))
