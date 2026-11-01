"""Proportionate direct-consumer compile/include evidence, without native archives."""
import hashlib,json,pathlib,subprocess
out=pathlib.Path('build/sse-readability/consumers');out.mkdir(exist_ok=False)
flags=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-I.']
sources=['src/input/input_mod_axes.cpp','src/input/input_mod_pad.cpp','src/input/input.cpp','src/math/d3dx_math.cpp','src/engine/ninja_projected_depth.cpp','src/engine/ninja_view_native.cpp','src/runtime/native_input.cpp','src/runtime/logo_renderer.cpp']
records=[]
for source in sources:
    name=pathlib.Path(source).stem
    command=flags+['-MMD','-MF',str(out/(name+'.d')),'-c',source,'-o',str(out/(name+'.o'))]
    result=subprocess.run(command,capture_output=True,text=True)
    row=dict(source=source,source_sha256=hashlib.sha256(pathlib.Path(source).read_bytes()).hexdigest(),command=command,exit=result.returncode,stderr=result.stderr)
    if result.returncode==0:
        deps=(out/(name+'.d')).read_text().replace('\\\n','').split(':',1)[1].split()
        row['includes']={p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in deps}
    records.append(row)
    print(source,result.returncode,flush=True)
    (out/'results.json').write_text(json.dumps(records,indent=2)+'\n')
