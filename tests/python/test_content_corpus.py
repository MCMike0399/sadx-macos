"""Historical replay inputs survive later builds without changing expected hashes."""
import hashlib
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/journey'))
from content_corpus import archive_artifact, resolve_artifact


class ContentCorpusTests(unittest.TestCase):
    def test_rebuild_cannot_replace_archived_input(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / 'build/journeys/route.log'
            source.parent.mkdir(parents=True)
            source.write_bytes(b'qualified historical run')
            expected = hashlib.sha256(source.read_bytes()).hexdigest()
            frozen = archive_artifact(root, 'build/journeys/route.log', expected)
            source.write_bytes(b'new run with new diagnostics')
            self.assertEqual(resolve_artifact(root, 'build/journeys/route.log', expected), frozen)
            self.assertEqual(frozen.read_bytes(), b'qualified historical run')
            self.assertEqual(archive_artifact(root, 'build/journeys/route.log', expected), frozen)

    def test_bad_source_and_tampered_archive_fail_closed(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / 'capture.ppm'
            source.write_bytes(b'original')
            expected = hashlib.sha256(source.read_bytes()).hexdigest()
            with self.assertRaises(ValueError):
                archive_artifact(root, 'capture.ppm', '0' * 64)
            frozen = archive_artifact(root, 'capture.ppm', expected)
            frozen.chmod(0o600)
            frozen.write_bytes(b'corrupt')
            with self.assertRaises(ValueError):
                resolve_artifact(root, 'capture.ppm', expected)

    def test_unarchived_inputs_still_require_exact_hashes(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'input').write_bytes(b'input')
            expected = hashlib.sha256(b'input').hexdigest()
            self.assertEqual(resolve_artifact(root, 'input', expected), root.resolve() / 'input')
            with self.assertRaises(ValueError):
                resolve_artifact(root, 'input', '0' * 64)
            for name in ('../outside', '/outside'):
                with self.assertRaises(ValueError):
                    archive_artifact(root, name, expected)
