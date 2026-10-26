"""Synthetic frame fixtures: no original images, game binaries or Metal required."""
from pathlib import Path
import sys
import tempfile
import unittest
import json
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/gate'))
from run_journeys import JOURNEYS, check_checkpoint, picture_facts, script_text, signature_distance, run
from record_gate import PROGRAM_LINE, JOURNEY_LINE
import capture_content


def log_lines(count, textured=40, skipped=()):
    return '\n'.join(f'frame {n}: guest present {n + 1}, 41 draws ({textured} textured), 1 clears'
                     + ('; not drawn: FVF 0x142' if n in skipped else '')
                     for n in range(1, count + 1))


class JourneyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.shot = Path(self.temp.name) / 'fixture.ppm'
        # Synthetic expectations isolate the pre-existing log/threshold tests.
        # They are never written to the production committed fixture.
        self.colourful()
        expectation = {'provenance': 'synthetic test only',
                       'signature': picture_facts(self.shot)['signature']}
        fixture = Path(self.temp.name) / 'synthetic-content.json'
        fixture.write_text(json.dumps({'references': {'test': expectation},
            'checkpoints': {'reference/title': 'test', 'reference/character-select': 'test'}}))
        self.content_patch = patch.object(capture_content, 'FIXTURE', fixture)
        self.content_patch.start()
        self.addCleanup(self.content_patch.stop)
        self.shot.unlink()

    def ppm(self, pixels, width=640, height=480):
        self.shot.write_bytes(f'P6\n{width} {height}\n255\n'.encode() + pixels)

    def colourful(self):
        self.ppm(bytes(c for i in range(640 * 480)
                       for c in (i % 256, (i // 256) % 256, (i // 65536) % 256)))

    def test_logo_properties_and_single_colour_break(self):
        self.ppm(bytes([0, 0, 255]) * 3000 + bytes([255, 255, 255]) * (640 * 480 - 3000))
        facts = picture_facts(self.shot)
        self.assertEqual(facts['blue'], 3000)
        self.assertGreater(facts['white_fraction'], 0.8)
        self.assertTrue(check_checkpoint(JOURNEYS[0], log_lines(120), self.shot, {})['passed'])
        self.ppm(bytes([0, 0, 0]) * (640 * 480))
        self.assertFalse(check_checkpoint(JOURNEYS[0], log_lines(120), self.shot, {})['passed'])

    def test_title_colour_texture_and_skipped_draws(self):
        self.colourful()
        self.assertTrue(check_checkpoint(JOURNEYS[1], log_lines(700), self.shot, {})['passed'])
        for log in (log_lines(700, textured=19), log_lines(700, skipped=(455,)),
                    log_lines(699), log_lines(700) + '\nframe 700: guest present 701, 41 draws (40 textured), 1 clears',
                    log_lines(700) + '\nguest stopped: broken'):
            with self.subTest(log=log[-90:]):
                self.assertFalse(check_checkpoint(JOURNEYS[1], log, self.shot, {})['passed'])

    def test_later_3d_draws_do_not_contaminate_earlier_checkpoint(self):
        self.colourful()
        text = log_lines(2000, skipped=(1900,))
        self.assertTrue(check_checkpoint(JOURNEYS[2], text, self.shot, {})['passed'])
        self.assertFalse(check_checkpoint(JOURNEYS[3], text, self.shot, {})['passed'])

    def test_missing_truncated_invalid_dimensions_and_max_value_fail(self):
        self.assertFalse(check_checkpoint(JOURNEYS[0], log_lines(120), self.shot, {})['passed'])
        for data in (b'P6\n0 480\n255\n', b'P6\n640 480\n255\nshort',
                     b'P6\n640 480\n65535\n', b'not ppm'):
            self.shot.write_bytes(data)
            self.assertFalse(check_checkpoint(JOURNEYS[0], log_lines(120), self.shot, {})['passed'])
        self.ppm(bytes([255, 255, 255]) * 4, 2, 2)
        self.assertFalse(check_checkpoint(JOURNEYS[0], log_lines(120), self.shot, {})['passed'])

    def test_golden_threshold_and_shape(self):
        self.colourful()
        goldens = {}
        self.assertTrue(check_checkpoint(JOURNEYS[1], log_lines(700), self.shot, goldens, True)['passed'])
        self.assertEqual(signature_distance(goldens['title'], goldens['title']), 0)
        goldens['title'] = [[c + 13 for c in cell] for cell in goldens['title']]
        self.assertFalse(check_checkpoint(JOURNEYS[1], log_lines(700), self.shot, goldens)['passed'])
        with self.assertRaises(ValueError):
            signature_distance([], [])

    def test_shared_route_has_each_capture_and_key_once(self):
        script = script_text(JOURNEYS, Path('build/journeys'))
        self.assertEqual(script.count('@450 press enter 5'), 1)
        for journey in JOURNEYS:
            self.assertIn(f'@{journey["frames"]} screenshot build/journeys/{journey["name"]}.ppm', script)
            self.assertIn(f'@{journey["frames"]} state build/journeys/{journey["name"]}.state.json', script)
        self.assertNotIn('@800 press', script_text(JOURNEYS[:2], Path('build/journeys')))
        frames = [int(line.split()[0][1:]) for line in script.splitlines()]
        self.assertEqual(frames, sorted(frames))

    def test_gate_recorder_recognizes_timed_and_untimed_programs(self):
        for prefix in (b'', b'/usr/bin/time -p '):
            match = PROGRAM_LINE.match(prefix + b'./build/fixture-test args')
            self.assertEqual(match[1], b'fixture-test')
        self.assertIsNone(PROGRAM_LINE.match(b'real 1.23'))
        for line, verdict in [(b'R2  title pass  title screen', b'pass'),
                              (b'R4  world-3d open  skipped draws', b'open'),
                              (b'R3  character-select FAIL  blank', b'FAIL')]:
            self.assertEqual(JOURNEY_LINE.match(line)[3], verdict)

    def test_run_uses_one_offscreen_process_and_refuses_stale_captures(self):
        from contextlib import redirect_stdout
        import io
        import json
        import subprocess
        from unittest.mock import patch
        output = Path(self.temp.name)
        target = output / 'title.ppm'
        self.colourful()
        pixels = self.shot.read_bytes()
        def simulate(command, stdout, **kwargs):
            self.assertIn('--offscreen', command)
            self.assertIn('--script', command)
            stdout.write(log_lines(700))
            target.write_bytes(pixels)
            (output / 'title.state.json').write_text(json.dumps({'frame': 700}))
            return subprocess.CompletedProcess(command, 0)
        with patch('run_journeys.subprocess.run', side_effect=simulate) as launch:
            with redirect_stdout(io.StringIO()):
                self.assertEqual(run([JOURNEYS[1]], 'fake-app', 'game', 'system', output), 0)
            self.assertEqual(launch.call_count, 1)
        # Leave the passing picture and state above in place, then simulate a run
        # that emits no captures. It must fail rather than reuse the previous run.
        with patch('run_journeys.subprocess.run', return_value=subprocess.CompletedProcess([], 0)):
            with redirect_stdout(io.StringIO()):
                self.assertEqual(run([JOURNEYS[1]], 'fake-app', 'game', 'system', output), 1)
        self.assertFalse(target.exists())

    def test_world_capture_checks_stage_and_mode(self):
        from contextlib import redirect_stdout
        import io
        import json
        import subprocess
        from unittest.mock import patch
        output = Path(self.temp.name)
        self.colourful()
        pixels = self.shot.read_bytes()
        for mode, stage, expected in [(5, 26, 0), (12, 26, 1), (5, 0, 1)]:
            def simulate(command, stdout, **kwargs):
                stdout.write(log_lines(2000))
                (output / 'world-3d.ppm').write_bytes(pixels)
                (output / 'world-3d.state.json').write_text(json.dumps(
                    {'frame': 2000, 'game_mode': mode, 'stage': stage}))
                return subprocess.CompletedProcess(command, 0)
            with patch('run_journeys.subprocess.run', side_effect=simulate):
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(run([JOURNEYS[3]], 'fake-app', 'game', 'system', output), expected)

    def test_unit_registration_is_complete(self):
        root = Path(__file__).resolve().parents[2]
        sources = {p.stem.replace('_', '-') for p in (root / 'tests/unit').iterdir()
                   if p.suffix in ('.cpp', '.mm')}
        rows = [line.split('\t')[0] for line in (root / 'tests/unit/manifest.tsv').read_text().splitlines()
                if line.strip() and not line.startswith('#')]
        self.assertEqual(len(rows), len(set(rows)), 'duplicate unit-test registration')
        self.assertEqual(set(rows), sources, 'every built unit test must run')

    def test_corpus_rule_rebuilds_after_probe_changes(self):
        import os
        import subprocess
        root = Path(__file__).resolve().parents[2]
        rule = next(line for line in (root / 'Makefile').read_text().splitlines()
                    if line.startswith('build/oracles/%.cases.bin:'))
        folder = Path(self.temp.name)
        for name in ('tools/oracles/verify_original_controller_init.py',
                     'build/native-controller-init-probe.dylib',
                     'build/oracles/controller_init.cases.bin'):
            path = folder / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('fixture')
            os.utime(path, (100, 100))
        target = folder / 'build/oracles/controller_init.cases.bin'
        os.utime(target, (200, 200))
        (folder / 'Makefile').write_text('.SECONDEXPANSION:\n' + rule +
            '\n\t$(MAKE) oracle-$(subst _,-,$*)\n'
            '.PHONY: oracle-controller-init\noracle-controller-init:\n\t@echo regenerated\n')
        def dry_run():
            return subprocess.run(['make', '-n', 'build/oracles/controller_init.cases.bin'],
                                  cwd=folder, capture_output=True, text=True, check=True).stdout
        self.assertNotIn('regenerated', dry_run())
        probe = folder / 'build/native-controller-init-probe.dylib'
        os.utime(probe, (300, 300))
        self.assertIn('regenerated', dry_run())


if __name__ == '__main__':
    unittest.main()
