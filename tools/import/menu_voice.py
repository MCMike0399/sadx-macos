#!/usr/bin/env python3
"""Import source menu-voice resource rows, preserving each row/request identity.

The version1 character field is the MenuVoice row byte, not the playable-character
ID: source GetMenuVoice maps saved Sonic0 to row1. Production imports all nine
reachable rows; declarations retains its bounded row-inspection API.
"""
import hashlib,json,os,struct,sys,tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'lib'))
from pe_image import PEImage
from extract import SOURCE_SHA256
TABLE=0x7ee930
ROWS=9
REQUESTS=9

def publish_json(path,value):
    """Fresh inodes avoid modifying source files hardlinked from previous outputs."""
    path=Path(path)
    if path.is_symlink():raise ValueError('import manifest destination cannot be a symlink')
    path.parent.mkdir(parents=True,exist_ok=True)
    descriptor,name=tempfile.mkstemp(prefix='.menu-voice-',dir=path.parent)
    temporary=Path(name)
    try:
        with os.fdopen(descriptor,'w') as stream:stream.write(json.dumps(value,indent=2)+'\n')
        os.replace(temporary,path)
    finally:temporary.unlink(missing_ok=True)

def declarations(image,characters=(0,)):
    if image.sha256!=SOURCE_SHA256:raise ValueError('menu voices require the pinned executable')
    characters=list(dict.fromkeys(characters))
    if any(type(row) is not int or not 0<=row<ROWS for row in characters):
        raise ValueError('menu voice row is outside GetMenuVoice source result range')
    entries=[];rows=[]
    for row in characters:
        address=TABLE+row*REQUESTS*4;raw=image.read(address,REQUESTS*4)
        values=struct.unpack('<9i',raw)
        current=[{'character':row,'request':index,'voice':value} for index,value in enumerate(values)]
        if struct.pack('<9i',*(entry['voice'] for entry in current))!=raw:raise ValueError('menu voice inverse mismatch')
        entries.extend(current);rows.append({'character':row,'table_address':hex(address),'table_bytes':raw.hex(),'table_sha256':hashlib.sha256(raw).hexdigest()})
    return {'version':1,'entries':entries},{'version':1,'source_sha256':image.sha256,'table_address':hex(TABLE),'rows':rows,'round_trip_exact':True,'scope':{'characters':characters,'requests_per_row':REQUESTS,'mapping':'505a40 reads signed32 voice at7ee930+4*(9*MenuVoicebyte+request); GetMenuVoice5068e0 yields0..8'}}

def required_voices(image,characters=range(ROWS)):
    metadata,_=declarations(image,characters)
    return list(dict.fromkeys(entry['voice'] for entry in metadata['entries']))

def import_menu_voice(game,output,*,characters=range(ROWS)):
    game,output=Path(game).resolve(),Path(output).resolve()
    if output==game or game in output.parents:raise ValueError('menu voice output must be outside the owned install')
    metadata,proof=declarations(PEImage(game/'sonic.exe'),characters)
    publish_json(output/'menu-voice.json',metadata);publish_json(output/'menu-voice-evidence.json',proof)
    return metadata
