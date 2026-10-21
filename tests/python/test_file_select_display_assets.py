import importlib.util
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest import mock

PATH=Path(__file__).resolve().parents[2]/'tools/import/file_select_display.py'
spec=importlib.util.spec_from_file_location('file_select_display_assets',PATH)
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class Image:
    sha256=module.SOURCE_SHA256
    def __init__(self):
        self.blocks={}
        def put(a,fmt,*values):self.blocks[a]=struct.pack(fmt,*values)
        for i in range(3):put(0x7eca38+i*24,'<3fII4B',568,413,-12+i*2,4294942464,4294961920,13+i,14+i,15+i,0)
        put(0x10d7cfc,'<h',8)
        for i in range(8):put(0x10d7cfe+i*6,'<3h',i,i*64,i*32)
        for a,fmt,values in [(0x7eca20,'<4B',[6,3,5,4]),(0x7eca24,'<7B',list(range(7))),
            (0x7eca2c,'<7B',list(range(7))),(0x7ef6f0,'<7B',list(range(7))),
            (0x7ef65c,'<44B',list(range(44))),(0x7ef6b4,'<8H',list(range(8))),
            (0x7ef6c4,'<7I',list(range(72,79))),(0x7ef704,'<10B',list(range(1,11)))]:put(a,fmt,*values)
        put(0x10d7cb0,'<5I',*[0x20000000+i*4096 for i in range(5)])
        expected={0:0x10d7c90,1:0x915088,3:0x10d7c08,7:0x10d7c28,8:0x10d7c30,10:0x915068,
            11:0x10d7c18,13:0x10d7c38,15:0x10d7be8,22:0x10d7c98,24:0x10d7ca8,28:0x10d7bf0}
        for language in range(5):
            for index,address in expected.items():put(0x20000000+language*4096+index*8+4,'<I',address)
    def read(self,address,size):
        for start,raw in self.blocks.items():
            if start<=address and address+size<=start+len(raw):return raw[address-start:address-start+size]
        raise ValueError('unmapped fixture bytes')

class Tests(unittest.TestCase):
    def test_exact_tables_and_inverse(self):
        image=Image();metadata,proof=module.declarations(image)
        self.assertEqual(len(metadata['frame_rows']),3);self.assertEqual(len(metadata['sky_tiles']),8)
        self.assertEqual(metadata['digits'],list(range(1,11)))
        self.assertEqual(metadata['stage_names'],list(range(44)))
        for record in proof['records']:
            self.assertTrue(record['round_trip_exact'])
            self.assertEqual(bytes.fromhex(record['bytes']),image.read(int(record['address'],16),len(record['bytes'])//2))
        self.assertEqual(proof['menu_list_bindings']['1'],'0x915088')
        self.assertEqual(proof['menu_list_bindings']['10'],'0x915068')

    def test_source_identity_required(self):
        image=Image();image.sha256='bad'
        with self.assertRaises(ValueError):module.declarations(image)

    def test_binding_mismatch_all_languages_checked(self):
        image=Image();image.blocks[0x20000000+4*4096+10*8+4]=struct.pack('<I',0)
        with self.assertRaises(ValueError):module.declarations(image)

    def test_sky_count_not_guessed(self):
        image=Image();image.blocks[0x10d7cfc]=struct.pack('<h',9)
        with self.assertRaises(ValueError):module.declarations(image)

    def test_nonfinite_layout_rejected(self):
        image=Image();image.blocks[0x7eca38]=struct.pack('<3fII4B',float('nan'),413,-12,1,2,1,2,3,0)
        with self.assertRaises(ValueError):module.declarations(image)

    def test_atomic_metadata_hardlink_preserves_original(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);game=root/'game';output=root/'output';game.mkdir();output.mkdir()
            original=game/'sonic.exe';original.write_bytes(b'owned immutable source')
            before=original.read_bytes(),original.stat().st_mtime_ns,original.stat().st_ino
            os.link(original,output/'file-select-display.json')
            os.link(original,output/'file-select-display-evidence.json')
            with mock.patch.object(module,'PEImage',return_value=Image()):
                metadata=module.import_file_select_display(game,output)
            self.assertEqual(json.loads((output/'file-select-display.json').read_text()),metadata)
            self.assertEqual((original.read_bytes(),original.stat().st_mtime_ns,original.stat().st_ino),before)
            self.assertFalse(list(output.glob('.display-metadata-*')))

    def test_no_install_output(self):
        with tempfile.TemporaryDirectory() as directory:
            game=Path(directory)
            with self.assertRaises(ValueError):module.import_file_select_display(game,game/'output')

    def test_symlink_output_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);source=root/'source';source.write_bytes(b'keep')
            target=root/'metadata.json';target.symlink_to(source)
            with self.assertRaises(ValueError):module.atomic_json(target,{'new':True})
            self.assertEqual(source.read_bytes(),b'keep')
