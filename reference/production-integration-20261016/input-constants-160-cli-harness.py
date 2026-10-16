from pathlib import Path
import subprocess,json,hashlib
base=Path('reference/production-integration-20261016');build=Path('build')
BASE='1cad20c'
for name in ['reference','session']:
 source=f'src/guest/native_game_{name}.cpp'
 (build/f'input-constants-old-{name}.cpp').write_bytes(subprocess.check_output(['git','show',BASE+':'+source]))
common=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures']
rows=[]
for name,ref,session in [('before','build/input-constants-old-reference.cpp','build/input-constants-old-session.cpp'),('after','src/guest/native_game_reference.cpp','src/guest/native_game_session.cpp')]:
 exe=build/('input-constants-'+name+'-boot')
 subprocess.run([*common,ref,session,'src/apps/sadx_boot.cpp','build/libsadx-macos.a','build/libsadx-guest.a','build/libsadx-core.a','-o',str(exe)],check=True)
 result=subprocess.run([str(exe.resolve()),'game/Sonic Adventure DX/sonic.exe','game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll','3','--survey'],capture_output=True,check=True)
 (base/('input-constants-160-'+name+'-boot.txt')).write_bytes(result.stdout);rows.append(result.stdout)
assert rows[0]==rows[1]
(base/'input-constants-160-cli-comparison.json').write_text(json.dumps(dict(passed=True,base_commit=BASE,bytes_compared=len(rows[0]),sha256=hashlib.sha256(rows[0]).hexdigest()),indent=2)+'\n')
print('Historical CLI reports exact:',len(rows[0]),'bytes')
