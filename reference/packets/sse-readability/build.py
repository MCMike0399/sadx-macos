"""Small direct-source builds; never link a maintained project archive."""
import json, pathlib, subprocess, sys
out=pathlib.Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=False)
sse=sys.argv[2] if len(sys.argv)>2 else 'src/cpu/sse_math.cpp'
flags=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-arch','arm64','-mmacosx-version-min=14.0','-Isrc','-I.']
archives=['build/sse-readability/pinned/sse/softfloat-sse.a','build/sse-readability/pinned/base/softfloat.a']
commands=[]
def build(name,sources,extra=[]):
    command=flags+extra+sources+archives+['-o',str(out/name)]
    result=subprocess.run(command,capture_output=True,text=True)
    commands.append(dict(command=command,exit=result.returncode,stdout=result.stdout,stderr=result.stderr))
    (out/'build.json').write_text(json.dumps(commands,indent=2)+'\n')
    if result.returncode:print(result.stderr);raise SystemExit(result.returncode)
build('probes.dylib',['tests/probes/native_sse_math_probe.cpp','tests/probes/native_input_mod_axes_probe.cpp','tests/probes/native_input_mod_pad_probe.cpp',sse,'src/input/input_mod_axes.cpp','src/input/input_mod_pad.cpp','src/input/input.cpp','src/engine/game_controller.cpp'],['-dynamiclib'])
build('sse-unit',['tests/unit/sse_math.cpp',sse,'src/cpu/x87_math.cpp'])
build('sse-contract',['tests/unit/sse_math_contract.cpp',sse,'src/cpu/x87_math.cpp'])
build('axes-contract',['tests/unit/input_mod_axes_contract.cpp',sse,'src/input/input_mod_axes.cpp'])
