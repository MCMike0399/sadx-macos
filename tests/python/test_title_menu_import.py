"""Title metadata imports preserve source records, aliases and sentinels."""
import importlib.util
import json
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("title_menu_import", ROOT / "tools/import/title_menu.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class Image:
    sha256 = module.SOURCE_SHA

    def __init__(self):
        self.bytes = {}
        self.write(module.SEQUENCE_POINTERS, struct.pack("<3I", 4096, 8192, 8192))
        self.write(4096, struct.pack("<9i", 5, 0, 2, 2, 2, 5, 6, 6, 0))
        self.write(8192, struct.pack("<5i", 1, 6, 8, 8, 0))
        self.write(module.INITIAL_TEXTURE_IDS, struct.pack("<i", 30))

    def write(self, address, data):
        self.bytes.update({address + index: value for index, value in enumerate(data)})

    def read(self, address, size):
        return bytes(self.bytes[address + index] for index in range(size))


class ImportTests(unittest.TestCase):
    def test_background_layout_exact_float_words(self):
        image = Image()
        words = [0x43200000, 0x43700000, 0xc2dc0000, 0x80000000, 0x4457c000,
                 0xc3d00000, 0x42f00000, 0x43150000, 0x42d60000]
        image.write(0x7EF928, struct.pack("<9I", *words))
        values, bits, proof = module.extract_background_layout(image)
        inverse = struct.pack("<9f", *(values[name] for name in module.BACKGROUND_FIELDS))
        self.assertEqual(inverse, struct.pack("<9I", *words))
        self.assertEqual([bits[name] for name in module.BACKGROUND_FIELDS], words)
        self.assertEqual(list(proof), ["0x7ef928-0x7ef94c"])

    def test_background_rejects_nonfinite_source(self):
        image = Image()
        image.write(0x7EF928, struct.pack("<9I", *[0x7f800000] * 9))
        with self.assertRaisesRegex(ValueError, "nonfinite"):
            module.extract_background_layout(image)

    def mode_image(self):
        image = Image()
        image.write(0x10D7CC4, struct.pack("<14I", *[32768 + index * 128 for index in range(14)]))
        for index in range(14):
            image.write(32768 + index * 128, struct.pack("<3i", index, index, 30))
        return image

    def test_mode_texture_order_and_duplicates(self):
        values, proof = module.extract_mode_textures(self.mode_image())
        self.assertEqual(values, [[index, index] for index in range(14)])
        self.assertEqual(len(proof), 15)

    def test_mode_texture_invalid_id(self):
        image = self.mode_image()
        image.write(32768, struct.pack("<i", 31))
        with self.assertRaisesRegex(ValueError, "texture ID"):
            module.extract_mode_textures(image)

    def test_mode_texture_bounded_terminator(self):
        image = self.mode_image()
        image.write(32768, struct.pack("<31i", *[0] * 31))
        with self.assertRaisesRegex(ValueError, "terminator"):
            module.extract_mode_textures(image)

    def selection_image(self):
        image = Image()
        for base in (0x7EF5EC, 0x7EF624):
            for character in range(7):
                pointer = 16384 + character * 32 + (base == 0x7EF624) * 512
                image.write(base + character * 8, struct.pack("<IB3x", pointer, 2))
                image.write(pointer, bytes([1, 2, 1, 3]))
        image.write(0x7EF6B4, struct.pack("<7H", 0, 2, 3, 5, 7, 6, 0))
        image.write(0x7EF688, bytes(range(44)))
        return image

    def test_selection_preserves_duplicates_and_order(self):
        content, proof = module.extract_title_selection(self.selection_image())
        self.assertEqual(content["trial_stages"][0], [{"stage": 1, "act": 2}, {"stage": 1, "act": 3}])
        self.assertEqual(content["character_ids"], [0, 2, 3, 5, 7, 6, 0])
        self.assertEqual(content["display_stage_ids"], list(range(44)))
        self.assertEqual(len(content["subgame_stages"]), 7)
        self.assertNotIn("0x", json.dumps(content))
        self.assertEqual(len(proof), 30)

    def test_selection_rejects_invalid_count(self):
        image = self.selection_image()
        image.write(0x7EF5EC + 4, bytes([11]))
        with self.assertRaisesRegex(ValueError, "count"):
            module.extract_title_selection(image)

    def test_selection_rejects_invalid_stage(self):
        image = self.selection_image()
        image.write(16384, bytes([43]))
        with self.assertRaisesRegex(ValueError, "stage"):
            module.extract_title_selection(image)

    def test_selection_rejects_invalid_save_character(self):
        image = self.selection_image()
        image.write(0x7EF6B4, struct.pack("<H", 8))
        with self.assertRaisesRegex(ValueError, "character"):
            module.extract_title_selection(image)

    def test_inverse_and_shared_entry(self):
        image = Image()
        content, proof = module.extract_title_menu(image)
        self.assertEqual(content["entry_sequences"], [0, 1, 1])
        self.assertEqual(len(content["sequences"]), 2)
        self.assertEqual(content["initial_texture_ids"], [])
        self.assertTrue(proof["inverse_matches"])
        self.assertEqual(module.encode_sequence(content["sequences"][0]), image.read(4096, 36))
        self.assertEqual(module.encode_sequence(content["sequences"][1]), image.read(8192, 20))
        self.assertNotIn("0x", json.dumps(content))

    def test_duplicate_texture_loads_preserved(self):
        image = Image()
        image.write(module.INITIAL_TEXTURE_IDS, struct.pack("<4i", 3, 3, 2, 30))
        self.assertEqual(module.extract_title_menu(image)[0]["initial_texture_ids"], [3, 3, 2])

    def test_empty_sequence(self):
        image = Image()
        image.write(8192, struct.pack("<i", 0))
        sequence = module.extract_title_menu(image)[0]["sequences"][1]
        self.assertEqual(sequence["born_mode"], "None")
        self.assertIsNone(sequence["initial_transition"])
        self.assertEqual(module.encode_sequence(sequence), struct.pack("<i", 0))

    def test_wrong_source(self):
        image = Image()
        image.sha256 = "unsupported"
        with self.assertRaisesRegex(ValueError, "pinned"):
            module.extract_title_menu(image)

    def test_invalid_transition(self):
        image = Image()
        image.write(4096 + 8, struct.pack("<i", -1))
        with self.assertRaisesRegex(ValueError, "mode"):
            module.extract_title_menu(image)

    def test_invalid_texture(self):
        image = Image()
        image.write(module.INITIAL_TEXTURE_IDS, struct.pack("<i", 31))
        with self.assertRaisesRegex(ValueError, "texture"):
            module.extract_title_menu(image)

    def test_unterminated_sequence(self):
        image = Image()
        image.write(4096, struct.pack("<4i", 5, 0, 2, 2) * module.MAX_RECORDS)
        with self.assertRaisesRegex(ValueError, "terminator"):
            module.extract_title_menu(image)

    def test_unterminated_textures(self):
        image = Image()
        image.write(module.INITIAL_TEXTURE_IDS, struct.pack("<i", 0) * module.MAX_RECORDS)
        with self.assertRaisesRegex(ValueError, "terminator"):
            module.extract_title_menu(image)


if __name__ == "__main__":
    unittest.main()
