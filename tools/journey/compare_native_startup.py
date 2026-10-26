#!/usr/bin/env python3
"""Compare native/reference startup or per-frame traces driven by the same normal input.

No game-state writes, alternate initial modes, image tolerances or stored goldens.
Both applications produce the checkpoints from their own startup during this run.
"""
import argparse
import hashlib
import json
import math
import re
from pathlib import Path
import subprocess

CHECKPOINTS = (1, 2, 16, 31, 32, 120, 153, 180, 194, 195, 220, 300, 346, 370, 387, 388)


def run(command, log):
    with log.open("w") as output:
        result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=600)
    if result.returncode:
        raise RuntimeError(f"application exited {result.returncode}; see {log}")


# Exact fields shared by the existing startup check. Missing is never equal to missing.
STARTUP_FIELDS = ("game_mode", "game_state", "global_timer", "stage", "rings")
TRACE_FIELDS = (*STARTUP_FIELDS, "act")


def compare(native, reference, fields=STARTUP_FIELDS):
    problems = []
    for field in fields:
        if field not in native or field not in reference:
            problems.append(f"{field}: missing required startup field")
        elif type(native[field]) is not int or type(reference[field]) is not int:
            problems.append(f"{field}: expected integer")
        elif native[field] != reference[field]:
            problems.append(f"{field}: native={native[field]} reference={reference[field]}")
    return problems


def frame_number(state):
    # Native's existing public report uses 'frames'; reference uses 'frame'.
    if "frame" in state and "frames" in state and state["frame"] != state["frames"]:
        raise ValueError("conflicting frame and frames")
    value = state.get("frame", state.get("frames"))
    if type(value) is not int or value < 1:
        raise ValueError("missing or invalid frame")
    return value


def validate_state(state):
    problems = compare(state, state, TRACE_FIELDS)
    for field in ("player", "camera", "tasks"):
        if field not in state:
            problems.append(f"{field}: missing required field")
        if field + "_error" in state:
            problems.append(f"{field}: {state[field + '_error']}")
    for field, scalars, vectors in (
        ("player", ("action", "status", "character"), ("rotation", "position", "speed")),
        ("camera", (), ("rotation", "position")),
    ):
        value = state.get(field)
        if value is None:
            continue  # observed absence; an *_error above distinguishes unknown
        if not isinstance(value, dict):
            problems.append(f"{field}: expected object or observed null")
            continue
        for name in scalars:
            if type(value.get(name)) is not int:
                problems.append(f"{field}.{name}: missing or invalid integer")
        for name in vectors:
            vector = value.get(name)
            if (not isinstance(vector, list) or len(vector) != 3 or
                    any(type(v) not in (int, float) or not math.isfinite(v) for v in vector)):
                problems.append(f"{field}.{name}: missing or invalid finite vector")
    tasks = state.get("tasks")
    if not isinstance(tasks, list):
        problems.append("tasks: expected observed task list")
    else:
        paths = set()
        for task in tasks:
            if not isinstance(task, dict):
                problems.append("tasks: expected task object")
                continue
            path = task.get("path")
            if not isinstance(path, str) or not re.fullmatch(r"[0-7](?:/[0-9]+)+", path):
                problems.append("tasks.path: missing or invalid structural path")
            elif path in paths:
                problems.append(f"tasks.path: duplicate {path}")
            else:
                paths.add(path)
            for name in ("exec", "display", "destructor"):
                if type(task.get(name)) is not bool:
                    problems.append(f"tasks.{name}: missing or invalid callback presence")
    return problems


def compare_traces(native, reference, frames):
    """Compare ordered observations; never index away duplicates or hide missing frames."""
    expected = list(frames)
    if not expected or expected != sorted(set(expected)) or any(
            type(f) is not int or f < 1 for f in expected):
        raise ValueError("expected frames must be nonempty, positive, unique and increasing")
    problems = []
    for label, trace in (("native", native), ("reference", reference)):
        observed = []
        for index, state in enumerate(trace):
            if not isinstance(state, dict):
                problems.append(f"{label} record {index}: expected object")
                continue
            try:
                observed.append(frame_number(state))
            except ValueError as error:
                problems.append(f"{label} record {index}: {error}")
            problems.extend(f"{label} record {index}: {p}" for p in validate_state(state))
        if observed != expected:
            problems.append(f"{label} frame sequence: expected {expected}, observed {observed}")
    for index, (a, b) in enumerate(zip(native, reference)):
        if not isinstance(a, dict) or not isinstance(b, dict):
            continue
        problems.extend(f"record {index}: {p}" for p in compare(a, b, TRACE_FIELDS))
        # Independently compare each presence/value, even when another field is absent.
        for field in ("player", "camera", "tasks"):
            if field in a and field in b and a[field] != b[field]:
                problems.append(f"record {index}: {field} differs: native={a[field]} reference={b[field]}")
    return {"passed": not problems, "frames": len(expected), "problems": problems}


def load_trace(path):
    def unique_object(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON field {key}")
            result[key] = value
        return result
    return [json.loads(line, object_pairs_hook=unique_object) for line in path.read_text().splitlines()]


def compile_input(text, last_frame):
    """Normal keyboard input only; reference Present F -> native pre-update F+1."""
    names = {"enter": 13, "return": 13, "esc": 27, "escape": 27, "space": 32,
             "tab": 9, "backspace": 8, "left": 37, "up": 38, "right": 39,
             "down": 40, "shift": 160, "ctrl": 162, "alt": 164,
             "rshift": 161, "rctrl": 163, "ralt": 165}
    events = []
    releases = []
    previous = 0
    for number, line in enumerate(text.splitlines(), 1):
        words = line.split("#", 1)[0].split()
        if not words:
            continue
        if len(words) < 2 or not re.fullmatch(r"@[1-9][0-9]*", words[0]):
            raise ValueError(f"line {number}: expected @FRAME normal input")
        frame = int(words[0][1:])
        if frame < previous or frame >= last_frame:
            raise ValueError(f"line {number}: input frames must increase and precede final capture")
        previous = frame
        due = [event for event in releases if event[0] <= frame]
        events.extend(due)
        releases = [event for event in releases if event[0] > frame]
        verb, args = words[1], words[2:]
        if verb not in ("down", "up", "press", "release"):
            raise ValueError(f"line {number}: unsupported normal input {verb}")
        if verb == "release":
            if args:
                raise ValueError(f"line {number}: release takes no arguments")
            releases.clear()
            events.extend((frame, key, "up") for key in range(1, 256))
            continue
        if len(args) != (2 if verb == "press" and len(args) == 2 else 1):
            raise ValueError(f"line {number}: wrong key argument count")
        token = args[0].lower()
        key = names.get(token)
        if key is None:
            key = ord(token.upper()) if len(token) == 1 and token.isascii() and token.isalnum() else int(token, 0)
        if not 1 <= key <= 255:
            raise ValueError(f"line {number}: invalid Windows virtual key")
        events.append((frame, key, "up" if verb == "up" else "down"))
        if verb == "press":
            duration = int(args[1]) if len(args) == 2 else 5
            if duration < 1:
                raise ValueError(f"line {number}: press duration must be positive")
            releases.append((frame + duration, key, "up"))
    # Stable sorting preserves simultaneous key event ordering in both apps.
    events.extend(releases)
    events.sort(key=lambda event: event[0])
    return events


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", default="build/sadx-native")
    parser.add_argument("--reference", default="build/sadx-app")
    parser.add_argument("--assets", default="game/imported")
    parser.add_argument("--game", default="reference/updated-pc-reference/game")
    parser.add_argument("--windows", default="reference/windows-system")
    parser.add_argument("--output", type=Path, default=Path("build/native-startup-journey"))
    parser.add_argument("--script", type=Path, help="same normal keyboard input for both apps")
    parser.add_argument("--frames", type=int, help="compare every frame 1..N, including gameplay state")
    parser.add_argument("--native-trace", type=Path, help="offline JSONL observations")
    parser.add_argument("--reference-trace", type=Path)
    args = parser.parse_args()
    if bool(args.native_trace) != bool(args.reference_trace):
        parser.error("offline comparison requires both trace paths")
    if args.frames is not None and (args.frames < 1 or
            (not args.native_trace and args.frames < CHECKPOINTS[-1])):
        parser.error("per-frame run must retain all 16 startup checkpoints (N >= 388)")
    if args.script and not args.frames:
        parser.error("--script requires --frames")
    args.output.mkdir(parents=True, exist_ok=True)
    if args.native_trace:
        if not args.frames:
            parser.error("offline traces require --frames")
        report = compare_traces(load_trace(args.native_trace), load_trace(args.reference_trace),
                                range(1, args.frames + 1))
        (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(report))
        return int(not report["passed"])
    final_frame = args.frames or CHECKPOINTS[-1]
    capture_frames = list(range(1, final_frame + 1)) if args.frames else CHECKPOINTS
    input_text = args.script.read_text() if args.script else ""
    events = compile_input(input_text, final_frame)
    (args.output / "input.script").write_text(input_text)
    native = args.output / "native"
    reference = args.output / "reference"
    if any(path.exists() for path in (native, reference, args.output / "overlay", args.output / "native-save")):
        parser.error("use a fresh output directory; capture/overlay paths already exist")
    if any(character.isspace() for character in str(args.output.resolve())):
        parser.error("output path must have no whitespace (reference harness path token contract)")
    native.mkdir(parents=True, exist_ok=True)
    reference.mkdir(parents=True, exist_ok=True)
    commands = [f"@{frame} {action} {key}" for frame, key, action in events
                if frame <= final_frame]
    for frame in capture_frames:
        commands += [f"@{frame} screenshot {reference / f'{frame}.ppm'}",
                     f"@{frame} state {reference / f'{frame}.json'}"]
    commands.sort(key=lambda command: int(command.split()[0][1:]))
    commands.append(f"@{final_frame} quit")
    script = args.output / "reference.script"
    script.write_text("\n".join(commands) + "\n")
    native_command = [args.native, "--assets", args.assets, "--offscreen", "--exit-after",
                      str(final_frame), "--capture-directory", str(native),
                      "--capture-at", ",".join(map(str, capture_frames))]
    native_command += ["--save-directory", str(args.output / "native-save"), "--key-events", ",".join(
        f"{frame + 1}:{key}:{action}" for frame, key, action in events if frame < final_frame)]
    reference_command = [args.reference, "--guest", args.game, args.windows, "--offscreen",
                         "--overlay", str(args.output / "overlay"), "--script", str(script)]
    runtime_problems = []
    for command, log in ((native_command, "native.log"), (reference_command, "reference.log")):
        try:
            run(command, args.output / log)
        except (RuntimeError, subprocess.TimeoutExpired, OSError) as error:
            runtime_problems.append(str(error))
    rows = []
    for frame in CHECKPOINTS:
        try:
            native_state = load_trace(native / f"{frame}.json")[0]
            reference_state = load_trace(reference / f"{frame}.json")[0]
            a = (native / f"{frame}.ppm").read_bytes()
            b = (reference / f"{frame}.ppm").read_bytes()
        except (OSError, ValueError, IndexError) as error:
            rows.append({"frame": frame, "passed": False, "problems": [str(error)]})
            continue
        problems = compare(native_state, reference_state)
        if frame_number(native_state) != frame or frame_number(reference_state) != frame:
            problems.append("capture frame does not match requested frame")
        if a != b:
            problems.append("image bytes differ")
        rows.append({"frame": frame, "passed": not problems, "problems": problems,
                     "native_state": native_state,
                     "native_image_sha256": hashlib.sha256(a).hexdigest(),
                     "reference_image_sha256": hashlib.sha256(b).hexdigest()})
    trace_report = None
    if args.frames:
        traces = []
        for directory in (native, reference):
            trace = []
            for frame in capture_frames:
                try:
                    records = load_trace(directory / f"{frame}.json")
                    if len(records) != 1:
                        raise ValueError("capture must contain exactly one state")
                    trace.extend(records)
                except (OSError, ValueError) as error:
                    runtime_problems.append(f"{directory} frame {frame}: {error}")
            (directory / "trace.jsonl").write_text(
                "".join(json.dumps(state) + "\n" for state in trace))
            traces.append(trace)
        trace_report = compare_traces(*traces, capture_frames)
    report = {"runtime_problems": runtime_problems, "input_script_sha256": hashlib.sha256(input_text.encode()).hexdigest(),
              "input_events": events,
              "input_alignment": "reference Present F -> native pre-update F+1",
              "trace": trace_report, "native_command": native_command, "reference_command": reference_command,
              "native_app_sha256": hashlib.sha256(Path(args.native).read_bytes()).hexdigest(),
              "reference_app_sha256": hashlib.sha256(Path(args.reference).read_bytes()).hexdigest(),
              "checkpoints": rows, "passed": not runtime_problems and all(row["passed"] for row in rows) and
                        (trace_report is None or trace_report["passed"]),
              "limits": ["The 16 exact image/state checkpoints cover Start, logos and transition to Movie.",
                         "Per-frame state validation requires observed player, camera and task fields.",
                         "Task paths cover active list topology and callback presence, not callback identity or work.",
                         "Normal input compiler supports keyboard only; unsupported verbs fail.",
                         "WinMain equivalence, movie/title, native story and gameplay FPS remain open."]}
    (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"checkpoints": len(rows), "passed": report["passed"],
                      "failures": [row["frame"] for row in rows if not row["passed"]],
                      "trace_passed": trace_report["passed"] if trace_report else None,
                      "runtime_problems": runtime_problems}))
    return int(not report["passed"])


if __name__ == "__main__":
    raise SystemExit(main())
