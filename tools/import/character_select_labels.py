"""Import the source CharacterSelect label lookup, retaining its actual menu-list identities."""
import hashlib,os,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/import')]
from pe_image import PEImage
from extract import SOURCE_SHA256
from file_select_display import atomic_json

def declarations(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('CharacterSelect labels require the pinned executable')
    labels=image.read(0x7ef6e8,7)
    if struct.pack('<7B',*labels)!=labels:
        raise ValueError('CharacterSelect label inverse mismatch')
    table=image.unpack('<I',0x10d7cb0)[0]
    expected={14:0x10d7be0,15:0x10d7be8,28:0x10d7bf0}
    bindings=[]
    for index,pointer in expected.items():
        raw=image.read(table+index*8,8)
        if struct.unpack('<II',raw)[1]!=pointer:
            raise ValueError('CharacterSelect label texture identity changed')
        bindings.append({'index':index,'bytes':raw.hex()})
    metadata={'version':1,'label_textures':list(labels),
              'menu_textures':{'labels':14,'names':15,'metal_name':28}}
    proof={'source_sha256':image.sha256,'source_label_address':'0x7ef6e8',
           'source_label_bytes':labels.hex(),'source_label_sha256':hashlib.sha256(labels).hexdigest(),
           'inverse_exact':True,'menu_texture_bindings':bindings,
           'existing_name_owner':'FileSelectDisplay.tables.character_names imported from0x7ef6f0; borrowed, not copied here.'}
    return metadata,proof

def import_character_select_labels(game,output):
    game,output=Path(game).resolve(),Path(output).resolve()
    if output==game or game in output.parents:
        raise ValueError('label metadata output must be outside owned install')
    metadata,proof=declarations(PEImage(game/'sonic.exe'))
    output.mkdir(parents=True,exist_ok=True)
    atomic_json(output/'character-select-labels.json',metadata)
    atomic_json(output/'character-select-labels-evidence.json',proof)
    return metadata
