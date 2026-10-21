import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "character_texture_importer", ROOT / "tools/import/character_textures.py")
IMPORTER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORTER)


class Image:
    sha256 = IMPORTER.SOURCE_SHA
    base = 0x900000

    def __init__(self):
        self.data = bytearray(0x30000)
        self.table = 0x910000
        self.name = 0x920000
        self.texlist = 0x921000
        self.put(self.name, b"SONIC\0")
        self.put(self.texlist, struct.pack("<II", 0x930000, 16))
        self.put(self.table, struct.pack("<4I", self.name, self.texlist, 0, 0))
        self.put(IMPORTER.CHARACTER_TABLE, struct.pack("<9I", *([self.table] * 9)))

    def put(self, address, value):
        offset = address - self.base
        self.data[offset:offset + len(value)] = value

    def read(self, address, size):
        offset = address - self.base
        if offset < 0 or size > len(self.data) - offset:
            raise ValueError("unmapped source bytes")
        return bytes(self.data[offset:offset + size])

    def unpack(self, format, address):
        return struct.unpack(format, self.read(address, struct.calcsize(format)))

    def string(self, address):
        offset = address - self.base
        end = self.data.index(0, offset)
        return bytes(self.data[offset:end]).decode("ascii")


class CharacterTextureImportTests(unittest.TestCase):
    def test_ordered_nine_character_tables_and_aliases_round_trip(self):
        metadata, proof = IMPORTER.declarations(Image())
        self.assertEqual(metadata["version"], 1)
        self.assertEqual([c["id"] for c in metadata["characters"]], list(range(9)))
        expected = [{"archive": "SONIC", "texture_count": 16, "resource_name": "sonic"}]
        for char in metadata["characters"]:
            self.assertEqual(char["bindings"], expected)
        self.assertTrue(proof["directory_round_trip_exact"])
        self.assertTrue(all(c["round_trip_exact"] for c in proof["characters"]))
        self.assertEqual({c["source_table"] for c in proof["characters"]}, {"0x910000"})
        self.assertNotIn("source_table", str(metadata))
        self.assertNotIn("0x", str(metadata))

    def test_unknown_source_hash_rejected(self):
        image = Image()
        image.sha256 = "different"
        with self.assertRaisesRegex(ValueError, "SHA256"):
            IMPORTER.declarations(image)

    def test_zero_table_rejected(self):
        image = Image()
        image.put(IMPORTER.CHARACTER_TABLE + 8 * 4, struct.pack("<I", 0))
        with self.assertRaisesRegex(ValueError, "table is absent"):
            IMPORTER.declarations(image)

    def test_missing_name_and_invalid_counts_rejected(self):
        image = Image()
        image.put(image.table, struct.pack("<I", 0))
        with self.assertRaisesRegex(ValueError, "name is absent"):
            IMPORTER.declarations(image)
        for count in (0, 4097):
            image = Image()
            image.put(image.texlist + 4, struct.pack("<I", count))
            with self.assertRaisesRegex(ValueError, "texture declaration"):
                IMPORTER.declarations(image)

    def test_missing_terminator_rejected(self):
        image = Image()
        image.put(image.table, struct.pack("<II", image.name, image.texlist) * 32)
        with self.assertRaisesRegex(ValueError, "unterminated"):
            IMPORTER.declarations(image)

    def test_changed_emitted_count_fails_inverse_byte_proof(self):
        image = Image()
        original = image.unpack
        def altered(format, address):
            values = original(format, address)
            return (values[0], 15) if address == image.texlist else values
        image.unpack = altered
        with self.assertRaisesRegex(ValueError, "source-byte roundtrip"):
            IMPORTER.declarations(image)

    def test_file_export_has_separate_provenance(self):
        with tempfile.TemporaryDirectory() as directory:
            with patch.object(IMPORTER, "PEImage", return_value=Image()):
                result = IMPORTER.import_character_textures(Path("game.exe"), Path(directory))
            output = Path(directory)
            self.assertTrue((output / "character-textures.json").is_file())
            self.assertTrue((output / "character-textures.provenance.json").is_file())
            self.assertEqual(len(result["characters"]), 9)


if __name__ == "__main__":
    unittest.main()
