"""Import the seven authored CharacterSelect completion catalogs from the pinned image."""
import hashlib,os,struct,sys
from pathlib import Path
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT',Path(__file__).resolve().parents[2]))
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/import')]
from pe_image import PEImage
from extract import SOURCE_SHA256
from file_select_display import atomic_json
DESCRIPTORS=0x7f0bdc
COUNTS=(19,14,10,10,11,10,1)

def declarations(image):
    if image.sha256!=SOURCE_SHA256:
        raise ValueError('character completion requires the pinned executable')
    descriptors=image.read(DESCRIPTORS,56);catalogs=[];records=[]
    for selection,expected_count in enumerate(COUNTS):
        pointer,count=struct.unpack_from('<Ii',descriptors,selection*8)
        if count!=expected_count or pointer==0:
            raise ValueError('source completion catalog descriptor changed')
        raw=image.read(pointer,count*12);rows=[];inverse=bytearray()
        for value,source,flag in struct.iter_unpack('<iii',raw):
            if source in (0,1) and not 0<=flag<512:
                raise ValueError('source completion flag exceeds its actual owner')
            rows.append({'value':value,'source':source,'flag':flag});inverse+=struct.pack('<iii',value,source,flag)
        if bytes(inverse)!=raw:
            raise ValueError('completion catalog inverse mismatch')
        catalogs.append({'selection':selection,'rows':rows})
        records.append({'selection':selection,'source_address':hex(pointer),'source_bytes':raw.hex(),'sha256':hashlib.sha256(raw).hexdigest(),'inverse_exact':True})
    metadata={'version':1,'catalogs':catalogs}
    proof={'source_sha256':image.sha256,'descriptor_address':hex(DESCRIPTORS),'descriptor_bytes':descriptors.hex(),'descriptor_sha256':hashlib.sha256(descriptors).hexdigest(),'source_catalog_counts':list(COUNTS),'records':records,'round_trip_exact':True,'semantics':'Last passing authored row wins; source0 uses nonzero persistent story flag3b18808, source1 uses signed cutscene byte3b18608. No popcount, maximum, character remapping or drawing.'}
    return metadata,proof

def import_character_completion(game,output):
    game,output=Path(game).resolve(),Path(output).resolve()
    if output==game or game in output.parents:
        raise ValueError('completion metadata must be outside owned install')
    metadata,proof=declarations(PEImage(game/'sonic.exe'));output.mkdir(parents=True,exist_ok=True)
    atomic_json(output/'character-completion.json',metadata)
    atomic_json(output/'character-completion-evidence.json',proof)
    return metadata
