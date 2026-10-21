#!/usr/bin/env python3
"""Import the proved Sonic menu-voice row without source code pointers."""
import hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'lib'))
from pe_image import PEImage
from extract import SOURCE_SHA256
TABLE=0x7ee930

def declarations(image):
    if image.sha256!=SOURCE_SHA256:raise ValueError('menu voices require the pinned executable')
    raw=image.read(TABLE,36);values=struct.unpack('<9i',raw)
    entries=[{'character':0,'request':index,'voice':value} for index,value in enumerate(values)]
    if struct.pack('<9i',*(entry['voice'] for entry in entries))!=raw:raise ValueError('menu voice inverse mismatch')
    return {'version':1,'entries':entries},{'version':1,'source_sha256':image.sha256,'table_address':hex(TABLE),'table_bytes':raw.hex(),'table_sha256':hashlib.sha256(raw).hexdigest(),'round_trip_exact':True,'scope':'Sonic row, nine source requests; other character rows remain explicit missing imported boundaries'}

def import_menu_voice(game,output):
    metadata,proof=declarations(PEImage(Path(game)/'sonic.exe'));output=Path(output);output.mkdir(parents=True,exist_ok=True)
    (output/'menu-voice.json').write_text(json.dumps(metadata,indent=2)+'\n');(output/'menu-voice-evidence.json').write_text(json.dumps(proof,indent=2)+'\n');return metadata
