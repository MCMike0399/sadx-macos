"""Manifest drift and proposed hook fixtures, independent of game binaries."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[2] / 'tools/gate'
sys.path.insert(0, str(TOOLS))
from check_judge import DIRECTORIES, FILES, MANIFEST, check, digest, protected, snapshot
from guard_judge_edit import guard


class JudgeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        for name in DIRECTORIES:
            (self.root / name).mkdir(parents=True, exist_ok=True)
        for name in FILES:
            self.write(name, 'fixture\n')
        self.write('tools/oracles/check.py', 'pass\n')
        self.write('tests/probes/probe.cpp', 'void probe();\n')
        self.write('reference/oracle-reports/fixture.json', '{}\n')
        self.write('evidence.txt', 'fixture verification passed\n')
        self.write(MANIFEST, json.dumps(dict(schema=1, files=snapshot(self.root), change=dict(
            reason='fixture', evidence='evidence.txt', evidence_sha256=digest(self.root / 'evidence.txt')))))

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        return path

    def cli(self, *arguments):
        return subprocess.run([sys.executable, str(TOOLS / 'check_judge.py'), '--root',
                               str(self.root), *arguments], capture_output=True, text=True)

    def git(self, *arguments):
        return subprocess.run(['git', *arguments], cwd=self.root, check=True,
                              capture_output=True, text=True)

    def commit_fixture(self):
        self.git('init', '-q')
        self.git('config', 'user.email', 'fixture@example.invalid')
        self.git('config', 'user.name', 'Judge Fixture')
        self.git('add', '.')
        self.git('commit', '-qm', 'fixture baseline')

    def test_hash_addition_removal_and_generated_files(self):
        self.assertEqual(check(self.root)[0], [])
        self.write('tools/oracles/check.py', 'mutated\n')
        self.assertTrue(any('changed judge input' in e for e in check(self.root)[0]))
        self.write('tools/oracles/check.py', 'pass\n')
        added = self.write('tools/gate/new.py', 'pass\n')
        self.assertTrue(any('unmanifested' in e for e in check(self.root)[0]))
        added.unlink()
        self.write('tools/gate/__pycache__/generated.pyc', 'ignored')
        self.assertEqual(check(self.root)[0], [])
        (self.root / 'tests/probes/probe.cpp').unlink()
        self.assertTrue(any('missing judge input' in e for e in check(self.root)[0]))

    def test_frozen_corpus_summary_and_mutant_coverage(self):
        for path in ['reference/frame-control-corpus.tsv', 'reference/oracle-reports/fixture.json',
                     'reference/production-integration-20990101/new-mutants.json']:
            with self.subTest(path=path):
                original = (self.root / path).read_text() if (self.root / path).exists() else None
                self.write(path, 'tampered\n')
                self.assertTrue(check(self.root)[0])
                if original is None:
                    (self.root / path).unlink()
                else:
                    self.write(path, original)
        self.assertTrue(protected('tests/python/test_judge.py'))
        self.assertTrue(protected('tools/lib/pe_image.py'))
        self.assertTrue(protected(MANIFEST))

    def test_update_requires_evidence_and_separate_game_commit(self):
        self.assertEqual(self.cli('--update').returncode, 1)
        self.commit_fixture()
        self.write('tools/oracles/check.py', 'new judge\n')
        self.assertEqual(self.cli('--update', '--reason', 'fixture change', '--evidence', 'evidence.txt').returncode, 0)
        self.write('src/game/flow.cpp', 'void flow() {}\n')
        self.assertTrue(any('mixed with game-source' in e for e in check(self.root)[0]))
        self.assertIn('mixed with game-source', self.cli('--update', '--reason', 'mixed',
                                                      '--evidence', 'evidence.txt').stdout)

    def test_invalid_manifest_and_symlink(self):
        self.write(MANIFEST, '{"schema": 1, "files": {"../escape": "no"}}')
        self.assertEqual(self.cli().returncode, 1)
        (self.root / 'tools/oracles/link.py').symlink_to(self.root / 'evidence.txt')
        with self.assertRaises(ValueError):
            snapshot(self.root)

    def test_new_frozen_corpus_and_alias_hook_target(self):
        self.write('reference/frozen-corpora/another.tsv', 'new frozen input\n')
        self.assertTrue(any('unmanifested' in e for e in check(self.root)[0]))
        alias = self.root / 'alias.py'
        alias.symlink_to(self.root / 'tools/oracles/check.py')
        with self.assertRaises(OSError):
            guard(dict(tool_name='Edit', tool_input=dict(file_path=str(alias))), self.root)

    def test_hook_declared_step_expiry_and_unprotected_edits(self):
        self.commit_fixture()
        payload = dict(tool_name='Edit', cwd=str(self.root), tool_input=dict(
            file_path=str(self.root / 'tools/oracles/check.py')))
        with self.assertRaises(OSError):
            guard(payload, self.root)
        result = self.cli('--declare-change', '--reason', 'fixture judge step', '--evidence', 'evidence.txt')
        self.assertEqual(result.returncode, 0, result.stdout)
        guard(payload, self.root)
        self.write('evidence.txt', 'changed evidence\n')
        with self.assertRaises(ValueError):
            guard(payload, self.root)
        self.write('evidence.txt', 'fixture verification passed\n')
        self.git('commit', '--allow-empty', '-qm', 'next step')
        with self.assertRaises(ValueError):
            guard(payload, self.root)
        payload['tool_input']['file_path'] = str(self.root / 'src/game/flow.cpp')
        guard(payload, self.root)

    def test_hook_json_protocol_and_worktree_cwd(self):
        self.commit_fixture()
        payload = dict(hook_event_name='PreToolUse', tool_name='Write', cwd=str(self.root),
                       tool_input=dict(file_path=str(self.root / MANIFEST)))
        def invoke(data):
            return subprocess.run([sys.executable, str(TOOLS / 'guard_judge_edit.py')],
                                  input=json.dumps(data), capture_output=True, text=True)
        self.assertEqual(invoke(payload).returncode, 2)
        self.assertEqual(self.cli('--declare-change', '--reason', 'test', '--evidence', 'evidence.txt').returncode, 0)
        self.assertEqual(invoke(payload).returncode, 0)
        payload['tool_input'] = {}
        self.assertEqual(invoke(payload).returncode, 2)
        payload['tool_name'] = 'Read'
        self.assertEqual(invoke(payload).returncode, 0)


if __name__ == '__main__':
    unittest.main()
