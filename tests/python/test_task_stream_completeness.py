"""Task ABI comparison must prove capture completeness before equality."""
import base64
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / 'tools/oracles/task_judges/compare_streams.py'
SLUG = 'task_work'


def record(index):
    value = base64.b64encode(bytes([index])).decode()
    return dict(call=index, symbol='sadx_native_task_work', result=0,
                scalar_arguments=[None], before={'0': value}, after={'0': value})


def ledger(rows):
    result = []
    for row in rows:
        item = {key: row[key] for key in ('call', 'symbol', 'result')}
        for phase in ('before', 'after'):
            item[phase] = {}
            for key, encoded in row[phase].items():
                data = base64.b64decode(encoded)
                item[phase][key] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
        result.append(item)
    return result


class TaskStreamCompleteness(unittest.TestCase):
    def compare(self, rows, calls=None, identity=True, compressed_digest=None,
                candidate_rows=None, candidate_exit=0, candidate_exclusions=None):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for world in ('old', 'current'):
                captured = (candidate_rows if world == 'current' and candidate_rows is not None
                            else rows)
                folder = root / world
                folder.mkdir()
                stream = folder / (SLUG + '.native.jsonl.gz')
                with gzip.open(stream, 'wt') as sink:
                    for row in captured:
                        sink.write(json.dumps(row) + '\n')
                report = ({'source_unsupported': candidate_exclusions}
                          if world == 'current' and candidate_exclusions is not None else {})
                (folder / (SLUG + '.json')).write_text(json.dumps(report))
                if identity:
                    receipt = dict(slug=SLUG, oracle_exit=candidate_exit if world == 'current' else 0,
                                   calls=ledger(captured) if calls is None else calls,
                                   compressed_sha256=compressed_digest or hashlib.sha256(stream.read_bytes()).hexdigest())
                    (folder / (SLUG + '.identity.json')).write_text(json.dumps(receipt))
            result = subprocess.run([sys.executable, str(SCRIPT), str(root / 'old'),
                                     str(root / 'current'), '--slug', SLUG,
                                     '--output', str(root / 'result.json')],
                                    capture_output=True, text=True)
            return result.returncode, result.stdout + result.stderr

    def test_complete_capture_passes(self):
        self.assertEqual(self.compare([record(0), record(1)])[0], 0)

    def test_empty_capture_rejected_even_with_empty_ledger(self):
        self.assertNotEqual(self.compare([])[0], 0)

    def test_both_truncated_captures_rejected(self):
        self.assertNotEqual(self.compare([record(0)], ledger([record(0), record(1)]))[0], 0)

    def test_duplicate_records_rejected(self):
        self.assertNotEqual(self.compare([record(0), record(0)])[0], 0)

    def test_reordered_records_rejected(self):
        self.assertNotEqual(self.compare([record(1), record(0)])[0], 0)

    def test_missing_identity_rejected(self):
        self.assertNotEqual(self.compare([record(0)], identity=False)[0], 0)

    def test_changed_payload_rejected_against_ledger(self):
        rows = [record(0)]
        calls = ledger(rows)
        rows[0]['after']['0'] = base64.b64encode(b'changed').decode()
        self.assertNotEqual(self.compare(rows, calls)[0], 0)

    def test_changed_return_rejected_against_ledger(self):
        rows = [record(0)]
        calls = ledger(rows)
        rows[0]['result'] = 1
        self.assertNotEqual(self.compare(rows, calls)[0], 0)

    def test_wrong_compressed_identity_rejected(self):
        self.assertNotEqual(self.compare([record(0)], compressed_digest='0' * 64)[0], 0)

    def test_failed_candidate_receipt_rejected(self):
        self.assertNotEqual(self.compare([record(0)], candidate_exit=1)[0], 0)

    def test_one_sided_truncation_rejected(self):
        rows = [record(0), record(1)]
        self.assertNotEqual(self.compare(rows, ledger(rows), candidate_rows=rows[:1])[0], 0)

    def test_valid_candidate_scalar_difference_rejected(self):
        candidate = record(0)
        candidate['scalar_arguments'] = [1]
        self.assertNotEqual(self.compare([record(0)], candidate_rows=[candidate])[0], 0)

    def test_valid_candidate_payload_difference_rejected(self):
        candidate = record(0)
        candidate['after']['0'] = base64.b64encode(b'different').decode()
        self.assertNotEqual(self.compare([record(0)], candidate_rows=[candidate])[0], 0)

    def test_valid_candidate_return_difference_rejected(self):
        candidate = record(0)
        candidate['result'] = 1
        self.assertNotEqual(self.compare([record(0)], candidate_rows=[candidate])[0], 0)

    def test_unequal_exclusion_cohorts_rejected(self):
        self.assertNotEqual(self.compare([record(0)],
                                        candidate_exclusions=[{'case': 0, 'reason': 'boundary'}])[0], 0)


if __name__ == '__main__':
    unittest.main()
