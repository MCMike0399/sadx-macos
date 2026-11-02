"""Synthetic source declarations test the dedicated importer independently."""
import importlib.util
from pathlib import Path
import struct
import unittest

PATH = Path(__file__).resolve().parents[2] / 'tools/import/gamma_setup_resource.py'
spec = importlib.util.spec_from_file_location('gamma_setup_import', PATH)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class Image:
    sha256 = module.SOURCE_SHA

    def __init__(self):
        self.ranges = {
            0x9820f8: struct.pack('<5I', 0x91c570, 0x982058, 8, 8, 10) + bytes(range(60)),
            0x982058: bytes(range(160)),
            0x91c570: struct.pack('<II', 0x3c53ed8, 12),
            0x982148: struct.pack('<I', 0x9820f8),
        }

    def read(self, address, size):
        result = self.ranges[address]
        assert len(result) == size
        return result


class GammaSetupImportTests(unittest.TestCase):
    def test_inverse_preserves_all_scalar_and_frame_bytes(self):
        image = Image()
        packed, proof = module.build(image)
        self.assertEqual(len(packed), 244)
        self.assertEqual(packed[12:84], image.ranges[0x9820f8][8:])
        self.assertEqual(packed[84:], image.ranges[0x982058])
        self.assertTrue(proof['inverse_exact'])
        self.assertEqual(len(proof['source_ranges']), 4)

    def test_rejects_wrong_image(self):
        image = Image()
        image.sha256 = 'wrong'
        with self.assertRaisesRegex(ValueError, 'source image'):
            module.build(image)

    def test_rejects_every_changed_definition_header_word(self):
        for offset in range(0, 20, 4):
            image = Image()
            payload = bytearray(image.ranges[0x9820f8])
            payload[offset] ^= 1
            image.ranges[0x9820f8] = bytes(payload)
            with self.subTest(offset=offset), self.assertRaisesRegex(ValueError, 'declaration'):
                module.build(image)

    def test_rejects_changed_input_pointer(self):
        image = Image()
        image.ranges[0x982148] = bytes(4)
        with self.assertRaisesRegex(ValueError, 'declaration'):
            module.build(image)

    def test_rejects_texture_identity_or_count_change(self):
        for word in (0, 1):
            image = Image()
            payload = bytearray(image.ranges[0x91c570])
            payload[word * 4] ^= 1
            image.ranges[0x91c570] = bytes(payload)
            with self.subTest(word=word), self.assertRaisesRegex(ValueError, 'texture identity'):
                module.build(image)


if __name__ == '__main__':
    unittest.main()
