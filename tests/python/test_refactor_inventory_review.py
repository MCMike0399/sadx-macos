"""A reviewed scope must survive projection without inventing or retaining stale closure."""
import importlib.util
from pathlib import Path
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / 'tools/inventory/refactor_inventory.py'
SPEC = importlib.util.spec_from_file_location('refactor_inventory', SCRIPT)
inventory = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(inventory)


class ReviewProjection(unittest.TestCase):
    def test_open_static_scope_never_becomes_conforming(self):
        result = inventory.reviewed_scope({}, 'a' * 64)
        self.assertEqual(result['status'], 'closure_open')
        self.assertEqual(result['resolved_owners'], [])

    def test_complete_authored_review_preserves_identity_and_limits(self):
        review = {'disposition': 'already_conforming', 'reviewed_source_sha256': 'a' * 64,
                  'semantic_justification': 'Structured source predicate with no fixture state.',
                  'resolved_owners': ['Borrows canonical progress; owns no persistent data.'],
                  'resolved_consumers': ['Real menu caller'], 'accepted_judges': ['Frozen original predicate oracle'],
                  'classification_limits': ['No new runtime/whole-campaign qualification.']}
        result = inventory.reviewed_scope(review, 'a' * 64)
        self.assertEqual(result['status'], 'reviewed_conforming')
        self.assertEqual(result['resolved_consumers'], review['resolved_consumers'])
        self.assertEqual(result['limits'], review['classification_limits'])
        with self.assertRaisesRegex(ValueError, 'source changed'):
            inventory.reviewed_scope(review, 'b' * 64)
        del review['accepted_judges']
        with self.assertRaisesRegex(ValueError, 'judge evidence'):
            inventory.reviewed_scope(review, 'a' * 64)


if __name__ == '__main__':
    unittest.main()
