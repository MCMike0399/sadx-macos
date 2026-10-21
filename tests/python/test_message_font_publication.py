"""Import publication must never mutate the owned install or prior parsed assets."""
import importlib.util
import os
from pathlib import Path
import shutil
import unittest
from tempfile import TemporaryDirectory
from unittest.mock import patch

STAGE = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('message_fonts_delivery', STAGE/'tools/import/message_fonts.py')
fonts = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fonts)

class Publication(unittest.TestCase):
    def setUp(self):
        self.temporary = TemporaryDirectory(prefix='sadx-message-publication-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root/'owned'
        (self.source/'system').mkdir(parents=True)
        original = fonts.REPO/'reference/updated-pc-reference/game'
        (self.source/'sonic.exe').symlink_to(original/'sonic.exe')
        for name in ('FONTDATA0.BIN', 'FONTDATA1.BIN'):
            shutil.copyfile(original/'system'/name, self.source/'system'/name)
        self.output = self.root/'assets'
        self.profile = patch.object(fonts, 'profile_evidence', return_value={})
        self.mods = patch.object(fonts, 'selected_mods', return_value=([], None))
        self.profile.start(); self.mods.start()
        self.addCleanup(self.profile.stop); self.addCleanup(self.mods.stop)
    def run_import(self, output=None):
        return fonts.import_message_fonts(self.source, output or self.output)
    def snapshot(self, directory):
        return {p.relative_to(directory).as_posix():fonts.digest(p.read_bytes())
                for p in directory.rglob('*') if p.is_file()}
    def test_owned_output_rejected(self):
        before = self.snapshot(self.source)
        with self.assertRaises(ValueError): self.run_import(self.source)
        self.assertEqual(self.snapshot(self.source), before)
    def test_owned_child_output_rejected(self):
        with self.assertRaises(ValueError): self.run_import(self.source/'imported')
        self.assertFalse((self.source/'imported').exists())
    def test_parent_symlink_rejected(self):
        redirected = self.root/'redirected'; redirected.mkdir()
        alias = self.root/'alias'; alias.symlink_to(redirected, target_is_directory=True)
        with self.assertRaises(ValueError): self.run_import(alias/'new'/'assets')
        self.assertEqual(list(redirected.iterdir()), [])
    def test_payload_directory_symlink_rejected(self):
        self.output.mkdir()
        (self.output/'message-fonts').symlink_to(self.source/'system', target_is_directory=True)
        before = self.snapshot(self.source)
        with self.assertRaises(ValueError): self.run_import()
        self.assertEqual(self.snapshot(self.source), before)
    def test_manifest_symlink_rejected_before_payload_publication(self):
        self.run_import(); before = self.snapshot(self.output)
        (self.output/'message-fonts.json').unlink()
        (self.output/'message-fonts.json').symlink_to(self.source/'system/FONTDATA1.BIN')
        owned = (self.source/'system/FONTDATA1.BIN').read_bytes()
        with self.assertRaises(ValueError): self.run_import()
        self.assertEqual((self.source/'system/FONTDATA1.BIN').read_bytes(), owned)
        for key, value in before.items():
            if key != 'message-fonts.json': self.assertEqual(fonts.digest((self.output/key).read_bytes()), value)
    def test_payload_hardlink_replaced_with_fresh_inode(self):
        (self.output/'message-fonts').mkdir(parents=True)
        source = self.source/'system/FONTDATA1.BIN'
        target = self.output/'message-fonts/single-glyphs.bin'
        os.link(source, target); before = source.read_bytes()
        self.run_import()
        self.assertEqual(source.read_bytes(), before)
        self.assertNotEqual(source.stat().st_ino, target.stat().st_ino)
    def test_manifest_hardlink_replaced_with_fresh_inode(self):
        self.output.mkdir()
        source = self.source/'system/FONTDATA1.BIN'
        target = self.output/'message-fonts.json'
        os.link(source, target); before = source.read_bytes()
        self.run_import()
        self.assertEqual(source.read_bytes(), before)
        self.assertNotEqual(source.stat().st_ino, target.stat().st_ino)
    def test_ancestor_output_payload_cannot_mutate_owned_install(self):
        # The output itself is outside game; its exact payload destination is inside.
        self.output.mkdir()
        relocated = self.output/'message-fonts'
        self.source.rename(relocated); self.source = relocated
        (self.source/'wide-glyphs.bin').write_bytes(b'owned existing wide resource')
        (self.source/'single-glyphs.bin').write_bytes(b'owned existing single resource')
        before = self.snapshot(self.source)
        rejected = False
        try: self.run_import()
        except ValueError: rejected = True
        self.assertEqual(self.snapshot(self.source), before)
        self.assertTrue(rejected, 'an actual publication destination overlaps the owned install')
    def test_harmless_ancestor_output_allowed(self):
        self.output.mkdir()
        relocated = self.output/'owned-install'
        self.source.rename(relocated); self.source = relocated
        before = self.snapshot(self.source)
        self.run_import()
        self.assertEqual(self.snapshot(self.source), before)
        self.assertTrue((self.output/'message-fonts.json').is_file())
    def assert_case_alias_protected(self, suffix):
        alias = self.root/'OWNED'
        if not alias.exists() or not alias.samefile(self.source):
            self.skipTest('the temporary volume treats these case variants as distinct paths')
        before = self.snapshot(self.source)
        rejected = False
        try: self.run_import(alias/suffix)
        except ValueError: rejected = True
        self.assertEqual(self.snapshot(self.source), before)
        self.assertTrue(rejected, 'case alias publication resolves into the owned install')
    def test_case_alias_owned_output_rejected(self):
        self.assert_case_alias_protected(Path())
    def test_case_alias_owned_ancestor_rejected(self):
        self.assert_case_alias_protected(Path('missing')/'assets')
    def test_parse_failure_preserves_previous_success(self):
        self.run_import(); before = self.snapshot(self.output)
        # Wide parses first. Change it validly, then make the later single parse fail.
        wide = self.source/'system/FONTDATA0.BIN'
        raw = bytearray(wide.read_bytes()); raw[16] ^= 128; wide.write_bytes(raw)
        single = self.source/'system/FONTDATA1.BIN'
        single.write_bytes(single.read_bytes()[:-1])
        with self.assertRaises(ValueError): self.run_import()
        self.assertEqual(self.snapshot(self.output), before)

if __name__ == '__main__': unittest.main()
