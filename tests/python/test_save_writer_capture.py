"""Writer cohort structure must be complete before any instruction comparison."""
import importlib.util
from pathlib import Path
import unittest

PARSER = Path(__file__).resolve().parents[2] / 'reference/packets/save-io-frame-judges/writer_capture.py'
spec = importlib.util.spec_from_file_location('sadx_writer_capture', PARSER)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def case(index):
    return (f'writer:{index}\nreturn:0\n99:' + ','.join(['0'] * 14) + '\n' +
            '00' * 1392 + '\n' + '0,' * 14 + '\n')


class WriterCaptureTests(unittest.TestCase):
    def test_complete_ordered_cohort(self):
        rows = module.native_cases('unrelated-prefix\n' + ''.join(case(i) for i in range(39)) + 'list:later\n')
        self.assertEqual(len(rows), 39)
        self.assertEqual(rows[0]['snapshots'], [(99, [0] * 14, '00' * 1392)])

    def test_missing_duplicate_reordered_extra_and_malformed_ids(self):
        cohorts = [[], list(range(38)), [0] + list(range(1, 38)) + [37],
                   [1, 0] + list(range(2, 39)), list(range(40))]
        for ids in cohorts:
            with self.subTest(ids=ids), self.assertRaises(ValueError):
                module.native_cases(''.join(case(i) for i in ids))
        with self.assertRaises(ValueError):
            module.native_cases(''.join(case(i) for i in range(39)) + 'writer:bad\n')

    def test_terminal_snapshot_and_payload_are_required(self):
        prefix = ''.join(case(i) for i in range(38))
        last = case(38)
        variants = [
            'writer:38\nreturn:0\n',
            last.split('99:')[0],
            last.rsplit('\n', 2)[0],
            last.replace('00' * 1392, '00' * 1391),
            last.replace('00' * 1392, 'gg' * 1392),
            last.replace('99:' + ','.join(['0'] * 14), '99:0,0'),
            last.replace('return:0\n', ''),
            last + last[last.index('99:'):],
        ]
        for value in variants:
            with self.subTest(length=len(value)), self.assertRaises(ValueError):
                module.native_cases(prefix + value)


if __name__ == '__main__':
    unittest.main()
