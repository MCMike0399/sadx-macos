import importlib.util,json,os,struct,tempfile,unittest
from pathlib import Path
STAGE=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('labels',STAGE/'tools/import/character_select_labels.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT', STAGE));GAME=ROOT/'reference/updated-pc-reference/game'
class Changed:
 def __init__(self,image,*,sha=None,binding=False):self.image=image;self.sha256=sha or image.sha256;self.binding=binding
 def read(self,a,n):
  raw=self.image.read(a,n)
  if self.binding and a==self.image.unpack('<I',0x10d7cb0)[0]+14*8 and n==8:return raw[:4]+bytes(4)
  return raw
 def unpack(self,fmt,a):
  values=self.image.unpack(fmt,a)
  return (values[0],0) if self.binding and fmt=='<II' else values
class Labels(unittest.TestCase):
 def test_pinned_source_and_inverse(self):
  m,p=module.declarations(module.PEImage(GAME/'sonic.exe'));self.assertEqual(m['label_textures'],list(range(7)));self.assertEqual(bytes(m['label_textures']).hex(),p['source_label_bytes']);self.assertTrue(p['inverse_exact']);self.assertEqual(m['menu_textures'],{'labels':14,'names':15,'metal_name':28})
 def test_wrong_source_rejected(self):
  with self.assertRaisesRegex(ValueError,'pinned'):module.declarations(Changed(module.PEImage(GAME/'sonic.exe'),sha='wrong'))
 def test_changed_source_binding_rejected(self):
  with self.assertRaisesRegex(ValueError,'texture identity'):module.declarations(Changed(module.PEImage(GAME/'sonic.exe'),binding=True))
 def test_atomic_hardlink_publication_preserves_previous_owner(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp);owned=root/'owned';owned.write_bytes(b'preserve-owned-bytes');out=root/'out';out.mkdir();target=out/'character-select-labels.json';os.link(owned,target);before=(owned.read_bytes(),owned.stat().st_ino)
   module.import_character_select_labels(GAME,out);self.assertEqual((owned.read_bytes(),owned.stat().st_ino),before);self.assertNotEqual(target.stat().st_ino,owned.stat().st_ino);self.assertEqual(json.loads(target.read_text())['label_textures'],list(range(7)))
 def test_symlink_destination_rejected(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp);owned=root/'owned';owned.write_bytes(b'preserve');out=root/'out';out.mkdir();(out/'character-select-labels.json').symlink_to(owned)
   with self.assertRaisesRegex(ValueError,'symlink'):module.import_character_select_labels(GAME,out)
   self.assertEqual(owned.read_bytes(),b'preserve')
 def test_output_inside_owned_install_rejected(self):
  with self.assertRaisesRegex(ValueError,'outside'):module.import_character_select_labels(GAME,GAME/'unpublished-labels')
if __name__=='__main__':unittest.main()
