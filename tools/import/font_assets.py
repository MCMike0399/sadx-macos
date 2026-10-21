#!/usr/bin/env python3
"""Import original ASCII font metadata and prove its raw texture profile."""
import hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'lib'))
sys.path.insert(0,str(Path(__file__).resolve().parent))
from pe_image import PEImage
from extract import SOURCE_SHA256
from regular_textures import initial_bytes,profile_evidence
FONT=0x90e494

def declarations(image):
    if image.sha256!=SOURCE_SHA256:raise ValueError('font requires the pinned executable SHA256')
    raw=image.read(FONT,40);words=list(struct.unpack('<10I',raw))
    if struct.unpack('<4f',raw[:16])!=(16.0,16.0,256.0,128.0):raise ValueError('unsupported ASCII font dimensions')
    names,count=image.unpack('<2I',words[4])
    if count!=1:raise ValueError('ASCII font texture count differs')
    name_raw=initial_bytes(image,names,12);filename,attributes,memory=struct.unpack('<3I',name_raw);name=image.string(filename)
    if attributes!=0 or memory!=0 or name!='abc_txt':raise ValueError('unrecovered ASCII font texture construction')
    proportions=list(image.read(words[5],128))
    result={'version':1,'ascii':{'dimension_words':words[:4],'proportions':proportions,'colors':words[6:],'texture_list':{'texture_count':count,'filenames':[name],'attributes':[attributes]}}}
    inverse=struct.pack('<10I',*result['ascii']['dimension_words'],words[4],words[5],*result['ascii']['colors'])
    if inverse!=raw or bytes(proportions)!=image.read(words[5],128):raise ValueError('font inverse-byte validation failed')
    proof={'version':1,'source_sha256':image.sha256,'descriptor_address':hex(FONT),'descriptor_bytes':raw.hex(),'descriptor_sha256':hashlib.sha256(raw).hexdigest(),'proportions_address':hex(words[5]),'proportions_bytes':bytes(proportions).hex(),'texture_list_address':hex(words[4]),'texture_list_bytes':image.read(words[4],8).hex(),'texture_names_address':hex(names),'texture_names_bytes':name_raw.hex(),'filename_address':hex(filename),'filename_bytes':(name.encode()+b'\0').hex(),'round_trip_exact':True}
    return result,proof

def import_font_assets(game,output,mod_directories=None):
    result,proof=declarations(PEImage(Path(game)/'sonic.exe'))
    profile={'groups':[[{'archive':None,'texture_list':0}]],'lists':[result['ascii']['texture_list']],'localizations':[]}
    proof['profile']=profile_evidence(game,profile,mod_directories)
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    (output/'font-assets.json').write_text(json.dumps(result,indent=2)+'\n')
    (output/'font-assets-evidence.json').write_text(json.dumps(proof,indent=2)+'\n')
    return result
