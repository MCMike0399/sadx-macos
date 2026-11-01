"""Final-run admission/refusal tests never generate a reserved input or build."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import py_compile
import select
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
from types import SimpleNamespace


FILE = Path(__file__).resolve().parents[2] / 'tools/oracles/task_judges/reserved_run.py'
SPEC = importlib.util.spec_from_file_location('task_reserved_run', FILE)
RUN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUN)


class TaskReservedRun(unittest.TestCase):
    def test_timestamp_valid_stale_cache_cannot_replace_checked_source(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'helper.py'
            path.write_text("VALUE = 'stale'\n")
            timestamp = path.stat()
            py_compile.compile(str(path), doraise=True)
            path.write_text("VALUE = 'fresh'\n")
            os.utime(path, ns=(timestamp.st_atime_ns, timestamp.st_mtime_ns))
            module = RUN.source_module('task_stale_cache_test', path)
            self.assertEqual(module.VALUE, 'fresh')
            self.assertEqual(module.__file__, str(path.resolve()))

    def test_toolchain_header_library_and_link_changes_are_pinned(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header, library = root / 'header.h', root / 'library.tbd'
            header.write_text('old header')
            library.write_text('old library')
            link = root / 'current.h'
            link.symlink_to('header.h')
            before = RUN.tree_inputs(root)
            self.assertTrue(all(RUN.digest(name) == value for name, value in before.items()))
            header.write_text('new header')
            self.assertNotEqual(RUN.tree_inputs(root), before)
            header.write_text('old header')
            library.write_text('new library')
            self.assertNotEqual(RUN.tree_inputs(root), before)
            library.write_text('old library')
            link.unlink()
            link.symlink_to('library.tbd')
            self.assertNotEqual(RUN.tree_inputs(root), before)

    def test_build_environment_cannot_inherit_search_overrides(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with patch.dict(os.environ, {'CPATH': 'foreign', 'SDKROOT': 'foreign',
                                         'LIBRARY_PATH': 'foreign', 'CPLUS_INCLUDE_PATH': 'foreign'}), \
                    patch.object(RUN.subprocess, 'run', return_value=SimpleNamespace(returncode=0)) as run:
                RUN.command(['compiler'], root, root / 'log')
            environment = run.call_args.kwargs['env']
            for name in ('CPATH', 'SDKROOT', 'LIBRARY_PATH', 'CPLUS_INCLUDE_PATH'):
                self.assertNotIn(name, environment)

    def test_terminal_and_missing_parent_admission_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            with patch.object(RUN, 'OUT', out):
                with self.assertRaisesRegex(ValueError, 'live admitted parent'):
                    RUN.admit_child('generate', None, None)
                (out / 'RUN.json').write_text('{}')
                with self.assertRaisesRegex(ValueError, 'terminal'):
                    RUN.admit_child('generate', None, None)

    def test_child_is_bound_to_parent_stage_and_inherited_pipe(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            read_fd, write_fd = os.pipe()
            try:
                descriptor = os.fstat(read_fd)
                active = dict(parent_pid=os.getppid(), stage='old-task_work', token='exact',
                              pipe_device=descriptor.st_dev, pipe_inode=descriptor.st_ino)
                (out / 'ACTIVE.json').write_text(json.dumps(active))
                with patch.object(RUN, 'OUT', out):
                    RUN.admit_child('old-task_work', read_fd, 'exact', monitor=False)
                    for stage, token in [('current-task_work', 'exact'), ('old-task_work', 'wrong')]:
                        with self.assertRaisesRegex(ValueError, 'identity differs'):
                            RUN.admit_child(stage, read_fd, token, monitor=False)
                    active['parent_pid'] += 1
                    (out / 'ACTIVE.json').write_text(json.dumps(active))
                    with self.assertRaisesRegex(ValueError, 'identity differs'):
                        RUN.admit_child('old-task_work', read_fd, 'exact', monitor=False)
            finally:
                os.close(read_fd)
                os.close(write_fd)

    def test_parent_channel_eof_stops_admitted_child(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            read_fd, write_fd = os.pipe()
            descriptor = os.fstat(read_fd)
            (out / 'ACTIVE.json').write_text(json.dumps(dict(
                parent_pid=os.getpid(), stage='generate', token='exact',
                pipe_device=descriptor.st_dev, pipe_inode=descriptor.st_ino)))
            script = ("import importlib.util,sys,time; from pathlib import Path; "
                      "s=importlib.util.spec_from_file_location('runner',sys.argv[1]); "
                      "m=importlib.util.module_from_spec(s);s.loader.exec_module(m); "
                      "m.OUT=Path(sys.argv[2]);m.admit_child('generate',int(sys.argv[3]),'exact'); "
                      "print('ready',flush=True);time.sleep(30)")
            child = subprocess.Popen([sys.executable, '-I', '-c', script, str(FILE),
                                      str(out), str(read_fd)], pass_fds=(read_fd,),
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            try:
                self.assertTrue(select.select([child.stdout], [], [], 3)[0])
                self.assertEqual(child.stdout.readline().strip(), 'ready')
                os.close(write_fd)
                write_fd = None
                self.assertEqual(child.wait(timeout=3), 125)
            finally:
                os.close(read_fd)
                if write_fd is not None:
                    os.close(write_fd)
                if child.poll() is None:
                    child.terminate()
                    child.wait(timeout=3)
                child.stdout.close()
                child.stderr.close()

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
