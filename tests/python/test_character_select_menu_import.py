import importlib.util, json, os, struct, tempfile, unittest
from pathlib import Path
STAGE=Path(__file__).resolve().parents[2]
ROOT=Path(os.environ['SADX_SOURCE_ROOT'])
spec=importlib.util.spec_from_file_location('character_select_menu',STAGE/'tools/import/character_select_menu.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class Mutated:
    def __init__(self,image,address,data):self.image=image;self.sha256=image.sha256;self.address=address;self.data=data
    def read(self,address,size):
        data=bytearray(self.image.read(address,size))
        for i,b in enumerate(self.data):
            if address<=self.address+i<address+size:data[self.address+i-address]=b
        return bytes(data)
    def unpack(self,format,address):return struct.unpack(format,self.read(address,struct.calcsize(format)))
class ImportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.image=module.PEImage(ROOT/'game/Sonic Adventure DX/sonic.exe')
    def test_definitions(self):
        metadata,proof=module.declarations(self.image)
        self.assertEqual([(x['dialog'],x['style'],x['menu_texture'],x['choice_count'],x['cancel_choice'])for x in metadata['definitions']],[(7,5,5,3,2),(8,5,5,4,3),(9,5,5,2,1),(10,5,5,3,2)])
        self.assertTrue(proof['round_trip_exact'])
    def test_source_inverse(self):
        _,proof=module.declarations(self.image)
        for row in proof['records']:
            self.assertEqual(bytes.fromhex(row['bytes']),self.image.read(int(row['address'],16),len(bytes.fromhex(row['bytes']))))
    def test_widths(self):
        metadata,_=module.declarations(self.image)
        self.assertEqual(struct.unpack('<7f',struct.pack('<7I',*metadata['name_width_words'])),(143,122,227,102,89,209,230))
    def test_wrong_source(self):
        bad=Mutated(self.image,0,b'');bad.sha256='0'*64
        with self.assertRaises(ValueError):module.declarations(bad)
    def test_wrong_style(self):
        with self.assertRaises(ValueError):module.declarations(Mutated(self.image,0x7ee328+7*60,struct.pack('<I',4)))
    def test_callback(self):
        with self.assertRaises(ValueError):module.declarations(Mutated(self.image,0x7ee328+7*60+4,struct.pack('<I',1)))
    def test_geometry(self):
        with self.assertRaises(ValueError):module.declarations(Mutated(self.image,0x7ee328+7*60+28,struct.pack('<I',0x7fc00000)))
    def test_atomic(self):
        with tempfile.TemporaryDirectory()as d:
            game=ROOT/'game/Sonic Adventure DX';output=Path(d)
            module.import_character_select_menu(game,output);p=output/'character-select-menu.json';inode=p.stat().st_ino
            module.import_character_select_menu(game,output);self.assertNotEqual(inode,p.stat().st_ino)
            self.assertEqual(json.loads(p.read_text())['version'],1)
    def test_game_output_rejected(self):
        with self.assertRaises(ValueError):module.import_character_select_menu(ROOT/'game/Sonic Adventure DX',ROOT/'game/Sonic Adventure DX/forbidden')
if __name__=='__main__':unittest.main()
