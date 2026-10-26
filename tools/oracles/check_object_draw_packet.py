"""Private source-only build and unchanged recovered object-draw judge.

All outputs stay below --output. No root Make registration or archive is used.
The old probe/unit are force-included with compatibility declarations; the game
translation unit is compiled separately without that reference-only header.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
FROZEN = {
    "tests/probes/native_object_draw_probe.cpp":
        "ff770b7dc8d84d8b9ba4b4a317acf2a3417ded84980b39c5cd91691a9feaecb2",
    "tests/unit/object_draw.cpp":
        "ced626b4bbfe345c0eef36efbe18c22b9974c672f3578bc2bce8e60e43bbcf15",
    "tools/oracles/verify_original_object_draw.py":
        "012abf1afad8e6c9c65e47d1990a9cb013ca21843822cdd36220d2e2ed5a85e3",
    "tests/fixtures/object_draw_original_mutants.json":
        "60e7225cbcdd2af591bac4a3aaeec4667f3aa53424123a091e8c10d2b837b54b",
}
# Original 15 mutation intents retained, with tokens mapped to the fresh API.
# The last four distinguish additional premature reads/restoration.
MUTATIONS = [
    ("wrong-fog-bits", "(state.fog_special_flags & both) == both",
     "(static_cast<void>(both), (state.fog_special_flags & enabled) == enabled)"),
    ("wrong-fog-active", "state.fog_active == 0", "state.fog_active != 0"),
    ("wrong-late-depth", "-47952.0f", "-48000.0f"),
    ("wrong-flag-restore", "state.fog_special_flags |= emulated;",
     "state.fog_special_flags = both;"),
    ("wrong-pending-guard", "state.pending_logic_ticks == 0", "state.pending_logic_ticks >= 0"),
    ("wrong-gate-polarity", "!texture_list_not_ready(context)", "texture_list_not_ready(context)"),
    ("wrong-transform-case", "!has(flags, ObjectFlags::UnitTransform)",
     "(static_cast<std::uint32_t>(flags) & "
     "static_cast<std::uint32_t>(ObjectFlags::UnitTransform)) != 0"),
    ("wrong-translation-mask", "!has(flags, ObjectFlags::UnitPosition)",
     "!has(flags, ObjectFlags::UnitAngle)"),
    ("wrong-rotation-order", "has(flags, ObjectFlags::AlternateRotation)",
     "!has(flags, ObjectFlags::AlternateRotation)"),
    ("wrong-scale-mask", "!has(flags, ObjectFlags::UnitScale)",
     "!has(flags, ObjectFlags::UnitAngle)"),
    ("wrong-model-mask", "!has(flags, ObjectFlags::Hide)",
     "!has(flags, ObjectFlags::UnitScale)"),
    ("wrong-child-mask", "!has(flags, ObjectFlags::Break)", "has(flags, ObjectFlags::Break)"),
    ("copied-node-not-live", "const auto &node = object(context, root);",
     "const auto node = object(context, root);"),
    ("missing-pop", "pop_matrix_ex(context);", "(void)0;"),
    ("simple-return", "return 0;", "return 1;"),
    ("model-before-push", "if (transformed) {",
     "const auto cached_model = node.model;\n        if (transformed) {"),
    ("child-before-render", "if (!has(flags, ObjectFlags::Hide)",
     "const auto cached_child = node.child;\n        if (!has(flags, ObjectFlags::Hide)"),
    ("sibling-before-child", "if (!has(flags, ObjectFlags::Break))",
     "const auto cached_sibling = node.sibling;\n        if (!has(flags, ObjectFlags::Break))"),
    ("overwrite-late-depth", "draw_late_object(context, root, kLateObjectFlags);",
     "draw_late_object(context, root, kLateObjectFlags);\n        state.queued_depth = kLateObjectDepth;"),
]


def run(command, log, expected=0):
    with log.open("w") as stream:
        result = subprocess.run([str(x) for x in command], cwd=ROOT,
                                stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode != expected:
        raise RuntimeError(f"{command[0]} exited {result.returncode}: {log}")
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--python", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "build/object-draw")
    parser.add_argument("--mutants", action="store_true")
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / "build"):
        parser.error("output must be inside this worktree's build directory")
    output.mkdir(parents=True, exist_ok=True)
    for name, digest in FROZEN.items():
        assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == digest, name
    original_mutations = json.loads(
        (ROOT / "tests/fixtures/object_draw_original_mutants.json").read_text())["mutations"]
    assert [row[0] for row in original_mutations] == [row[0] for row in MUTATIONS[:15]]
    flags = ["clang++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
             "-ffp-contract=off", "-fno-fast-math", "-fPIC", "-Isrc"]
    compatibility = ["-include", "tests/probes/object_draw_compat.hpp"]
    sources = {"core": "src/engine/object_draw.cpp",
               "compat": "tests/probes/object_draw_compat.cpp",
               "probe": "tests/probes/native_object_draw_probe.cpp",
               "queue": "src/engine/ninja_sprite_queue.cpp"}
    for name, source in sources.items():
        extra = compatibility if name == "probe" else []
        run(flags + extra + ["-c", source, "-o", output / f"{name}.o"],
            output / f"{name}-build.log")
    shared = [output / f"{name}.o" for name in ("compat", "probe", "queue")]

    def link(core, library):
        run(["clang++", "-dynamiclib", core, *shared,
             "-Wl,-dead_strip,-exported_symbol,_sadx_native_object_draw_probe", "-o", library],
            library.with_suffix(".link.log"))

    def judge(library, report, holdout=False, expected=0):
        command = [args.python, "tools/oracles/verify_original_object_draw.py", args.exe,
                   library, "--output", report]
        if holdout:
            command += ["--cases", "4400", "--seed", "884035"]
        run(command, report.with_suffix(".log"), expected)
        return json.loads(report.read_text())

    library = output / "probe.dylib"
    link(output / "core.o", library)
    dev = judge(library, output / "dev.json")
    holdout = judge(library, output / "holdout.json", holdout=True)
    run(flags + compatibility + ["tests/unit/object_draw.cpp", output / "core.o",
                                output / "compat.o", "-o", output / "object-test"],
        output / "unit-build.log")
    run([output / "object-test"], output / "unit.log")
    run(["make", "-j4", "build/softfloat/softfloat.a"], output / "softfloat-build.log")
    run(flags + ["tests/unit/object_draw_native.cpp", output / "core.o",
                 "src/engine/ninja_matrix.cpp", "src/cpu/x87_math.cpp",
                 "build/softfloat/softfloat.a", "-Wl,-dead_strip",
                 "-o", output / "native-test"], output / "native-build.log")
    run([output / "native-test"], output / "native.log")
    mutants = []
    if args.mutants:
        source = (ROOT / sources["core"]).read_text()
        for index, (name, before, after) in enumerate(MUTATIONS):
            assert source.count(before) == 1, name
            mutant = source.replace(before, after)
            if name == "model-before-push":
                mutant = mutant.replace("&& node.model != 0", "&& cached_model != 0")
                mutant = mutant.replace("(node.model, context)", "(cached_model, context)")
            elif name == "child-before-render":
                mutant = mutant.replace("draw_objects(node.child,", "draw_objects(cached_child,")
            elif name == "sibling-before-child":
                mutant = mutant.replace("root = node.sibling;", "root = cached_sibling;")
            directory = output / "mutants" / name
            directory.mkdir(parents=True, exist_ok=True)
            cpp = directory / "object_draw.cpp"
            cpp.write_text(mutant)
            obj, library = directory / "core.o", directory / "probe.dylib"
            run(flags + ["-c", cpp, "-o", obj], directory / "build.log")
            link(obj, library)
            # The frozen receipts mutate the current node, not a parent from a
            # child callback. The frozen alias unit supplies that discriminator.
            alias_only = name == "sibling-before-child"
            report = judge(library, directory / "report.json", expected=0 if alias_only else 1)
            if alias_only:
                run(flags + compatibility + ["tests/unit/object_draw.cpp", obj,
                                            output / "compat.o", "-o", directory / "alias-test"],
                    directory / "unit-build.log")
                unit_status = run([directory / "alias-test"], directory / "unit.log", expected=-6)
            else:
                assert report["mismatches"] > 0 and not report["passed"], name
                unit_status = None
            mutants.append({"name": name, "group": "preserved" if index < 15 else "new",
                            "oracle_exit_status": 0 if alias_only else 1,
                            "alias_unit_exit_status": unit_status, "executed": report["executed"],
                            "mismatches": report["mismatches"],
                            "source_sha256": hashlib.sha256(cpp.read_bytes()).hexdigest()})
            print(f"caught {name}", flush=True)
    summary = {"dev": {k: dev[k] for k in ("cases", "executed", "mismatches", "passed")},
               "holdout": {k: holdout[k] for k in ("cases", "executed", "mismatches", "passed")},
               "recovered_unit_exit_status": 0, "native_unit_exit_status": 0,
               "frozen_inputs": FROZEN,
               "mutants": mutants}
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({k: v for k, v in summary.items() if k != "mutants"}))
    print(f"mutants caught: {len(mutants)}")


if __name__ == "__main__":
    main()
