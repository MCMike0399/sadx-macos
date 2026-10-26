"""Content checks with genuine native states and independently sourced expectations.

No game imagery is committed. Synthetic RGB tests exercise the helper; captured
states come from completed successful runs and are never image expectations.
"""
import importlib.util
import json
from pathlib import Path
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


def load_checker(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / f'tools/journey/check_native_{name}.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class CaptureContentTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.folder = Path(temporary.name)
        self.image = self.folder / 'capture.ppm'
        self.fixture = json.loads(content.FIXTURE.read_text())
        self.recording = json.loads((ROOT / 'reference/frozen-corpora/capture-content-state-replay.json').read_text())

    def write_image(self, pixels):
        self.image.write_bytes(b'P6\n640 480\n255\n' + pixels)

    def unrelated(self):
        return bytes(channel for i in range(640 * 480)
                     for channel in (i % 256, (i // 256) % 256, (i // 65536) % 256))

    def test_all_captured_checkpoints_have_qualified_expectations(self):
        main = json.loads((ROOT / 'reference/frozen-corpora/native-main-menu-state-replay.json').read_text())
        expected = (set(self.recording['states']) |
                    {f'native-main-menu/{frame}' for frame in main['states']} |
                    {'reference/title', 'reference/character-select'})
        self.assertEqual(set(self.fixture['checkpoints']), expected)
        self.assertFalse(self.fixture['blockers'])
        for key in expected:
            reference = self.fixture['references'][self.fixture['checkpoints'][key]]
            self.assertTrue(reference['provenance'])
        for key in ('native-file-select/1450', 'native-main-menu/1450'):
            reference = self.fixture['references'][self.fixture['checkpoints'][key]]
            self.assertEqual(reference['provenance']['frame'], 1450)
            self.assertEqual(len(reference['signature']), 768)

    def test_all_genuine_native_frames_reject_draw_suppression(self):
        # Black, white and a coloured clear all retain valid PPM headers/payloads.
        main = json.loads((ROOT / 'reference/frozen-corpora/native-main-menu-state-replay.json').read_text())
        keys = set(self.recording['states']) | {f'native-main-menu/{f}' for f in main['states']}
        self.assertEqual(len(keys), 50)
        self.assertEqual(keys, {key for key in self.fixture['checkpoints']
                                if key.startswith('native-')} |
                         {key for key in self.fixture['blockers'] if key.startswith('native-')})
        for rgb in (b'\0\0\0', b'\xff\xff\xff', b'\x12\x34\x56'):
            self.write_image(rgb * (640 * 480))
            for key in sorted(keys):
                with self.subTest(checkpoint=key, clear=rgb):
                    with self.assertRaisesRegex(ValueError, 'full-frame blank'):
                        content.check_content(self.image, key)

    def test_rich_unrelated_image_rejected_at_every_qualified_checkpoint(self):
        self.write_image(self.unrelated())
        for key in self.fixture['checkpoints']:
            with self.subTest(checkpoint=key):
                with self.assertRaisesRegex(ValueError, 'reference content (region|signature)'):
                    content.check_content(self.image, key)

    def test_missing_reference_is_explicit_and_not_blessable(self):
        self.write_image(self.unrelated())
        for key in self.fixture['blockers']:
            with self.subTest(checkpoint=key):
                with self.assertRaisesRegex(ValueError, 'missing qualified reference'):
                    content.check_content(self.image, key)
        # Remove qualified keys only in a disposable fixture to prove local
        # goldens and --bless still cannot replace committed provenance.
        fixture = self.folder / 'missing-reference.json'
        fixture.write_text(json.dumps({'checkpoints': {}, 'references': {}}))
        replay = json.loads((ROOT / 'reference/frozen-corpora/'
                             'capture-content-reference-replay.json').read_text())
        log = '\n'.join(replay['frame_lines'])
        with patch.object(content, 'FIXTURE', fixture):
            for journey in run_journeys.JOURNEYS[1:3]:
                for bless in (False, True):
                    goldens = {journey['name']: run_journeys.picture_facts(self.image)['signature']}
                    result = run_journeys.check_checkpoint(journey, log, self.image, goldens, bless)
                    self.assertFalse(result['passed'])
                    self.assertTrue(any('missing qualified reference' in p for p in result['problems']))

    def test_malformed_truncated_and_trailing_payloads_rejected(self):
        for data in (b'P6\n640 480\n255\n', b'P6\n640 480\n255\nshort',
                     b'P6\n640 480\n65535\n' + bytes(640 * 480 * 3),
                     b'P6\n640 480\n255\n' + bytes(640 * 480 * 3 + 1)):
            self.image.write_bytes(data)
            with self.assertRaisesRegex(ValueError, 'malformed'):
                content.check_content(self.image, 'native-main-menu/1250')

    def test_committed_reference_signature_positive_and_threshold(self):
        # Render the coarse cells themselves, not copyrighted reference imagery.
        cells = self.fixture['references']['main-menu']['signature']
        pixels = bytes(cells[(y // 20) * 32 + x // 20][channel]
                       for y in range(480) for x in range(640) for channel in range(3))
        self.write_image(pixels)
        self.assertEqual(content.coarse_signature(pixels),
                         run_journeys.picture_facts(self.image)['signature'])
        content.check_content(self.image, 'native-main-menu/1250')
        self.write_image(bytes(min(value + 30, 255) for value in pixels))
        with self.assertRaisesRegex(ValueError, 'signature distance'):
            content.check_content(self.image, 'native-main-menu/1250')

    def test_exact_region_and_provenance_requirements(self):
        import hashlib
        pixels = self.unrelated()
        self.write_image(pixels)
        region = {'name': 'synthetic', 'rectangle': [0, 0, 2, 1],
                  'rgb_sha256': hashlib.sha256(pixels[:6]).hexdigest()}
        fixture = self.folder / 'synthetic.json'
        reference = {'provenance': 'synthetic test only', 'regions': [region]}
        def save():
            fixture.write_text(json.dumps({'checkpoints': {'test': 'test'},
                                           'references': {'test': reference}}))
        save()
        with patch.object(content, 'FIXTURE', fixture):
            content.check_content(self.image, 'test')
            self.write_image(bytes([pixels[0] ^ 1]) + pixels[1:])
            with self.assertRaisesRegex(ValueError, 'reference content region'):
                content.check_content(self.image, 'test')
            reference.pop('provenance')
            save()
            with self.assertRaisesRegex(ValueError, 'no reference provenance'):
                content.check_content(self.image, 'test')
            reference.update(provenance='synthetic test only', regions=[])
            save()
            with self.assertRaisesRegex(ValueError, 'empty reference'):
                content.check_content(self.image, 'test')

    def test_reference_raster_suppression_with_genuine_log_and_states(self):
        from contextlib import redirect_stdout
        import io
        replay = json.loads((ROOT / 'reference/frozen-corpora/'
                             'capture-content-reference-replay.json').read_text())
        log = '\n'.join(replay['frame_lines']) + '\n'
        self.assertEqual(len(run_journeys.log_facts(log)['frames']), 1700)
        for journey in run_journeys.JOURNEYS[1:3]:
            name = journey['name']
            state = replay['states'][name]['state']
            self.assertEqual(state['frame'], journey['frames'])
            output = self.folder / name

            def suppressed(command, stdout, **kwargs):
                # Full real run() path: only raster delivery is suppressed.
                # Captured state, log lines, script and successful exit survive.
                stdout.write(log)
                (output / (name + '.ppm')).write_bytes(
                    b'P6\n640 480\n255\n' + bytes(921600))
                (output / (name + '.state.json')).write_text(json.dumps(state))
                return subprocess.CompletedProcess(command, 0)

            with self.subTest(journey=name), patch.object(
                    run_journeys.subprocess, 'run', suppressed):
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(run_journeys.run([journey], 'fake-app', 'game',
                                                     'system', output), 1)
                report = json.loads((output / 'report.json').read_text())
                result = report['journeys'][0]
                self.assertEqual(result['game_state'], state)
                self.assertFalse(report['run_problems'])
                self.assertTrue(any('full-frame blank' in p for p in result['problems']))
                self.assertEqual((output / 'route.log').read_text(), log)
                self.assertEqual((output / 'route.txt').read_text(),
                                 run_journeys.script_text([journey], output))

    def test_file_select_run_rejects_suppressed_genuine_state_replay(self):
        checker = load_checker('file_select')
        assets = self.folder / 'assets'
        (assets / 'saves').mkdir(parents=True)

        def suppressed(command, **kwargs):
            captures = Path(command[command.index('--capture-directory') + 1])
            captures.mkdir()
            for frame in command[command.index('--capture-at') + 1].split(','):
                state = self.recording['states'][f'native-file-select/{frame}']['state']
                (captures / f'{frame}.json').write_text(json.dumps(state))
                (captures / f'{frame}.ppm').write_bytes(b'P6\n640 480\n255\n' + bytes(921600))
            return subprocess.CompletedProcess(command, 0)

        with patch.object(checker.subprocess, 'run', suppressed):
            with self.assertRaisesRegex(RuntimeError, 'full-frame blank'):
                checker.run(self.folder / 'fake-app', assets, self.folder / 'output')
        self.assertFalse((self.folder / 'output/result.json').exists())

    def test_fresh_save_each_play_rejects_suppressed_genuine_states(self):
        checker = load_checker('fresh_save')
        for route in ('create', 'restart', 'create-empty', 'restart-empty'):
            states = {key.rsplit('/', 1)[1]: row['state'] for key, row in self.recording['states'].items()
                      if key.startswith(f'native-fresh-save/{route}/')}

            def suppressed(command, **kwargs):
                captures = Path(command[command.index('--capture-directory') + 1])
                captures.mkdir()
                for frame, state in states.items():
                    (captures / f'{frame}.json').write_text(json.dumps(state))
                    (captures / f'{frame}.ppm').write_bytes(b'P6\n640 480\n255\n' + bytes(921600))
                return subprocess.CompletedProcess(command, 0)

            with self.subTest(route=route), patch.object(checker.subprocess, 'run', suppressed):
                with self.assertRaisesRegex(RuntimeError, 'full-frame blank'):
                    checker.play(self.folder / 'fake-app', self.folder, self.folder,
                                 self.folder / route, 2150, tuple(map(int, states)), '')


if __name__ == '__main__':
    unittest.main()
