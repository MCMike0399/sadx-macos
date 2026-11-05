"""Tiny private-tree fixtures: preservation, symlink and append-contract failures."""
import copy
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/testing'))
import private_native_assets as assets
import check_native_character_select_display as display


class PrivateAssets(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / 'source'
        (self.source / 'characters').mkdir(parents=True)
        self.canonical = b'SDXACT03' + struct.pack('<II', 9, 1) + b'canonical payload'
        (self.source / assets.PACK).write_bytes(self.canonical)
        (self.source / 'sound.wav').write_bytes(b'sound')
        self.output = self.root / 'private'
        self.append = self.root / 'append/sonic-selection.actions'
        self.append.parent.mkdir()
        self.appended = self.canonical[:8] + struct.pack('<I', 10) + self.canonical[12:] + b'new object'
        self.declarations = [{'index': row, 'object': 10} for row in assets.ROWS]
        self.proof = {'round_trip_exact': True, 'pack_sha256': assets.digest(self.appended),
            'display_append': {'prefix_records': 9, 'prefix_bytes': len(self.canonical),
                'prefix_sha256': assets.digest(self.canonical),
                'preserved_record_bytes_sha256': assets.digest(self.canonical[12:]),
                'new_records': 1, 'declarations': self.declarations}}
        self.actor = {'pack_schema': 'SDXACT03', 'character': 6, 'display_objects': self.declarations}
        self.write_append()

    def write_append(self):
        self.append.write_bytes(self.appended)
        self.append.with_suffix('.provenance.json').write_text(json.dumps(self.proof))
        (self.append.parent / 'gamma-actor.json').write_text(json.dumps(self.actor))

    def build(self, append=None):
        return assets.build_tree(self.source, self.output, append, assets.digest(self.canonical))

    def test_canonical_bytes_preserved_and_independent(self):
        result = self.build()
        self.assertEqual(result['canonical_before_sha256'], result['private_prefix_sha256'])
        self.assertEqual((self.output / assets.PACK).read_bytes(), self.canonical)
        (self.output / 'sound.wav').write_bytes(b'private changes')
        self.assertEqual((self.source / 'sound.wav').read_bytes(), b'sound')
        self.assertNotEqual((self.output / assets.PACK).stat().st_ino,
                            (self.source / assets.PACK).stat().st_ino)

    def test_source_symlink_rejected(self):
        (self.source / 'link').symlink_to(self.source / 'sound.wav')
        with self.assertRaisesRegex(ValueError, 'symlink'):
            self.build()
        self.assertFalse(self.output.exists())

    def test_source_directory_symlink_rejected(self):
        link = self.root / 'linked'
        link.symlink_to(self.source, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'symlink'):
            assets.build_tree(link, self.output)

    def test_overwrite_refused(self):
        self.output.mkdir()
        with self.assertRaisesRegex(ValueError, 'refusing overwrite'):
            self.build()

    def test_output_inside_source_refused(self):
        self.output = self.source / 'private'
        with self.assertRaisesRegex(ValueError, 'outside'):
            self.build()

    def test_wrong_canonical_hash_refused(self):
        with self.assertRaisesRegex(ValueError, 'canonical selection pack hash'):
            assets.build_tree(self.source, self.output)
        self.assertFalse(self.output.exists())

    def test_append_validated_and_prefix_preserved(self):
        report = self.build(self.append)
        self.assertTrue(report['append_metadata_validated'])
        self.assertEqual((self.source / assets.PACK).read_bytes(), self.canonical)
        self.assertEqual((self.output / assets.PACK).read_bytes(), self.appended)
        self.assertEqual(json.loads((self.output / 'characters/gamma-actor.json').read_text()), self.actor)

    def test_append_corruptions_refused_before_copy(self):
        for field in ('prefix_records', 'prefix_bytes', 'prefix_sha256',
                      'preserved_record_bytes_sha256', 'new_records'):
            with self.subTest(field=field):
                proof = copy.deepcopy(self.proof)
                proof['display_append'][field] = 'wrong'
                self.append.with_suffix('.provenance.json').write_text(json.dumps(proof))
                with self.assertRaises(ValueError):
                    self.build(self.append)
                self.assertFalse(self.output.exists())

    def test_wrong_gamma_metadata_refused(self):
        self.actor['display_objects'] = []
        self.write_append()
        with self.assertRaisesRegex(ValueError, 'metadata'):
            self.build(self.append)

    def test_changed_old_bytes_refused_even_with_new_digest(self):
        self.appended = self.appended.replace(b'canonical', b'wrong-old')
        self.proof['pack_sha256'] = assets.digest(self.appended)
        self.write_append()
        with self.assertRaisesRegex(ValueError, 'canonical bytes'):
            self.build(self.append)

    def arm_fixture(self):
        old = self.appended
        self.appended = old[:8] + struct.pack('<I', 11) + old[12:] + b'arm action payload'
        self.proof['display_append']['new_records'] = 2
        self.proof['pack_sha256'] = assets.digest(self.appended)
        self.proof['arm_append'] = {
            'declaration': 50, 'action': 11, 'round_trip_exact': True,
            'prefix_records': 10, 'prefix_bytes': len(old), 'prefix_sha256': assets.digest(old),
            'preserved_record_bytes_sha256': assets.digest(old[12:]), 'new_records': 1,
            'bytes': len(self.appended), 'sha256': assets.digest(self.appended)}
        self.actor['declarations'] = [None] * 50 + [
            {'index': 50, 'status': 'loaded', 'action': 11, 'resource_boundary': None}]
        self.actor['recipes'] = [{'declaration': 50, 'status': 'loaded', 'action': 11,
                                 'resource_boundary': None}]
        self.actor['display_flag_objects'] = [10] * 9
        self.proof['flag_initializer'] = {'objects': [10] * 9}
        self.write_append()

    def test_arm_append_preserves_display_prefix(self):
        self.arm_fixture()
        self.assertTrue(self.build(self.append)['append_metadata_validated'])

    def test_loaded_arm_requires_proof(self):
        self.arm_fixture()
        del self.proof['arm_append']
        self.write_append()
        with self.assertRaisesRegex(ValueError, 'requires append proof'):
            self.build(self.append)

    def test_arm_append_rejects_wrong_loaded_action(self):
        self.arm_fixture()
        self.actor['declarations'][50]['action'] = 10
        self.write_append()
        with self.assertRaisesRegex(ValueError, 'arm declaration'):
            self.build(self.append)

    def test_arm_append_rejects_changed_display_proof(self):
        self.arm_fixture()
        self.proof['arm_append']['prefix_sha256'] = 'wrong'
        self.write_append()
        with self.assertRaisesRegex(ValueError, 'arm append prefix'):
            self.build(self.append)

    def test_append_symlink_refused(self):
        self.append.unlink()
        self.append.symlink_to(self.source / assets.PACK)
        with self.assertRaisesRegex(ValueError, 'ordinary files'):
            self.build(self.append)


class DisplayContent(unittest.TestCase):
    def test_observed_semantics_and_wrong_owners(self):
        fixture = json.loads((ROOT / 'reference/packets/cs-journey/SCHEDULER-OBSERVATION.json').read_text())
        state = fixture['state']
        display.semantic_state(state)
        for field, value in [('game_state', 2), ('active_save_name', 'SonicDX01.snc'),
                             ('menu_selected_save', 0), ('preview_camera_live', False)]:
            with self.subTest(field=field):
                changed = copy.deepcopy(state)
                changed[field] = value
                with self.assertRaises(ValueError):
                    display.semantic_state(changed)
        for field in ('exec', 'display'):
            changed = copy.deepcopy(state)
            task = next(task for task in changed['tasks'] if task['path'] == '1/2')
            task[field] = False
            with self.assertRaises(ValueError):
                display.semantic_state(changed)

    def test_uniform_draw_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'blank.ppm'
            path.write_bytes(b'P6\n640 480\n255\n' + bytes(640 * 480 * 3))
            with self.assertRaisesRegex(ValueError, 'full-frame blank'):
                display.drawn_content(path)

    def test_nonblank_wrong_draw_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'wrong.ppm'
            path.write_bytes(b'P6\n640 480\n255\n' + b'\xff\x00\xff' * (640 * 480 - 1) + b'\x00\xff\x00')
            with self.assertRaisesRegex(ValueError, 'signature distance'):
                display.drawn_content(path)


if __name__ == '__main__':
    unittest.main()
