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
import venv
from unittest.mock import patch
from types import SimpleNamespace


FILE = Path(__file__).resolve().parents[2] / 'tools/oracles/task_judges/reserved_run.py'
SPEC = importlib.util.spec_from_file_location('task_reserved_run', FILE)
RUN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUN)


class TaskReservedRun(unittest.TestCase):
    def continuation_proof(self):
        return dict(terminal_sha256='1'*64, freeze_sha256='2'*64,
                    inputs={slug: '3'*64 for slug in RUN.SLUGS},
                    libraries={str(RUN.ORIGINAL_OUT/world/'libraries'/(slug+'.dylib')): '4'*64
                               for world in ('old', 'current') for slug in RUN.SLUGS},
                    original_output=str(RUN.ORIGINAL_OUT))

    def test_invalid_complete_proof_and_unequal_valid_proof_refuse_before_output(self):
        for proof in (dict.fromkeys(self.continuation_proof()), self.continuation_proof()):
            with tempfile.TemporaryDirectory() as directory:
                out = Path(directory)/'out'
                frozen = dict(execution_mode='continuation', continuation=proof)
                with patch.object(RUN, 'EXECUTION_MODE', 'continuation'), patch.object(RUN, 'OUT', out), \
                        patch.object(RUN, 'verify_freeze', return_value=frozen), \
                        patch.object(RUN, 'unattempted_previous_run', return_value={'different': True}), \
                        patch.object(RUN, 'archive') as archive, patch.object(RUN, 'build') as build, \
                        patch.object(RUN, 'command') as command:
                    with self.assertRaisesRegex(ValueError, 'proof contents|proof changed'):
                        RUN.run(True)
                    self.assertFalse(out.exists())
                    archive.assert_not_called()
                    build.assert_not_called()
                    command.assert_not_called()

    def test_selected_continuation_never_falls_back_to_build_or_generation(self):
        for proof in (None, {}, ''):
            with tempfile.TemporaryDirectory() as directory:
                out = Path(directory)/'out'
                frozen = dict(execution_mode='continuation', continuation=proof)
                with patch.object(RUN, 'EXECUTION_MODE', 'continuation'), patch.object(RUN, 'OUT', out), \
                        patch.object(RUN, 'verify_freeze', return_value=frozen), \
                        patch.object(RUN, 'archive') as archive, patch.object(RUN, 'build') as build, \
                        patch.object(RUN, 'command') as command:
                    with self.assertRaisesRegex(ValueError, 'complete reuse proof'):
                        RUN.run(True)
                    archive.assert_not_called()
                    build.assert_not_called()
                    command.assert_not_called()
                    self.assertFalse(out.exists())

    def test_selected_mode_and_freeze_identity_must_agree(self):
        with patch.object(RUN, 'EXECUTION_MODE', 'continuation'):
            for frozen in ({}, {'execution_mode': 'original', 'continuation': None}):
                with self.assertRaisesRegex(ValueError, 'mode differs'):
                    RUN.validate_mode(frozen)
            with patch.object(RUN, 'admit_child') as admit:
                with self.assertRaisesRegex(ValueError, 'cannot generate'):
                    RUN.child('generate')
                admit.assert_not_called()
        with patch.object(RUN, 'EXECUTION_MODE', 'original'):
            with self.assertRaisesRegex(ValueError, 'cannot select'):
                RUN.validate_mode({'execution_mode': 'original', 'continuation': {'proof': True}})

    def test_continuation_requires_exact_inputs_models_and_zero_prior_attempts(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            old = root/'original'
            old.mkdir()
            freeze, pins = root/'old-freeze.json', root/'old-pins.json'
            pins.write_text(json.dumps({str(FILE.absolute()): 'old runner'}))
            freeze.write_text(json.dumps(dict(before=RUN.BEFORE, candidate=RUN.CANDIDATE,
                protocol={'immutable_before_source': RUN.BEFORE}, toolchain={}, input_manifest={
                'path': 'old-pins.json', 'sha256': RUN.digest(pins)})))
            (old/'EXECUTED_ONCE.json').write_text(json.dumps({'freeze_sha256': RUN.digest(freeze)}))
            terminal = dict(error={'message': 'pre-observation failure'}, completed=[])
            (old/'RUN.json').write_text(json.dumps(terminal))
            inputs, models = {}, {}
            for slug in RUN.SLUGS:
                path = old/(slug+'-inputs.json')
                path.write_text(json.dumps({'cases': [[1]]}))
                inputs[slug] = RUN.digest(path)
                for world in ('old', 'current'):
                    library = old/world/'libraries'/(slug+'.dylib')
                    library.parent.mkdir(parents=True, exist_ok=True)
                    library.write_text('model')
                    models[str(library)] = RUN.digest(library)
            (old/'INPUTS.json').write_text(json.dumps(inputs))
            (old/'BUILD.json').write_text(json.dumps({'libraries': models}))
            failure = dict(terminal_receipt_sha256=RUN.digest(old/'RUN.json'),
                           frozen_marker_sha256=RUN.digest(old/'EXECUTED_ONCE.json'),
                           counts=[dict(slug=slug, input_file_sha256=value, generated=1)
                                   for slug, value in inputs.items()])
            anchors = {'ROOT-EXECUTION-ADMISSION.json': {'freeze_sha256': RUN.digest(freeze)},
                       'RESERVED-ATTEMPT-1-FAILURE.json': failure,
                       'ATTEMPT-1-MODEL-ANCHOR.json': {'original_build_sha256': RUN.digest(old/'BUILD.json'),
                                                     'models': models}}
            with patch.object(RUN, 'ROOT', root), patch.object(RUN, 'ORIGINAL_OUT', old), \
                    patch.object(RUN, 'ORIGINAL_FREEZE', freeze), patch.object(RUN, 'ORIGINAL_PINS', pins), \
                    patch.object(RUN, 'toolchain', return_value={}), \
                    patch.object(RUN, 'committed_evidence', side_effect=lambda commit, name: anchors[name]), \
                    patch.object(RUN, 'root_inputs', return_value={str(FILE.absolute()): 'corrected runner'}):
                proof = RUN.unattempted_previous_run()
                self.assertEqual(proof['inputs'], inputs)
                self.assertEqual(proof['libraries'], models)
                path = old/'task_work-inputs.json'
                original = path.read_bytes()
                path.write_text('changed')
                with self.assertRaisesRegex(ValueError, 'inputs changed'):
                    RUN.unattempted_previous_run()
                path.write_bytes(original)
                # A coherent replacement of input bytes AND its mutable manifest
                # must still fail against the independently committed selection.
                path.write_text(json.dumps({'cases': [[999]]}))
                substituted = dict(inputs, task_work=RUN.digest(path))
                (old/'INPUTS.json').write_text(json.dumps(substituted))
                with self.assertRaisesRegex(ValueError, 'committed failure'):
                    RUN.unattempted_previous_run()
                path.write_bytes(original)
                (old/'INPUTS.json').write_text(json.dumps(inputs))
                original_terminal = (old/'RUN.json').read_bytes()
                (old/'RUN.json').write_text(json.dumps(dict(terminal, error={'message': 'different failure'})))
                with self.assertRaisesRegex(ValueError, 'committed failure'):
                    RUN.unattempted_previous_run()
                (old/'RUN.json').write_bytes(original_terminal)
                original_freeze = freeze.read_bytes()
                freeze.write_text(json.dumps({'other': 'freeze'}))
                with self.assertRaisesRegex(ValueError, 'committed admission'):
                    RUN.unattempted_previous_run()
                freeze.write_bytes(original_freeze)
                captures = old/'old/captures'
                captures.mkdir()
                with self.assertRaisesRegex(ValueError, 'already attempted'):
                    RUN.unattempted_previous_run()
                captures.rmdir()
                (old/'RUN.json').write_text(json.dumps(dict(terminal, completed=['task_work'])))
                with self.assertRaisesRegex(ValueError, 'not the uncompleted failed'):
                    RUN.unattempted_previous_run()

    def child_dispatch(self, changed_continuation_inputs=False):
        class ReachedObserver(Exception):
            pass
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            freeze = out/'freeze.json'
            freeze.write_text('{}')
            (out/'EXECUTED_ONCE.json').write_text(json.dumps({'freeze_sha256': RUN.digest(freeze)}))
            libraries = {}
            models_root = out/'original-models' if changed_continuation_inputs else out
            for world in ('old', 'current'):
                folder = models_root/world/'libraries'
                folder.mkdir(parents=True)
                for slug in RUN.SLUGS:
                    path = folder/(slug+'.dylib')
                    path.write_text('fake model')
                    libraries[str(path)] = RUN.digest(path)
            (out/'BUILD.json').write_text(json.dumps({'libraries': libraries}))
            inputs = {}
            for slug in RUN.SLUGS:
                path = out/(slug+'-inputs.json')
                path.write_text(json.dumps(dict(cases=[[1]], labels=['fresh'], definition={})))
                inputs[slug] = RUN.digest(path)
            (out/'INPUTS.json').write_text(json.dumps(inputs))
            proof = dict(terminal_sha256='1'*64, freeze_sha256='2'*64, inputs=dict(inputs),
                         libraries=libraries, original_output=str(models_root))
            if changed_continuation_inputs:
                (out/'task_work-inputs.json').write_text(json.dumps(dict(cases=[[999]], labels=['fresh'], definition={})))
                inputs['task_work'] = RUN.digest(out/'task_work-inputs.json')
                (out/'INPUTS.json').write_text(json.dumps(inputs))
            mode = 'continuation' if changed_continuation_inputs else 'original'
            def observe(*args):
                raise ReachedObserver()
            prior_path = list(sys.path)
            try:
                with patch.object(RUN, 'OUT', out), patch.object(RUN, 'FREEZE', freeze), \
                        patch.object(RUN, 'ORIGINAL_OUT', models_root), patch.object(RUN, 'EXECUTION_MODE', mode), \
                        patch.object(RUN, 'admit_child'), \
                        patch.object(RUN, 'verify_freeze', return_value={'before': RUN.BEFORE, 'candidate': RUN.CANDIDATE, 'execution_mode': mode, 'continuation': proof if changed_continuation_inputs else None}), \
                        patch.object(RUN, 'unattempted_previous_run', return_value=proof), \
                        patch.object(RUN, 'verify_archive'), patch.object(RUN, 'verify_origins'), \
                        patch.object(RUN, 'pinned_unicorn'), \
                        patch.object(RUN, 'source_module', return_value=SimpleNamespace(observe=observe)), \
                        patch.object(RUN.importlib, 'import_module'):
                    expected = ValueError if changed_continuation_inputs else ReachedObserver
                    with self.assertRaises(expected):
                        RUN.child('observe', 'old', 'task_work', 99, 'fake')
                self.assertEqual((out/'old-task_work.started.json').is_file(), not changed_continuation_inputs)
                self.assertFalse(list((out/'current/libraries').glob('*.started.json')))
            finally:
                sys.path[:] = prior_path

    def test_real_child_dispatch_keeps_stage_name_after_library_validation(self):
        self.child_dispatch()

    def test_coherent_copied_input_replacement_refused_before_child_observation(self):
        self.child_dispatch(changed_continuation_inputs=True)

    def test_complete_unchanged_symlink_freeze_round_trip_and_selection_refusal(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ('tools/oracles', 'tools/lib', 'packet', 'sdk', 'resource', 'cxx',
                         'game/Sonic Adventure DX'):
                (root / name).mkdir(parents=True)
            (root / 'packet/ROOT-RESERVED-PROTOCOL.json').write_text('{}')
            (root / 'game/Sonic Adventure DX/sonic.exe').write_text('PE')
            prefix = root / 'venv'
            package = prefix / 'lib' / f'python{sys.version_info.major}.{sys.version_info.minor}' / 'site-packages/unicorn'
            package.mkdir(parents=True)
            (package / '__init__.py').write_text('package')
            compiler = root / 'clang'
            compiler.write_text('compiler')
            spelling = root / 'clang++'
            spelling.symlink_to('clang')
            linker, lto = root / 'ld', root / 'libLTO'
            linker.write_text('linker')
            lto.write_text('support')
            tools = dict(compiler=str(spelling), linker=str(linker), lto=str(lto),
                         sdk=str(root/'sdk'), resource=str(root/'resource'), cxx=str(root/'cxx'),
                         include_search=[str(root/'cxx')])
            freeze = root / 'freeze.json'
            with patch.object(RUN, 'ROOT', root), patch.object(RUN, 'PACKET', root/'packet'), \
                    patch.object(RUN.sys, 'prefix', str(prefix)), patch.object(RUN, 'FREEZE', freeze), \
                    patch.object(RUN, 'PIN_DATA', root/'raw-pins.json'), \
                    patch.object(RUN, 'toolchain', return_value=tools):
                inputs = RUN.root_inputs(tools)
                self.assertEqual(inputs[str(spelling)], RUN.digest(spelling))
                self.assertEqual(inputs[str(compiler.resolve())], RUN.digest(compiler))
                frozen = dict(before=RUN.BEFORE, candidate=RUN.CANDIDATE, execution_mode='original', continuation=None,
                              input_manifest=RUN.write_pin_manifest(inputs), toolchain=tools)
                freeze.write_text(json.dumps(frozen))
                with patch.object(RUN, 'git', side_effect=lambda *args: freeze.read_bytes()):
                    self.assertEqual(RUN.verify_freeze(), frozen)
                    for key in ('compiler', 'linker', 'lto', 'sdk', 'resource', 'cxx', 'include_search'):
                        changed = dict(tools)
                        changed[key] = ['foreign'] if key == 'include_search' else 'foreign'
                        freeze.write_text(json.dumps(dict(frozen, toolchain=changed)))
                        with self.assertRaisesRegex(ValueError, 'Selected toolchain'):
                            RUN.verify_freeze()

    def test_no_site_startup_ignores_pth_preloaded_original(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            environment = root / 'venv'
            venv.EnvBuilder(with_pip=False).create(environment)
            site = next((environment/'lib').glob('python*/site-packages'))
            (site/'poison.pth').write_text("import sys,types; m=types.ModuleType('verify_original_task_work'); m.VALUE='poison'; m.__file__='/outside.py'; sys.modules[m.__name__]=m\n")
            archive = root / 'archive'
            archive.mkdir()
            (archive/'verify_original_task_work.py').write_text("VALUE='sealed'\n")
            code = "import sys;sys.path.insert(0,sys.argv[1]);import verify_original_task_work as m;print(m.VALUE)"
            python = str(environment/'bin/python')
            before = subprocess.run([python, '-I', '-B', '-c', code, str(archive)], capture_output=True, text=True)
            after = subprocess.run([python, '-I', '-S', '-B', '-c', code, str(archive)], capture_output=True, text=True)
            self.assertEqual(before.returncode, 0, before.stderr)
            self.assertEqual(before.stdout.strip(), 'poison')
            self.assertEqual(after.returncode, 0, after.stderr)
            self.assertEqual(after.stdout.strip(), 'sealed')

    def test_preloaded_project_module_with_foreign_origin_is_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            tree = Path(directory)
            folder = tree/'tools/oracles'
            folder.mkdir(parents=True)
            (tree/'tools/lib').mkdir()
            (folder/'verify_original_task_work.py').write_text('sealed')
            module = SimpleNamespace(__file__='/outside.py')
            with patch.dict(sys.modules, {'verify_original_task_work': module}):
                with self.assertRaisesRegex(ValueError, 'Unsealed module origin'):
                    RUN.verify_origins(tree, tree/'package')

    def test_unicorn_native_loader_refuses_fallback_before_loading(self):
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory)
            (package/'lib').mkdir()
            (package/'lib/libunicorn.2.dylib').write_text('fake library')
            original_loader = RUN.ctypes.cdll.LoadLibrary
            def import_module(name):
                RUN.ctypes.cdll.LoadLibrary('/outside/libunicorn.2.dylib')
            with patch.object(RUN.importlib, 'import_module', side_effect=import_module), \
                    patch.object(RUN.ctypes.cdll, 'LoadLibrary') as load:
                with self.assertRaisesRegex(OSError, 'Unsealed Unicorn native loader'):
                    RUN.pinned_unicorn(package)
                load.assert_not_called()
            self.assertEqual(RUN.ctypes.cdll.LoadLibrary, original_loader)

    def test_unicorn_loaded_model_identity_checked(self):
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory)
            (package/'lib').mkdir()
            expected = package/'lib/libunicorn.2.dylib'
            expected.write_text('fake library')
            def imported(name):
                if name == 'unicorn':
                    RUN.ctypes.cdll.LoadLibrary(str(expected))
                    return SimpleNamespace(__version__='2.1.4')
                return SimpleNamespace(uclib=SimpleNamespace(_name=str(expected)))
            with patch.object(RUN.importlib, 'import_module', side_effect=imported), \
                    patch.object(RUN.ctypes.cdll, 'LoadLibrary') as load:
                RUN.pinned_unicorn(package)
                load.assert_called_once_with(str(expected.resolve()))

    def test_declared_sdk_cpp_search_preserves_cpp_driver_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binary = root / 'toolchain/usr/bin'
            binary.mkdir(parents=True)
            (binary / 'clang').write_text('compiler')
            compiler = binary / 'clang++'
            compiler.symlink_to('clang')
            linker = binary / 'ld'
            linker.write_text('linker')
            support = root / 'toolchain/usr/lib/libLTO.dylib'
            support.parent.mkdir(parents=True)
            support.write_text('support')
            sdk, resource = root / 'sdk', root / 'resource'
            cxx = sdk / 'usr/include/c++/v1'
            cxx.mkdir(parents=True)
            resource.mkdir()
            search = '#include <...> search starts here:\n ' + str(cxx) + '\nEnd of search list.\n'
            answers = [str(compiler), str(linker), str(sdk), str(resource)]
            with patch.object(RUN.subprocess, 'check_output', side_effect=answers), \
                    patch.object(RUN.subprocess, 'run', return_value=SimpleNamespace(returncode=0, stderr=search)):
                tools = RUN.toolchain()
            self.assertEqual(tools['compiler'], str(compiler))
            self.assertEqual(tools['cxx'], str(cxx.resolve()))
            self.assertEqual(tools['include_search'], [str(cxx.resolve())])
            self.assertEqual(tools['lto'], str(support))

    def test_build_uses_explicit_current_linker_option_and_pinned_search(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            (out / 'old').mkdir()
            tools = dict(compiler='/pinned/clang++', sdk='/pinned/sdk', resource='/pinned/resource',
                         cxx='/pinned/cxx', linker='/pinned/ld')
            with patch.object(RUN, 'OUT', out), patch.object(RUN, 'command') as command:
                RUN.build('old', out / 'tree', tools)
            self.assertEqual(command.call_count, 4)
            for call in command.call_args_list:
                argv = call.args[0]
                self.assertEqual(argv[0], '/pinned/clang++')
                self.assertIn('--ld-path=/pinned/ld', argv)
                self.assertIn('--no-default-config', argv)
                self.assertIn('/pinned/sdk', argv)
                self.assertIn('/pinned/resource', argv)
                self.assertIn('/pinned/cxx', argv)

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
            with patch.object(RUN, 'OUT', out), patch.object(RUN, 'verify_freeze', return_value={'execution_mode': 'original', 'continuation': None}):
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
            frozen = dict(before=RUN.BEFORE, candidate=RUN.CANDIDATE, execution_mode='original', continuation=None,
                          inputs={str(dependency): '0' * 64}, toolchain={})
            freeze.write_text(json.dumps(frozen))
            with patch.object(RUN, 'ROOT', root), patch.object(RUN, 'FREEZE', freeze), \
                    patch.object(RUN, 'git', return_value=freeze.read_bytes()), \
                    patch.object(RUN, 'toolchain', return_value={}), \
                    patch.object(RUN, 'frozen_inputs', return_value=frozen['inputs']), \
                    patch.object(RUN, 'root_inputs') as inspect:
                with self.assertRaisesRegex(ValueError, 'dependency changed'):
                    RUN.verify_freeze()
                inspect.assert_not_called()

    def test_changed_dependency_set_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            freeze = root / 'freeze.json'
            freeze.write_text(json.dumps(dict(before=RUN.BEFORE, candidate=RUN.CANDIDATE, inputs={}, toolchain={}, execution_mode='original', continuation=None)))
            with patch.object(RUN, 'ROOT', root), patch.object(RUN, 'FREEZE', freeze), \
                    patch.object(RUN, 'git', return_value=freeze.read_bytes()), \
                    patch.object(RUN, 'toolchain', return_value={}), \
                    patch.object(RUN, 'frozen_inputs', return_value={}), \
                    patch.object(RUN, 'root_inputs', return_value={'new': 'digest'}):
                with self.assertRaisesRegex(ValueError, 'set/interpreter/toolchain'):
                    RUN.verify_freeze()

    def test_raw_pin_map_hash_count_and_exclusive_creation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            data = root/'raw-pins.json'
            with patch.object(RUN, 'ROOT', root), patch.object(RUN, 'PIN_DATA', data):
                manifest = RUN.write_pin_manifest({'source': 'hash'})
                frozen = {'input_manifest': manifest}
                self.assertEqual(RUN.frozen_inputs(frozen), {'source': 'hash'})
                with self.assertRaises(FileExistsError):
                    RUN.write_pin_manifest({'other': 'hash'})
                with self.assertRaisesRegex(ValueError, 'cardinality'):
                    RUN.frozen_inputs({'input_manifest': dict(manifest, entries=2)})
                original = data.read_bytes()
                data.write_text('{}')
                with self.assertRaisesRegex(ValueError, 'raw pin manifest differs'):
                    RUN.frozen_inputs(frozen)
                data.write_bytes(original)
                data.unlink()
                with self.assertRaises(FileNotFoundError):
                    RUN.frozen_inputs(frozen)

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
