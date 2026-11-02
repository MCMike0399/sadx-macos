"""Discriminating mutations of the shipping queue core, compiled one at a time."""

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
MUTANTS = {
    "input-pending-flag": (
        "primary->orientation.flags |= kInputPending;",
        "primary->orientation.flags |= (kInputPending >> 1);",
    ),
    "unrelated-primary-zero-store": (
        "    const auto event = event_work(context.arena, primary);\n    auto owner = event;",
        "    const auto event = event_work(context.arena, primary);\n"
        "    (void)context.arena.fill(primary, 4, 0, 4);\n    auto owner = event;",
    ),
    "tail-traversal": (
        "while (current) {\n        owner = current;",
        "if (current) {\n        owner = current;",
    ),
    "link-byte": (
        "value.link_frames = static_cast<std::int8_t>(static_cast<std::uint8_t>(link_frames));",
        "value.link_frames = static_cast<std::int8_t>(static_cast<std::uint8_t>(link_frames + 1U));",
    ),
    "action-copy-order": (
        "value.action.object = context.actions.object(action);\n    value.action.motion = context.actions.motion(action);",
        "value.action.motion = context.actions.motion(action);\n    value.action.object = context.actions.object(action);",
    ),
    "initialize-primary-recapture": (
        "context.arena.set_reference(captured, kPrimaryEvent, event)",
        "context.arena.set_reference(primary, kPrimaryEvent, event)",
    ),
    "allocation-failure-publication": (
        "require(context.arena.set_reference(owner, field, allocation),",
        "if (allocation) require(context.arena.set_reference(owner, field, allocation),",
    ),
    "cleanup-head-order": (
        """    auto current = reference(context.arena, event, kActionHead, Boundary::ActionHead);
    while (current) {""",
        """    auto current = reference(context.arena, event, kActionHead, Boundary::ActionHead);
    require(context.arena.set_reference(event, kActionHead, {}), Boundary::ActionHead);
    while (current) {""",
    ),
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe")
    parser.add_argument("--output-dir", default="build/event-actions/mutants")
    args = parser.parse_args()
    out = Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)
    core = (ROOT / "src/player/event_actions.cpp").read_text()
    results = []
    for name, (before, after) in MUTANTS.items():
        assert core.count(before) == 1, (name, "ambiguous mutation")
        mutated = core.replace(before, after)
        if name == "cleanup-head-order":
            # Move the existing store rather than adding an extra store.
            after_loop = """    const auto old = reference(context.arena, event, kActionOld, Boundary::ActionOld);
    require(context.arena.set_reference(event, kActionHead, {}), Boundary::ActionHead);"""
            assert mutated.count(after_loop) == 1
            mutated = mutated.replace(
                after_loop,
                "    const auto old = reference(context.arena, event, kActionOld, Boundary::ActionOld);",
            )
        path = out / (name + ".cpp")
        path.write_text(mutated)
        library = out / (name + ".dylib")
        compile_command = [
            "clang++",
            "-std=c++17",
            "-O2",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            "-dynamiclib",
            str(path),
            "src/engine/task.cpp",
            "src/engine/task_work.cpp",
            "tests/probes/native_event_actions_probe.cpp",
            "-o",
            str(library),
        ]
        compiled = subprocess.run(compile_command, cwd=ROOT, capture_output=True, text=True)
        (out / (name + "-compile.log")).write_text(compiled.stdout + compiled.stderr)
        if compiled.returncode:
            raise RuntimeError(f"{name}: mutant did not compile")
        report = out / (name + ".json")
        run = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools/oracles/verify_original_event_actions.py"),
                args.exe,
                str(library.resolve()),
                "--cases",
                "0",
                "--output",
                str(report),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        (out / (name + "-run.log")).write_text(run.stdout + run.stderr)
        document = json.loads(report.read_text()) if report.exists() else {}
        caught = run.returncode == 1 and bool(document.get("failures"))
        result = {
            "name": name,
            "compiled": True,
            "caught": caught,
            "exit_status": run.returncode,
            "failed_cases": len(document.get("failures", [])),
            "compiled_source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "library_sha256": hashlib.sha256(library.read_bytes()).hexdigest(),
        }
        results.append(result)
        print(json.dumps(result), flush=True)
        if not caught:
            raise RuntimeError(f"{name}: mutant survived or judge crashed")
    (out / "summary.json").write_text(json.dumps(results, indent=2) + "\n")


if __name__ == "__main__":
    main()
