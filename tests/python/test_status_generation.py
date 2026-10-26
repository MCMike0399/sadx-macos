"""Generated status must describe recorded evidence and actual source presence."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/gate'))
import gen_status


class StatusGenerationTests(unittest.TestCase):
    def test_duplicate_smoke_row_preserves_failure(self):
        passed = {'row': 'R1', 'name': 'sega-logo', 'verdict': 'pass'}
        failed = dict(passed, verdict='FAIL')
        for rows in ([passed, passed], [failed, passed], [passed, failed]):
            with self.subTest(rows=rows):
                result = gen_status.distinct_journeys(rows)
                self.assertEqual(len(result), 1)
                self.assertEqual(result[0]['verdict'], 'FAIL' if failed in rows else 'pass')

    def test_native_frontier_requires_certified_result(self):
        rows = [{'journey': 'native-file-select', 'exit_status': 0,
                 'result': {'passed': True}},
                {'journey': 'native-character-select', 'exit_status': 0,
                 'result': {'passed': False}},
                {'journey': 'native-story', 'exit_status': 2,
                 'result': {'passed': True}}]
        description = gen_status.native_evidence({'native_journeys': rows})
        self.assertIn('native-file-select', description)
        self.assertNotIn('native-character-select', description)
        self.assertNotIn('native-story', description)
        self.assertIn('No native checkpoints', gen_status.native_evidence({}))

    def test_generator_reads_current_override_and_inventory_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ('docs', 'src/guest', 'reference/inventory'):
                (root / name).mkdir(parents=True)
            (root / 'src/guest/override_table.cpp').write_text('// existing implementation\n')
            (root / 'reference/inventory/functions.json').write_text(json.dumps({'procedures': 123}))
            with patch.object(gen_status, 'ROOT', root):
                self.assertEqual(gen_status.main(), 0)
            status = (root / 'docs/STATUS.md').read_text()
            self.assertIn('Functions inventoried: 123', status)
            self.assertIn('src/guest/override_table.cpp', status)
            self.assertNotIn('not built yet', status)
            self.assertNotIn('Current milestone: **M1', status)
            self.assertIn('No native checkpoints certified', status)


if __name__ == '__main__':
    unittest.main()
