"""R1–R3 from c4242a1, plus positive history/import continuity in Git and archives."""
import copy
import json
import shutil
import subprocess
import unittest

import test_refactor_inventory_lineage as fixture

inv = fixture.inv


class CommittedLineageReproductions(unittest.TestCase):
    def setUp(self):
        self.case = fixture.LineageFixture()
        self.case.setUp()
        self.addCleanup(self.case.doCleanups)

    def archive(self, receipt=None):
        c = self.case
        receipt = receipt or c.receipt
        # Explicit trusted export fingerprint, including when testing a forged successor.
        c.put_json('docs/refactor-inventory/summary.json', {
            'lineage_receipt': {'sha256': inv.sha256((c.root / c.policy['source_lineage_receipt']).read_bytes())}})
        if not (c.root / 'docs/refactor-inventory/boundaries.jsonl').exists():
            c.put('docs/refactor-inventory/boundaries.jsonl', '')
        shutil.rmtree(c.root / '.git')

    def chain(self):
        c = self.case
        later = 'src/player/later.hpp'
        c.run_git('mv', c.new, later)
        moved = c.commit()
        c.put('docs/later.txt', c.new_bytes.decode())
        second = copy.deepcopy(c.edge)
        second['before'] = c.edge['replacements'][0]
        second['member']['module'] = inv.module(c.new)
        second['member']['remaining_debt'] = ['Preserve this specific intermediate consumer obligation.']
        second['replacements'] = [c.snapshot(later, c.new_bytes, moved, 'docs/later.txt')]
        c.seal([c.edge, second])
        self.assertEqual(json.loads(c.render()['summary.json'])['historical_retained_scope'], 2)
        return second

    def test_r1_committed_reseal_cannot_squash_history(self):
        second = self.chain()
        c = self.case
        direct = copy.deepcopy(c.edge)
        direct['replacements'] = second['replacements']
        c.seal([direct])
        with self.assertRaisesRegex(ValueError, 'Prior accepted historical'):
            c.render()

    def test_r1_archive_cannot_squash_history_even_with_new_export_fingerprint(self):
        second = self.chain()
        c = self.case
        direct = copy.deepcopy(c.edge)
        direct['replacements'] = second['replacements']
        c.seal([direct])
        self.archive()
        with self.assertRaisesRegex(ValueError, 'Prior accepted historical'):
            c.render()

    def test_r1_reseal_cannot_rewrite_counts_disposition_or_obligations(self):
        c = self.case
        c.edge['member'].update(prior_disposition='already_conforming', observed_counts={},
                                remaining_debt=['Unrelated generic limit.'])
        # Even a new matching origin document and accepted review cannot erase prior acceptance.
        name = 'docs/replacement-member.json'
        c.put_json(name, dict(historical_path=c.old, source_sha256=inv.sha256(c.old_bytes), member=c.edge['member']))
        c.origins[c.old] = c.snapshot(name, (c.root / name).read_bytes(), c.commit(), name)
        c.seal([c.edge])
        with self.assertRaisesRegex(ValueError, 'Prior accepted historical'):
            c.render()
        self.archive()
        with self.assertRaisesRegex(ValueError, 'Prior accepted historical'):
            c.render()

    def test_r1_initial_metadata_is_anchored_beyond_enum_shape(self):
        c = self.case
        c.edge['member']['observed_counts'] = {}
        c.seal([c.edge])
        with self.assertRaisesRegex(ValueError, 'anchored prior review metadata'):
            c.render()

    def test_r1_append_only_chain_verifies_all_receipts_without_git(self):
        self.chain()
        c = self.case
        outputs = c.render()
        self.archive()
        self.assertEqual(c.render(), outputs)
        previous = c.receipt['previous']
        (c.root / previous['ledger']['path']).write_text('{}')
        with self.assertRaisesRegex(ValueError, 'Stale lineage evidence'):
            c.render()

    def test_r1_deleted_receipt_cannot_reset_history_to_genesis(self):
        c = self.case
        c.run_git('rm', c.policy['source_lineage_receipt'])
        c.commit()
        c.seal([c.edge])
        self.assertIsNone(c.previous)  # Adversarial fixture tries genesis after deletion.
        with self.assertRaisesRegex(ValueError, 'Missing prior accepted'):
            c.render()

    def test_r1_changed_receipt_path_cannot_reset_history(self):
        c = self.case
        c.policy['source_lineage_receipt'] = 'docs/refactor-inventory/other-receipt.json'
        c.put_json('docs/refactor-inventory/review.json', c.policy)
        c.commit()
        c.receipt['freeze_commit'] = c.run_git('rev-parse', 'HEAD')
        c.receipt['archive_paths'].append(c.policy['source_lineage_receipt'])
        c.put_json(c.policy['source_lineage_receipt'], c.receipt)
        c.commit()
        with self.assertRaisesRegex(ValueError, 'receipt path cannot be replaced'):
            c.render()

    def test_r1_missing_predecessor_file_fails_preparation_and_consumption(self):
        c = self.case
        self.chain()
        name = c.receipt['previous']['receipt']['path']
        c.run_git('rm', name)
        c.commit()
        with self.assertRaisesRegex(ValueError, 'untracked lineage evidence'):
            c.render()
        with self.assertRaisesRegex(ValueError, 'untracked lineage evidence'):
            inv.prepare_lineage_receipt(c.policy['source_lineage_receipt'], 'docs/lineage.json')

    def test_r2_working_tree_acceptance_cannot_override_committed_pending(self):
        c = self.case
        accepted = (c.root / 'docs/review-0.json').read_bytes()
        pending = json.loads(accepted)
        pending['verdict'] = 'pending'
        c.put_json('docs/review-0.json', pending)
        c.receipt['freeze_commit'] = c.commit()
        c.put_json(c.policy['source_lineage_receipt'], c.receipt)
        c.commit()
        (c.root / 'docs/review-0.json').write_bytes(accepted)
        for action in [c.render, lambda: inv.prepare_lineage_receipt(c.policy['source_lineage_receipt'], 'docs/lineage.json')]:
            with self.assertRaisesRegex(ValueError, 'committed freeze snapshot'):
                action()

    def test_r2_every_anchor_kind_is_frozen_at_prepare_and_consume(self):
        c = self.case
        head = c.run_git('rev-parse', 'HEAD')
        anchors = [c.edge['consumer_evidence'][0], c.edge['behavior_evidence'][0],
                   c.edge['before']['proof'], c.edge['replacements'][0]['proof'], c.edge['prior_member']['proof']]
        for anchor in anchors:
            with self.subTest(path=anchor['path']):
                c.run_git('reset', '--hard', head)
                c.receipt = json.loads(c.run_git('show', 'HEAD:' + c.policy['source_lineage_receipt']))
                accepted = (c.root / anchor['path']).read_bytes()
                c.put(anchor['path'], 'Committed bytes contradict the accepted proof.\n')
                c.receipt['freeze_commit'] = c.commit()
                c.put_json(c.policy['source_lineage_receipt'], c.receipt)
                c.commit()
                (c.root / anchor['path']).write_bytes(accepted)
                for action in [c.render, lambda: inv.prepare_lineage_receipt(c.policy['source_lineage_receipt'], 'docs/lineage.json')]:
                    with self.assertRaisesRegex(ValueError, 'committed freeze snapshot'):
                        action()

    def test_r2_committed_symlink_is_not_an_immutable_regular_proof(self):
        c = self.case
        path = c.root / 'docs/review-0.json'
        accepted = path.read_bytes()
        path.unlink()
        path.symlink_to(accepted.decode())
        c.receipt['freeze_commit'] = c.commit()
        c.put_json(c.policy['source_lineage_receipt'], c.receipt)
        c.commit()
        path.unlink()
        path.write_bytes(accepted)
        for action in [c.render, lambda: inv.prepare_lineage_receipt(c.policy['source_lineage_receipt'], 'docs/lineage.json')]:
            with self.assertRaisesRegex(ValueError, 'regular in committed freeze'):
                action()

    def side_branch(self, first_activation=False):
        c = self.case
        if first_activation:
            c.run_git('reset', '--hard', c.replacement_commit)
        main = c.run_git('rev-parse', 'HEAD')
        c.run_git('checkout', '-qb', 'unmerged-snapshot', c.base)
        candidate = b'struct AllegedlyReviewedButNeverIntegrated {};\n'
        c.put(c.new, candidate.decode())
        unmerged = c.commit()
        c.run_git('checkout', '-q', '--detach', main)
        c.put('docs/unmerged.txt', candidate.decode())
        c.edge['replacements'] = [c.snapshot(c.new, candidate, unmerged, 'docs/unmerged.txt')]
        return candidate, unmerged

    def test_r3_unmerged_source_snapshot_is_rejected(self):
        c = self.case
        self.side_branch()
        c.seal([c.edge])
        with self.assertRaisesRegex(ValueError, 'never integrated'):
            c.render()
        with self.assertRaisesRegex(ValueError, 'never integrated'):
            inv.prepare_lineage_receipt(c.policy['source_lineage_receipt'], 'docs/lineage.json')

    def test_r3_ours_merge_does_not_prove_source_integration(self):
        c = self.case
        _, unmerged = self.side_branch()
        c.run_git('merge', '-s', 'ours', '--no-edit', unmerged)
        c.seal([c.edge])
        with self.assertRaisesRegex(ValueError, 'never integrated'):
            c.render()

    def test_archive_cannot_drop_predecessor_under_the_original_export_hash(self):
        self.chain()
        c = self.case
        self.archive()
        ledger = json.loads((c.root / 'docs/lineage.json').read_text())
        ledger['previous'] = None
        c.put_json('docs/lineage.json', ledger)
        c.receipt.update(previous=None, policy=c.anchor('docs/lineage.json'), proofs=inv.lineage_anchors(ledger))
        c.put_json(c.policy['source_lineage_receipt'], c.receipt)
        with self.assertRaisesRegex(ValueError, 'export fingerprint'):
            c.render()

    def test_r3_content_equivalent_import_and_later_edit_remain_valid(self):
        c = self.case
        candidate, _ = self.side_branch(first_activation=True)
        c.put(c.new, candidate.decode())
        imported = c.commit()
        c.edge['replacements'][0]['integrated'] = c.snapshot(c.new, candidate, imported, 'docs/unmerged.txt')
        c.put('docs/old.txt', c.old_bytes.decode())
        c.put('docs/consumers.md', 'Preserved consumer import')
        c.put('docs/behavior.md', 'Preserved import behavior')
        c.edge['consumer_evidence'] = [c.anchor('docs/consumers.md')]
        c.edge['behavior_evidence'] = [c.anchor('docs/behavior.md')]
        c.seal([c.edge])
        before = c.render()
        c.put(c.new, 'struct LegitimateLaterRevision {};\n')
        self.assertEqual(json.loads(c.render()['summary.json'])['historical_retained_scope'], 1)
        self.assertNotEqual(before, c.render())  # Current source fingerprint changes; history does not.
        after = c.render()
        self.archive()
        self.assertEqual(c.render(), after)

    def test_r3_import_anchor_must_be_content_equivalent(self):
        c = self.case
        self.side_branch()
        c.edge['replacements'][0]['integrated'] = c.snapshot(c.new, c.new_bytes, c.replacement_commit, 'docs/new.txt')
        c.seal([c.edge])
        with self.assertRaisesRegex(ValueError, 'not content-equivalent'):
            c.render()

    def test_archive_current_receipt_requires_the_provided_export_hash(self):
        c = self.case
        self.archive()
        c.put_json('docs/refactor-inventory/summary.json', {'lineage_receipt': {'sha256': '0' * 64}})
        with self.assertRaisesRegex(ValueError, 'export fingerprint'):
            c.render()


if __name__ == '__main__':
    unittest.main()
