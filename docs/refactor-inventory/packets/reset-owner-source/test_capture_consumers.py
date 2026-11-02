"""Adequacy tests for the exact capture implementation, without native loads/cohorts."""
import ctypes
import importlib.util
import io
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
