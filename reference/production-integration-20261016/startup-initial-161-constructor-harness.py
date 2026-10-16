from pathlib import Path
import subprocess, hashlib, json
root=Path.cwd();base=root/'reference/production-integration-20261016';build=root/'build'
old=subprocess.check_output(['git','show','8a03588:src/guest/native_game_reference.cpp'],text=True)
new=(root/'src/guest/native_game_reference.cpp').read_text()
a=new.index('std::array<std::optional<std::uint32_t>, 41> NativeGame::startup_facts()');b=new.index('void NativeGame::initialize_queue_defaults()',a)
old+='\nnamespace sadx {\n'+new[a:b]+'\n}\n'
(build/'startup-initial-old-reference.cpp').write_text(old)
common=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures']
outputs=[]
args=['game/Sonic Adventure DX/sonic.exe','build/startup-initial-poisoned.exe','build/startup-initial-varied.exe','build/startup-initial-partial.exe','-']
for name,cpp in [('before',str(build/'startup-initial-old-reference.cpp')),('after','src/guest/native_game_reference.cpp')]:
 exe=build/('startup-initial-'+name+'-constructor')
 subprocess.run([*common,cpp,str(base/'startup-initial-161-constructor-driver.cpp'),'build/libsadx-guest.a','build/libsadx-core.a','-o',str(exe)],check=True)
 outputs.append(subprocess.check_output([str(exe),*args]))
assert outputs[0]==outputs[1]
rows=outputs[0].decode().splitlines()
assert len(rows)==5 and all(len(row.split(';'))-1==425 for row in rows)
report=dict(passed=True,inputs=args,configurations=5,facts_per_configuration=425,facts_compared=2125,bytes_compared=len(outputs[0]),sha256=hashlib.sha256(outputs[0]).hexdigest(),rows_sha256=[hashlib.sha256(row.encode()).hexdigest() for row in rows])
(base/'startup-initial-161-constructor-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
