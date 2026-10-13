import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from pe_image import PEImage
from extract import SOURCE_SHA256, TABLE_ADDRESS, extract_stage, triangulate


class SourceBoundaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.image = PEImage(ROOT / 'game' / 'Sonic Adventure DX' / 'sonic.exe')

    def test_source_identity_imports_and_unbacked_memory(self):
        image = self.image
        self.assertEqual(image.sha256, SOURCE_SHA256)
        imports = {item['dll'].lower(): item['symbols'] for item in image.imports()}
        self.assertIn({'name': 'Direct3DCreate8'}, imports['d3d8.dll'])
        self.assertIn({'name': 'DirectInput8Create'}, imports['dinput8.dll'])
        self.assertEqual(image.file_offset(0xE99CB8, 36), 0xA99CB8)
        data = next(s for s in image.sections if s['name'] == '.data')
        # Zero-initialized runtime globals cannot be read as original file bytes.
        with self.assertRaisesRegex(ValueError, 'file-backed'):
            image.read(image.base + data['rva'] + data['raw_size'], 4)
        with self.assertRaisesRegex(ValueError, 'file-backed'):
            image.read(0xFFFFFFFF, 12)

    def test_declared_pe_ranges_reject_truncation(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = pathlib.Path(temporary) / 'truncated.exe'
            path.write_bytes(self.image.data[:256])
            with self.assertRaises(ValueError):
                PEImage(path)

    def test_original_table_and_complete_static_stage_coverage(self):
        raw = self.image.read(TABLE_ADDRESS, 4100)
        self.assertEqual(hashlib.sha256(raw).hexdigest(),
                         '3acbb71a623df2d12dc44e34a48caf049d0e50560593ed003ca15d827947feed')
        mesh, report = extract_stage(self.image, struct.unpack('<1025f', raw))
        self.assertEqual(report['col_count'], 480)
        self.assertEqual(len(report['records']), 480)
        self.assertEqual(report['object_count'], 480)
        self.assertEqual(report['mesh_types'], {3: 1130})
        self.assertEqual(report['triangle_count'], 31351)
        self.assertEqual(len(mesh), 31351 * 3 * 32)
        # Corrupt a real mesh index: producer must reject rather than silently
        # omit the offending polygon and claim complete stage coverage.
        image = PEImage(self.image.path)
        col = image.unpack('<4hf6I', 0xE99CB8)[5]
        object_va = image.unpack('<6f3I', col)[6]
        model_va = image.unpack('<2I', object_va)[1]
        point_count, mesh_va = image.unpack('<iI', model_va + 8)
        indices_va = image.unpack('<2HI', mesh_va)[2]
        corrupted = bytearray(image.data)
        struct.pack_into('<H', corrupted, image.file_offset(indices_va + 2, 2), point_count)
        image.data = bytes(corrupted)
        with self.assertRaisesRegex(ValueError, 'Vertex index'):
            extract_stage(image, struct.unpack('<1025f', raw))

    def test_strip_winding_and_unsupported_polygon_type(self):
        self.assertEqual(triangulate(3, [4, 5, 6, 7]), [(4, 5, 6), (6, 5, 7)])
        self.assertEqual(triangulate(3, [4, 5, 6, 7], True), [(5, 4, 6), (5, 6, 7)])
        with self.assertRaisesRegex(ValueError, 'unsupported'):
            triangulate(2, [0, 1, 2, 3, 4])


if __name__ == '__main__':
    unittest.main()
