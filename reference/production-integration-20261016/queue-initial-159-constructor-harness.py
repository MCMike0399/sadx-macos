from pathlib import Path
import subprocess, hashlib, json
root=Path.cwd();base=root/'reference/production-integration-20261016';build=root/'build'
old=subprocess.check_output(['git','show','ef45485033351c9a018151c0dac8969de0a9d55f:src/guest/native_game_reference.cpp'],text=True)
new=(root/'src/guest/native_game_reference.cpp').read_text()
a=new.index('std::array<std::optional<std::uint32_t>, 21> NativeGame::queue_facts()');b=new.index('void NativeGame::initialize_view_defaults()',a)
old+='\nnamespace sadx {\n'+new[a:b]+'\n}\n'
(build/'queue-initial-old-reference.cpp').write_text(old)
common=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures']
outputs=[]
args=['game/Sonic Adventure DX/sonic.exe','build/queue-initial-poisoned.exe','build/queue-initial-varied.exe','build/queue-initial-partial.exe','-']
for name,cpp in [('before',str(build/'queue-initial-old-reference.cpp')),('after','src/guest/native_game_reference.cpp')]:
 exe=build/('queue-initial-'+name+'-constructor')
 subprocess.run([*common,cpp,str(base/'queue-initial-159-constructor-driver.cpp'),'build/libsadx-guest.a','build/libsadx-core.a','-o',str(exe)],check=True)
 outputs.append(subprocess.check_output([str(exe),*args]))
assert outputs[0]==outputs[1]
rows=outputs[0].decode().splitlines()
assert len(rows)==5 and all(len(row.split(';'))-1==197 for row in rows)
report=dict(passed=True,inputs=args,configurations=5,facts_per_configuration=197,facts_compared=985,bytes_compared=len(outputs[0]),sha256=hashlib.sha256(outputs[0]).hexdigest(),rows_sha256=[hashlib.sha256(row.encode()).hexdigest() for row in rows])
(base/'queue-initial-159-constructor-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
