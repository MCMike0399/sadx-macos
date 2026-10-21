import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('regular_textures', ROOT / 'tools/import/regular_textures.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class Image:
    sha256 = module.SOURCE_SHA256
    base = 0
    sections = [{'rva':0x3000,'virtual_size':4096,'raw_size':0}]
    def __init__(self):
        self.data = {module.TABLE:struct.pack('<3I',0x1000,0x1000,0),
                     0x1000:struct.pack('<2I',0x4000,0x2000),
                     0x1008:struct.pack('<2I',0,0x2010),
                     0x1010:bytes(8), 0x2000:struct.pack('<2I',0x3000,2),
                     0x2010:struct.pack('<2I',0x3020,1),
                     module.LOCALIZATION:struct.pack('<iII',0,0x4100,0x4200),
                     0x4000:b'CON_REGULAR\0',0x4100:b'b32ascii\0',0x4200:b'b32ascii_j\0'}
    def read(self,address,size):
        for start,raw in self.data.items():
            if start <= address and address+size <= start+len(raw): return raw[address-start:address-start+size]
        raise ValueError('fixture BSS or invalid source read')
    def unpack(self,fmt,address):return struct.unpack(fmt,self.read(address,struct.calcsize(fmt)))
    def string(self,address):return self.data[address][:-1].decode()

class RegularTextures(unittest.TestCase):
    def setUp(self):
        self.pins = patch.object(module, 'PINNED_MODS', {'input':{'input.dll':module.sha(b'fixture')}})
        self.loader = patch.object(module, 'PINNED_LOADER_SHA', module.sha(b'loader'))
        self.pins.start();self.loader.start()
        self.addCleanup(self.pins.stop);self.addCleanup(self.loader.stop)
    def test_aliases_and_initial_zero_storage(self):
        result,proof = module.declarations(Image())
        self.assertEqual(result['entries'],[0,0,None])
        self.assertEqual(result['lists'][0],{'texture_count':2,'filenames':[None,None],'attributes':[0,0]})
        self.assertEqual(result['groups'][0][1]['texture_list'],1)
        self.assertTrue(proof['directory_round_trip_exact'])
        self.assertTrue(all(row['round_trip_exact'] for row in proof['records']))
    def test_rejects_wrong_source(self):
        image=Image();image.sha256='0'*64
        with self.assertRaisesRegex(ValueError,'SHA256'):module.declarations(image)
    def test_initial_bytes_does_not_invent_unmapped_storage(self):
        with self.assertRaises(ValueError):module.initial_bytes(Image(),0x9000,12)
    def setup_game(self,root):
        (root/'system').mkdir()
        for name in ['CON_REGULAR.PVM','CON_REGULAR_E.PVM']:(root/'system'/name).write_bytes(b'PVMHdata')
        for name in ['b32ascii.PVR','b32ascii_j.PVR']:(root/'system'/name).write_bytes(b'PVRTdata')
        mod=root/'mods/input';mod.mkdir(parents=True);(mod/'mod.ini').write_text('Name=Input\nDLLFile=input.dll\n');(mod/'input.dll').write_bytes(b'fixture')
        folder=root/'mods/.modloader/profiles';folder.mkdir(parents=True)
        (folder.parent/'SADXModLoader.dll').write_bytes(b'loader')
        (folder/'Profiles.json').write_text(json.dumps({'ProfileIndex':0,'ProfilesList':[{'Filename':'selected.json'}]}))
        (folder/'selected.json').write_text(json.dumps({'EnabledMods':['input']}))
        metadata={'groups':[[{'archive':'CON_REGULAR','texture_list':0}]],'lists':[],
                  'localizations':[{'source':'b32ascii','replacement':'b32ascii_j'}]}
        return metadata
    def test_selected_profile_accepts_sectionless_ini_and_proves_assets(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);metadata=self.setup_game(root)
            proof=module.profile_evidence(root,metadata)
            self.assertTrue(proof['raw_texture_profile_proved'])
            self.assertEqual(proof['enabled_mods'],['input'])
            self.assertEqual(len(proof['assets']),4)
            self.assertIn('input.dll',proof['mods'][0]['dlls'])
    def test_missing_profile_is_not_vanilla_fallback(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,'selected loader profile'):module.selected_mods(Path(temp),None)
    def test_rejects_modified_mod_code(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);metadata=self.setup_game(root);(root/'mods/input/input.dll').write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError,'code identity'):module.profile_evidence(root,metadata)
    def test_rejects_pinned_mod_with_all_dlls_missing(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);metadata=self.setup_game(root);(root/'mods/input/input.dll').unlink()
            with self.assertRaisesRegex(ValueError,'code identity'):module.profile_evidence(root,metadata)
    def test_explicit_non_code_directory_remains_supported(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);metadata=self.setup_game(root)
            directory=root/'mods/plain';directory.mkdir();(directory/'mod.ini').write_text('Name=Plain\n')
            proof=module.profile_evidence(root,metadata,[directory])
            self.assertTrue(proof['raw_texture_profile_proved'])
            self.assertEqual(proof['mods'][0]['dlls'],{})
    def test_rejects_mod_texture_overlay(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);metadata=self.setup_game(root);(root/'mods/input/test.dds').write_bytes(b'x')
            with self.assertRaisesRegex(ValueError,'overlay'):module.profile_evidence(root,metadata)
    def test_rejects_file_replacement_mapping(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);metadata=self.setup_game(root)
            (root/'mods/input/mod.ini').write_text('Name=Input\n[ReplaceFiles]\nsystem/x.pvm=other\n')
            with self.assertRaisesRegex(ValueError,'replacement mappings'):module.profile_evidence(root,metadata)
    def test_rejects_alternate_gvr_and_missing_asset(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);metadata=self.setup_game(root);(root/'system/b32ascii.GVR').write_bytes(b'x')
            with self.assertRaisesRegex(ValueError,'alternate'):module.profile_evidence(root,metadata)
            (root/'system/b32ascii.GVR').unlink();(root/'system/b32ascii.PVR').unlink()
            with self.assertRaisesRegex(ValueError,'missing or ambiguous'):module.profile_evidence(root,metadata)
    def test_rejects_packed_content_with_raw_extension(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);metadata=self.setup_game(root);(root/'system/CON_REGULAR.PVM').write_bytes(b'PVZpacked')
            with self.assertRaisesRegex(ValueError,'packed/replacement'):module.profile_evidence(root,metadata)

if __name__=='__main__':unittest.main()
