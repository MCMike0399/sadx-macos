import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('menu_textures', ROOT/'tools/import/menu_textures.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class Image:
    sha256 = module.SOURCE_SHA256
    base = 0
    sections = [{'rva':0x6000,'virtual_size':4096,'raw_size':0}]
    def __init__(self):
        self.data = {module.TABLE:struct.pack('<5I',*[0x1000+language*0x200 for language in range(5)]),
                     0x4000:struct.pack('<2I',0x5000,2),0x4010:struct.pack('<2I',0x6000,1),
                     0x5000:struct.pack('<6I',0x7200,7,0,0,9,0),
                     0x7200:b'filename\0',module.JAPANESE_TITLE_NAME:b'ava_gtitle0\0',
                     module.JAPANESE_TITLE_LIST:struct.pack('<2I',0x6020,3)}
        for language in range(5):
            name=0x7000+language*64
            self.data[name]=f'menu_{language}\0'.encode()
            raw=bytearray()
            for item in range(30):raw+=struct.pack('<2I',0x7400 if item==26 else name,0x4000 if item%2==0 else 0x4010)
            self.data[0x1000+language*0x200]=bytes(raw)
        self.data[0x7400]=b'unused\0'
        self.data[0x51014e]=b'\x68'+struct.pack('<I',module.JAPANESE_TITLE_LIST)+b'\x68'+struct.pack('<I',module.JAPANESE_TITLE_NAME)+b'\xe8'+struct.pack('<i',0x421180-0x51015d)
    def read(self,address,size):
        for start,raw in self.data.items():
            if start<=address and address+size<=start+len(raw):return raw[address-start:address-start+size]
        raise ValueError('fixture BSS or invalid read')
    def string(self,address):return self.data[address][:-1].decode()

class MenuTextures(unittest.TestCase):
    def setUp(self):
        self.regular = patch.object(module,'regular_declarations',return_value=({}, {'records':[{'kind':'texture_list','address':'0x4000'}]}))
        self.regular.start();self.addCleanup(self.regular.stop)
    def test_all_language_entries_and_aliases_preserved(self):
        result,proof=module.declarations(Image())
        self.assertEqual([len(row) for row in result['languages']],[30]*5)
        self.assertEqual(len(result['lists']),3)
        self.assertEqual(result['languages'][0][1]['texture_list'],result['languages'][4][1]['texture_list'])
        self.assertNotEqual(result['languages'][0][1]['archive'],result['languages'][4][1]['archive'])
        self.assertEqual(result['lists'][0]['regular_texture_list'],0)
        self.assertIsNone(result['lists'][1]['regular_texture_list'])
        self.assertTrue(proof['directory_round_trip_exact'])
        self.assertTrue(all(record['round_trip_exact'] for record in proof['records']))
    def test_initial_fields_and_separate_japanese_identity(self):
        result,proof=module.declarations(Image())
        self.assertEqual(result['lists'][0]['declaration'],{'texture_count':2,'filenames':['filename',None],'attributes':[7,9]})
        self.assertEqual(result['lists'][1]['declaration'],{'texture_count':1,'filenames':[None],'attributes':[0]})
        self.assertEqual(result['japanese_title'],{'archive':'ava_gtitle0','texture_list':2})
        self.assertNotEqual(result['japanese_title']['texture_list'],result['languages'][0][17]['texture_list'])
        self.assertEqual(len(bytes.fromhex(proof['japanese_call_bytes'])),15)
    def test_wrong_source_rejected(self):
        image=Image();image.sha256='0'*64
        with self.assertRaisesRegex(ValueError,'SHA256'):module.declarations(image)
    def test_actual_japanese_call_operands_are_checked(self):
        image=Image();image.data[0x51014e]=bytes(15)
        with self.assertRaisesRegex(ValueError,'call operands'):module.declarations(image)
    def test_preloaded_texture_is_not_silently_zeroed(self):
        image=Image();image.data[0x5000]=struct.pack('<6I',0x7200,7,99,0,9,0)
        with self.assertRaisesRegex(ValueError,'preloaded'):module.declarations(image)
    def test_null_list_and_anonymous_archive_rejected(self):
        for position in (0,4):
            image=Image();raw=bytearray(image.data[0x1000]);struct.pack_into('<I',raw,position,0);image.data[0x1000]=bytes(raw)
            with self.assertRaisesRegex(ValueError,'anonymous|null'):module.declarations(image)
    def test_emitted_metadata_contains_no_original_pointers(self):
        result,_=module.declarations(Image())
        self.assertNotIn('address',json.dumps(result))
        self.assertNotIn('0x',json.dumps(result))
    def test_absent_unused_asset_is_explicit_but_required_missing_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);system=root/'system';system.mkdir()
            for name in [f'menu_{i}' for i in range(5)]+['ava_gtitle0']:(system/(name+'.PVM')).write_bytes(b'PVMHfixture')
            calls=[]
            def profile(game,metadata,directories):calls.append(metadata);return {'raw_texture_profile_proved':True}
            with patch.object(module,'PEImage',return_value=Image()),patch.object(module,'profile_evidence',side_effect=profile):
                result=module.import_menu_textures(root,root/'out')
                self.assertEqual(result['unavailable_archives'],['unused'])
                self.assertNotIn('unused',[binding['archive'] for row in calls[0]['groups'] for binding in row])
                proof=json.loads((root/'out/menu-textures-evidence.json').read_text())
                self.assertEqual(proof['unavailable_archives'][0]['archive'],'unused')
                (system/'menu_1.PVM').unlink()
                with self.assertRaisesRegex(ValueError,'required title'):module.import_menu_textures(root,root/'out')

if __name__=='__main__':unittest.main()
