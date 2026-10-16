from pathlib import Path
import subprocess,tempfile,json,hashlib
root=Path.cwd();source=root/'src/guest/native_game_reference.cpp';original=source.read_text()
rows=[]
for name,old,new in [('baseline',None,None),('ignore_native_controls','if (config_.control_defaults) {','if (false && config_.control_defaults) {'),('drop_known_zero_movie','mode_state_.word_at_3b2c45c = values.flow.movie_number;','mode_state_.word_at_3b2c45c.reset();'),('wrong_frequency_high','std::uint64_t(values.renderer.counter_frequency) >> 32','std::uint64_t(values.renderer.counter_frequency) >> 0')]:
 with tempfile.TemporaryDirectory(prefix='sadx-runtime-tables-mutant-') as temporary:
  base=Path(temporary);text=original
  if old:
   assert text.count(old)==1,(name,text.count(old));text=text.replace(old,new,1)
  cpp=base/'native_game_reference.cpp';cpp.write_text(text);exe=base/'runtime-test'
  command=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures',str(cpp),'tests/unit/game_runtime.cpp','tests/probes/native_session_control_defaults_probe.cpp','build/libsadx-guest.a','build/libsadx-core.a','-o',str(exe)]
  built=subprocess.run(command,cwd=root,text=True,capture_output=True);assert built.returncode==0,(name,built.stderr)
  trial=subprocess.run([str(exe),'game/Sonic Adventure DX/sonic.exe','game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll','build/engine-code-tables-poisoned.exe','build/session-control-poisoned.exe','build/session-control-varied.exe','build/session-control-partial.exe'],cwd=root,text=True,capture_output=True)
  assert trial.returncode==(0 if name=='baseline' else 1),(name,trial)
  row=dict(name=name,killed=trial.returncode!=0,exit_status=trial.returncode,output=(trial.stdout+trial.stderr).strip());rows.append(row);print(row,flush=True)
report=dict(passed=True,baseline=rows[0],mutants=rows[1:],source_sha256=hashlib.sha256(original.encode()).hexdigest(),limits=['Temporary source copies only; links explicit mutant composition before unchanged archives.','Three-frame survey composition only; no sprite draws or full native game claim.'])
(root/'reference/production-integration-20261016/runtime-control-152-bridge-mutants.json').write_text(json.dumps(report,indent=2)+'\n')
