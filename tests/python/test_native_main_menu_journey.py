"""The real MainMenu semantic replay must reject suppressed raster output."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('native_main_menu', ROOT / 'tools/journey/check_native_main_menu.py')
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)


class NativeMainMenuRaster(unittest.TestCase):
    def test_genuine_states_with_draw_suppression_fail(self):
        recording = json.loads((ROOT / 'reference/frozen-corpora/native-main-menu-state-replay.json').read_text())
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            app = root / 'sadx-native'
            app.touch()
            assets = root / 'assets'
            (assets / 'saves').mkdir(parents=True)
            for name in ('main-menu-dialog.json', 'music-assets.json', 'menu-dialog.json'):
                (assets / name).write_text('{}')
            for slot in ('01', '02'):
                (assets / 'saves' / f'SonicDX{slot}.snc').write_bytes(f'unchanged save {slot}'.encode())

            def suppressed_draw(command, **kwargs):
                captures = Path(command[command.index('--capture-directory') + 1])
                captures.mkdir()
                profile = Path(command[command.index('--save-directory') + 1])
                shutil.copytree(assets / 'saves', profile)
                for frame in command[command.index('--capture-at') + 1].split(','):
                    (captures / f'{frame}.json').write_text(json.dumps(recording['states'][frame]))
                    (captures / f'{frame}.ppm').write_bytes(b'P6\n640 480\n255\n' + bytes(640 * 480 * 3))
                return subprocess.CompletedProcess(command, 0)

            with patch.object(checker.subprocess, 'run', suppressed_draw):
                with self.assertRaisesRegex(RuntimeError, 'MainMenu source image region'):
                    checker.run(app, assets, root / 'output')
            self.assertFalse((root / 'output/result.json').exists())


if __name__ == '__main__':
    unittest.main()
