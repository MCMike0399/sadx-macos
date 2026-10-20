#!/usr/bin/env python3
"""Compare normal native Start-to-logo updates with the pinned reference application.

No game-state writes, alternate initial modes, image tolerances or stored goldens.
Both applications produce the checkpoints from their own startup during this run.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

CHECKPOINTS = (1, 2, 16, 31, 32, 120, 153, 180, 194, 195, 220, 300, 346, 370, 387, 388)


def run(command, log):
    with log.open("w") as output:
        result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=600)
    if result.returncode:
        raise RuntimeError(f"application exited {result.returncode}; see {log}")


def compare(native, reference):
    problems = []
    for field in ("game_mode", "game_state", "global_timer", "stage", "rings"):
        if native.get(field) != reference.get(field):
            problems.append(f"{field}: native={native.get(field)} reference={reference.get(field)}")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", default="build/sadx-native")
    parser.add_argument("--reference", default="build/sadx-app")
    parser.add_argument("--assets", default="game/imported")
    parser.add_argument("--game", default="reference/updated-pc-reference/game")
    parser.add_argument("--windows", default="reference/windows-system")
    parser.add_argument("--output", type=Path, default=Path("build/native-startup-journey"))
    args = parser.parse_args()
    native = args.output / "native"
    reference = args.output / "reference"
    native.mkdir(parents=True, exist_ok=True)
    reference.mkdir(parents=True, exist_ok=True)
    commands = []
    for frame in CHECKPOINTS:
        commands += [f"@{frame} screenshot {reference / f'{frame}.ppm'}",
                     f"@{frame} state {reference / f'{frame}.json'}"]
    commands.append(f"@{CHECKPOINTS[-1]} quit")
    script = args.output / "reference.script"
    script.write_text("\n".join(commands) + "\n")
    native_command = [args.native, "--assets", args.assets, "--offscreen", "--exit-after",
                      str(CHECKPOINTS[-1]), "--capture-directory", str(native),
                      "--capture-at", ",".join(map(str, CHECKPOINTS))]
    reference_command = [args.reference, "--guest", args.game, args.windows, "--offscreen",
                         "--overlay", str(args.output / "overlay"), "--script", str(script)]
    run(native_command, args.output / "native.log")
    run(reference_command, args.output / "reference.log")
    rows = []
    for frame in CHECKPOINTS:
        native_state = json.loads((native / f"{frame}.json").read_text())
        reference_state = json.loads((reference / f"{frame}.json").read_text())
        a = (native / f"{frame}.ppm").read_bytes()
        b = (reference / f"{frame}.ppm").read_bytes()
        problems = compare(native_state, reference_state)
        if a != b:
            problems.append("image bytes differ")
        rows.append({"frame": frame, "passed": not problems, "problems": problems,
                     "native_state": native_state,
                     "native_image_sha256": hashlib.sha256(a).hexdigest(),
                     "reference_image_sha256": hashlib.sha256(b).hexdigest()})
    report = {"native_command": native_command, "reference_command": reference_command,
              "native_app_sha256": hashlib.sha256(Path(args.native).read_bytes()).hexdigest(),
              "reference_app_sha256": hashlib.sha256(Path(args.reference).read_bytes()).hexdigest(),
              "checkpoints": rows, "passed": all(row["passed"] for row in rows),
              "limits": ["Covers Start, both logos/fades, and transition to Movie only.",
                         "WinMain equivalence, movie/title, native story and gameplay FPS remain open."]}
    (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"checkpoints": len(rows), "passed": report["passed"],
                      "failures": [row["frame"] for row in rows if not row["passed"]]}))
    return int(not report["passed"])


if __name__ == "__main__":
    raise SystemExit(main())
