"""Focused source inverse, alias, prefix and output publication checks."""
import hashlib
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/import'))
import gamma_actor_resources as g
import gamma_declarations as d

SOURCE = Path(os.environ.get('SADX_GAMMA_INSTALL', ROOT / 'reference/updated-pc-reference/game'))
DLL = SOURCE / 'system/CHRMODELS_orig.dll'
EXE = SOURCE / 'sonic.exe'


class Changed:
    def __init__(self, image, address, data):
        self.image, self.address, self.data = image, address, data

    def read(self, address, size):
        result = bytearray(self.image.read(address, size))
        lo, hi = max(address, self.address), min(address + size, self.address + len(self.data))
        if lo < hi:
            result[lo-address:hi-address] = self.data[lo-self.address:hi-self.address]
        return bytes(result)

    def __getattr__(self, name):
        return getattr(self.image, name)


class GammaResources(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dll, cls.exe = g.PEImage(DLL), g.PEImage(EXE)
        cls.temporary = tempfile.TemporaryDirectory()
        cls.output = Path(cls.temporary.name) / 'sonic-selection.actions'
        cls.result = g.import_gamma_actor_resources(DLL, EXE, cls.output)
        cls.metadata, cls.proof = cls.result['metadata'], cls.result['proof']

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def test_existing_namespace_prefix_exact(self):
        data = self.output.read_bytes()
        old = bytearray(data[:g.OLD_BYTES])
        struct.pack_into('<I', old, 8, g.OLD_RECORDS)
        self.assertEqual(hashlib.sha256(old).hexdigest(), g.OLD_PACK_SHA)
        self.assertEqual(struct.unpack_from('<I', data, 12)[0], 22)
        self.assertTrue(all(r['round_trip_exact'] for r in self.proof['records']))
        self.assertEqual(self.proof['appended_ids'], list(range(3303, self.result['records'] + 1)))

    def test_recipes_complete_and_inverse(self):
        table = g.export(self.dll, '___E102_ACTIONS')
        declarations = self.dll.unpack('<78I', table)
        rows, raw, _ = d.recipes(self.exe, table, declarations)
        for row, actual in zip(rows, self.metadata['recipes']):
            for key in row:
                self.assertEqual(actual[key], row[key])
        self.assertEqual(len(rows), 91)
        inverse = b''.join(struct.pack('<IBbHII',
            0x3c5d530 if r['declaration'] == 'constructed' else declarations[r['declaration']],
            r['object_count'], r['mode'], r['next'], int(r['transition_bits'], 16),
            int(r['speed_bits'], 16)) for r in rows)
        self.assertEqual(inverse, raw)
        self.assertEqual([r['row'] for r in rows if r['declaration'] == 'constructed'], [72])

    def test_stand_identity_and_missing_payloads(self):
        by_id = {r['id']: r for r in self.proof['records']}
        initial = self.metadata['initial_animation_action']
        self.assertEqual(by_id[initial]['source_address'], '0x1020e344')
        self.assertEqual(self.metadata['recipes'][0]['object_count'], 77)
        self.assertEqual(self.metadata['declarations'][61]['status'], 'known_null')
        self.assertIsNone(self.metadata['declarations'][61]['action'])
        self.assertEqual(self.metadata['constructed_action']['status'], 'unavailable')
        for row in self.metadata['recipes']:
            if row['status'] == 'unavailable':
                self.assertIsNone(row['action'])
                self.assertIsNotNone(row['resource_boundary'])

    def test_objects_and_weld_inverse(self):
        sources = {r['id']: int(r['source_address'], 16) for r in self.proof['records']}
        table = g.export(self.dll, '___E102_OBJECTS')
        for row in self.metadata['objects']:
            self.assertEqual(sources[row['object']], self.dll.unpack('<I', table + 4 * row['index'])[0])
        inverse = bytearray()
        for row in self.metadata['joins']:
            inverse += struct.pack('<IIIBbBBII', sources.get(row['object'], 0),
                sources.get(row['source_object'], 0), 0, row['vertex_count'],
                row['input_mode'], 0, 0, 0, 0)
        raw, _ = d.fold(self.exe, d.WELD, {0x3a8af6c: table},
                        {table: self.dll.read(table, 28 * 4)})
        self.assertEqual(inverse, raw)
        self.assertEqual(inverse[-24:], bytes(24))

    def test_matrix_nodes_are_preorder_identities_not_export_rows(self):
        records = {r['id']: r for r in self.proof['records']}
        root = self.dll.unpack('<I', g.export(self.dll, '___E102_OBJECTS'))[0]
        stack, traversal = [root], []
        while stack:
            address = stack.pop()
            if not address:
                continue
            traversal.append(address)
            child, sibling = self.dll.unpack('<II', address + 44)
            stack.extend([sibling, child])
        self.assertEqual(len(traversal), 77)
        expected = {54: 0x10201ae4, 30: 0x1020491c, 39: 0x10202c9c,
                    2: 0x10207228, 37: 0x1020323c}
        for row in self.metadata['matrix_nodes']:
            self.assertEqual(traversal[row['node']], expected[row['node']])
            self.assertEqual(int(records[row['object']]['source_address'], 16), traversal[row['node']])
        self.assertEqual([r['index'] for r in self.metadata['objects']], [0, 24, 25, 26, 27])
        self.assertEqual(self.metadata['matrix_cache_dependency']['status'], 'animated_runtime_result_required')

    def test_material_bindings_keep_source_aliases(self):
        records = {r['id']: r for r in self.proof['records']}
        table = g.export(self.dll, '___E102_MATERIALS')
        pointers = []
        for row in self.metadata['materials']:
            base = self.dll.unpack('<I', table + 4 * row['table_row'])[0]
            pointer = base + 20 * row['material_index']
            imported = records[row['materials']]
            self.assertEqual(int(imported['source_address'], 16) + 20 * row['record_index'], pointer)
            pointers.append((pointer, row['materials'], row['record_index']))
        for a in pointers:
            for b in pointers:
                if a[0] == b[0]:
                    self.assertEqual(a[1:], b[1:])

    def test_collision_colors_and_cursor_sources(self):
        self.assertEqual(struct.pack('<36I', *self.metadata['collision_words']), self.exe.read(0x91c5b8, 144))
        self.assertEqual(struct.pack('<2I', *self.metadata['line_color_words']), self.exe.read(0x91c648, 8))
        self.assertEqual(self.metadata['cursor_templates'][0]['points'], self.metadata['cursor_templates'][1]['points'])
        self.assertEqual(self.metadata['cursor_templates'][0]['indices'], self.metadata['cursor_templates'][1]['indices'])
        self.assertEqual(self.metadata['cursor_templates'][0]['line_count'], 12)

    def test_changed_and_unsupported_declarations_rejected(self):
        table = g.export(self.dll, '___E102_ACTIONS')
        with self.assertRaisesRegex(ValueError, 'initializer bytes changed'):
            d.recipes(Changed(self.exe, d.ANIMATION[0], b'\x90'), table, self.dll.unpack('<78I', table))
        for code, message in [(b'\x90\xc3', 'beyond static'),
                              (b'\xb0\x04\xa3' + struct.pack('<I', d.WELD[2]) + b'\xc3', 'indeterminate')]:
            spec = (d.WELD[0], d.WELD[0] + len(code), d.WELD[2], 24, g.digest(code))
            with self.assertRaisesRegex(ValueError, message):
                d.fold(Changed(self.exe, spec[0], code), spec, {}, {})

    def test_strict_motion_rejection_is_unchanged(self):
        importer, actions, *_ = g.build_sonic_actor_resources(self.dll, self.exe)
        motion = self.dll.unpack('<II', 0x1020e344)[1]
        with patch.object(importer.image, 'read', wraps=importer.image.read) as read:
            real = importer.image.dll.read
            def changed(address, size):
                if address == motion and size == 12:
                    raw = bytearray(real(address, size));struct.pack_into('<H', raw, 8, 7);return bytes(raw)
                return (importer.image.dll if address >= self.dll.base else self.exe).read(address, size)
            read.side_effect = changed
            with self.assertRaisesRegex(ValueError, 'MDATA2'):
                importer.action(0x1020e344)

    def test_atomic_output_preserves_source_inodes_and_existing_metadata(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            output = root / 'sonic-selection.actions'
            os.link(DLL, output)
            os.link(EXE, output.with_suffix('.provenance.json'))
            os.link(DLL, root / 'gamma-actor.json')
            old = root / 'sonic-actor.json';old.write_bytes(b'untouched metadata')
            before = [(p.stat().st_ino, p.stat().st_mtime_ns, g.digest(p.read_bytes())) for p in (DLL, EXE)]
            g.import_gamma_actor_resources(DLL, EXE, output)
            self.assertEqual(before, [(p.stat().st_ino, p.stat().st_mtime_ns, g.digest(p.read_bytes())) for p in (DLL, EXE)])
            self.assertEqual(old.read_bytes(), b'untouched metadata')
            self.assertNotEqual(output.stat().st_ino, DLL.stat().st_ino)

    def test_failed_import_never_publishes(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / 'sonic-selection.actions';output.write_bytes(b'previous pack')
            with patch.object(g, 'append_gamma', side_effect=ValueError('unsupported actual data')):
                with self.assertRaisesRegex(ValueError, 'unsupported actual'):
                    g.import_gamma_actor_resources(DLL, EXE, output)
            self.assertEqual(output.read_bytes(), b'previous pack')
            self.assertEqual(list(output.parent.iterdir()), [output])

    def test_atomic_pack_failed_writer_keeps_old_inode(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / 'sonic-selection.actions'
            output.write_bytes(b'previous pack')
            inode = output.stat().st_ino
            class FailedWriter:
                def write(self, temporary, actions, selection):
                    temporary.write_bytes(b'incomplete replacement')
                    raise ValueError('failed inverse proof')
            with self.assertRaisesRegex(ValueError, 'failed inverse'):
                g.atomic_pack(FailedWriter(), [], output)
            self.assertEqual(output.read_bytes(), b'previous pack')
            self.assertEqual(output.stat().st_ino, inode)
            self.assertEqual(list(output.parent.iterdir()), [output])

    def test_output_symlink_is_replaced_without_following_it(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            owned = root / 'owned.bin';owned.write_bytes(b'owned bytes')
            output = root / 'sonic-selection.actions';output.symlink_to(owned)
            inode = owned.stat().st_ino
            g.import_gamma_actor_resources(DLL, EXE, output)
            self.assertFalse(output.is_symlink())
            self.assertEqual(owned.read_bytes(), b'owned bytes')
            self.assertEqual(owned.stat().st_ino, inode)

    def test_owned_directory_rejected(self):
        with self.assertRaisesRegex(ValueError, 'outside owned'):
            g.import_gamma_actor_resources(DLL, EXE, EXE.parent / 'forbidden.actions')


if __name__ == '__main__':
    unittest.main()
