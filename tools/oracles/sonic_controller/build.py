"""Build the Sonic packet from maintained source, isolated outputs, max two compilers."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

from observe import sha

ROOT = Path(__file__).resolve().parents[3]
OLDER = ROOT / "tools/oracles/animation_judges"
sys.path.insert(0, str(OLDER))
SPEC = importlib.util.spec_from_file_location("sonic_support_definition", OLDER / "build.py")
previous = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(previous)
FLAGS = previous.FLAGS + ["-Isrc", "-Ithird_party/bochs_fpatan",
                          "-Ithird_party/bochs_fpatan/source",
                          "-Ithird_party/bochs_fpatan/source/softfloat3e/include", "-fPIC"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--assets", type=Path, help="Run reference units with existing test constants")
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    receipts = []

    def run(command):
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        receipt = dict(command=command, exit_status=result.returncode,
                       output=result.stdout + result.stderr)
        receipts.append(receipt)
        (out / "commands.json").write_text(json.dumps(receipts, indent=2) + "\n")
        if result.returncode:
            raise RuntimeError(receipt)

    for name, script in [("softfloat", "third_party/softfloat/build.sh"),
                         ("bochs", "third_party/bochs_fpatan/build.sh")]:
        run(["sh", script, str(out / name)])
    numeric = [str(out / "softfloat/softfloat.a"), str(out / "bochs/libbochs_fpatan.a")]
    support = [path for path in previous.SONIC if path not in
               ("src/player/sonic_player.cpp", "src/guest/sonic_player_reference.cpp")]
    support += ["src/engine/collision_component.cpp", "src/player/ground_acceleration_native.cpp",
                "src/math/ninja_length.cpp"]
    special = [("src/player/sonic_player.cpp", "core", []),
               ("src/guest/sonic_player_reference.cpp", "reference", []),
               ("src/runtime/native_sonic_binding.cpp", "native", []),
               ("tests/probes/native_sonic_player_probe.cpp", "sonic_probe",
                ["-include", "tests/reference/actor_current_fixture.hpp"]),
               ("tests/probes/native_sonic_action_probe.cpp", "action_probe", [])]
    jobs = [(path, path.replace("/", "_"), []) for path in dict.fromkeys(support)] + special

    def compile(job):
        source, name, extra = job
        obj = out / (name + ".o")
        command = FLAGS + extra + ["-MMD", "-c", source, "-o", str(obj)]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        dependencies = []
        if result.returncode == 0:
            text = obj.with_suffix(".d").read_text().replace("\\\n", " ")
            dependencies = text.split(":", 1)[1].split()
        return dict(source=source, name=name, object=str(obj), command=command,
                    exit_status=result.returncode, output=result.stdout + result.stderr,
                    dependencies={path: sha((ROOT / path).read_bytes()) for path in dependencies})

    with ThreadPoolExecutor(max_workers=2) as workers:
        compiled = list(workers.map(compile, jobs))
    (out / "source-build.json").write_text(json.dumps(compiled, indent=2) + "\n")
    if any(row["exit_status"] for row in compiled):
        raise RuntimeError([row for row in compiled if row["exit_status"]])
    objects = {row["name"]: row["object"] for row in compiled}
    for variant in ("reference", "native"):
        selected = [row["object"] for row in compiled if row["source"] in support
                    and ((variant == "native" and not row["source"].startswith("src/guest/"))
                         or (variant == "reference" and row["source"] in previous.SONIC))]
        run(["xcrun", "libtool", "-static", "-o", str(out / (variant + "-support.a"))] + selected)
    reference = [objects["core"], objects["reference"], str(out / "reference-support.a")] + numeric
    for slug, probe in [("sonic_player", "sonic_probe"), ("sonic_actions", "action_probe")]:
        run(FLAGS + ["-dynamiclib", objects[probe]] + reference + ["-o", str(out / (slug + ".dylib"))])
    for slug, source, extra, fixture in [
            ("sonic-player", "sonic_player", ["-include", "tests/reference/actor_current_fixture.hpp"],
             ["sonic-player-constants.bin", "sonic-action-dispatch.bin", "ninja-trig.f32", "ninja-unit.f32"]),
            ("sonic-actions", "sonic_actions", ["-include", "tests/reference/actor_current_fixture.hpp"],
             ["sonic-player-constants.bin"]),
            ("animation-sonic-composition", "animation_sonic_composition", ["-DANIMATION_TYPED"],
             ["sonic-player-constants.bin", "player-parameters.bin"]),
            ("actor-current", "actor_current", [], []),
            ("sonic-controller-reference", "sonic_controller_reference", [], [])]:
        executable = out / (slug + "-test")
        run(FLAGS + extra + ["tests/unit/" + source + ".cpp"] + reference + ["-o", str(executable)])
        if args.assets is not None:
            run([str(executable)] + [str((args.assets / path).resolve()) for path in fixture])
    native = [objects["core"], objects["native"], str(out / "native-support.a")] + numeric
    executable = out / "native-harness"
    run(FLAGS + ["tests/unit/sonic_controller_native.cpp"] + native +
        ["-Wl,-map," + str(out / "native-harness.map"), "-o", str(executable)])
    run([str(executable)])
    run(["nm", "-u", str(executable)])
    print("Private Sonic reference and native closures built from source; max two compilers.")


if __name__ == "__main__":
    main()
