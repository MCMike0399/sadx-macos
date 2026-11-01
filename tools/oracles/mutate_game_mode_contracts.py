#!/usr/bin/env python3
"""Compile bounded copied faults against mode forwarding and diagnostic contracts."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


NATIVE = "src/game/game_mode_native.cpp"
CORE = "src/game/game_mode.cpp"
REFERENCE = "src/guest/game_mode_reference.cpp"
FAULTS = [
    ("swapped-frame-operands", NATIVE, "flow.native->set_frame_rate_mode(mode, multiplier);",
     "flow.native->set_frame_rate_mode(multiplier, mode);", "native"),
    ("lost-movie-identity", NATIVE, "flow.native->run_movie(movie)",
     "flow.native->run_movie(movie & 0U)", "native"),
    ("wrong-cleanup-owner", NATIVE, "flow.native->clear_game();",
     "flow.native->reset_game();", "native"),
    ("lost-destructor-kind", NATIVE, "flow.native->run_level_destructor(kind);",
     "flow.native->run_level_destructor(kind + 1U);", "native"),
    ("swallowed-callback-failure", NATIVE, "flow.native->clear_game();",
     "try { flow.native->clear_game(); } catch (...) {}", "native"),
    ("reordered-destructors", CORE, "{4U, 0U, 5U, 7U, 6U, 8U, 9U}",
     "{4U, 0U, 5U, 6U, 7U, 8U, 9U}", "native"),
    ("early-super-sonic-store", CORE,
     "set_wait_vsync_count(flow, 1);\n    write_mode(flow, next_mode(read_mode(flow)));\n    write_playing_super_sonic(flow, 0);",
     "write_playing_super_sonic(flow, 0);\n    set_wait_vsync_count(flow, 1);\n    write_mode(flow, next_mode(read_mode(flow)));", "native"),
    ("cached-mode-before-callback", CORE,
     "set_wait_vsync_count(flow, 1);\n    write_mode(flow, next_mode(read_mode(flow)));",
     "const auto cached_mode = read_mode(flow);\n    set_wait_vsync_count(flow, 1);\n    write_mode(flow, next_mode(cached_mode));", "native"),
    ("lost-nested-boundary", REFERENCE,
     "result.nested_source_boundary = receipt.owner.source_boundary;",
     "result.nested_source_boundary = 0;", "reference"),
    ("lost-pending-payload", REFERENCE,
     "result.pending_call = GameModeCall{match->site, callee, args};",
     "result.pending_call = GameModeCall{match->site, callee, {}};\n    (void)args;", "reference"),
]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("Use a new directory; prior fault receipts are retained")
    args.output.mkdir(parents=True)
    units = {"native": "tests/unit/game_mode_forwarding.cpp",
             "reference": "tests/unit/game_mode_diagnostics.cpp"}
    frozen_units = {kind: Path(path).read_bytes() for kind, path in units.items()}
    frozen = json.loads(Path("docs/refactor-inventory/packets/mode-forwarding/source-freeze.json").read_text())
    sources = {}
    for path in (NATIVE, CORE, REFERENCE):
        data = subprocess.check_output(["git", "show", frozen["base"] + ":" + path])
        if sha(data) != frozen["files"][path] or Path(path).read_bytes() != data:
            raise RuntimeError("Immutable source anchor changed: " + path)
        sources[path] = data.decode()
    results = []
    for name, path, old, new, kind in FAULTS:
        if sources[path].count(old) != 1:
            raise RuntimeError("Nonunique fault anchor: " + name)
        directory = args.output / name
        directory.mkdir()
        changed = directory / Path(path).name
        changed.write_text(sources[path].replace(old, new))
        unit = units[kind]
        copied_unit = directory / "contract.cpp"
        copied_unit.write_bytes(frozen_units[kind])
        pair = (CORE, NATIVE) if kind == "native" else (CORE, REFERENCE)
        binary = directory / "contract"
        command = ["xcrun", "clang++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
                   "-ffp-contract=off", "-fno-fast-math", "-Isrc", str(copied_unit),
                   *[str(changed) if p == path else p for p in pair], "-o", str(binary)]
        compiled = subprocess.run(command, capture_output=True)
        (directory / "compile.log").write_bytes(compiled.stdout + compiled.stderr)
        executed = subprocess.run([str(binary)], capture_output=True) if compiled.returncode == 0 else None
        if executed:
            (directory / "run.log").write_bytes(executed.stdout + executed.stderr)
        results.append({"fault": name, "source": path, "unit": unit, "command": command,
                        "unit_sha256": sha(frozen_units[kind]),
                        "copied_source_sha256": sha(changed.read_bytes()),
                        "compile_exit": compiled.returncode,
                        "run_exit": executed.returncode if executed else None,
                        "caught": bool(executed and executed.returncode != 0)})
        (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return int(any(row["compile_exit"] != 0 or not row["caught"] for row in results))


if __name__ == "__main__":
    raise SystemExit(main())
