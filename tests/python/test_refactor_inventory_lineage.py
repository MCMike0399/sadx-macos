"""Adversarial whole-render tests in disposable repositories; no game data required."""
import copy
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location('lineage_inventory', Path(__file__).resolve().parents[2] /
                                              'tools/inventory/refactor_inventory.py')
inv = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(inv)


class LineageFixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.old = 'src/player/operations.hpp'
        self.new = 'src/player/initializer.hpp'
        self.run_git('init', '-q')
        self.run_git('config', 'user.email', 'fixture@example.invalid')
        self.run_git('config', 'user.name', 'Fixture')
        self.put(self.old, 'constexpr int recipe[][2] = {{17, 33}};\n')
        self.put('Makefile', 'NATIVE_SOURCES := $(wildcard src/player/*.cpp)\n')
        self.put('native-sources.txt', '')
        self.put('.gitmodules', '')
        self.base = self.commit()
        self.old_bytes = (self.root / self.old).read_bytes()
        (self.root / self.old).unlink()
        self.put(self.new, 'struct TypedContent { int frame; };\n')
        self.replacement_commit = self.commit()
        self.new_bytes = (self.root / self.new).read_bytes()
        self.put('docs/old.txt', self.old_bytes.decode())
        self.put('docs/new.txt', self.new_bytes.decode())
        self.put('docs/consumers.md', 'Native and reference include the typed initializer.\n')
        self.put('docs/behavior.md', 'Frozen full streams retain exact ordered stores.\n')
        self.edge = dict(before=self.snapshot(self.old, self.old_bytes, self.base, 'docs/old.txt'),
                         replacements=[self.snapshot(self.new, self.new_bytes, self.replacement_commit, 'docs/new.txt')],
                         member=dict(module=inv.module(self.old), prior_disposition='needs_migration',
                                     remaining_debt=['Whole-system closure remains open.'],
                                     observed_counts=dict(entries=1027, reads=145, stores=882)),
                         disposition='reviewed_migration', rationale='Replace instruction recipes with typed initialization.',
                         consumer_evidence=[self.anchor('docs/consumers.md')],
                         behavior_evidence=[self.anchor('docs/behavior.md')])
        self.policy = dict(schema_version=1, base_commit=self.base, files={}, systems={
            'example': dict(destination='src/player', acceptance=[])},
            system_rules=[dict(pattern='.*', system='example')], external_reference_sources=[],
            source_lineage_receipt='docs/refactor-inventory/receipt.json')
        self.put('docs/refactor-inventory/boundaries.jsonl', '')
        self.seal([self.edge])
        self.root_patch = patch.object(inv, 'ROOT', self.root)
        self.out_patch = patch.object(inv, 'OUT', self.root / 'docs/refactor-inventory')
        self.root_patch.start()
        self.out_patch.start()
        self.addCleanup(self.root_patch.stop)
        self.addCleanup(self.out_patch.stop)

    def run_git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.root), *args], stderr=subprocess.PIPE).decode().strip()

    def put(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)

    def put_json(self, name, value):
        self.put(name, json.dumps(value, sort_keys=True, indent=2) + '\n')

    def commit(self):
        self.run_git('add', '-A')
        self.run_git('commit', '-qm', 'fixture', '--allow-empty')
        return self.run_git('rev-parse', 'HEAD')

    def anchor(self, name):
        return dict(path=name, sha256=inv.sha256((self.root / name).read_bytes()))

    def snapshot(self, name, data, commit, proof):
        return dict(path=name, sha256=inv.sha256(data), commit=commit,
                    proof=dict(path=proof, sha256=inv.sha256(data)))

    def seal(self, edges):
        for n, edge in enumerate(edges):
            review_path = f'docs/review-{n}.json'
            self.put_json(review_path, dict(edge_sha256=inv.canonical_hash({k: v for k, v in edge.items() if k != 'review'}),
                                           reviewer='Independent fixture reviewer', independent=True,
                                           verdict='accept', limits='File lineage only; no module closure.'))
            edge['review'] = self.anchor(review_path)
        self.put_json('docs/lineage.json', dict(schema_version=1, edges=edges))
        self.put_json('docs/refactor-inventory/review.json', self.policy)
        freeze = self.commit()
        base_paths = self.run_git('ls-tree', '-r', '--name-only', self.base).splitlines()
        baseline = {p: inv.sha256(subprocess.check_output(['git', '-C', str(self.root), 'show', self.base + ':' + p]))
                    for p in base_paths if p.startswith(inv.SCOPE_ROOTS)}
        paths = self.run_git('ls-files').splitlines()
        self.receipt = dict(schema_version=2, base_commit=self.base, freeze_commit=freeze,
                            policy=self.anchor('docs/lineage.json'), baseline_members=baseline,
                            archive_paths=sorted(set(paths) | {'docs/refactor-inventory/receipt.json'}),
                            source_freeze={p: inv.sha256((self.root / p).read_bytes()) for p in paths if p.startswith('src/')})
        self.put_json('docs/refactor-inventory/receipt.json', self.receipt)
        self.commit()

    def render(self):
        return inv.render()

    def mutate_edge(self, mutate):
        edge = copy.deepcopy(self.edge)
        mutate(edge)
        self.seal([edge])

    def test_reviewed_deletion_keeps_scope_counts_debt_and_current_paths_once(self):
        outputs = self.render()
        summary = json.loads(outputs['summary.json'])
        rows = [json.loads(line) for name, data in outputs.items() if name.startswith('files/') for line in data.splitlines()]
        history = [json.loads(line) for name, data in outputs.items() if name.startswith('lineage-') for line in data.splitlines()]
        self.assertEqual([r['path'] for r in rows], [self.new])
        self.assertEqual(rows[0]['disposition'], 'open_review')
        self.assertEqual(summary['current_tracked_denominator'], 1)
        self.assertEqual(summary['historical_retained_scope'], 1)
        self.assertEqual(summary['accounted_scope'], 2)
        self.assertEqual(history[0]['member']['observed_counts'], dict(entries=1027, reads=145, stores=882))
        self.assertEqual(history[0]['member']['prior_disposition'], 'needs_migration')
        self.assertEqual(history[0]['source_sha256'], inv.sha256(self.old_bytes))
        self.assertEqual(summary['historical_open_debt'], 1)
        self.assertEqual(summary['current_open_entries'], 1)
        self.assertEqual(summary['accounted_open_entries'], 2)
        self.assertEqual(sum(s['records'] for s in summary['lineage_shards']), 1)
        for shard in summary['lineage_shards']:
            self.assertEqual(inv.sha256(outputs[shard['path']].encode()), shard['sha256'])
            self.assertLessEqual(len(outputs[shard['path']].encode()), 2 * 1024 * 1024)

    def test_blank_semantics_missing_evidence_unknown_fields_and_overlap_fail(self):
        mutations = [lambda e: e.__setitem__('rationale', ' '),
                     lambda e: e.__setitem__('consumer_evidence', []),
                     lambda e: e.__setitem__('behavior_evidence', []),
                     lambda e: e.__setitem__('disposition', 'obsolete'),
                     lambda e: e['member'].__setitem__('remaining_debt', []),
                     lambda e: e.__setitem__('exempt_successors', True),
                     lambda e: e['replacements'].append(copy.deepcopy(e['replacements'][0]))]
        for mutate in mutations:
            with self.subTest(mutation=mutations.index(mutate)):
                self.mutate_edge(mutate)
                with self.assertRaises(ValueError):
                    self.render()

    def test_missing_snapshot_nonexistent_successor_and_empty_evidence_fail(self):
        self.mutate_edge(lambda e: e['before']['proof'].__setitem__('path', 'docs/missing.txt'))
        with self.assertRaisesRegex(ValueError, 'untracked lineage evidence'):
            self.render()
        self.mutate_edge(lambda e: e['replacements'][0].__setitem__('path', 'src/missing.hpp'))
        with self.assertRaises(subprocess.CalledProcessError):
            self.render()
        self.put('docs/behavior.md', '')
        self.edge['behavior_evidence'] = [self.anchor('docs/behavior.md')]
        self.seal([self.edge])
        with self.assertRaisesRegex(ValueError, 'Empty lineage'):
            self.render()

    def test_wrong_committed_baseline_hash_is_not_a_refresh(self):
        self.receipt['baseline_members'][self.old] = '0' * 64
        self.put_json('docs/refactor-inventory/receipt.json', self.receipt)
        self.commit()
        with self.assertRaisesRegex(ValueError, 'immutable baseline fingerprints'):
            self.render()

    def test_policy_must_match_frozen_git_object(self):
        ledger = json.loads((self.root / 'docs/lineage.json').read_text())
        ledger['edges'][0]['rationale'] = 'Changed after freeze'
        self.put_json('docs/lineage.json', ledger)
        self.receipt['policy'] = self.anchor('docs/lineage.json')
        self.put_json('docs/refactor-inventory/receipt.json', self.receipt)
        self.commit()
        with self.assertRaisesRegex(ValueError, 'frozen Git policy'):
            self.render()

    def test_unaccepted_or_nonindependent_review_fails(self):
        for field, value in [('independent', False), ('verdict', 'pending'), ('reviewer', ''), ('limits', '')]:
            with self.subTest(field=field):
                self.seal([self.edge])
                review = json.loads((self.root / 'docs/review-0.json').read_text())
                review[field] = value
                self.put_json('docs/review-0.json', review)
                self.edge['review'] = self.anchor('docs/review-0.json')
                self.put_json('docs/lineage.json', dict(schema_version=1, edges=[self.edge]))
                freeze = self.commit()
                self.receipt.update(freeze_commit=freeze, policy=self.anchor('docs/lineage.json'))
                self.put_json('docs/refactor-inventory/receipt.json', self.receipt)
                self.commit()
                with self.assertRaisesRegex(ValueError, 'independent lineage review'):
                    self.render()

    def test_receipt_preparation_requires_committed_source_and_policy(self):
        self.put(self.new, 'struct Changed {};\n')
        with self.assertRaisesRegex(ValueError, 'source identical'):
            inv.prepare_lineage_receipt('docs/refactor-inventory/receipt.json', 'docs/lineage.json')
        self.run_git('checkout', '--', self.new)
        self.put('src/player/untracked.hpp', 'int unknown;\n')
        with self.assertRaisesRegex(ValueError, 'source identical'):
            inv.prepare_lineage_receipt('docs/refactor-inventory/receipt.json', 'docs/lineage.json')
        (self.root / 'src/player/untracked.hpp').unlink()
        self.put('docs/lineage.json', '{}')
        with self.assertRaisesRegex(ValueError, 'committed policy'):
            inv.prepare_lineage_receipt('docs/refactor-inventory/receipt.json', 'docs/lineage.json')

    def test_prepared_receipt_requires_own_commit_before_use(self):
        inv.prepare_lineage_receipt('docs/refactor-inventory/receipt.json', 'docs/lineage.json')
        with self.assertRaisesRegex(ValueError, 'immutable HEAD'):
            self.render()
        self.commit()
        self.assertEqual(json.loads(self.render()['summary.json'])['historical_retained_scope'], 1)

    def test_no_policy_still_rejects_unexplained_omission(self):
        del self.policy['source_lineage_receipt']
        self.put_json('docs/refactor-inventory/review.json', self.policy)
        with self.assertRaisesRegex(ValueError, 'Baseline paths removed'):
            self.render()

    def test_successor_legitimate_edit_preserves_history_requires_fresh_conformity(self):
        self.put(self.new, 'struct TypedContent { int frame; int speed; };\n')
        self.assertEqual(json.loads(self.render()['summary.json'])['historical_retained_scope'], 1)
        self.policy['files'][self.new] = dict(disposition='already_conforming', reason='Prior review',
            reviewed_source_sha256=inv.sha256(self.new_bytes), semantic_justification='Typed data',
            resolved_owners=['owner'], resolved_consumers=['consumer'], accepted_judges=['judge'],
            evidence=[dict(path='docs/behavior.md')])
        self.put_json('docs/refactor-inventory/review.json', self.policy)
        with self.assertRaisesRegex(ValueError, 'source changed'):
            self.render()

    def test_added_instruction_table_has_no_inherited_exception(self):
        self.put('src/player/added.hpp', 'constexpr int operations[][2] = {{0x401000, 99}};\n')
        self.run_git('add', 'src/player/added.hpp')
        rows = [json.loads(line) for name, data in self.render().items() if name.startswith('files/') for line in data.splitlines()]
        self.assertEqual({r['path']: r['disposition'] for r in rows}, {self.new: 'open_review', 'src/player/added.hpp': 'open_review'})

    def test_successor_renamed_deleted_or_untracked_fails(self):
        for mode in ['rename', 'delete', 'untrack']:
            with self.subTest(mode=mode):
                if mode == 'rename':
                    self.run_git('mv', self.new, 'src/player/later.hpp')
                elif mode == 'delete':
                    self.run_git('rm', self.new)
                else:
                    self.run_git('rm', '--cached', self.new)
                with self.assertRaisesRegex(ValueError, 'successor'):
                    self.render()
                self.run_git('reset', '--hard', 'HEAD')
                self.run_git('clean', '-fdq')

    def test_tracked_successor_symlink_cannot_replace_real_source(self):
        (self.root / self.new).unlink()
        (self.root / self.new).symlink_to('../../docs/new.txt')
        self.run_git('add', self.new)
        with self.assertRaisesRegex(ValueError, 'successor'):
            self.render()

    def test_stale_or_untracked_behavior_evidence_fails(self):
        self.put('docs/behavior.md', 'Changed receipt\n')
        with self.assertRaisesRegex(ValueError, 'Stale lineage evidence'):
            self.render()
        self.run_git('checkout', '--', 'docs/behavior.md')
        self.run_git('rm', '--cached', 'docs/behavior.md')
        with self.assertRaisesRegex(ValueError, 'untracked lineage evidence'):
            self.render()

    def test_wrong_snapshot_hash_and_git_object_fail(self):
        for field, value in [('sha256', '0' * 64), ('commit', self.replacement_commit)]:
            with self.subTest(field=field):
                # The second case uses a real object, where the old path no longer exists.
                self.mutate_edge(lambda e: e['before'].__setitem__(field, value))
                with self.assertRaises((ValueError, subprocess.CalledProcessError)):
                    self.render()

    def test_duplicate_overlapping_and_unknown_edges_fail(self):
        self.seal([self.edge, copy.deepcopy(self.edge)])
        with self.assertRaisesRegex(ValueError, 'Duplicate/stale'):
            self.render()
        extra = copy.deepcopy(self.edge)
        extra['before']['path'] = 'src/player/unknown.hpp'
        extra['member']['module'] = inv.module(extra['before']['path'])
        # No immutable Git object exists for this asserted source.
        self.seal([self.edge, extra])
        with self.assertRaises((ValueError, subprocess.CalledProcessError)):
            self.render()

    def test_independent_review_cannot_be_reused_after_semantic_mapping_edit(self):
        ledger = json.loads((self.root / 'docs/lineage.json').read_text())
        ledger['edges'][0]['rationale'] = 'A different claimed rewrite'
        self.put_json('docs/lineage.json', ledger)
        self.receipt['policy'] = self.anchor('docs/lineage.json')
        freeze = self.commit()
        self.receipt['freeze_commit'] = freeze
        self.put_json('docs/refactor-inventory/receipt.json', self.receipt)
        self.commit()
        with self.assertRaisesRegex(ValueError, 'independent lineage review'):
            self.render()

    def test_edited_tree_cannot_refresh_immutable_receipt(self):
        self.receipt['source_freeze'] = {}
        self.put_json('docs/refactor-inventory/receipt.json', self.receipt)
        with self.assertRaisesRegex(ValueError, 'immutable HEAD'):
            self.render()
        self.commit()
        with self.assertRaisesRegex(ValueError, 'source freeze'):
            self.render()

    def test_archive_without_git_validates_supplied_proof_and_exact_membership(self):
        outputs = self.render()
        # Archive carries the existing boundary metadata (empty for this fixture).
        self.put('docs/refactor-inventory/boundaries.jsonl', '')
        shutil.rmtree(self.root / '.git')
        self.assertEqual(self.render(), outputs)
        self.put('docs/new.txt', 'Corrupt snapshot')
        with self.assertRaisesRegex(ValueError, 'Stale lineage evidence'):
            self.render()

    def test_legacy_archive_ordinary_render_has_no_git_requirement(self):
        del self.policy['source_lineage_receipt']
        self.policy['base_commit'] = self.replacement_commit
        self.put_json('docs/refactor-inventory/review.json', self.policy)
        outputs = self.render()
        for name, content in outputs.items():
            self.put('docs/refactor-inventory/' + name, content)
        shutil.rmtree(self.root / '.git')
        self.assertEqual(self.render(), outputs)

    def test_archive_untracked_evidence_is_not_authorized_by_existing_on_disk(self):
        self.put('docs/new-untracked-evidence.md', 'Exists but absent from tracked export')
        self.edge['behavior_evidence'] = [self.anchor('docs/new-untracked-evidence.md')]
        # Keep the supplied archive membership unchanged; recompute only the ledger/review.
        self.put_json('docs/review-0.json', dict(edge_sha256=inv.canonical_hash(
            {k: v for k, v in self.edge.items() if k != 'review'}), reviewer='Fixture',
            independent=True, verdict='accept', limits='Lineage only'))
        self.edge['review'] = self.anchor('docs/review-0.json')
        self.put_json('docs/lineage.json', dict(schema_version=1, edges=[self.edge]))
        self.receipt['policy'] = self.anchor('docs/lineage.json')
        self.put_json('docs/refactor-inventory/receipt.json', self.receipt)
        shutil.rmtree(self.root / '.git')
        with self.assertRaisesRegex(ValueError, 'untracked lineage evidence'):
            self.render()

    def test_archive_preserves_gitlink_as_one_boundary_without_upstream_files(self):
        (self.root / 'third_party/upstream').mkdir(parents=True)
        self.run_git('update-index', '--add', '--cacheinfo', '160000', self.base, 'third_party/upstream')
        self.commit()
        inv.prepare_lineage_receipt('docs/refactor-inventory/receipt.json', 'docs/lineage.json')
        self.commit()
        outputs = self.render()
        self.put('docs/refactor-inventory/boundaries.jsonl', outputs['boundaries.jsonl'])
        shutil.rmtree(self.root / '.git')
        self.assertEqual(self.render(), outputs)
        summary = json.loads(outputs['summary.json'])
        self.assertEqual(summary['boundary_entries'], 1)
        self.assertEqual(summary['current_tracked_denominator'], 2)

    def test_archive_extra_or_missing_membership_fails(self):
        self.put('docs/refactor-inventory/boundaries.jsonl', '')
        shutil.rmtree(self.root / '.git')
        self.put('src/player/extra.hpp', 'int extra;\n')
        with self.assertRaisesRegex(ValueError, 'Archive membership'):
            self.render()

    def test_reviewed_chain_survives_later_move_and_preserves_each_historical_member(self):
        next_path = 'src/player/typed_initializer.hpp'
        edited = b'struct TypedContent { int frame; int speed; };\n'
        self.put(self.new, edited.decode())
        before_move = self.commit()
        self.put('docs/edited.txt', edited.decode())
        self.run_git('mv', self.new, next_path)
        moved = self.commit()
        self.put('docs/later.txt', edited.decode())
        edge = copy.deepcopy(self.edge)
        edge['before'] = self.snapshot(self.new, edited, before_move, 'docs/edited.txt')
        edge['member']['module'] = inv.module(self.new)
        edge['replacements'] = [self.snapshot(next_path, edited, moved, 'docs/later.txt')]
        self.seal([self.edge, edge])
        summary = json.loads(self.render()['summary.json'])
        self.assertEqual(summary['current_tracked_denominator'], 1)
        self.assertEqual(summary['historical_retained_scope'], 2)
        self.assertEqual(summary['historical_baseline_members'], 1)

    def test_existing_baseline_successor_can_later_move_without_losing_its_old_identity(self):
        self.run_git('reset', '--hard', self.base)
        baseline_successor = b'struct PreviousContent { int old_frame; };\n'
        self.put(self.new, baseline_successor.decode())
        self.base = self.commit()
        self.policy['base_commit'] = self.base
        (self.root / self.old).unlink()
        self.put(self.new, self.new_bytes.decode())
        replacement = self.commit()
        later = 'src/player/later.hpp'
        self.run_git('mv', self.new, later)
        moved = self.commit()
        for name, data in [('docs/old.txt', self.old_bytes), ('docs/new.txt', self.new_bytes),
                           ('docs/base-existing.txt', baseline_successor), ('docs/later.txt', self.new_bytes)]:
            self.put(name, data.decode())
        self.put('docs/consumers.md', 'Preserved both consumers')
        self.put('docs/behavior.md', 'Preserved both behaviors')
        self.edge.update(before=self.snapshot(self.old, self.old_bytes, self.base, 'docs/old.txt'),
                         replacements=[self.snapshot(self.new, self.new_bytes, replacement, 'docs/new.txt')],
                         consumer_evidence=[self.anchor('docs/consumers.md')],
                         behavior_evidence=[self.anchor('docs/behavior.md')])
        edge = copy.deepcopy(self.edge)
        edge['before'] = self.snapshot(self.new, self.new_bytes, replacement, 'docs/new.txt')
        edge['member']['module'] = inv.module(self.new)
        edge['member']['baseline_snapshot'] = self.snapshot(self.new, baseline_successor, self.base, 'docs/base-existing.txt')
        edge['replacements'] = [self.snapshot(later, self.new_bytes, moved, 'docs/later.txt')]
        self.seal([self.edge, edge])
        outputs = self.render()
        summary = json.loads(outputs['summary.json'])
        self.assertEqual(summary['historical_baseline_members'], 2)
        self.assertEqual(summary['accounted_scope'], 3)
        rows = [json.loads(line) for line in outputs['lineage-001.jsonl'].splitlines()]
        retained = next(row for row in rows if row['historical_path'] == self.new)
        self.assertEqual(retained['source_sha256'], inv.sha256(baseline_successor))
        # Omitting the original identity cannot hide inside the later reviewed transition.
        del edge['member']['baseline_snapshot']
        self.seal([self.edge, edge])
        with self.assertRaisesRegex(ValueError, 'baseline omission/fingerprint'):
            self.render()

    def test_cyclic_chain_fails_even_with_independently_accepted_edges(self):
        # Snapshot Git verification is tested separately; an archive proves graph rejection.
        edge = copy.deepcopy(self.edge)
        edge['before'] = self.snapshot(self.new, self.new_bytes, self.replacement_commit, 'docs/new.txt')
        edge['member']['module'] = inv.module(self.new)
        edge['replacements'] = [self.edge['before']]
        (self.root / self.new).unlink()
        self.run_git('rm', '--cached', self.new)
        self.seal([self.edge, edge])
        self.put('docs/refactor-inventory/boundaries.jsonl', '')
        shutil.rmtree(self.root / '.git')
        with self.assertRaisesRegex(ValueError, 'Cyclic lineage'):
            self.render()


if __name__ == '__main__':
    unittest.main()
