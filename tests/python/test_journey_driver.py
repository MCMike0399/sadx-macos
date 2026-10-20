"""Driver boundary regressions using a fake app process, with no game or Metal."""
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/journey'))
from sadx_driver import HarnessError, Session
from check_gameplay import distance, player


class Process:
    def __init__(self, output):
        self.stdin = io.StringIO()
        self.stdout = io.StringIO(output)
        self.finished = False
        self.waited = False

    def poll(self):
        return 0 if self.finished else None

    def wait(self, timeout):
        self.waited = True
        self.finished = True
        return 0


class JourneyDriverTests(unittest.TestCase):
    def test_isolated_paths_are_distinct_process_arguments(self):
        process = Process('harness: 1 state ok {"frame":1}\n')
        with tempfile.TemporaryDirectory() as temporary:
            with patch('sadx_driver.subprocess.Popen', return_value=process) as launch:
                with Session(overlay='build/new save overlay', game_dir='my game',
                             system_dir='my windows', log_path=f'{temporary}/app.log',
                             original_tasks=True) as game:
                    self.assertEqual(game.frame, 1)
                command = launch.call_args.args[0]
                self.assertEqual(command[2:4], ['my game', 'my windows'])
                self.assertEqual(command[command.index('--overlay') + 1], 'build/new save overlay')
                self.assertIn('--offscreen', command)
                self.assertIn('--original-tasks', command)
                self.assertTrue(process.waited)

    def test_guest_stop_is_reported_before_waiting_for_future_frame(self):
        process = Process('harness: 1 state ok {"frame":1}\n'
                          'frame 2: guest present 2, guest stopped: missing save API\n')
        with tempfile.TemporaryDirectory() as temporary:
            with patch('sadx_driver.subprocess.Popen', return_value=process) as launch:
                with Session(log_path=f'{temporary}/app.log') as game:
                    with self.assertRaisesRegex(HarnessError, 'missing save API'):
                        game.advance(150)
                self.assertNotIn('--original-tasks', launch.call_args.args[0])
                self.assertIn('missing save API', Path(temporary, 'app.log').read_text())

    def test_failed_boot_cleans_up_app_process(self):
        process = Process('no Metal device\n')
        with tempfile.TemporaryDirectory() as temporary:
            with patch('sadx_driver.subprocess.Popen', return_value=process):
                with self.assertRaisesRegex(HarnessError, 'exited while waiting'):
                    Session(log_path=f'{temporary}/app.log')
        self.assertTrue(process.waited)

    def test_observations_reject_unavailable_or_nonfinite_vectors(self):
        for invalid in (None, [0, 1], [0, float('nan'), 0], [0, 'inf', 0]):
            with self.subTest(invalid=invalid):
                with self.assertRaises(HarnessError):
                    distance(invalid, [0, 0, 0])
        self.assertEqual(distance([0, 0, 0], [3, 4, 0]), 5)
        for state in ({'player': None}, {'player': {'character': 1}}):
            with self.assertRaisesRegex(HarnessError, 'Sonic is absent'):
                player(state)


if __name__ == '__main__':
    unittest.main()
