from pathlib import Path
import subprocess, hashlib, json
root=Path.cwd();base=root/'reference/production-integration-20261016';build=root/'build'
old=subprocess.check_output(['git','show','817d0eb7671bcf5659392dcc52fc4589e52551b1:src/guest/native_game_reference.cpp'],text=True)
new=(root/'src/guest/native_game_reference.cpp').read_text()
a=new.index('std::array<std::optional<std::uint32_t>, 30> NativeGame::view_facts()');b=new.index('void NativeGame::initialize_sound_defaults()',a)
old+='\nnamespace sadx {\n'+new[a:b]+'\n}\n'
(build/'view-initial-old-reference.cpp').write_text(old)
common=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures']
outputs=[]
args=['game/Sonic Adventure DX/sonic.exe','build/view-initial-poisoned.exe','build/view-initial-varied.exe','build/view-initial-partial.exe','-']
for name,cpp in [('before',str(build/'view-initial-old-reference.cpp')),('after','src/guest/native_game_reference.cpp')]:
 exe=build/('view-initial-'+name+'-constructor')
 subprocess.run([*common,cpp,str(base/'view-initial-158-constructor-driver.cpp'),'build/libsadx-guest.a','build/libsadx-core.a','-o',str(exe)],check=True)
 outputs.append(subprocess.check_output([str(exe),*args]))
assert outputs[0]==outputs[1]
rows=outputs[0].decode().splitlines()
assert len(rows)==5 and all(len(row.split(';'))-1==174 for row in rows)
report=dict(passed=True,inputs=args,configurations=5,facts_per_configuration=174,facts_compared=870,bytes_compared=len(outputs[0]),sha256=hashlib.sha256(outputs[0]).hexdigest(),rows_sha256=[hashlib.sha256(row.encode()).hexdigest() for row in rows])
(base/'view-initial-158-constructor-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
