"""Build and check Sonic physics identity scopes without copying game algorithms.

The native yaw observer throws at the existing uncomposed callee boundary. It
never supplies successful yaw behavior. The one regression mutation is exercised
only with a nonnull replacement, so its deliberate reload cannot create null-ref
UB. Every witness explicitly retains original physics objects.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import subprocess

from build import FLAGS, ROOT
from observe import sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--support", type=Path, required=True)
    parser.add_argument("--old", type=Path, required=True)
    args = parser.parse_args()
    out, support = args.output.resolve(), args.support.resolve()
    out.mkdir(parents=True, exist_ok=True)
    commands = []

    def save():
        (out / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")

    def run(command, expected=0):
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        row = dict(command=command, exit_status=result.returncode,
                   output=result.stdout + result.stderr)
        commands.append(row)
        save()
        if result.returncode != expected:
            raise RuntimeError(row)
        return row

    jobs = [("src/player/sonic_player.cpp", "core", []),
            ("src/guest/sonic_player_reference.cpp", "reference", []),
            ("src/runtime/native_sonic_binding.cpp", "native", []),
            ("src/player/ground_acceleration_native.cpp", "ground-observer",
             ["-Dturn_ground_yaw=unobserved_ground_yaw_stub"])]

    def compile(job):
        source, name, extra = job
        obj = out / (name + ".o")
        command = FLAGS + extra + ["-MMD", "-c", source, "-o", str(obj)]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        dependencies = {}
        if result.returncode == 0:
            paths = obj.with_suffix(".d").read_text().replace("\\\n", " ").split(":", 1)[1].split()
            dependencies = {path: sha((ROOT / path).read_bytes()) for path in paths}
        return dict(command=command, exit_status=result.returncode,
                    output=result.stdout + result.stderr, dependencies=dependencies)

    with ThreadPoolExecutor(max_workers=2) as workers:
        commands.extend(workers.map(compile, jobs))
    save()
    if any(row["exit_status"] for row in commands):
        raise RuntimeError("Source closure did not compile")
    numeric = [str(support / "softfloat/softfloat.a"),
               str(support / "bochs/libbochs_fpatan.a")]
    reference = [str(out / "core.o"), str(out / "reference.o"),
                 str(support / "reference-support.a")] + numeric
    native = [str(out / "core.o"), str(out / "native.o"), str(out / "ground-observer.o"),
              str(support / "native-support.a")] + numeric
    for slug, probe in [("sonic_player", "sonic_probe.o"), ("sonic_actions", "action_probe.o")]:
        run(FLAGS + ["-dynamiclib", str(support / probe)] + reference +
            ["-o", str(out / (slug + ".dylib"))])
    source = "tests/unit/sonic_physics_capture_reference.cpp"
    run(FLAGS + [source, str(args.old.resolve()), "-o", str(out / "old-reference")])
    old = run([str(out / "old-reference")])
    run(FLAGS + [source] + reference + ["-o", str(out / "reference-test")])
    current = run([str(out / "reference-test")])
    if old["output"] != current["output"]:
        raise ValueError("Old/current scope observations differ")
    run(FLAGS + ["tests/unit/sonic_physics_capture_native.cpp"] + native +
        ["-o", str(out / "native-test")])
    native_result = run([str(out / "native-test")])
    # Restore precisely the reviewed same-record physics reload in a private copy.
    original = (ROOT / "src/guest/sonic_player_reference.cpp").read_text()
    before = "fn(*f.task, *f.world, *p.physics, parameters, yaw, fp())"
    after = "fn(*f.task, *f.world, *p.record->player, parameters, yaw, fp())"
    if original.count(before) != 1:
        raise ValueError("Capture mutation anchor is not unique")
    mutant = out / "reloaded-physics.cpp"
    mutant.write_text(original.replace(before, after))
    run(FLAGS + [source, str(mutant), str(out / "core.o"),
                 str(support / "reference-support.a")] + numeric +
        ["-o", str(out / "capture-mutant")])
    mutated = run([str(out / "capture-mutant"), "repro"], expected=1)
    report = dict(old_library_sha256=sha(args.old.read_bytes()),
                  old_observations=old["output"], current_observations=current["output"],
                  native_observations=native_result["output"],
                  regression_mutant_observation=mutated["output"],
                  regression_mutant_caught=True,
                  mutation_original_sha256=sha(original.encode()),
                  mutation_replacement_sha256=sha(mutant.read_bytes()),
                  reference_cases=8, native_cases=7,
                  limits=["Controlled helper/callee-boundary witnesses, not a game frame",
                          "Original objects are retained explicitly; borrows add no physics ownership",
                          "All previous default observations and mutants remain required"])
    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("Old/current reference scopes, native physics/contact scopes and capture regression fault verified.")


if __name__ == "__main__":
    main()
