"""Compile semantic faults privately; never mutate the shipping file."""
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'build/sonic-initialization-faults';OUT.mkdir(exist_ok=True)
body=(ROOT/'src/player/sonic_model.cpp').read_text()
faults={
 'metal_cached_before_loader':('    calls.load_textures(0);\n    if (calls.metal())','    const auto cached_metal = calls.metal();\n    calls.load_textures(0);\n    if (cached_metal)'),
 'wrong_texture_character':('calls.load_textures(8);','calls.load_textures(6);'),
 'common_motion_omitted':('    calls.set_motion(); // Common tail: the source calls PSetMotion a second time.',''),
 'second_free_store_omitted':('    calls.clear_free_word(1);',''),
 'act_high_byte_ignored':('(((stage << 8) | act) & kStageMask)','(((stage << 8) | (act & 255U)) & kStageMask)'),
 'join_table_wrong_owner':('auto &table = metal ? state.metal_joins : state.joins;','auto &table = metal ? state.joins : state.metal_joins;'),
 'join_mode_neighbor':('join.input_mode = final_egg ? 2 : 0;','join.source_depth = final_egg ? 2 : 0;'),
 'post_join_metal_cached':('    if (!calls.metal()) {\n        calls.create_effect(2);','    if (!metal) {\n        calls.create_effect(2);'),
 'setup_allocation_size':('arena.allocate(136)','arena.allocate(128)'),
 'setup_primary_index':('player_index = player;','player_index = static_cast<std::uint8_t>(player ^ 1);'),
 'setup_object_order':('work->objects[index] = objects[index];','work->objects[index] = objects[2 - index];'),
 'common_shoe_slot':('calls.model_slot(58)','calls.model_slot(60)'),
 'common_primary_flag':('kSkipModelJoin = 0x100;','kSkipModelJoin = 0x200;'),
}
receipts=[]
for name,(old,new) in faults.items():
 assert body.count(old)==1,(name,body.count(old))
 source=OUT/(name+'.cpp');source.write_text(body.replace(old,new));library=OUT/(name+'.dylib')
 command=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-arch','arm64','-mmacosx-version-min=14.0','-Isrc','-dynamiclib','tests/probes/native_sonic_initialization_probe.cpp',str(source),'build/libsadx-native.a','-o',str(library)]
 compiled=subprocess.run(command,cwd=ROOT,capture_output=True,text=True);assert compiled.returncode==0,compiled.stderr
 judge='verify_helpers.py' if name.startswith('setup_') or name in ('common_shoe_slot', 'common_primary_flag') else 'verify_source.py'
 report=OUT/(name+'.json');judged=subprocess.run([sys.executable,str(Path(__file__).with_name(judge)),str(library),'--output',str(report)],cwd=ROOT,capture_output=True,text=True)
 assert judged.returncode!=0 and 'AssertionError' in judged.stderr,(name,judged.stdout,judged.stderr)
 receipts.append(dict(name=name,command=command,compile_exit=0,judge_exit=judged.returncode,diagnostic=judged.stderr[-3500:]))
(OUT/'receipts.json').write_text(json.dumps(receipts,indent=2)+'\n')
print(len(receipts),'compiled semantic faults caught')
