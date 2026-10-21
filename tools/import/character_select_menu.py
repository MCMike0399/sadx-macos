"""Import CharacterSelect dialog declarations and carousel name widths from the pinned source."""
import hashlib, math, os, struct, sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/import')]
from pe_image import PEImage
from extract import SOURCE_SHA256
from file_select_display import atomic_json

def declarations(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('CharacterSelect declarations require pinned sonic.exe')
    records=[]
    def record(name,address,size):
        raw=image.read(address,size)
        records.append({'name':name,'address':hex(address),'bytes':raw.hex(),'sha256':hashlib.sha256(raw).hexdigest()})
        return raw
    menu_table=image.unpack('<I',0x10d7cb0)[0]
    bindings=[image.unpack('<II',menu_table+i*8) for i in range(30)]
    definitions=[]
    for identity,count in [(7,3),(8,4),(9,2),(10,3)]:
        raw=record('dialog_'+str(identity),0x7ee328+identity*60,60)
        words=struct.unpack('<15I',raw)
        style,draw,texture,panels,sounds=words[:5]
        choices,cancel=struct.unpack('<bb',raw[56:58])
        if style!=5 or draw!=0 or choices!=count or cancel!=count-1:
            raise ValueError('CharacterSelect dialog identity/style changed')
        matches=[i for i,(_,pointer) in enumerate(bindings) if pointer==texture]
        if len(matches)!=1:
            raise ValueError('ambiguous CharacterSelect dialog texture')
        rows=[]
        for i in range(count):
            panel=record('panel_'+str(identity)+'_'+str(i),panels+i*12,12)
            x,y,index,padding=struct.unpack('<2IB3s',panel)
            if not all(math.isfinite(v) for v in struct.unpack('<2f',panel[:8])):
                raise ValueError('nonfinite CharacterSelect dialog panel')
            if struct.pack('<2IB3s',x,y,index,padding)!=panel:
                raise ValueError('panel inverse mismatch')
            rows.append({'position_words':[x,y],'texture':index})
        cues=list(struct.unpack('<5i',record('sounds_'+str(identity),sounds,20))) if sounds else None
        if not all(math.isfinite(v) for v in struct.unpack('<7f',raw[28:56])):
            raise ValueError('nonfinite CharacterSelect dialog geometry')
        if struct.pack('<14Ibb2s',*words[:14],choices,cancel,raw[58:60])!=raw:
            raise ValueError('dialog inverse mismatch')
        definitions.append({'dialog':identity,'style':style,'content':'none','menu_texture':matches[0],
          'panels':rows,'sounds':cues,'upper_color':words[5],'lower_color':words[6],
          'geometry_words':list(words[7:14]),'choice_count':choices,'cancel_choice':cancel})
    raw=record('carousel_name_widths',0x7efb2c,28)
    widths=list(struct.unpack('<7I',raw))
    if struct.pack('<7I',*widths)!=raw:
        raise ValueError('width inverse mismatch')
    clear_raw=record('character_clear_flags',0x7ef6c4,28)
    voice_raw=record('selected_character_voice_cues',0x7ee90c,32)
    metadata={'version':1,'definitions':definitions,'name_width_words':widths,
      'clear_flags':list(struct.unpack('<7I',clear_raw)),
      'selected_voice_cues':list(struct.unpack('<8i',voice_raw))}
    proof={'version':1,'source_sha256':image.sha256,'round_trip_exact':True,'records':records,
      'limits':['Only source declarations; texture binding and real draw/control composition remain runtime owners.',
                'Dialog7..10 use existing style5 renderer and no custom content callback.']}
    return metadata,proof

def import_character_select_menu(game,output):
    game,output=Path(game).resolve(),Path(output).resolve()
    if output==game or game in output.parents:
        raise ValueError('metadata must be outside owned game install')
    metadata,proof=declarations(PEImage(game/'sonic.exe'))
    output.mkdir(parents=True,exist_ok=True)
    atomic_json(output/'character-select-menu.json',metadata)
    atomic_json(output/'character-select-menu-evidence.json',proof)
    return metadata
