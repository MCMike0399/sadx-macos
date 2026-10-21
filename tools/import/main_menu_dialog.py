"""Source declarations for MainMenu dialogs; style4 renderer remains a separate required port."""
import hashlib,json,math,os,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/import')]
from pe_image import PEImage
from extract import SOURCE_SHA256
from file_select_display import atomic_json

def declarations(image):
    if image.sha256!=SOURCE_SHA256:raise ValueError('main menu declarations require pinned sonic.exe')
    records=[]
    def record(name,address,size):
        data=image.read(address,size);records.append({'name':name,'address':hex(address),'bytes':data.hex(),'sha256':hashlib.sha256(data).hexdigest()});return data
    menu_table=image.unpack('<I',0x10d7cb0)[0]
    bindings=[image.unpack('<II',menu_table+i*8) for i in range(30)]
    definitions=[]
    for identity,count,callback in [(2,6,0x50b4e0),(3,5,0x50b4e0),(4,3,0x50b590)]:
        raw=record('dialog_'+str(identity),0x7ee328+identity*60,60);words=struct.unpack('<15I',raw)
        style,draw,texture,panels,sounds=words[:5]
        if style!=4 or draw!=callback:raise ValueError('main menu style/content changed')
        choices,cancel=struct.unpack('<bb',raw[56:58])
        if choices!=count or cancel!=count:raise ValueError('main menu choice/cancel declaration changed')
        matches=[n for n,(_,pointer) in enumerate(bindings) if pointer==texture]
        if matches!=[19]:raise ValueError('main menu common texture identity changed')
        rows=[]
        for i in range(count):
            panel=record('panel_'+str(identity)+'_'+str(i),panels+12*i,12)
            x,y,index,padding=struct.unpack('<2IB3s',panel)
            if not all(math.isfinite(v) for v in struct.unpack('<2f',panel[:8])):raise ValueError('nonfinite main menu panel')
            if struct.pack('<2IB3s',x,y,index,padding)!=panel:raise ValueError('panel inverse mismatch')
            rows.append({'position_words':[x,y],'texture':index})
        cues=list(struct.unpack('<5i',record('sounds_'+str(identity),sounds,20))) if sounds else None
        geometry=list(words[7:14])
        if not all(math.isfinite(v) for v in struct.unpack('<7f',raw[28:56])):raise ValueError('nonfinite main menu geometry')
        inverse=struct.pack('<5I2I7Ibb2s',style,draw,texture,panels,sounds,words[5],words[6],*geometry,choices,cancel,raw[58:60])
        if inverse!=raw:raise ValueError('dialog inverse mismatch')
        definitions.append({'dialog':identity,'style':style,'content':'main_menu_backdrop' if identity!=4 else 'trial_menu_backdrop','menu_texture':19,'panels':rows,'sounds':cues,'upper_color':words[5],'lower_color':words[6],'geometry_words':geometry,'choice_count':choices,'cancel_choice':cancel})
    variant_raw=record('variant_words_and_adjacent_width_constant',0x7ef8e8,28);variant_words=list(struct.unpack('<7I',variant_raw))
    if variant_words[:6]!=list(range(6)) or variant_words[6]!=0x3f7e0419:raise ValueError('main menu variant source alias changed')
    if struct.pack('<7I',*variant_words)!=variant_raw:raise ValueError('variant inverse mismatch')
    trial_raw=record('trial_backdrop_layout',0x10d7e14,16);trial_words=list(struct.unpack('<4I',trial_raw))
    if struct.pack('<4I',*trial_words)!=trial_raw:raise ValueError('trial layout inverse mismatch')
    metadata={'version':1,'definitions':definitions,'variant_words':variant_words,'variant_mapping':[v&255 for v in variant_words],'trial_backdrop_layout_words':trial_words,'main_common_texture':19,'main_background_texture':20}
    proof={'version':1,'source_sha256':image.sha256,'round_trip_exact':True,'records':records,'runtime_ready':False,'required_renderer':'Dialogstyle4 uses source431cb0 DrawSquareC and431bb0 DrawSquareCursor; existingstyle5-onlydraw_panels must not be made to accept4without that source port.','quirks':['Cancel index6 reads the low byte25 of the adjacent0.99225f compiler word; preserve instead of guessing tutorialvariant0.','Dialog2/3 share callback50b4e0 and common texlist10d7c78; dialog4 callback50b590 reads its live four-float data layout10d7e14.','MainMenu first quick dialog falls through to result in the same control call; Reopen5 does not.']}
    return metadata,proof

def import_main_menu_dialog(game,output):
    game,output=Path(game).resolve(),Path(output).resolve()
    if output==game or game in output.parents:raise ValueError('metadata must be outside owned game install')
    metadata,proof=declarations(PEImage(game/'sonic.exe'));output.mkdir(parents=True,exist_ok=True)
    atomic_json(output/'main-menu-dialog.json',metadata);atomic_json(output/'main-menu-dialog-evidence.json',proof);return metadata
