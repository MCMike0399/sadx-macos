"""Census summaries preserve unknown coverage and reject changed replay evidence."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/analysis'))
from run_fp_census import parity, player_targets, summarize


class FpCensusTests(unittest.TestCase):
    def site(self, samples):
        return dict(address=0x44c270, name='PGetAcceleration', bound=False,
                    samples=samples, missing_return=0)

    def test_unobserved_is_unknown(self):
        row = summarize(self.site([]))
        self.assertEqual(row['calls'], 0)
        self.assertIsNone(row['observed_cw_precondition'])
        self.assertEqual(row['fpcw'], [])

    def test_control_words_counts_and_mxcsr_status(self):
        row = summarize(self.site([[0x7f, 0x1f80, 0x401000, 2], [0x7f, 0x1fa0, 0x401000, 3]]))
        self.assertEqual(row['calls'], 5)
        self.assertTrue(row['observed_cw_precondition'])
        self.assertEqual(row['mxcsr_control'], ['0x1f80'])
        self.assertEqual(row['mxcsr'], ['0x1f80', '0x1fa0'])
        self.assertFalse(summarize(self.site([[0x27f, 0x1f80, 0x401000, 1]]))['observed_cw_precondition'])
        self.assertFalse(summarize(self.site([[0x7f, 0x1f80, 0, 1], [0x37f, 0x1f80, 0, 1]]))['observed_cw_precondition'])

    def test_pre_device_samples_are_retained_without_blocking_post_device_evidence(self):
        row = summarize(self.site([[0x23f, 0x1f80, 0x401000, 5, 0],
                                   [0x7f, 0x1fa0, 0x402000, 3, 1]]))
        self.assertEqual(row['calls'], 8)
        self.assertEqual(row['fpcw'], ['0x7f', '0x23f'])
        self.assertFalse(row['observed_cw_precondition'])
        self.assertEqual(row['post_device_calls'], 3)
        self.assertEqual(row['post_device_fpcw'], ['0x7f'])
        self.assertTrue(row['post_device_cw_precondition'])
        self.assertFalse(row['samples'][0]['device_created'])
        self.assertTrue(row['samples'][1]['device_created'])
        wrong = summarize(self.site([[0x23f, 0x1f80, 0x401000, 5, 0],
                                     [0x27f, 0x1f80, 0x402000, 3, 1]]))
        self.assertFalse(wrong['post_device_cw_precondition'])
        self.assertIsNone(summarize(self.site([]))['post_device_cw_precondition'])

    def test_replay_digests_clocks_modes_and_stop_all_matter(self):
        original = dict(digests=[1, 2, 3], clocks=[10, 20, 30], modes=[[1, 8]],
                        frames=3, stop='census frames done')
        self.assertTrue(parity(original, original)['passed'])
        for key, value in [('digests', [1, 5, 3]), ('clocks', [10, 20, 31]),
                           ('modes', [[1, 12]]), ('stop', 'missing source fact')]:
            with self.subTest(key=key):
                changed = {**original, key: value}
                self.assertFalse(parity(original, changed)['passed'])
        self.assertEqual(parity(original, {**original, 'digests': [1, 5, 3]})['first_digest_mismatch'], 2)

    def test_primary_physics_entries_are_in_conservative_inventory(self):
        targets = player_targets()
        for at in ['0x44c270', '0x443f50', '0x43bdc0', '0x43f2a0', '0x44cdf0']:
            with self.subTest(at=at):
                self.assertIn(at, targets)
                self.assertTrue(targets[at]['evidence_paths'])


if __name__ == '__main__':
    unittest.main()
