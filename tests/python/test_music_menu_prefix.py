"""Exercise the newly recovered FileSelect request without changing title fixtures."""
import struct
import unittest
from test_music_assets import Image, module


class FileSelectMusicPrefix(unittest.TestCase):
    def image(self):
        image = Image()
        image.data[module.MENU_REQUESTS] = struct.pack('<3i', 102, 101, 72)
        return image

    def test_whole_recovered_prefix_and_inverse(self):
        catalog, proof = module.read_catalog(self.image(), menu_request_count=3)
        self.assertEqual(catalog['menu_requests'], [102, 101, 72])
        request = proof['records'][-1]
        self.assertEqual(bytes.fromhex(request['bytes']), struct.pack('<3i', 102, 101, 72))
        self.assertTrue(request['inverse_exact'])
        title, _ = module.read_catalog(self.image())
        self.assertEqual(title['menu_requests'], [102])

    def test_unrecovered_prefix_is_explicit(self):
        for count in (0, 4, True, 1.0):
            with self.subTest(count=count), self.assertRaisesRegex(ValueError, 'prefix'):
                module.read_catalog(self.image(), menu_request_count=count)

    def test_file_select_request_is_validated(self):
        for song in (-1, 125):
            image = self.image()
            image.data[module.MENU_REQUESTS] = struct.pack('<3i', 102, 101, song)
            with self.subTest(song=song), self.assertRaisesRegex(ValueError, 'mapping'):
                module.read_catalog(image, menu_request_count=3)


if __name__ == '__main__':
    unittest.main()
