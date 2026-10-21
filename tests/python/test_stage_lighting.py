import importlib.util
from pathlib import Path
import random
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('stage_lighting', ROOT / 'tools/import/stage_lighting.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class StageLightingImport(unittest.TestCase):
    def test_float_word_roundtrip(self):
        randomizer = random.Random(20261020)
        for palette, count in ((False, 11), (True, 23)):
            for _ in range(100):
                words = [randomizer.getrandbits(32) for _ in range(count)]
                words = [word if word & 0x7f800000 != 0x7f800000 else 0 for word in words]
                raw = bytes(randomizer.getrandbits(8) for _ in range(4)) + struct.pack('<' + 'I' * count, *words)
                self.assertEqual(module.encode(module.decode(raw, palette), palette), raw)

    def test_nonfinite_rejected(self):
        raw = bytes(4) + struct.pack('<I', 0x7fc00001) + bytes(40)
        with self.assertRaises(ValueError):
            module.decode(raw)

    def test_wrong_record_length_rejected(self):
        with self.assertRaises(ValueError):
            module.decode(bytes(47))

    def test_negative_zero_retained(self):
        raw = bytes(4) + struct.pack('<I', 0x80000000) + bytes(40)
        self.assertEqual(module.encode(module.decode(raw)), raw)


if __name__ == '__main__':
    unittest.main()
