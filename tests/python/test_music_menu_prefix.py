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
        for count in (0, 6, True, 1.0):
            with self.subTest(count=count), self.assertRaisesRegex(ValueError, 'prefix'):
                module.read_catalog(self.image(), menu_request_count=count)

    def test_file_select_request_is_validated(self):
        for song in (-1, 125):
            image = self.image()
            image.data[module.MENU_REQUESTS] = struct.pack('<3i', 102, 101, song)
            with self.subTest(song=song), self.assertRaisesRegex(ValueError, 'mapping'):
                module.read_catalog(image, menu_request_count=3)


class CharacterSelectMusicPrefix(unittest.TestCase):
    def image(self):
        image = Image()
        image.data[module.MENU_REQUESTS] = struct.pack('<5i', 102, 101, 72, 24, 103)
        return image

    def test_all_recovered_prefixes_and_exact_inverse(self):
        requests = [102, 101, 72, 24, 103]
        for count in range(1, 6):
            with self.subTest(count=count):
                catalog, proof = module.read_catalog(self.image(), menu_request_count=count)
                self.assertEqual(catalog['menu_requests'], requests[:count])
                self.assertEqual(bytes.fromhex(proof['records'][-1]['bytes']),
                                 struct.pack(f'<{count}i', *requests[:count]))
                self.assertTrue(proof['records'][-1]['inverse_exact'])

    def test_both_character_variant_requests_are_validated(self):
        for index in (3, 4):
            for invalid in (-1, 125):
                with self.subTest(index=index, invalid=invalid):
                    image = self.image()
                    values = [102, 101, 72, 24, 103]
                    values[index] = invalid
                    image.data[module.MENU_REQUESTS] = struct.pack('<5i', *values)
                    with self.assertRaisesRegex(ValueError, 'mapping'):
                        module.read_catalog(image, menu_request_count=5)


if __name__ == '__main__':
    unittest.main()
