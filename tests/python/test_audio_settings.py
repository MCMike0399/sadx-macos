import importlib.util
from pathlib import Path
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('audio_settings', ROOT / 'tools/import/audio_settings.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
class AudioSettings(unittest.TestCase):
    def read(self, values):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'sonicDX.ini'
            path.write_text('[sonicDX]\n' + values)
            return module.audio_settings(path)
    def test_raw_settings(self):
        value = self.read('bgm=-3\nbgmv=101\nsevoice=9\nvoicev=70\n')
        self.assertEqual([value[k] for k in ['bgm_enabled', 'bgm_volume_percent', 'voice_enabled', 'voice_volume_percent']], [-3, 101, 9, 70])
        self.assertEqual(len(value['settings_sha256']), 64)
    def test_no_inferred_defaults(self):
        with self.assertRaises(Exception):
            self.read('bgm=1\nbgmv=30\nsevoice=1\n')
    def test_invalid_values(self):
        for value in ['0x10', '1.5', '2147483648', '-2147483649']:
            with self.subTest(value=value), self.assertRaises(ValueError):
                self.read(f'bgm=1\nbgmv=30\nsevoice=1\nvoicev={value}\n')
if __name__ == '__main__':
    unittest.main()
