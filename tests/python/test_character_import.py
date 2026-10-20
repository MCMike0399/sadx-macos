import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "character_actions_importer", ROOT / "tools/import/character_actions.py")
IMPORTER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORTER)


class Image:
    def __init__(self):
        self.data = bytearray(8192)

    def read(self, address, size):
        if address < 0 or address + size > len(self.data):
            raise ValueError("unmapped source range")
        return bytes(self.data[address:address + size])

    def unpack(self, format, address):
        return struct.unpack(format, self.read(address, struct.calcsize(format)))

    def object(self, address, child=0):
        struct.pack_into("<II3f3i3fII", self.data, address,
                         0, 0, 1, 2, 3, 0, 0, 0, 1, 1, 1, child, 0)


class CharacterImportTests(unittest.TestCase):
    def test_resolved_graph_round_trips_original_pointers(self):
        image = Image()
        image.object(1024, 2048)
        image.object(2048)
        importer = IMPORTER.Importer(image)
        identity = importer.object(1024)
        self.assertEqual(identity, 1)
        self.assertEqual(struct.unpack_from("<I", importer.records[0][1], 44)[0], 2)
        importer.verify_round_trip()
        self.assertTrue(all(row["round_trip_exact"] for row in importer.evidence))
        self.assertEqual(sum(row["source_size"] for row in importer.evidence), 104)

    def test_changed_imported_value_cannot_pass_source_comparison(self):
        image = Image()
        struct.pack_into("<3f", image.data, 1024, 1, 2, 3)
        importer = IMPORTER.Importer(image)
        importer.array("vectors", 1024, 1, 12)
        kind, payload = importer.records[0]
        damaged = bytearray(payload)
        struct.pack_into("<f", damaged, 4, 4)
        importer.records[0] = kind, bytes(damaged)
        with self.assertRaisesRegex(ValueError, "source-byte roundtrip"):
            importer.verify_round_trip()

    def test_object_cycle_fails_at_import(self):
        image = Image()
        image.object(1024, 1024)
        with self.assertRaisesRegex(ValueError, "cyclic object"):
            IMPORTER.Importer(image).object(1024)

    def test_runtime_model_buffer_is_not_imported_as_a_pointer(self):
        image = Image()
        struct.pack_into("<I", image.data, 1024 + 40, 1234)
        with self.assertRaisesRegex(ValueError, "runtime buffer"):
            IMPORTER.Importer(image).model(1024)

    def test_missing_required_model_array_fails(self):
        image = Image()
        struct.pack_into("<i", image.data, 1024 + 8, 1)
        with self.assertRaisesRegex(ValueError, "model vertices"):
            IMPORTER.Importer(image).model(1024)

    def test_unrecovered_animation_channels_are_explicit(self):
        image = Image()
        struct.pack_into("<IIHH", image.data, 1024, 2048, 10, 8, 1)
        with self.assertRaisesRegex(ValueError, "position/rotation"):
            IMPORTER.Importer(image).motion(1024, 1)

    def test_shared_immutable_arrays_keep_their_identity(self):
        importer = IMPORTER.Importer(Image())
        first = importer.array("vectors", 1024, 3, 12)
        self.assertEqual(importer.array("vectors", 1024, 3, 12), first)
        self.assertEqual(len(importer.records), 1)
        importer.verify_round_trip()


if __name__ == "__main__":
    unittest.main()
