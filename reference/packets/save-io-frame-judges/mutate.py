"""Compile copied faults only. Source tree and initial evidence never overwritten."""
from pathlib import Path
import hashlib
import json
import subprocess

root=Path('build/save-io-judges/mutants');root.mkdir(parents=True,exist_ok=True)
flags=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures','-Wl,-dead_strip']
io=['src/game/save_write.cpp','src/game/save_file_list.cpp','src/game/save_data.cpp','src/game/file_select.cpp']
frame=['src/game/save_frame.cpp','src/guest/game_loop.cpp']
mutants=[
 ('eager-encoding','src/game/save_write.cpp','    const auto file = calls.open_write(path);','    const auto encoded = encode(state.data);\n    const auto file = calls.open_write(path);', 'calls.write_save(file, encode(state.data));','calls.write_save(file, encoded);'),
 ('premature-ok','src/game/save_write.cpp','    calls.close_write(file);\n    state.save_ok = 1;','    state.save_ok = 1;\n    calls.close_write(file);'),
 ('copy-before-publication','src/game/save_write.cpp','        names.copy(destination, name);','        state.save_name.value = nullptr;\n        names.copy(destination, name);\n        state.save_name.value = destination;'),
 ('count-after-null-check','src/game/save_write.cpp','        ++state.file_count;\n        state.selected_file = slot;','        if (destination) { ++state.file_count; state.selected_file = slot; }'),
 ('reset-after-stores','src/game/save_write.cpp','    calls.reset_input();','', '    calls.close_write(file);','    calls.reset_input();\n    calls.close_write(file);'),
 ('eager-delete-guard','src/game/save_write.cpp','    calls.ensure_directory();\n    if (state.file_delete_flag == 0)','    const auto deleting = state.file_delete_flag;\n    calls.ensure_directory();\n    if (deleting == 0)'),
 ('skip-partial-read','src/game/save_file_list.cpp','    live = decode_save(bytes);','    if (count) live = decode_save(bytes);'),
 ('keep-retained-file','src/game/save_file_list.cpp','        previous->next->file.reset();','        // fault: retained alias retains filename'),
 ('close-search-eager','src/game/save_file_list.cpp','    std::uint8_t count = 0;','    calls.close_search(first->search);\n    std::uint8_t count = 0;'),
 ('frame-writer-late-clear','src/game/save_frame.cpp','        state.exec = 0;\n        calls.write_save_file();','        calls.write_save_file();\n        state.exec = 0;'),
 ('frame-lost-close-mutation','src/game/save_frame.cpp','        if (state.pause_state == 2)','        state.pause_state = 2;\n        if (state.pause_state == 2)'),
 ('frame-eager-phase','src/game/save_frame.cpp','    if (state.exec != 0) {','    const auto phase = state.now_saving;\n    if (state.exec != 0) {','    if (state.now_saving == 0) {','    if (phase == 0) {'),
 ('frame-missing-failure-prefix','src/game/save_frame.cpp','        calls.close_saving_message(state.message);\n        state.pending_message = 0;','        state.pending_message = 0;\n        calls.close_saving_message(state.message);'),
 ('reference-eager-exec','src/guest/game_loop.cpp','        call(GameLoopOperation::work_420840,0x40cc13,0x420840);','        const auto eager_exec = st.pending_at_3abdf76;\n        call(GameLoopOperation::work_420840,0x40cc13,0x420840);','if(need(st.pending_at_3abdf76,0x40cc1d)!=0)','if(need(eager_exec,0x40cc1d)!=0)'),
]
results=[]
for name,path,*edits in mutants:
 source=Path(path).read_text()
 for old,new in zip(edits[::2],edits[1::2]):
  assert old in source,(name,old)
  source=source.replace(old,new,1)
 copied=root/(name+'.cpp');copied.write_text(source)
 sources=frame if path in frame else io
 test='tests/unit/save_frame_characterization.cpp' if sources is frame else 'tests/unit/save_io_characterization.cpp'
 exe=root/name;cmd=flags+[test]+[str(copied) if p==path else p for p in sources]+['-o',str(exe)]
 built=subprocess.run(cmd,capture_output=True,text=True)
 (root/(name+'.compile.log')).write_text(built.stdout+built.stderr)
 result={'name':name,'source':path,'source_sha256':hashlib.sha256(Path(path).read_bytes()).hexdigest(),'fault_sha256':hashlib.sha256(copied.read_bytes()).hexdigest(),'compile_command':cmd,'compile_exit':built.returncode,'run_exit':None,'detected':False}
 if built.returncode==0:
  run=subprocess.run([str(exe)],capture_output=True,text=True)
  (root/(name+'.run.log')).write_text(run.stdout+run.stderr)
  result.update(run_exit=run.returncode,detected=run.returncode!=0)
 results.append(result)
 (root/'results.json').write_text(json.dumps(results,indent=2)+'\n')
 print(name,result['compile_exit'],result['run_exit'],flush=True)
