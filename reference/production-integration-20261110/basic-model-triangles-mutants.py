import json, pathlib, subprocess, time, os

SRC = pathlib.Path("src/engine/basic_model.cpp")
MUTANTS = [
    ["triangles draw as a strip",
     "draw_normal_uv_triangles(context, mesh, model.points, model.normals);",
     "draw_normal_uv_strip(context, mesh, model.points, model.normals);"],
    ["strip draws as triangles",
     "draw_normal_uv_strip(context, mesh, model.points, model.normals);",
     "draw_normal_uv_triangles(context, mesh, model.points, model.normals);"],
]
orig = SRC.read_text()

def run_oracle():
    out = pathlib.Path("build/oracles/basic_model.json")
    if out.exists():
        out.unlink()
    subprocess.run(["make", "-s", "-j10", "oracle-basic-model"], capture_output=True, text=True)
    if not out.exists():
        return ("aborted", -1)
    r = json.loads(out.read_text())
    return ("pass" if r.get("values_match") else "fail", r.get("mismatch_count"))

def build():
    return subprocess.run(["make", "-s", "-j10", "build/native-basic-model-probe.dylib"],
                          capture_output=True, text=True).returncode

assert build() == 0
print("baseline", run_oracle(), flush=True)
for name, a, b in MUTANTS:
    assert a in orig, name
    try:
        SRC.write_text(orig.replace(a, b, 1))
        time.sleep(1.1)
        os.utime(SRC, None)
        if build() != 0:
            print(name, "INVALID", flush=True)
            continue
        result = run_oracle()
        print(name, "caught" if result[0] != "pass" else "SURVIVED", result, flush=True)
    finally:
        SRC.write_text(orig)
        time.sleep(1.1)
        os.utime(SRC, None)
        assert build() == 0
print("restored", run_oracle(), flush=True)
