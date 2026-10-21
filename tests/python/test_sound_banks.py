import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import wave
import io
ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('sound_banks', ROOT / 'tools/import/sound_banks.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
class Image:
    sha256 = module.SOURCE_SHA256
    def read(self, address, size):
        if module.TABLE <= address < module.TABLE + 123 * 8:
            index = (address - module.TABLE) // 8
            if size == 984:
                return b''.join(self.read(module.TABLE + n * 8, 8) for n in range(123))
            return struct.pack('<II', 2 if index == 42 else 0, 4096 if index == 42 else 0)
        if address == 4096:
            return struct.pack('<IIII', 0, 8192, 5, 8200)
        raise ValueError('unexpected fixture address')
    def string(self, address):
        return {8192: 'TEST', 8200: ''}[address]
def archive():
    stream = io.BytesIO()
    with wave.open(stream, 'wb') as audio:
        audio.setparams((1, 2, 22050, 4, 'NONE', 'none'))
        audio.writeframes(struct.pack('<4h', 100, -100, 200, -200))
    payload = stream.getvalue()
    name = b'B00_00_01.WAV\0'
    return b'archive  V2.2\0\0\0' + struct.pack('<I', 1) + struct.pack('<III', 32, 32 + len(name), len(payload)) + name + payload, payload
class ImportBanks(unittest.TestCase):
    def test_preserves_lists_ordinals_and_pcm_bytes(self):
        data, payload = archive()
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp);source = root / 'source';source.mkdir();(source / 'test.DAT').write_bytes(data)
            with patch.object(module, 'PEImage', return_value=Image()):
                result = module.import_banks('unused', source, root / 'out')
            self.assertEqual(len(result['lists']), 123)
            self.assertEqual(result['lists'][42], [{'slot': 0, 'name': 'TEST'}, {'slot': 5, 'name': ''}])
            sample = result['banks'][0]['samples'][0]
            self.assertEqual((sample['ordinal'], sample['name'], sample['rate']), (0, 'B00_00_01.WAV', 22050))
            self.assertEqual((root / 'out' / sample['path']).read_bytes(), payload)
            self.assertEqual(result['missing'], [{'origin': 'system', 'name': '', 'reason': 'source file absent'}])
    def test_rejects_negative_selected_list(self):
        with self.assertRaises(ValueError):
            module.import_banks('unused', '.', '.', [-1])
    def test_rejects_wrong_source_identity(self):
        image = Image();image.sha256 = '0' * 64
        with self.assertRaises(ValueError):module.read_catalog(image)
    def test_rejects_unsupported_catalog_slot(self):
        image = Image();original = image.read
        image.read = lambda address, size: struct.pack('<IIII', 16, 8192, 5, 8200) if address == 4096 else original(address, size)
        with self.assertRaises(ValueError):module.read_catalog(image)
    def test_rejects_ambiguous_case_paths(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            with patch.object(module, 'PEImage', return_value=Image()), patch.object(Path, 'iterdir', return_value=[root / 'test.DAT', root / 'TEST.dat']), self.assertRaisesRegex(ValueError, 'Ambiguous case-insensitive DAT path'):
                module.import_banks('unused', root, root / 'out')
if __name__ == '__main__':unittest.main()
