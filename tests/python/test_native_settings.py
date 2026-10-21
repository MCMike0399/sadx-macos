import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('native_settings', ROOT / 'tools/import/native_settings.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class NativeSettingsImport(unittest.TestCase):
    def fixture(self, directory, settings):
        game = Path(directory) / 'game'
        profiles = game / 'mods/.modloader/profiles'
        profiles.mkdir(parents=True)
        (profiles / 'Profiles.json').write_text(json.dumps({'ProfileIndex': 1, 'ProfilesList': [
            {'Filename': 'unused.json'}, {'Filename': 'selected.json'}]}))
        (profiles / 'selected.json').write_text(json.dumps({'TestSpawn': settings}))
        return game

    def test_selected_effective_language(self):
        with tempfile.TemporaryDirectory() as directory:
            game = self.fixture(directory, {'GameTextLanguage': 3, 'GameVoiceLanguage': 0})
            result = module.import_settings(game, Path(directory) / 'out')
            self.assertEqual((result['text_language'], result['voice_language']), (3, 0))
            self.assertEqual(len(result['profile_sha256']), 64)

    def test_loader_defaults(self):
        with tempfile.TemporaryDirectory() as directory:
            game = self.fixture(directory, {})
            result = module.import_settings(game, Path(directory) / 'out')
            self.assertEqual((result['text_language'], result['voice_language']), (1, 1))

    def test_wrong_type_is_not_coerced(self):
        with tempfile.TemporaryDirectory() as directory:
            game = self.fixture(directory, {'GameTextLanguage': True})
            with self.assertRaises(ValueError):
                module.import_settings(game, Path(directory) / 'out')

    def test_missing_selected_profile_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            game = self.fixture(directory, {})
            (game / 'mods/.modloader/profiles/selected.json').unlink()
            with self.assertRaises(FileNotFoundError):
                module.import_settings(game, Path(directory) / 'out')


if __name__ == '__main__':
    unittest.main()
