from pathlib import Path
import subprocess,tempfile,json,hashlib
root=Path.cwd()
sources={name:(root/name).read_text() for name in ['src/guest/native_game_session.cpp','src/guest/native_game_reference.cpp']}
selection='config.input_constants=input_mod_261_constants();'
decoded='''const auto &dll=*config.input_mod_dll;
    const auto coefficient=[&](std::uint32_t address){return Sse32{dll.u32(address).value_or(0)};};
    config.input_constants={{coefficient(0x1000bb6c),coefficient(0x1000bbb0),coefficient(0x1000bba0),coefficient(0x1000bba4),coefficient(0x1000bb7c)},coefficient(0x1000bba8),coefficient(0x1000bb78)};'''
mutations=[('decode_DLL_in_boot','src/guest/native_game_session.cpp',selection,decoded),
           ('omit_boot_coefficients','src/guest/native_game_session.cpp',selection,'config.input_constants={};'),
           ('erase_injected_coefficients','src/guest/native_game_reference.cpp','    initialize_source_facts();','    if(!config_.input_mod_dll)config_.input_constants={};\n    initialize_source_facts();')]
rows=[]
for name,target,before,after in [('baseline',None,None,None),*mutations]:
 with tempfile.TemporaryDirectory(prefix='sadx-input-constant-bridge-') as directory:
  base=Path(directory);files=[]
  for path,original in sources.items():
   text=original
   if path==target:
    assert text.count(before)==1,(name,text.count(before))
    text=text.replace(before,after,1)
   cpp=base/Path(path).name;cpp.write_text(text);files.append(str(cpp))
  executable=base/'boot-test'
  command=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures',*files,'tests/unit/input_constants_boot.cpp','tests/probes/native_input_mod_constants_probe.cpp','build/libsadx-guest.a','build/libsadx-core.a','-o',str(executable)]
  built=subprocess.run(command,cwd=root,capture_output=True,text=True)
  assert built.returncode==0,(name,built.stderr)
  result=subprocess.run([str(executable),'game/Sonic Adventure DX/sonic.exe','game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll','build/input-constants'],cwd=root,capture_output=True,text=True)
  assert result.returncode==(0 if target is None else 1),(name,result)
  row=dict(name=name,exit_status=result.returncode,killed=result.returncode!=0,output=(result.stdout+result.stderr).strip(),target=target,replacement=[before,after]);rows.append(row);print(row,flush=True)
report=dict(passed=True,source_sha256={p:hashlib.sha256(s.encode()).hexdigest() for p,s in sources.items()},baseline=rows[0],mutants=rows[1:],worktree_mutated=False,limits=['Temporary source copies linked before unchanged archives; no worktree mutation.','Three-frame composition has no sprite draws; no physical-controller/graphics or full-session independence claim.'])
(root/'reference/production-integration-20261016/input-constants-160-boot-mutants.json').write_text(json.dumps(report,indent=2)+'\n')
