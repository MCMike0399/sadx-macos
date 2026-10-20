import importlib.util
import tempfile
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('input_config',ROOT/'tools/import/input_config.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class InputConfigTests(unittest.TestCase):
    def fixture(self):
        keys='\n'.join(f'{key}={100+i}' for i,key in enumerate(module.KEYS))
        controllers='\n'.join(f'[Controller {i}]\nDeadzoneL=7849\nDeadzoneR=8689\nRadialL=true\nRadialR=false\nTriggerThreshold=30' for i in range(1,9))
        return '[Config]\nDisableMouse=true\nKeyboardPlayer=0\n[Keyboard]\n'+keys+'\n'+controllers+'\n'
    def test_all_bindings_and_controller_settings(self):
        with tempfile.TemporaryDirectory() as temporary:
            path=Path(temporary)/'config.ini';path.write_text(self.fixture());r=module.input_configuration(path)
            self.assertEqual(r['keyboard_keys'],list(range(100,124)));self.assertEqual(len(r['controllers']),8)
            self.assertEqual(r['controllers'][0],dict(deadzone_left=7849,deadzone_right=8689,radial_left=True,radial_right=False,trigger_threshold=30))
            self.assertTrue(r['disable_mouse']);self.assertEqual(len(r['settings_sha256']),64)
    def test_raw_out_of_range_settings_are_preserved_for_native_owner(self):
        with tempfile.TemporaryDirectory() as temporary:
            path=Path(temporary)/'config.ini';path.write_text(self.fixture().replace('DeadzoneL=7849','DeadzoneL=-42').replace('KeyboardPlayer=0','KeyboardPlayer=-3'))
            r=module.input_configuration(path);self.assertEqual(r['keyboard_player'],-3);self.assertEqual(r['controllers'][0]['deadzone_left'],-42)
    def test_missing_binding_is_explicit_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            path=Path(temporary)/'config.ini';path.write_text(self.fixture().replace('rightstick=100\n',''))
            with self.assertRaises(Exception):module.input_configuration(path)
    def test_unsupported_boolean_literal_is_explicit(self):
        with tempfile.TemporaryDirectory() as temporary:
            path=Path(temporary)/'config.ini';path.write_text(self.fixture().replace('DisableMouse=true','DisableMouse=yes'))
            with self.assertRaises(ValueError):module.input_configuration(path)
    def test_pinned_profile(self):
        path=Path.cwd()/'reference/updated-pc-reference/game/mods/sadx-input-mod/config.ini'
        if not path.exists():self.skipTest('Pinned profile import fixture is not installed')
        r=module.input_configuration(path);self.assertEqual(r['keyboard_keys'],[160,69,88,90,65,83,66,67,86,13,81,87,104,98,100,102,38,40,37,39,73,77,74,76]);self.assertTrue(r['disable_mouse']);self.assertEqual(r['keyboard_player'],0)
if __name__=='__main__':unittest.main()
