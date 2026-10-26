"""Read-only original-producer proof and reached Gamma resource requirements.

Writes metadata only; it never exports model/motion payloads or changes a pack.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'tools/lib'),str(ROOT/'tools/oracles'),str(ROOT/'tools/import')]
from pe_image import PEImage
from character_actions import export, SOURCE_SHA
from extract import SOURCE_SHA256
from verify_original_gamma_selection_core import Source, DATA, STACK, STOP
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP

def sha(b): return hashlib.sha256(b).hexdigest()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('exe',type=Path);ap.add_argument('dll',type=Path)
    ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    exe=PEImage(a.exe);dll=PEImage(a.dll)
    if dll.sha256!=SOURCE_SHA or exe.sha256!=SOURCE_SHA256:raise ValueError('unpinned source image')
    source=Source(exe);m=source.m
    # Producers have no calls into the E102 receipt hooks.
    actions=export(dll,'___E102_ACTIONS');objects=export(dll,'___E102_OBJECTS')
    declarations=list(dll.unpack('<78I',actions))
    object_rows=list(dll.unpack('<28I',objects))
    source.put(0x3ab916c,DATA+0x7000);source.put(0x3a8af6c,DATA+0x7800)
    # Sentinel declarations recover source indices even when DLL payloads alias.
    for n in range(78):source.put(DATA+0x7000+4*n,0x50000000+16*n)
    for n in range(28):source.put(DATA+0x7800+4*n,0x60000000+16*n)
    def call(entry):
        source.put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(entry,STOP,count=10000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('producer did not return')
    call(0x7ca300)
    rows=[]
    for n in range(91):
        action,count,mode,next_action,transition,speed=struct.unpack('<IBbHII',m.mem_read(0x3c53fa8+16*n,16))
        if action==0x3c5d530:declaration='constructed'
        elif 0x50000000<=action<0x50000000+78*16 and action%16==0:declaration=(action-0x50000000)//16
        else:raise ValueError(f'unclassified action at row {n}: {action:x}')
        rows.append({'row':n,'declaration':declaration,'object_count':count,'mode':mode,
                     'next':next_action,'transition_bits':hex(transition),'speed_bits':hex(speed)})
    call(0x7cb560)
    weld=bytes(m.mem_read(0x3c54558,5*24))
    expected=[(0,24,2,4),(0,25,2,5),(0,26,0,6),(0,27,0,7)]
    for n,(root,child,count,mode) in enumerate(expected):
        row=struct.unpack('<3I4B2I',weld[n*24:(n+1)*24])
        if row!=(0x60000000+16*root,0x60000000+16*child,0,count,mode,0,0,0,0):
            raise ValueError('source weld recipe differs')
    if weld[96:]!=bytes(24):raise ValueError('source weld sentinel differs')
    stand=declarations[0]
    model,motion=dll.unpack('<II',stand)
    motion_data,frames,channels,interpolation=dll.unpack('<IIHH',motion)
    collision=exe.read(0x91c5b8,3*48)
    textures=[]
    for n in range(6):
        name,texture_list=exe.unpack('<II',0x90ed04+n*8)
        textures.append({'row':n,'archive':exe.string(name) if name else None,
                         'texture_list':hex(texture_list) if texture_list else None})
    material_export=export(dll,'___E102_MATERIALS')
    materials=[]
    for row,index in [(1,6),(5,2),(4,2),(3,2),(2,2)]:
        array=dll.unpack('<I',material_export+4*row)[0]
        record=array+20*index
        materials.append({'table_row':row,'material_index':index,'array':hex(array),
                          'record':hex(record),'sha256':sha(dll.read(record,20))})
    setup={}
    for start,end in [(0x49fb10,0x49fb29),(0x4c5420,0x4c5436),
                      (0x4c2e10,0x4c2e48),(0x4c2e90,0x4c2f42),(0x4c4cf2,0x4c4d2f)]:
        setup[f'{start:#x}-{end:#x}']=sha(exe.read(start,end-start))
    result={'exe_sha256':exe.sha256,'dll_sha256':dll.sha256,'recipe_count':len(rows),
        'declaration_count':len(declarations),'null_declarations':[i for i,v in enumerate(declarations) if not v],
        'constructed_rows':[r['row'] for r in rows if r['declaration']=='constructed'],
        'recipes':rows,'sentinel_recipe_bytes_sha256':sha(bytes(m.mem_read(0x3c53fa8,91*16))),
        'stand':{'export_index':0,'action':hex(stand),'object':hex(model),'motion':hex(motion),
                 'frames':frames,'channels':channels,'interpolation':interpolation,'object_count':rows[0]['object_count']},
        'weld':{'rows':[{'root_object_row':x[0],'source_object_row':x[1],'vertex_count':x[2],'input_mode':x[3]} for x in expected],
                'sentinel_row':4,'sentinel_bytes':24,'source_object_pointers':[hex(object_rows[n]) for n in [0,24,25,26,27]]},
        'collision':{'address':'0x91c5b8','count':3,'stride':48,'sha256':sha(collision)},
        'materials_export':hex(material_export),'materials':materials,
        'texture_character':6,'texture_declaration':textures,
        'line_vertices':{'address':'0x3c53f68','count':2,'stride':12,'storage':'mutable BSS'},
        'line_colors':{'address':'0x91c648','count':2,'stride':4,'sha256':sha(exe.read(0x91c648,8))},
        'line_point_count':2,
        'setup_source_ranges':setup,
        'limits':['No payloads exported; Root owns importer changes.','Constructed row72 references source action3c5d530; its producer is a separate dependency.',
                  'Controller source has no direct event580 call.','Source lower setup bodies are byte-hashed navigation; their implementation is not claimed.']}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['recipe_count','declaration_count','null_declarations','constructed_rows','stand']}))
    return 0
if __name__=='__main__':sys.exit(main())
