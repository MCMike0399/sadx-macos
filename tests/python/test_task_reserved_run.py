"""Final-run admission/refusal tests never generate a reserved input or build."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


FILE = Path(__file__).resolve().parents[2] / 'tools/oracles/task_judges/reserved_run.py'
SPEC = importlib.util.spec_from_file_location('task_reserved_run', FILE)
RUN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUN)


class TaskReservedRun(unittest.TestCase):
    def test_no_admission_does_not_verify_generate_or_create(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory) / 'out'
            with patch.object(RUN, 'OUT', out), patch.object(RUN, 'verify_freeze') as verify:
                with self.assertRaisesRegex(ValueError, 'explicitly admit'):
                    RUN.run(False)
                verify.assert_not_called()
                self.assertFalse(out.exists())

    def test_existing_execution_is_never_reused(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory) / 'out'
            out.mkdir()
            marker = out / 'EXECUTED_ONCE.json'
            marker.write_text('preserve')
            with patch.object(RUN, 'OUT', out), patch.object(RUN, 'verify_freeze', return_value={}):
                with self.assertRaises(FileExistsError):
                    RUN.run(True)
            self.assertEqual(marker.read_text(), 'preserve')

    def test_uncommitted_freeze_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            freeze = root / 'freeze.json'
            freeze.write_text('{}')
            with patch.object(RUN, 'ROOT', root), patch.object(RUN, 'FREEZE', freeze), \
                    patch.object(RUN, 'git', return_value=b'different'):
                with self.assertRaisesRegex(ValueError, 'committed unchanged'):
                    RUN.verify_freeze()

    def test_changed_dependency_refused_before_imports(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            dependency = root / 'helper.py'
            dependency.write_text('changed')
            freeze = root / 'freeze.json'
            frozen = dict(before=RUN.BEFORE, candidate=RUN.CANDIDATE,
                          inputs={str(dependency): '0' * 64})
            freeze.write_text(json.dumps(frozen))
            with patch.object(RUN, 'ROOT', root), patch.object(RUN, 'FREEZE', freeze), \
                    patch.object(RUN, 'git', return_value=freeze.read_bytes()), \
                    patch.object(RUN, 'root_inputs') as inspect:
                with self.assertRaisesRegex(ValueError, 'dependency changed'):
                    RUN.verify_freeze()
                inspect.assert_not_called()

    def test_changed_dependency_set_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            freeze = root / 'freeze.json'
            freeze.write_text(json.dumps(dict(before=RUN.BEFORE, candidate=RUN.CANDIDATE, inputs={})))
            with patch.object(RUN, 'ROOT', root), patch.object(RUN, 'FREEZE', freeze), \
                    patch.object(RUN, 'git', return_value=freeze.read_bytes()), \
                    patch.object(RUN, 'root_inputs', return_value={'new': 'digest'}):
                with self.assertRaisesRegex(ValueError, 'set/interpreter/toolchain'):
                    RUN.verify_freeze()

    def test_archive_checks_complete_content_and_extra_files(self):
        with tempfile.TemporaryDirectory() as directory:
            tree = Path(directory)
            path = tree / 'source.cpp'
            data = b'original'
            path.write_bytes(data)
            identity = hashlib.sha1(b'blob 8\0' + data).hexdigest()
            entry = f'100644 blob {identity}\tsource.cpp\0'.encode()
            with patch.object(RUN, 'git', return_value=entry):
                RUN.verify_archive(tree, 'commit')
                (tree / 'shadow.py').write_text('extra')
                with self.assertRaisesRegex(ValueError, 'set changed'):
                    RUN.verify_archive(tree, 'commit')
                (tree / 'shadow.py').unlink()
                path.write_text('modified')
                with self.assertRaisesRegex(ValueError, 'dependency changed'):
                    RUN.verify_archive(tree, 'commit')

    def test_exclusive_receipt_write(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'receipt.json'
            RUN.write_new(path, {'value': 1})
            with self.assertRaises(FileExistsError):
                RUN.write_new(path, {'value': 2})
            self.assertEqual(json.loads(path.read_text()), {'value': 1})


if __name__ == '__main__':
    unittest.main()
