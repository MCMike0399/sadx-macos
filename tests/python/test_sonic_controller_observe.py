"""Negative adequacy checks for the new complete-output preservation checker."""
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[2] / "tools/oracles/sonic_controller/observe.py"
SPEC = importlib.util.spec_from_file_location("sonic_observe", SOURCE)
observe = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(observe)


class ObservationTests(unittest.TestCase):
    def capture(self, folder, rows, count=None):
        raw = b"".join((json.dumps(row) + "\n").encode() for row in rows)
        encoded = gzip.compress(raw, mtime=0)
        (folder / "cases.jsonl.gz").write_bytes(encoded)
        (folder / "capture.json").write_text(json.dumps(dict(
            compressed_sha256=hashlib.sha256(encoded).hexdigest(),
            full_stream_sha256=hashlib.sha256(raw).hexdigest(), raw_bytes=len(raw),
            records=len(rows) if count is None else count)))

    def row(self, case=0):
        return dict(case=case, function="sadx_native_sonic_player", source={}, native_return=0,
                    arguments=[[0] * 52, 52, [0] * 36, [0] * 40])

    def test_empty_truncated_duplicate_and_reordered_are_rejected(self):
        for rows, count in [([], 0), ([self.row()], 2),
                            ([self.row(), self.row()], 2),
                            ([self.row(1), self.row()], 2)]:
            with self.subTest(rows=rows), tempfile.TemporaryDirectory() as temp:
                folder = Path(temp)
                self.capture(folder, rows, count)
                with self.assertRaises(ValueError):
                    observe.validate(folder)

    def test_baseline_anchor_cannot_be_replaced_by_a_self_consistent_receipt(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            self.capture(folder, [self.row()])
            expected = json.loads((folder / "capture.json").read_text())
            self.capture(folder, [self.row(1)])
            with self.assertRaises(ValueError):
                observe.validate(folder, expected)

    def test_missing_tail_word_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            row = self.row()
            row["arguments"][3].pop()
            self.capture(folder, [row])
            with self.assertRaises(ValueError):
                observe.validate(folder)

    def test_every_word_and_return_are_compared(self):
        for field in range(40):
            after = [0] * 40
            after[field] = 123
            self.assertEqual(observe.differences(0, [0] * 40, 0, after)["words"],
                             [[field, 0, 123]])
        self.assertEqual(observe.differences(0, [0], 1, [0])["returned"], [0, 1])
        with self.assertRaises(ValueError):
            observe.differences(0, [0, 1], 0, [0])

    def test_correct_capture_validates_and_hash_drift_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            self.capture(folder, [self.row()])
            self.assertEqual(len(observe.validate(folder)[1]), 1)
            path = folder / "cases.jsonl.gz"
            path.write_bytes(path.read_bytes() + b"changed")
            with self.assertRaises(ValueError):
                observe.validate(folder)


if __name__ == "__main__":
    unittest.main()
