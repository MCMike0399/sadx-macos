"""Inverse-source checks for reached dialog resources and message identity aliases."""
import importlib.util,json,os,struct,tempfile,unittest
from pathlib import Path
from pe_image import PEImage
ROOT=Path(__file__).resolve().parents[2]
STAGE=ROOT
spec=importlib.util.spec_from_file_location('menu_dialog_import',STAGE/'tools/import/menu_dialog.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class DialogImport(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.image=PEImage(ROOT/'game/Sonic Adventure DX/sonic.exe');cls.data,cls.proof=module.declarations(cls.image)
 def test_schema(self):
  self.assertEqual(self.data['version'],1);self.assertEqual(len(self.data['definitions']),1)
 def test_descriptor_inverse(self):
  d=self.data['definitions'][0];identity=self.image.unpack('<5I',0x7ee328);raw=struct.pack('<5I2I7Ibb',d['style'],*identity[1:],d['upper_color'],d['lower_color'],*d['geometry_words'],d['choice_count'],d['cancel_choice']);self.assertEqual(raw,self.image.read(0x7ee328,58))
 def test_panels_inverse(self):
  d=self.data['definitions'][0];base=self.image.unpack('<I',0x7ee334)[0]
  for i,p in enumerate(d['panels']):self.assertEqual(struct.pack('<2IB',*p['position_words'],p['texture']),self.image.read(base+i*12,9))
 def test_sound_inverse(self):
  ptr=self.image.unpack('<I',0x7ee338)[0];self.assertEqual(struct.pack('<5i',*self.data['definitions'][0]['sounds']),self.image.read(ptr,20))
 def test_curves_inverse(self):
  for key,ptr in [('opening',0x7dfe20),('closing',0x7dfea0)]:self.assertEqual(struct.pack('<32I',*(v for row in self.data[key] for v in row)),self.image.read(ptr,128))
 def test_directions_inverse(self):
  self.assertEqual(struct.pack('<12I',*(v for row in self.data['directions'] for v in row)),self.image.read(0x7dff20,48))
 def test_message_identity(self):
  addresses={r['name']:r['address'] for r in self.proof['records']};ids=self.data['help_resource_ids_by_choice']
  for first in range(15):
   for second in range(15):
    a,b=divmod(first,5);c,d=divmod(second,5);self.assertEqual(ids[a][b]==ids[c][d],addresses[f'help_{a}_{b}']==addresses[f'help_{c}_{d}'])
  for row in ids:self.assertEqual(self.data['help_resources'][row[1]],'')
 def test_written_payload(self):
  with tempfile.TemporaryDirectory() as target:
   module.import_menu_dialog(ROOT/'game/Sonic Adventure DX',target);self.assertEqual(json.loads((Path(target)/'menu-dialog.json').read_text()),self.data);self.assertTrue(json.loads((Path(target)/'menu-dialog-evidence.json').read_text())['round_trip_exact'])
 def test_output_does_not_mutate_hardlinked_owned_file(self):
  with tempfile.TemporaryDirectory() as temporary:
   root=Path(temporary);install=root/'owned.json';install.write_text('owned data')
   output=root/'imported';output.mkdir();(output/'menu-dialog.json').hardlink_to(install)
   module.import_menu_dialog(ROOT/'game/Sonic Adventure DX',output)
   self.assertEqual(install.read_text(),'owned data')
   self.assertNotEqual(install.stat().st_ino,(output/'menu-dialog.json').stat().st_ino)
 def test_install_cannot_be_metadata_destination(self):
  with self.assertRaisesRegex(ValueError,'outside the owned install'):
   module.import_menu_dialog(ROOT/'game/Sonic Adventure DX',ROOT/'game/Sonic Adventure DX/unused-output')
if __name__=='__main__':unittest.main()

