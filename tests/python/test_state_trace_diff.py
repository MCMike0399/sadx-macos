"""Judge regression cases: distinct nonzero observations, no application execution."""
import copy
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import json

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "state_trace_diff", ROOT / "tools/journey/compare_native_startup.py")
diff = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(diff)


def state(frame=1):
    return {"frame": frame, "game_mode": 4, "game_state": 7, "global_timer": frame,
            "stage": 15, "act": 2, "rings": 23,
            "player": {"action": 3, "status": 9, "character": 0,
                       "rotation": [41, 52, 63], "position": [1.25, 2.5, 3.75],
                       "speed": [4.25, 5.5, 6.75]},
            "camera": {"rotation": [71, 82, 93], "position": [7.25, 8.5, 9.75]},
            "tasks": [{"path": "2/0", "exec": True, "display": False, "destructor": True},
                      {"path": "2/0/0", "exec": True, "display": True, "destructor": False}]}


class StateTraceTests(unittest.TestCase):
    def check_bad(self, a, b, substring, frames=(1,)):
        result = diff.compare_traces(a, b, frames)
        self.assertFalse(result["passed"])
        self.assertTrue(any(substring in p for p in result["problems"]), result)

    def test_exact_and_native_frame_alias(self):
        a, b = state(), state()
        a["frames"] = a.pop("frame")
        self.assertTrue(diff.compare_traces([a], [b], [1])["passed"])

    def test_missing_duplicate_out_of_order_and_extra_frames(self):
        for trace in ([state(1)], [state(1), state(1)],
                      [state(2), state(1)], [state(1), state(2), state(3)]):
            with self.subTest(trace=trace):
                self.check_bad(trace, [state(1), state(2)], "frame sequence", (1, 2))

    def test_invalid_frame(self):
        for value in (None, True, 0, "1"):
            a = state()
            a["frame"] = value
            self.check_bad([a], [state()], "invalid frame")

    def test_missing_required_fields_on_both_sides(self):
        for field in ("game_mode", "game_state", "global_timer", "stage", "act", "rings",
                      "player", "camera", "tasks"):
            a = state()
            del a[field]
            self.check_bad([a], [copy.deepcopy(a)], field)

    def test_modes_independently(self):
        for field in diff.TRACE_FIELDS:
            a = state()
            a[field] += 1
            self.check_bad([a], [state()], field)

    def test_presence_changes_independently_in_both_directions(self):
        for field in ("player", "camera"):
            a = state()
            a[field] = None
            self.check_bad([a], [state()], field + " differs")
            self.check_bad([state()], [a], field + " differs")
        a = state()
        a["tasks"] = []
        self.check_bad([a], [state()], "tasks differs")
        self.check_bad([state()], [a], "tasks differs")

    def test_all_three_differences_reported(self):
        a = state()
        a.update(player=None, camera=None, tasks=[])
        result = diff.compare_traces([a], [state()], [1])
        for field in ("player", "camera", "tasks"):
            self.assertTrue(any(field + " differs" in p for p in result["problems"]))

    def test_changed_values_and_task_order(self):
        for field, subfield in (("player", "position"), ("player", "speed"),
                                ("player", "rotation"), ("camera", "position"),
                                ("camera", "rotation")):
            a = state()
            a[field][subfield][1] += 0.125
            self.check_bad([a], [state()], field)
        a = state()
        a["tasks"].reverse()
        self.check_bad([a], [state()], "tasks differs")
        a = state()
        a["tasks"][0]["exec"] = False
        self.check_bad([a], [state()], "tasks differs")

    def test_unrecovered_is_failure_even_when_equal(self):
        for field in ("player", "camera", "tasks"):
            a = state()
            a[field] = None
            a[field + "_error"] = "unrecovered canonical owner"
            self.check_bad([a], [copy.deepcopy(a)], "unrecovered")

    def test_known_absent_player_camera_can_match(self):
        a = state()
        a.update(player=None, camera=None, tasks=[])
        self.assertTrue(diff.compare_traces([a], [a], [1])["passed"])

    def test_incomplete_nonfinite_or_malformed_state(self):
        for field, subfield in (("player", "speed"), ("camera", "position")):
            a = state()
            del a[field][subfield]
            self.check_bad([a], [copy.deepcopy(a)], subfield)
        for value in (float("nan"), float("inf"), "nan", None, True):
            a = state()
            a["camera"]["position"][0] = value
            self.check_bad([a], [copy.deepcopy(a)], "finite vector")
        a = state()
        a["tasks"][0].pop("display")
        self.check_bad([a], [copy.deepcopy(a)], "tasks.display")
        a = state()
        a["tasks"].append(copy.deepcopy(a["tasks"][0]))
        self.check_bad([a], [copy.deepcopy(a)], "duplicate")

    def test_empty_or_invalid_expected_sequence_rejected(self):
        for frames in ([], [2, 1], [1, 1], [0]):
            with self.assertRaises(ValueError):
                diff.compare_traces([], [], frames)

    def test_duplicate_json_fields_not_silently_overwritten(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.jsonl"
            path.write_text('{"frame":1,"frame":2}\n')
            with self.assertRaisesRegex(ValueError, "duplicate JSON field"):
                diff.load_trace(path)

    def test_old_startup_comparison_remains_exact(self):
        self.assertEqual(len(diff.CHECKPOINTS), 16)
        self.assertEqual(diff.compare(state(), state()), [])
        a = state()
        a["global_timer"] += 1
        self.assertTrue(diff.compare(a, state()))
        self.assertTrue(diff.compare({}, {}))


class InputScriptTests(unittest.TestCase):
    def test_same_normal_keyboard_events_and_frame_translation(self):
        events = diff.compile_input("@1 down enter\n@3 up enter\n@5 press space 2\n", 388)
        self.assertEqual(events, [(1, 13, "down"), (3, 13, "up"),
                                  (5, 32, "down"), (7, 32, "up")])
        self.assertEqual([f"{f+1}:{k}:{a}" for f, k, a in events],
                         ["2:13:down", "4:13:up", "6:32:down", "8:32:up"])

    def test_reject_state_injection_unknown_or_malformed_commands(self):
        for script in ("@1 state x", "@1 mode 4", "@1 pad a", "@1 down",
                       "@0 down enter", "@388 down enter", "@2 up a\n@1 down a",
                       "@1 press a 0", "@1 down 999", "@1 down a extra"):
            with self.subTest(script=script), self.assertRaises(ValueError):
                diff.compile_input(script, 388)


class RunnerTests(unittest.TestCase):
    def run_mocked(self, *, trace=False, bad_image=False, stopped=False):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = root / "run"
            script = root / "input.script"
            script.write_text("@1 down a\n@3 up a\n")
            commands = []

            def fake_run(command, log):
                commands.append(command)
                native = "--capture-at" in command
                destination = output / ("native" if native else "reference")
                if stopped:
                    raise RuntimeError("explicit unrecovered application stop")
                frames = range(1, 389) if trace else diff.CHECKPOINTS
                for frame in frames:
                    record = state(frame)
                    if native:
                        record["frames"] = record.pop("frame")
                    (destination / f"{frame}.json").write_text(json.dumps(record))
                    image = b"exact-image" if not (bad_image and native and frame == 16) else b"changed"
                    (destination / f"{frame}.ppm").write_bytes(image)

            argv = ["compare", "--output", str(output),
                    "--native", __file__, "--reference", __file__]
            if trace:
                argv += ["--script", str(script), "--frames", "388"]
            with patch("sys.argv", argv), patch.object(diff, "run", fake_run):
                result = diff.main()
            report = json.loads((output / "report.json").read_text())
            generated = (output / "reference.script").read_text()
            return result, report, commands, generated

    def test_startup_retains_all_exact_images_and_detects_one_changed_byte(self):
        result, report, _, _ = self.run_mocked(bad_image=True)
        self.assertEqual(result, 1)
        self.assertEqual(len(report["checkpoints"]), 16)
        self.assertEqual([r["frame"] for r in report["checkpoints"] if not r["passed"]], [16])
        self.assertIn("image bytes differ", report["checkpoints"][2]["problems"])

    def test_per_frame_runner_uses_same_events_with_declared_alignment(self):
        result, report, commands, script = self.run_mocked(trace=True)
        self.assertEqual(result, 0)
        self.assertEqual(report["trace"]["frames"], 388)
        self.assertEqual(len(report["checkpoints"]), 16)
        native_events = commands[0][commands[0].index("--key-events") + 1]
        self.assertEqual(native_events, "2:65:down,4:65:up")
        self.assertIn("@1 down 65", script)
        self.assertIn("@3 up 65", script)
        self.assertEqual(sum(" state " in line for line in script.splitlines()), 388)

    def test_application_stop_and_missing_captures_leave_failing_report(self):
        result, report, commands, _ = self.run_mocked(stopped=True)
        self.assertEqual(result, 1)
        self.assertFalse(report["passed"])
        self.assertEqual(len(commands), 2)
        self.assertEqual(len(report["runtime_problems"]), 2)
        self.assertTrue(all(not row["passed"] for row in report["checkpoints"]))

    def test_release_cancels_automatic_press_release(self):
        events = diff.compile_input("@1 press a 10\n@2 release\n@3 down a", 388)
        self.assertNotIn((11, 65, "up"), events)
        self.assertEqual(events[-1], (3, 65, "down"))


if __name__ == "__main__":
    unittest.main()
