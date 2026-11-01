"""Focused actual unit builds, with maintained translation units compiled directly."""
import hashlib,json,pathlib,subprocess
out=pathlib.Path('build/sse-readability/extra-units');out.mkdir(exist_ok=False)
flags=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-I.']
archives=['build/sse-readability/pinned/sse/softfloat-sse.a','build/sse-readability/pinned/base/softfloat.a']
bochs=['-Ithird_party/bochs_fpatan','-Ithird_party/bochs_fpatan/source','-Ithird_party/bochs_fpatan/source/softfloat3e/include']
tests=[('d3dx-math',['tests/unit/d3dx_math.cpp','src/math/d3dx_math.cpp','src/cpu/sse_math.cpp','src/cpu/x87_math.cpp','src/cpu/x87_transcendental.cpp'],bochs,['build/sse-readability/pinned/bochs/libbochs_fpatan.a']),('input-mod-pad',['tests/unit/input_mod_pad.cpp','src/input/input_mod_pad.cpp','src/input/input_mod_axes.cpp','src/input/input.cpp','src/engine/game_controller.cpp','src/cpu/sse_math.cpp'],[],[]),('input-mod-axes',['tests/unit/input_mod_axes.cpp','src/input/input_mod_axes.cpp','src/cpu/sse_math.cpp'],[],[]),('sse-contract-ubsan',['tests/unit/sse_math_contract.cpp','src/cpu/sse_math.cpp','src/cpu/x87_math.cpp'],['-fsanitize=undefined','-fno-sanitize-recover=all'],[])]
records=[]
for name,sources,extra,libs in tests:
    command=flags+extra+sources+archives+libs+['-o',str(out/name)]
    built=subprocess.run(command,capture_output=True,text=True)
    record=dict(name=name,command=command,compile_exit=built.returncode,stderr=built.stderr)
    if built.returncode==0:
        run=subprocess.run([str(out/name)],capture_output=True,text=True)
        record.update(run_exit=run.returncode,stdout=run.stdout,stderr=run.stderr)
    records.append(record)
    (out/'results.json').write_text(json.dumps(records,indent=2)+'\n')
    print(name,record,flush=True)
