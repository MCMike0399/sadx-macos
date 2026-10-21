"""Import the source sound-bank thresholds used by the real native sound server."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'lib'))
from pe_image import PEImage
from extract import SOURCE_SHA256

def extract_tables(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('sound-effect metadata requires the pinned original image')
    words=image.unpack('<128i',0x910090)
    thresholds=[{'tone':words[n],'bank':words[n+1]} for n in range(0,len(words),2)]
    if thresholds[-1] != {'tone':-1,'bank':-1}:
        raise ValueError('sound-bank table has no source terminal record')
    for index,row in enumerate(thresholds[:-1]):
        if row['tone']<0 or not 0<=row['bank']<16:
            raise ValueError('invalid source sound-bank threshold')
        if index and thresholds[index-1]['tone']<=row['tone']:
            raise ValueError('source sound-bank thresholds are not descending')
    if b''.join(struct.pack('<ii',r['tone'],r['bank']) for r in thresholds) != image.read(0x910090,512):
        raise ValueError('sound-bank threshold inverse mismatch')
    pan,=image.unpack('<i',0x910290)
    return {'bank_thresholds':thresholds,'alternating_pan':pan}

def validate_initial_work(image):
    ranges=[(0x3b292f0,2),(0x3b291c0,4),(0x3b29ae0,4)]
    for channel in range(36):
        base=0x3b292f8+channel*56
        ranges += [(base,4),(base+28,4),(base+36,12),(base+48,4)]
    for address,size in ranges:
        relative=address-image.base
        section=next((s for s in image.sections if s['rva']<=relative and relative+size<=s['rva']+max(s['virtual_size'],s['raw_size'])),None)
        if section is None:raise ValueError('sound-effect initial work is outside the source image')
        offset=relative-section['rva']
        raw=bytes(image.data[section['file_offset']+i] if i<section['raw_size'] else 0 for i in range(offset,offset+size))
        if any(raw):raise ValueError('sound-effect initial work differs from native zero literals')
    return [{'start':hex(a),'size':n,'initial':'zero'} for a,n in ranges]

def extract_wave_metadata(raw):
    if len(raw)<12 or raw[:4]!=b'RIFF' or raw[8:12]!=b'WAVE':
        raise ValueError('source wave reader rejects non-RIFF/WAVE data')
    end=8+struct.unpack_from('<I',raw,4)[0]
    if end>len(raw):raise ValueError('truncated source RIFF payload')
    chunks=[];offset=12
    while offset+8<=end:
        name,size=struct.unpack_from('<4sI',raw,offset);start=offset+8
        if start+size>end:raise ValueError('truncated source RIFF chunk')
        chunks.append((name,start,size));offset=start+size+(size&1)
    fmt=next((entry for entry in chunks if entry[0]==b'fmt '),None)
    if not fmt or fmt[2]<16:raise ValueError('source wave reader requires16-byte format')
    _,start,size=fmt
    tag,channels,rate,average,alignment,bits=struct.unpack_from('<HHIIHH',raw,start)
    if tag!=1:
        if len(raw)<start+18:raise ValueError('source compressed wave format has no extension size')
        extra,=struct.unpack_from('<H',raw,start+16)
        if len(raw)<start+18+extra:raise ValueError('truncated source compressed wave extension')
    result={'source_bytes':len(raw),'source_rate':rate,'block_align':alignment,
            'loop_count':0,'loop_start':None,'loop_end':None}
    # ReadFormat searches after ascending the format chunk, not before it.
    smpl=next((entry for entry in chunks if entry[0]==b'smpl' and entry[1]>start+size),None)
    if smpl:
        _,position,length=smpl
        if len(raw)<position+36:raise ValueError('source sampler header read is shorter than36bytes')
        loops,=struct.unpack_from('<i',raw,position+28)
        if loops>1:raise ValueError('source wave reader rejects more than one sampler loop')
        result['loop_count']=loops
        if loops:
            if len(raw)<position+60:
                result['loop_count']=0
            else:
                first,last=struct.unpack_from('<II',raw,position+44)
                first=(first*alignment)&0xffffffff;last=(last*alignment)&0xffffffff
                result['loop_start']=first;result['loop_end']=last
                if first>=last:result['loop_count']=0
    return result

def sample_metadata(directory):
    manifest_path=directory/'sound-banks.json'
    if not manifest_path.exists():return []
    manifest=json.loads(manifest_path.read_text());result=[]
    if manifest['source_exe_sha256']!=SOURCE_SHA256:raise ValueError('sound-bank source digest mismatch')
    for bank in manifest['banks']:
        for sample in bank['samples']:
            relative=Path(sample['path'])
            if relative.is_absolute() or '..' in relative.parts:raise ValueError('sample path escapes imported assets')
            raw=(directory/relative).read_bytes()
            if hashlib.sha256(raw).hexdigest()!=sample['sha256']:raise ValueError('imported sample digest mismatch')
            result.append({'origin':bank['origin'],'bank':bank['name'],'ordinal':sample['ordinal'],
                           'path':sample['path'],'sha256':sample['sha256'],**extract_wave_metadata(raw)})
    return result

def import_sound_effects(exe,output):
    exe=Path(exe);exe=exe/'sonic.exe' if exe.is_dir() else exe
    image=PEImage(exe);payload={'version':1,'schema':1,'source_sha256':image.sha256,'tables':extract_tables(image)}
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    initial_work=validate_initial_work(image)
    payload['sample_metadata']=sample_metadata(output)
    target=output/'sound-effects.json';target.write_text(json.dumps(payload,indent=2)+'\n')
    proof={'source_sha256':image.sha256,'initial_work':initial_work,'source_ranges':{hex(a)+'-'+hex(a+n):hashlib.sha256(image.read(a,n)).hexdigest() for a,n in [(0x910090,512),(0x910290,4)]}}
    (output/'sound-effects.proof.json').write_text(json.dumps(proof,indent=2)+'\n');return payload

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('--output',required=True);a=p.parse_args()
    result=import_sound_effects(a.exe,a.output);print(json.dumps({'bank_thresholds':len(result['tables']['bank_thresholds'])}))
if __name__=='__main__':main()
