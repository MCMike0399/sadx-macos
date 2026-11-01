"""A reviewed scope must survive projection without inventing or retaining stale closure."""
import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch
import json

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


class ModuleReviewProjection(unittest.TestCase):
    def setUp(self):
        self.members = [dict(path='src/game/example.hpp', sha256='a' * 64,
                             disposition='already_conforming', review_status='reviewed_conforming'),
                        dict(path='src/game/example.cpp', sha256='b' * 64,
                             disposition='already_conforming', review_status='reviewed_conforming')]
        self.review = dict(disposition='already_conforming', reason='Whole-module review.',
                           reviewed_members={m['path']: m['sha256'] for m in self.members},
                           evidence=[dict(path='docs/review.md')],
                           semantic_justification='All implementation and interface inspected.',
                           resolved_owners=['Canonical borrowed progress'],
                           resolved_consumers=['Actual native and reference callers'],
                           accepted_judges=['Unchanged original comparison'],
                           classification_limits=['No full-system runtime claim.'])

    def test_conforming_files_do_not_infer_module_approval(self):
        result = inventory.reviewed_module({}, self.members)
        self.assertEqual(result['disposition'], 'open_review')
        self.assertEqual(result['status'], 'module_closure_open')

    def test_explicit_review_preserves_closure_and_limits(self):
        result = inventory.reviewed_module(self.review, self.members)
        self.assertEqual(result['status'], 'reviewed_conforming')
        self.assertEqual(result['reviewed_members'], self.review['reviewed_members'])
        self.assertEqual(result['limits'], self.review['classification_limits'])

    def test_changed_added_or_renamed_members_cannot_inherit_approval(self):
        changed = dict(self.members[0], sha256='c' * 64)
        renamed = dict(self.members[0], path='src/game/example.h')
        added = dict(path='src/game/example.mm', sha256='d' * 64,
                     disposition='already_conforming', review_status='reviewed_conforming')
        for members in [[changed, self.members[1]], [renamed, self.members[1]], self.members + [added]]:
            with self.subTest(members=members), self.assertRaisesRegex(ValueError, 'members changed'):
                inventory.reviewed_module(self.review, members)

    def test_unreviewed_member_or_missing_evidence_prevents_closure(self):
        members = [dict(self.members[0], review_status='open_review'), self.members[1]]
        with self.assertRaisesRegex(ValueError, 'every member'):
            inventory.reviewed_module(self.review, members)
        with self.assertRaisesRegex(ValueError, 'rationale and evidence'):
            inventory.reviewed_module(dict(self.review, evidence=[]), self.members)
        with self.assertRaisesRegex(ValueError, 'judge evidence'):
            inventory.reviewed_module(dict(self.review, accepted_judges=[]), self.members)

    def test_purpose_classification_also_requires_exact_members(self):
        review = dict(self.review, disposition='reference_infrastructure')
        members = [dict(m, disposition='reference_infrastructure') for m in self.members]
        inventory.reviewed_module(review, members)
        with self.assertRaisesRegex(ValueError, 'members changed'):
            inventory.reviewed_module(review, members[:1])
        with self.assertRaisesRegex(ValueError, 'every member disposition'):
            inventory.reviewed_module(review, self.members)

    def test_missing_empty_or_blank_limits_cannot_certify_module(self):
        for limits in [None, [], '', [''], [' ']]:
            with self.subTest(limits=limits), self.assertRaisesRegex(ValueError, 'classification limits'):
                inventory.reviewed_module(dict(self.review, classification_limits=limits), self.members)
        review = dict(self.review)
        del review['classification_limits']
        with self.assertRaisesRegex(ValueError, 'classification limits'):
            inventory.reviewed_module(review, self.members)

    def test_obsolescence_proof_survives_projection_and_link_validation(self):
        proof = dict(reason='No maintained consumers or behavior remain.',
                     evidence=[dict(path='docs/review.md')])
        review = dict(self.review, disposition='obsolete', obsolescence_proof=proof)
        members = [dict(m, disposition='obsolete') for m in self.members]
        result = inventory.reviewed_module(review, members)
        self.assertEqual(result['obsolescence_proof'], proof)
        self.assertEqual(result['status'], 'module_closure_open')
        inventory.validate_module_evidence(result, {'docs/review.md'})
        with self.assertRaisesRegex(ValueError, 'Missing module review evidence'):
            inventory.validate_module_evidence(result, set())

    def test_render_rejects_missing_nested_owner_consumer_and_judge_evidence(self):
        policy_path = inventory.OUT / 'review.json'
        policy = json.loads(policy_path.read_text())
        rendered = inventory.render()
        records = [json.loads(line) for name, text in rendered.items()
                   if name.startswith('files/') for line in text.splitlines()]
        name = 'src/platform/mac/mac_save_writer'
        members = [record for record in records if record['module'] == name]
        self.assertEqual(len(members), 2)
        review = dict(self.review, reviewed_members={m['path']: m['sha256'] for m in members},
                      evidence=[dict(path='docs/refactor-inventory/MODULE-REVIEW.md')])
        original_read = Path.read_text
        for field in ['resolved_owners', 'resolved_consumers', 'accepted_judges']:
            bad = dict(review)
            bad[field] = [dict(evidence=[dict(path='docs/DOES-NOT-EXIST.md')])]
            replacement = dict(policy, modules={name: bad})

            def read(path, *args, **kwargs):
                return json.dumps(replacement) if path == policy_path else original_read(path, *args, **kwargs)

            with self.subTest(field=field), patch.object(Path, 'read_text', read):
                with self.assertRaisesRegex(ValueError, 'Missing module review evidence'):
                    inventory.render()


if __name__ == '__main__':
    unittest.main()
