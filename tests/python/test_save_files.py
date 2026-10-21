import hashlib
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[2] / 'tools/import/save_files.py'
spec = importlib.util.spec_from_file_location('save_files', SOURCE)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class SaveFilesTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.game, self.output = self.root / 'game', self.root / 'assets'
        self.source = self.game / 'SAVEDATA'
        self.source.mkdir(parents=True)

    def source_file(self, name='SonicDX01.snc', content=b'owned save'):
        path = self.source / name
        path.write_bytes(content)
        os.utime(path, ns=(1700000000123456700, 1700000000123456700))
        return path

    def test_bytes_hash_mtime_and_original_preserved(self):
        source = self.source_file(content=bytes(range(256)) * 6)
        before = source.stat()
        result = module.import_save_files(self.game, self.output)
        record, = result['files']
        copy = self.output / record['path']
        self.assertEqual(source.read_bytes(), copy.read_bytes())
        self.assertEqual(record['sha256'], hashlib.sha256(source.read_bytes()).hexdigest())
        self.assertEqual(record['mtime_ns'], before.st_mtime_ns)
        self.assertEqual(copy.stat().st_mtime_ns, before.st_mtime_ns)
        self.assertEqual(record['last_write_time'], 133444736001234567)
        self.assertEqual(source.stat().st_mtime_ns, before.st_mtime_ns)
        self.assertEqual(json.loads((self.output / 'save-files.json').read_text()), result)

    def test_short_bad_crc_and_chao_not_filtered(self):
        for name, content in [('SonicDX01.snc', b'partial'), ('SonicDX02.snc', b'badcrc' * 232),
                              ('SONICADVENTURE_DX_CHAOGARDEN.snc', b'chao')]:
            self.source_file(name, content)
        result = module.import_save_files(self.game, self.output)
        self.assertEqual(len(result['files']), 3)
        for record in result['files']:
            self.assertEqual((self.source / record['name']).read_bytes(), (self.output / record['path']).read_bytes())

    def test_provider_order_and_non_save_exclusion(self):
        for name in ['SonicDX02.snc', 'SonicDX01.snc', 'notes.txt']:
            self.source_file(name)
        expected = [entry.name for entry in self.source.iterdir() if entry.suffix == '.snc']
        self.assertEqual([v['name'] for v in module.import_save_files(self.game, self.output)['files']], expected)

    def test_empty_existing_directory(self):
        self.assertEqual(module.import_save_files(self.game, self.output)['files'], [])

    def test_missing_directory_is_empty_snapshot(self):
        self.source.rmdir()
        self.assertEqual(module.import_save_files(self.game, self.output), {'version': 1, 'files': []})

    def test_cannot_write_inside_owned_install(self):
        with self.assertRaises(ValueError):
            module.import_save_files(self.game, self.game / 'imported')

    def test_source_case_alias_is_error_when_volume_supports_it(self):
        self.source_file('SonicDX01.snc')
        self.source_file('SONICDX01.SNC')
        if len(list(self.source.iterdir())) != 2:
            self.skipTest('native volume does not support two case aliases')
        with self.assertRaises(ValueError):
            module.import_save_files(self.game, self.output)

    def test_symlink_not_followed(self):
        real = self.root / 'outside'
        real.write_bytes(b'outside')
        (self.source / 'SonicDX01.snc').symlink_to(real)
        with self.assertRaises(ValueError):
            module.import_save_files(self.game, self.output)

    def test_directory_with_save_suffix_is_error(self):
        (self.source / 'SonicDX01.snc').mkdir()
        with self.assertRaises(ValueError):
            module.import_save_files(self.game, self.output)

    def test_filetime_epoch_and_limits(self):
        self.assertEqual(module.filetime(-module.FILETIME_EPOCH_NS), 0)
        self.assertEqual(module.filetime(0), 116444736000000000)
        with self.assertRaises(ValueError):
            module.filetime(-module.FILETIME_EPOCH_NS - 1)
        with self.assertRaises(ValueError):
            module.filetime(2**64 * 100)

    def test_output_symlink_cannot_mutate_original(self):
        source = self.source_file()
        original = source.read_bytes()
        self.output.mkdir()
        (self.output / 'saves').symlink_to(self.source, target_is_directory=True)
        with self.assertRaises(ValueError):
            module.import_save_files(self.game, self.output)
        self.assertEqual(source.read_bytes(), original)

    def test_destination_symlink_cannot_mutate_other_file(self):
        self.source_file()
        (self.output / 'saves').mkdir(parents=True)
        target = self.root / 'unrelated'
        target.write_bytes(b'keep')
        (self.output / 'saves' / 'SonicDX01.snc').symlink_to(target)
        with self.assertRaises(ValueError):
            module.import_save_files(self.game, self.output)
        self.assertEqual(target.read_bytes(), b'keep')

    def test_payload_hardlink_cannot_modify_any_owned_save(self):
        first = self.source_file('SonicDX01.snc', b'first owned save')
        second = self.source_file('SonicDX02.snc', b'second owned save')
        before = {p.name: (p.read_bytes(), p.stat().st_mtime_ns, p.stat().st_ino) for p in [first, second]}
        (self.output / 'saves').mkdir(parents=True)
        os.link(second, self.output / 'saves' / first.name)
        result = module.import_save_files(self.game, self.output)
        for record in result['files']:
            source = self.source / record['name']
            copy = self.output / record['path']
            self.assertEqual((source.read_bytes(), source.stat().st_mtime_ns, source.stat().st_ino), before[source.name])
            self.assertEqual(copy.read_bytes(), source.read_bytes())
            self.assertNotEqual(copy.stat().st_ino, source.stat().st_ino)
        self.assertFalse(list(self.output.rglob('.save-copy-*')))

    def test_manifest_hardlink_cannot_modify_owned_save(self):
        source = self.source_file(content=b'owned source bytes, not JSON')
        before = source.read_bytes(), source.stat().st_mtime_ns, source.stat().st_ino
        self.output.mkdir()
        manifest = self.output / 'save-files.json'
        os.link(source, manifest)
        result = module.import_save_files(self.game, self.output)
        self.assertEqual((source.read_bytes(), source.stat().st_mtime_ns, source.stat().st_ino), before)
        self.assertEqual(json.loads(manifest.read_text()), result)
        self.assertNotEqual(source.stat().st_ino, manifest.stat().st_ino)
        self.assertFalse(list(self.output.glob('.save-manifest-*')))
