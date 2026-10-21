"""Gate records preserve native checkpoint evidence separately from reference rows."""
import json
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]


class NativeGateReporting(unittest.TestCase):
    def test_recorder_keeps_actual_native_success_and_failure(self):
        checkpoints = [
            {"journey": "native-main-menu", "exit_status": 0,
             "result": {"passed": True, "execution": "native", "frames": 1850}},
            {"journey": "native-fresh-save", "exit_status": 1}]
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            log = root / "gate.log"
            log.write_text("R4  world-3d pass source fixture\n" +
                           "".join("NATIVE_JOURNEY " + json.dumps(item) + "\n"
                                   for item in checkpoints))
            output = root / "gate.json"
            result = subprocess.run([sys.executable, str(ROOT / "tools/gate/record_gate.py"),
                                     "--root", str(root), "--log", str(log),
                                     "--command", "make gate", "--exit-status", "2",
                                     "--output", str(output)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            recorded = json.loads(output.read_text())
            self.assertFalse(recorded["passed"])
            self.assertEqual(recorded["native_journeys"], checkpoints)
            self.assertEqual(recorded["journeys"], [
                {"row": "R4", "name": "world-3d", "verdict": "pass"}])

    def test_reference_runs_use_distinct_empty_overlays(self):
        path = ROOT / "tools/gate/run_journeys.py"
        spec = importlib.util.spec_from_file_location("reference_journeys", path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "journeys"
            overlays = []

            def launch(command, **kwargs):
                overlay = Path(command[command.index("--overlay") + 1])
                self.assertTrue(overlay.is_dir())
                self.assertEqual(list(overlay.iterdir()), [])
                overlays.append(overlay)
                (overlay / "prior-save.snc").write_bytes(b"retained evidence")
                (output / "sega-logo.state.json").write_text(json.dumps({"frame": 120}))
                return subprocess.CompletedProcess(command, 0)

            def judged(journey, *args):
                return {"name": journey["name"], "row": "R1", "state": "required",
                        "passed": True, "problems": []}

            with patch.object(module.subprocess, "run", side_effect=launch), \
                    patch.object(module, "check_checkpoint", side_effect=judged):
                for _ in range(2):
                    self.assertEqual(module.run(module.JOURNEYS[:1], "app", temporary,
                                                temporary, output), 0)
            self.assertNotEqual(overlays[0], overlays[1])
            self.assertTrue((overlays[0] / "prior-save.snc").is_file())
            self.assertEqual(json.loads((output / "report.json").read_text())["overlay"],
                             str(overlays[1]))


if __name__ == "__main__":
    unittest.main()
