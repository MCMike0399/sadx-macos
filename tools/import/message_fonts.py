#!/usr/bin/env python3
"""Import source MSG24 fonts without substituting the separate ASCII16 menu font."""
import hashlib
import json
import os
import tempfile
import struct
import sys
from pathlib import Path
REPO=Path(__file__).resolve().parents[2]
if not (REPO/'tools/lib/pe_image.py').is_file():REPO=Path.cwd()
sys.path.insert(0,str(REPO/'tools/lib'))
sys.path.insert(0,str(REPO/'tools/import'))
from pe_image import PEImage
from extract import SOURCE_SHA256
from regular_textures import profile_evidence,selected_mods
from menu_voice import publish_json

def digest(data):return hashlib.sha256(data).hexdigest()
def decode_records(raw):
    if len(raw)%88:raise ValueError('MSG24 font must contain complete16-metadata/72-bitmap records')
    records=[raw[i+16:i+88] for i in range(0,len(raw),88)]
    metadata=[raw[i:i+16] for i in range(0,len(raw),88)]
    if b''.join(a+b for a,b in zip(metadata,records))!=raw:raise ValueError('font inverse record proof failed')
    return records,metadata

def generated_width(bitmap):
    for column in (2,1,0):
        maximum=0
        for row in range(24):
            value=bitmap[row*3+column];distance=0
            while value:value=(value<<1)&255;distance+=1
            maximum=max(maximum,distance)
        if maximum:return column*8+maximum
    return 12

def checked_destination(path,owned_root):
    """Refuse redirection in every existing ancestor, including a missing leaf's parent."""
    path=Path(os.path.abspath(path))
    # macOS exposes its system temporary roots through /var and /tmp aliases.
    aliases={Path('/var'):Path('/private/var'),Path('/tmp'):Path('/private/tmp')}
    if any(parent.is_symlink() and
           not (parent in aliases and parent.resolve()==aliases[parent])
           for parent in (path,*path.parents)):
        raise ValueError('message font destination cannot redirect through a symlink')
    # Existing directory identity catches case/normalization aliases on macOS.
    # Walking every ancestor also covers a nonexistent destination suffix.
    if any(parent.exists() and parent.samefile(owned_root)
           for parent in (path,*path.parents)):
        raise ValueError('message font publication must be outside the owned install')
    if path.exists() and not path.is_file():
        raise ValueError('message font destination must be a regular file')
    return path


def publish_bytes(path,data,owned_root):
    """A fresh inode preserves an owned input hardlinked from an old output."""
    path=checked_destination(path,owned_root)
    path.parent.mkdir(parents=True,exist_ok=True)
    descriptor,name=tempfile.mkstemp(prefix='.message-font-',dir=path.parent)
    temporary=Path(name)
    try:
        with os.fdopen(descriptor,'wb') as stream:stream.write(data)
        os.replace(temporary,path)
    finally:temporary.unlink(missing_ok=True)


def import_message_fonts(game,output,mod_directories=None):
    game=Path(game);output=Path(output)
    owned_root=game.resolve()
    # Preflight every publication before touching an existing successful import.
    for relative in ('message-fonts/wide-glyphs.bin','message-fonts/single-glyphs.bin',
                     'message-fonts.json','message-fonts-evidence.json'):
        checked_destination(output/relative,owned_root)
    image=PEImage(game/'sonic.exe')
    if image.sha256!=SOURCE_SHA256:raise ValueError('MSG24 fonts require the pinned executable')
    raw_directories=image.read(0x909f30,100);languages=[]
    for index in range(5):
        mapping,widths,single,wide,lead=struct.unpack_from('<5I',raw_directories,index*20)
        if mapping or widths!=0x3abe250 or single or wide or lead>>16:raise ValueError('unrecovered initial message font directory')
        languages.append({'lead_first':lead&255,'lead_last':(lead>>8)&255,'remapping':None})
    profile=profile_evidence(game,{'groups':[],'lists':[],'localizations':[]},mod_directories)
    directories,_=selected_mods(game,mod_directories)
    for directory in directories:
        for path in directory.rglob('*'):
            relative=path.relative_to(directory).as_posix().casefold()
            if path.is_file() and relative in ('system/fontdata0.bin','system/fontdata1.bin'):
                raise ValueError('message font replacement requires an explicit profile binding')
    result={'version':1,'languages':languages,'fonts':{}};proof={'version':1,'source_sha256':image.sha256,'directory_bytes':raw_directories.hex(),'profile':profile,'fonts':[]}
    payloads={}
    for family,name in [('wide','fontdata0.bin'),('single','fontdata1.bin')]:
        matches=[p for p in (game/'system').iterdir() if p.name.casefold()==name]
        if len(matches)!=1 or not matches[0].is_file():raise ValueError('message font source missing or ambiguous: '+name)
        raw=matches[0].read_bytes();records,metadata=decode_records(raw)
        if len(records)!=(256 if family=='single' else 8836):raise ValueError('MSG24 font glyph count differs from the recovered source resource')
        glyphs=b''.join(records);relative='message-fonts/'+family+'-glyphs.bin';payloads[relative]=glyphs
        result['fonts'][family]={'glyph_file':relative,'glyph_count':len(records),'glyph_sha256':digest(glyphs),'fallback_bytes':raw[:72].hex()}
        proof['fonts'].append({'filename':matches[0].name,'size':len(raw),'sha256':digest(raw),'record_metadata':[x.hex() for x in metadata],'round_trip_exact':True})
        if family=='single':result['widths']=[generated_width(records[code]) for code in range(32,256)]
    text=image.string(0x7dcbf4)
    if image.read(0x7dcbf4,len(text)+1)!=text.encode('ascii')+b'\0':raise ValueError('saving text source inverse failed')
    result['saving_text']=text
    # Parsing and inverse proofs finish before the first publication. Manifest last.
    for relative,payload in payloads.items():publish_bytes(output/relative,payload,owned_root)
    publish_json(checked_destination(output/'message-fonts-evidence.json',owned_root),proof)
    publish_json(checked_destination(output/'message-fonts.json',owned_root),result)
    return result
