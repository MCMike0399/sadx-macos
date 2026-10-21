#!/usr/bin/env python3
"""Import the reached file-selection dialog and shared curves, with inverse-byte proof."""
import hashlib,json,math,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'lib'))
from pe_image import PEImage
from extract import SOURCE_SHA256
from file_select_display import atomic_json

TABLE=0x7ee328

def declarations(image):
    if image.sha256!=SOURCE_SHA256:raise ValueError('menu dialogs require the pinned executable')
    proof={'version':1,'source_sha256':image.sha256,'records':[]}
    def record(name,address,size):
        raw=image.read(address,size);proof['records'].append({'name':name,'address':hex(address),'bytes':raw.hex(),'sha256':hashlib.sha256(raw).hexdigest()});return raw
    raw=record('file_selection',TABLE,60);words=struct.unpack('<15I',raw);style,draw,texture,panels,sounds=words[:5]
    if style!=5 or draw!=0x5056a0:raise ValueError('unrecovered file-selection dialog style/content')
    menu_table=image.unpack('<I',0x10d7cb0)[0]
    matches=[i for i in range(30) if image.unpack('<I',menu_table+i*8+4)[0]==texture]
    if len(matches)!=1:raise ValueError('dialog texture has no unique canonical menu binding')
    choice_count,cancel=struct.unpack('<bb',raw[56:58])
    if choice_count!=3:raise ValueError('unqualified file-selection choice table')
    panel_rows=[]
    for i in range(choice_count):
        panel=record('file_selection_panel_'+str(i),panels+i*12,12);x,y,index,padding=struct.unpack('<2IB3s',panel)
        if not all(math.isfinite(v) for v in struct.unpack('<2f',panel[:8])):raise ValueError('nonfinite dialog panel')
        panel_rows.append({'position_words':[x,y],'texture':index})
        if struct.pack('<2IB3s',x,y,index,padding)!=panel:raise ValueError('dialog panel inverse mismatch')
    cues=list(struct.unpack('<5i',record('file_selection_sounds',sounds,20))) if sounds else None
    geometry=list(words[7:14])
    if not all(math.isfinite(x) for x in struct.unpack('<7f',raw[28:56])):raise ValueError('nonfinite dialog geometry')
    definition={'dialog':0,'style':style,'content':'file_select_help','menu_texture':matches[0],'panels':panel_rows,'sounds':cues,'upper_color':words[5],'lower_color':words[6],'geometry_words':geometry,'choice_count':choice_count,'cancel_choice':cancel}
    # Callback strings contain raw PC message bytes; never guess an extended-character codec.
    messages=[];resource_ids=[];resource_pool=[];source_identities={}
    for choice in range(3):
        row=image.unpack('<I',0x10d7b18+choice*4)[0];pointers=struct.unpack('<5I',record('help_directory_'+str(choice),row,20));languages=[];identities=[]
        for language,pointer in enumerate(pointers):
            data=bytearray()
            while True:
                value=image.read(pointer+len(data),1)[0]
                if value==0:break
                data.append(value)
                if len(data)>4096:raise ValueError('unterminated dialog help resource')
            record('help_'+str(choice)+'_'+str(language),pointer,len(data)+1);languages.append(data.hex())
            if pointer not in source_identities:
                source_identities[pointer]=len(resource_pool);resource_pool.append(data.hex())
            identities.append(source_identities[pointer])
        messages.append(languages);resource_ids.append(identities)
    curves={}
    for name,address in [('opening',0x7dfe20),('closing',0x7dfea0)]:
        data=record(name,address,128);values=list(struct.unpack('<32I',data));
        if not all(math.isfinite(x) for x in struct.unpack('<32f',data)):raise ValueError('nonfinite dialog curve')
        if struct.pack('<32I',*values)!=data:raise ValueError('dialog curve inverse mismatch')
        curves[name]=[values[i:i+2] for i in range(0,32,2)]
    directions=list(struct.unpack('<12I',record('directions',0x7dff20,48)))
    result={'version':1,'definitions':[definition],**curves,'directions':[directions[i:i+2] for i in range(0,12,2)],'help_bytes_by_choice':messages,'cursor_menu_texture':4,'help_resources':resource_pool,'help_resource_ids_by_choice':resource_ids}
    # Reconstruct the source descriptor using only typed data and recorded source identities.
    inverse=struct.pack('<5I2I7Ibb2s',style,draw,texture,panels,sounds,definition['upper_color'],definition['lower_color'],*geometry,choice_count,cancel,raw[58:60])
    if inverse!=raw:raise ValueError('dialog descriptor inverse mismatch')
    proof['round_trip_exact']=True;proof['scope']='Reached file-selection definition0; other dialog definitions remain explicit missing imported resources'
    return result,proof

def import_menu_dialog(game,output):
    game,output=Path(game).resolve(),Path(output).resolve()
    if output==game or game in output.parents:
        raise ValueError('dialog metadata must be outside the owned install')
    metadata,proof=declarations(PEImage(game/'sonic.exe'))
    output.mkdir(parents=True,exist_ok=True)
    atomic_json(output/'menu-dialog.json',metadata)
    atomic_json(output/'menu-dialog-evidence.json',proof)
    return metadata
