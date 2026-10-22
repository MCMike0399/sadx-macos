import importlib.util,json,os,struct,tempfile,unittest
from pathlib import Path
STAGE=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('completion',STAGE/'tools/import/character_completion.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
ROOT=Path(os.environ.get('SADX_SOURCE_ROOT', STAGE));GAME=ROOT/'reference/updated-pc-reference/game'
class Changed:
 def __init__(self,image,address=0,data=b'',sha=None):self.image=image;self.address=address;self.data=data;self.sha256=sha or image.sha256
 def read(self,address,size):
  raw=bytearray(self.image.read(address,size));left=max(address,self.address);right=min(address+size,self.address+len(self.data))
  if left<right:raw[left-address:right-address]=self.data[left-self.address:right-self.address]
  return bytes(raw)
class Completion(unittest.TestCase):
 @classmethod
 def setUpClass(cls):cls.image=module.PEImage(GAME/'sonic.exe')
 def test_all_source_rows_and_inverse(self):
  metadata,proof=module.declarations(self.image);self.assertEqual([len(c['rows']) for c in metadata['catalogs']],[19,14,10,10,11,10,1]);self.assertEqual(sum(len(c['rows']) for c in metadata['catalogs']),75)
  for catalog,record in zip(metadata['catalogs'],proof['records']):
   inverse=b''.join(struct.pack('<iii',r['value'],r['source'],r['flag']) for r in catalog['rows']);self.assertEqual(inverse.hex(),record['source_bytes']);self.assertTrue(record['inverse_exact'])
  self.assertEqual(metadata['catalogs'][0]['rows'][0],{'value':5,'source':0,'flag':176});self.assertEqual(metadata['catalogs'][1]['rows'][0],{'value':5,'source':1,'flag':50})
 def test_wrong_source_rejected(self):
  with self.assertRaisesRegex(ValueError,'pinned'):module.declarations(Changed(self.image,sha='wrong'))
 def test_changed_descriptor_rejected(self):
  for address,data in [(module.DESCRIPTORS,bytes(4)),(module.DESCRIPTORS+4,struct.pack('<i',0))]:
   with self.subTest(address=address),self.assertRaisesRegex(ValueError,'descriptor'):module.declarations(Changed(self.image,address,data))
 def test_known_flag_owner_boundary_rejected(self):
  pointer=self.image.unpack('<I',module.DESCRIPTORS)[0]
  with self.assertRaisesRegex(ValueError,'flag exceeds'):module.declarations(Changed(self.image,pointer+8,struct.pack('<i',512)))
 def test_unknown_kind_preserves_ignored_flag(self):
  pointer=self.image.unpack('<I',module.DESCRIPTORS)[0];metadata,_=module.declarations(Changed(self.image,pointer+4,struct.pack('<ii',2,-1)));self.assertEqual(metadata['catalogs'][0]['rows'][0],{'value':5,'source':2,'flag':-1})
 def test_atomic_hardlink_publication(self):
  with tempfile.TemporaryDirectory() as temporary:
   root=Path(temporary);owned=root/'owned';owned.write_bytes(b'owned-original');out=root/'out';out.mkdir();target=out/'character-completion.json';os.link(owned,target);before=(owned.read_bytes(),owned.stat().st_ino);module.import_character_completion(GAME,out);self.assertEqual((owned.read_bytes(),owned.stat().st_ino),before);self.assertNotEqual(target.stat().st_ino,owned.stat().st_ino)
 def test_symlink_destination_rejected(self):
  with tempfile.TemporaryDirectory() as temporary:
   root=Path(temporary);owned=root/'owned';owned.write_bytes(b'owned-original');out=root/'out';out.mkdir();(out/'character-completion.json').symlink_to(owned)
   with self.assertRaisesRegex(ValueError,'symlink'):module.import_character_completion(GAME,out)
   self.assertEqual(owned.read_bytes(),b'owned-original')
 def test_output_inside_owned_install_rejected(self):
  with self.assertRaisesRegex(ValueError,'outside'):module.import_character_completion(GAME,GAME/'unpublished-completion')
if __name__=='__main__':unittest.main()
