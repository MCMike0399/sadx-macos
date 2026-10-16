from pathlib import Path
import subprocess, hashlib, json
root=Path.cwd();base=root/'reference/production-integration-20261016';build=root/'build'
old=subprocess.check_output(['git','show','2078536:src/guest/native_game_reference.cpp'],text=True)
new=(root/'src/guest/native_game_reference.cpp').read_text()
a=new.index('std::array<std::optional<std::uint32_t>, 28> NativeGame::library_facts()');b=new.index('void NativeGame::initialize_startup_defaults()',a)
getters=new[a:b]
getters=getters.replace('    for (const auto &value : library_configuration_)\n        facts[next++] = value;', '    for (const auto address : std::array<std::uint32_t, 14>{0x892954, 0x892958, 0x892968, 0x89296c, 0x892970, 0x892974, 0x892978, 0x89297c, 0x892980, 0x892984, 0x892988, 0x89298c, 0x892990, 0x892994})\n        facts[next++] = config_.executable ? config_.executable->u32(address) : std::nullopt;')
getters=getters.replace('facts[i] = read_library_configuration(0x892940 + 4 * i);', 'facts[i] = config_.executable ? config_.executable->u32(0x892940 + 4 * i) : std::nullopt;')
old+='\nnamespace sadx {\n'+getters+'\n}\n'
(build/'library-initial-old-reference.cpp').write_text(old)
common=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures']
outputs=[]
args=['game/Sonic Adventure DX/sonic.exe','build/library-initial-poisoned.exe','build/library-initial-varied.exe','build/library-initial-partial.exe','-']
for name,cpp in [('before',str(build/'library-initial-old-reference.cpp')),('after','src/guest/native_game_reference.cpp')]:
 exe=build/('library-initial-'+name+'-constructor')
 subprocess.run([*common,cpp,str(base/'library-initial-162-constructor-driver.cpp'),'build/libsadx-guest.a','build/libsadx-core.a','-o',str(exe)],check=True)
 outputs.append(subprocess.check_output([str(exe),*args]))
assert outputs[0]==outputs[1]
rows=outputs[0].decode().splitlines()
assert len(rows)==5 and all(len(row.split(';'))-1==453 for row in rows)
report=dict(passed=True,inputs=args,configurations=5,facts_per_configuration=453,facts_compared=2265,bytes_compared=len(outputs[0]),sha256=hashlib.sha256(outputs[0]).hexdigest(),rows_sha256=[hashlib.sha256(row.encode()).hexdigest() for row in rows])
(base/'library-initial-162-constructor-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
