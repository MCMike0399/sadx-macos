import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/import'))
import menu_voice

class Image:
    sha256 = menu_voice.SOURCE_SHA256
    def read(self, address, size):
        assert address == menu_voice.TABLE and size == 36
        return struct.pack('<9i', 0, 1, 4, 12, 13, 14, 15, 16, 17)

class MenuVoiceAssets(unittest.TestCase):
    def test_inverse_and_scope(self):
        metadata, proof = menu_voice.declarations(Image())
        self.assertTrue(proof['round_trip_exact'])
        self.assertEqual(metadata['entries'][1], {'character': 0, 'request': 1, 'voice': 1})
        self.assertEqual(len(metadata['entries']), 9)
        self.assertTrue(all(entry['character'] == 0 for entry in metadata['entries']))
        self.assertNotIn('address', str(metadata))
    def test_wrong_source(self):
        image = Image()
        image.sha256 = 'wrong'
        with self.assertRaises(ValueError):
            menu_voice.declarations(image)

if __name__ == '__main__':
    unittest.main()
