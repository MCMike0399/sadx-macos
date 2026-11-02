"""Adequacy tests for the exact capture implementation, without native loads/cohorts."""
import ctypes
import importlib.util
import io
import hashlib
import tempfile
import json
from pathlib import Path
import struct
import sys
import unittest

spec = importlib.util.spec_from_file_location('capture', Path(__file__).with_name('capture_consumers.py'))
capture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(capture)


def records(data):
    result = []
    while data:
        if len(data) < 4:
            raise ValueError('truncated length')
        size = struct.unpack('<I', data[:4])[0]
        if len(data) < 4 + size:
            raise ValueError('truncated body')
        result.append(json.loads(data[4:4 + size]))
        data = data[4 + size:]
    if [r['ordinal'] for r in result] != list(range(len(result))):
        raise ValueError('reordered or missing call')
    return result


class CaptureTests(unittest.TestCase):
    def run_fake(self, count, capacity, payload=b'ini', extent=2):
        stream = io.BytesIO()
        state = {'stream': stream, 'count': 0}
        def fake(*args):
            return 1
        wrapped = capture.Function(fake, state)
        a, b = (ctypes.c_uint32 * extent)(1, 2), (ctypes.c_uint32 * 2)()
        wrapped(a, count, b, capacity, payload)
        wrapped(a, count, b, capacity, payload)
        return stream.getvalue()

    def test_scalars_bytes_extents_and_identity(self):
        baseline = self.run_fake(2, 2)
        for variant in (self.run_fake(999, 2), self.run_fake(2, 1),
                        self.run_fake(2, 2, b'other'), self.run_fake(2, 2, extent=3)):
            self.assertNotEqual(baseline, variant)
        parsed = records(baseline)
        self.assertEqual(len(parsed), 2)
        self.assertEqual(parsed[0]['argument_count'], 5)
        self.assertEqual(parsed[0]['inputs'][0]['bytes'], 8)
        self.assertEqual(parsed[0]['function'], 'fake')

    def test_real_trace_relative_filename(self):
        origin = 'build/reset-fake-observer.py'
        source = "def main():\n n=0; e=[1]; st={'trace':['original']}; got_cmp=[1]\n d=[i for i in range(256)]\nmain()\n"
        # compile uses a relative filename; the observer binds an absolute module origin.
        seen = []
        sys.settrace(capture.observation_trace(str(Path(origin).resolve()), 3, seen.append))
        try:
            exec(compile(source, origin, 'exec'), {})
        finally:
            sys.settrace(None)
        self.assertEqual(seen, [{'case': 0, 'source': [1], 'source_trace': ['original'],
                                 'native_comparison': [1]}])

    def test_stream_inline_trace(self):
        origin = 'build/reset-fake-stream.py'
        source = "def main():\n e=[0,123]; got=[0,123]\n got=got\nmain()\n"
        seen = []
        sys.settrace(capture.observation_trace(origin, 3, seen.append, 'stream_players'))
        try:
            exec(compile(source, str(Path(origin).resolve()), 'exec'), {})
        finally:
            sys.settrace(None)
        self.assertEqual(seen, [{'source': [0, 123], 'native_comparison': [0, 123]}])

    def test_exclusive_destination(self):
        with tempfile.TemporaryDirectory() as directory:
            destination = Path(directory) / 'capture'
            capture.new_destination(destination)
            (destination / 'partial').write_text('retained')
            with self.assertRaises(FileExistsError):
                capture.new_destination(destination)
            self.assertEqual((destination / 'partial').read_text(), 'retained')

    def test_independent_nonempty_domain(self):
        empty_digest = hashlib.sha256()
        contract = {'native_calls': 2, 'original_observations': 2, 'symbols': {'one': 1, 'two': 1},
                    'symbol_sequence_sha256': hashlib.sha256(b'one\ntwo').hexdigest(),
                    'inputs_sha256': empty_digest.hexdigest()}
        valid = {'count': 2, 'sequence': ['one', 'two'], 'inputs': empty_digest}
        capture.validate_domain(valid, [0, 1], contract)
        for count, sequence, observed in [(0, [], []), (1, ['one'], [0]),
                                         (2, ['two', 'one'], [0, 1]),
                                         (2, ['one', 'two'], [0, 0]),
                                         (2, ['one', 'two'], [1, 0]),
                                         (2, ['one', 'two'], [0])]:
            with self.assertRaises(ValueError):
                capture.validate_domain({'count': count, 'sequence': sequence,
                                         'inputs': empty_digest}, observed, contract)
        with self.assertRaises(ValueError):
            capture.validate_domain(valid, [0, 1], dict(contract, inputs_sha256='wrong'))
        with self.assertRaises(ValueError):
            capture.validate_domain({'count': 0, 'sequence': [], 'inputs': empty_digest}, [],
                                    dict(contract, native_calls=0, original_observations=0))

    def test_argument_schema_rejects_wrong_symbol_type_and_scalar(self):
        contract = {'signatures': {'fake': [{'kind': 'integer', 'value': 2}]}}
        capture.validate_arguments('fake', [{'kind': 'integer', 'value': 2}], contract)
        for name, arguments in [('other', [{'kind': 'integer', 'value': 2}]),
                                ('fake', []), ('fake', [{'kind': 'integer', 'value': 3}]),
                                ('fake', [{'kind': 'bytes', 'value': 2}])]:
            with self.assertRaises(ValueError):
                capture.validate_arguments(name, arguments, contract)

    def test_truncated_reordered_and_unknown_fail_closed(self):
        data = self.run_fake(2, 2)
        with self.assertRaises(ValueError):
            records(data[:-1])
        boundary = 4 + struct.unpack('<I', data[:4])[0]
        with self.assertRaises(ValueError):
            records(data[boundary:] + data[:boundary])
        with self.assertRaises(TypeError):
            capture.argument(object())


if __name__ == '__main__':
    unittest.main()
