"""Optional offline replay of qualified originals; never launches an application.

Set SADX_CONTENT_CORPUS to the checkout holding the ignored, completed captures.
Source hashes are checked before use. The default suite explicitly skips this
asset-dependent replay; portable synthetic/state tests live in test_capture_content.
"""
from contextlib import redirect_stdout
import ast
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/journey'))
sys.path.insert(0, str(ROOT / 'tools/gate'))
import capture_content as content
import run_journeys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def checker(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / f'tools/journey/check_native_{name}.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@unittest.skipUnless(os.environ.get('SADX_CONTENT_CORPUS'), 'qualified local raster corpus not supplied')
class QualifiedContentOffline(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = Path(os.environ['SADX_CONTENT_CORPUS'])
        cls.evidence = json.loads((ROOT / 'reference/frozen-corpora/capture-content-evidence.json').read_text())
        for relative, expected in cls.evidence['completion_final']['offline_replay_artifacts_sha256'].items():
            if digest(cls.source / relative) != expected:
                raise AssertionError(f'qualified replay artifact changed: {relative}')
        cls.fixture = json.loads(content.FIXTURE.read_text())
        cls.results = []
        cls.header = b'P6\n640 480\n255\n'
        cls.mutants = {
            'black': cls.header + bytes(921600),
            'white': cls.header + b'\xff' * 921600,
            'colour': cls.header + b'\x12\x34\x56' * (640 * 480),
            'unrelated': cls.header + bytes(channel for i in range(640 * 480)
                                          for channel in (i % 256, (i // 256) % 256, (i // 65536) % 256)),
        }

    @classmethod
    def tearDownClass(cls):
        output = ROOT / 'build/content-completion/offline-results.json'
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(cls.results, indent=2) + '\n')

    def verified(self, relative, expected):
        path = self.source / relative
        self.assertEqual(digest(path), expected, str(path))
        return path

    def test_all_50_original_native_rasters_and_200_mutants(self):
        rows = self.evidence['completed_native_content']
        self.assertEqual(len(rows), 50)
        with tempfile.TemporaryDirectory() as temporary:
            image = Path(temporary) / 'mutant.ppm'
            for row in rows:
                key = row['checkpoint']
                path = self.verified(row['capture'], row['capture_sha256'])
                with self.subTest(checkpoint=key):
                    content.check_content(path, key)
                    self.results.append({'checkpoint': key, 'mutation': 'original', 'passed': True})
                    for name, raster in self.mutants.items():
                        image.write_bytes(raster)
                        with self.assertRaisesRegex(ValueError, 'full-frame blank|reference content'):
                            content.check_content(image, key)
                        self.results.append({'checkpoint': key, 'mutation': name, 'rejected': True})

    def test_return_reference_provenance_and_fixed_frame(self):
        for row in self.evidence['completion_final']['return_transitions']:
            provenance = row['provenance']
            for key in ('capture', 'state', 'log', 'script', 'input_metadata', 'application'):
                self.verified(provenance[key], provenance[key + '_sha256'])
            self.assertEqual(row['expectation_frame'], 1450)
            self.assertEqual(row['ceiling'], 12)
            self.assertEqual(row['diagnostic_only']['source_frame'], 1451)
            self.assertTrue(row['diagnostic_only']['byte_exact_to_native1450'])
            reference = self.fixture['references'][self.fixture['checkpoints'][row['checkpoint']]]
            source = self.source / provenance['capture']
            self.assertEqual(content.coarse_signature(content.read_capture(source)), reference['signature'])
            metadata = json.loads((self.source / provenance['input_metadata']).read_text())
            constants = [node.value for node in ast.walk(ast.parse(
                (ROOT / metadata['native_checker']).read_text()))
                if isinstance(node, ast.Constant) and isinstance(node.value, str)]
            self.assertIn(metadata['native_keys'], constants)
            expected = [f'@{int(frame) - 1} {kind} {key}'
                        for frame, key, kind in (event.split(':') for event in metadata['native_keys'].split(','))
                        if int(frame) - 1 <= 1470]
            observed = [line for line in (self.source / provenance['script']).read_text().splitlines()
                        if len(line.split()) == 3 and line.split()[1] in ('down', 'up')]
            self.assertEqual(observed, expected)
            state = json.loads((self.source / provenance['state']).read_text())
            self.assertEqual((state['game_mode'], state['game_state'], state['player'], state['camera']),
                             (12, 21, None, None))

    def test_native_full_checkers_single_return_raster_mutants(self):
        # Only subprocess delivery is replaced. All captures/states/logs and real
        # seed-save bytes survive, except the single target return-frame raster.
        for name, directory in (('file_select', 'native-file-select-cvya2gsi'),
                                ('main_menu', 'native-main-menu-dor_txwa')):
            module = checker(name)
            source = self.source / 'build' / directory
            self.assertTrue(json.loads((source / 'result.json').read_text())['passed'])
            for mutation in ('original', *self.mutants):
                with self.subTest(checker=name, mutation=mutation), tempfile.TemporaryDirectory() as temporary:
                    output = Path(temporary) / 'output'
                    def deliver(command, stdout, **kwargs):
                        destination = Path(command[command.index('--capture-directory') + 1])
                        shutil.copytree(source / 'captures', destination)
                        if mutation != 'original':
                            (destination / '1450.ppm').write_bytes(self.mutants[mutation])
                        profile = Path(command[command.index('--save-directory') + 1])
                        shutil.copytree(self.source / 'game/imported/saves', profile)
                        stdout.write((source / 'application.log').read_text())
                        return subprocess.CompletedProcess(command, 0)
                    with patch.object(module.subprocess, 'run', deliver), redirect_stdout(io.StringIO()):
                        if mutation == 'original':
                            module.run(self.source / 'build/sadx-native', self.source / 'game/imported', output)
                            self.assertTrue(json.loads((output / 'result.json').read_text())['passed'])
                        else:
                            with self.assertRaisesRegex(RuntimeError, 'full-frame blank|reference content'):
                                module.run(self.source / 'build/sadx-native', self.source / 'game/imported', output)
                            self.assertFalse((output / 'result.json').exists())
                    self.results.append({'checker': name, 'target_frame': 1450, 'mutation': mutation,
                                         'accepted': mutation == 'original', 'scope': 'full checker offline replay'})

    def test_fresh_save_four_plays_with_originals_and_raster_mutants(self):
        module = checker('fresh_save')
        source = self.source / 'build/native-fresh-save-fpe02hag'
        self.assertTrue(json.loads((source / 'result.json').read_text())['passed'])
        for route in ('create', 'restart', 'create-empty', 'restart-empty'):
            captures = source / route / 'captures'
            frames = sorted(int(p.stem) for p in captures.glob('*.ppm'))
            for mutation in ('original', *self.mutants):
                with self.subTest(route=route, mutation=mutation), tempfile.TemporaryDirectory() as temporary:
                    def deliver(command, stdout, **kwargs):
                        destination = Path(command[command.index('--capture-directory') + 1])
                        shutil.copytree(captures, destination)
                        if mutation != 'original':
                            (destination / f'{frames[0]}.ppm').write_bytes(self.mutants[mutation])
                        stdout.write((source / route / 'application.log').read_text())
                        return subprocess.CompletedProcess(command, 0)
                    with patch.object(module.subprocess, 'run', deliver):
                        args = (Path('offline-app'), Path('assets'), Path('profile'),
                                Path(temporary) / route, frames[-1], frames, '')
                        if mutation == 'original':
                            module.play(*args)
                        else:
                            with self.assertRaisesRegex(RuntimeError, 'full-frame blank|reference content'):
                                module.play(*args)
                    self.results.append({'route': route, 'mutation': mutation, 'accepted': mutation == 'original',
                                         'scope': 'play() only; no fabricated save persistence'})

    def test_reference_R2_R3_full_run_with_originals_and_raster_mutants(self):
        replay = json.loads((ROOT / 'reference/frozen-corpora/capture-content-reference-replay.json').read_text())
        provenance = replay['provenance']
        # The report is provenance, not an input to run(); Root has since
        # refreshed it. Keep its old hash and verify every replay input unchanged.
        for key in ('log', 'script'):
            self.verified(provenance[key], provenance[key + '_sha256'])
        log = (self.source / provenance['log']).read_text()
        for journey in run_journeys.JOURNEYS[1:3]:
            name = journey['name']
            reference = self.fixture['references'][name]['provenance']
            image = self.verified(reference['capture'], reference['capture_sha256'])
            state = self.verified(reference['state'], reference['state_sha256'])
            for mutation in ('original', *self.mutants):
                with self.subTest(journey=name, mutation=mutation), tempfile.TemporaryDirectory() as temporary:
                    output = Path(temporary) / name
                    def deliver(command, stdout, **kwargs):
                        stdout.write(log)
                        (output / f'{name}.ppm').write_bytes(image.read_bytes() if mutation == 'original'
                                                           else self.mutants[mutation])
                        shutil.copyfile(state, output / f'{name}.state.json')
                        return subprocess.CompletedProcess(command, 0)
                    with patch.object(run_journeys.subprocess, 'run', deliver), redirect_stdout(io.StringIO()):
                        status = run_journeys.run([journey], 'offline-app', 'game', 'system', output)
                    self.assertEqual(status, 0 if mutation == 'original' else 1)
                    report = json.loads((output / 'report.json').read_text())
                    self.assertFalse(report['run_problems'])
                    self.assertEqual(report['journeys'][0]['game_state'], json.loads(state.read_text()))
                    self.results.append({'reference': name, 'mutation': mutation, 'exit_status': status,
                                         'scope': 'full run() offline replay; only raster mutated'})
