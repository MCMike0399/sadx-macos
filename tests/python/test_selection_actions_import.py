import hashlib,importlib.util,json,os,shutil,struct,sys,tempfile,unittest
from pathlib import Path
STAGE=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(STAGE/'tools/import'))
import selection_actions as s
DLL=Path('reference/updated-pc-reference/game/system/CHRMODELS_orig.dll')
EXE=Path('reference/updated-pc-reference/game/sonic.exe')
class Changed:
 def __init__(self,image,address,data):self.image=image;self.address=address;self.data=data;self.sha256=image.sha256;self.base=image.base
 def read(self,a,n):
  raw=bytearray(self.image.read(a,n));x=max(a,self.address);y=min(a+n,self.address+len(self.data))
  if x<y:raw[x-a:y-a]=self.data[x-self.address:y-self.address]
  return bytes(raw)
 def unpack(self,fmt,a):return struct.unpack(fmt,self.read(a,struct.calcsize(fmt)))
 def __getattr__(self,k):return getattr(self.image,k)
class Tests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):cls.dll=s.PEImage(DLL);cls.exe=s.PEImage(EXE)
 def test_full_original_records_and_initialized_actions(self):
  imp,actions,sequences,sources=s.build(self.dll,self.exe)
  self.assertEqual(len(imp.records),2949);self.assertEqual(len(actions),22);self.assertEqual([len(x['actions']) for x in sequences],[3,3,3,3,4,3]);self.assertEqual(len(sources.constructions),7)
  self.assertEqual(sum(x['direct_image_bytes'] for x in imp.evidence),2942)
  self.assertTrue(all(x['round_trip_exact'] for x in imp.evidence))
  for row in imp.evidence:
   if not row['direct_image_bytes']:self.assertEqual(row['source_component'],'constructed_action');self.assertIn('statically reconstructed',row['constructor_proof']['origin'])
 def test_legacy_startup_bytes_unchanged(self):
  imp=s.Importer(self.dll);table=s.export(self.dll,'___SONIC_ACTIONS');actions=[(i,imp.action(self.dll.unpack('<I',table+4*i)[0])) for i in (100,101)]
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'startup.actions';imp.write(p,actions);self.assertEqual(p.read_bytes(),Path('game/imported/characters/sonic-startup.actions').read_bytes())
 def test_legacy_title_bytes_unchanged(self):
  imp=s.Importer(self.dll);actions=[]
  for c,name,indices in s.TITLE_ACTIONS:
   table=s.export(self.dll,'___'+name+'_ACTIONS')
   for i in indices:actions.append((c,i,imp.action(self.dll.unpack('<I',table+4*i)[0])))
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'title.actions';imp.write(p,actions,title=True);self.assertEqual(p.read_bytes(),Path('game/imported/characters/title.actions').read_bytes())
 def test_source_hashes_rejected(self):
  x=Changed(self.dll,0,b'');x.sha256='0'*64
  with self.assertRaises(ValueError):s.Sources(x,self.exe)
  x=Changed(self.exe,0,b'');x.sha256='0'*64
  with self.assertRaises(ValueError):s.Sources(self.dll,x)
 def test_constructor_opcode_proof_rejects_mutation(self):
  with self.assertRaises(ValueError):s.Sources(self.dll,Changed(self.exe,0x7d23e0,b'\xa0'))
 def test_sequence_binding_proof_rejects_wrong_source_index(self):
  saved=s.SELECTION
  try:
   first=list(saved[0]);first[2]=(6,100);s.SELECTION=(tuple(first),)+saved[1:]
   with self.assertRaisesRegex(ValueError,'sequence initializer'):s.Sources(self.dll,self.exe)
  finally:s.SELECTION=saved
 def test_unknown_motion_channels_rejected(self):
  with self.assertRaisesRegex(ValueError,'MDATA2'):s.build(self.dll,Changed(self.exe,0x10c9018+8,struct.pack('<H',4)))
 def test_imported_scene_declarations_inverse(self):
  with tempfile.TemporaryDirectory() as d:
   out=Path(d)/'selection.actions';r=s.import_selection_actions(DLL,EXE,out);m=r['metadata']
   self.assertEqual(struct.pack('<21I',*[x for row in m['positions_words'] for x in row]),self.exe.read(0x7f07f8,84));self.assertEqual(struct.pack('<7H',*m['characters']),self.exe.read(0x7ef6b4,14));self.assertEqual(struct.pack('<i',m['sonic_aim_y']),self.exe.read(0x10db828,4));self.assertEqual(out.read_bytes()[:8],b'SDXACT03')
 def test_hardlink_outputs_leave_owned_sources_unchanged(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);owned=root/'owned';owned.mkdir();dll=owned/'CHRMODELS_orig.dll';exe=owned/'sonic.exe';shutil.copy2(DLL,dll);shutil.copy2(EXE,exe);out=root/'selection.actions';os.link(dll,out);os.link(exe,out.with_suffix('.provenance.json'));before={p:(p.read_bytes(),p.stat().st_mtime_ns,p.stat().st_ino) for p in (dll,exe)};s.import_selection_actions(dll,exe,out)
   for p,(data,mtime,inode) in before.items():self.assertEqual((p.read_bytes(),p.stat().st_mtime_ns,p.stat().st_ino),(data,mtime,inode))
 def test_owned_directory_output_rejected(self):
  with self.assertRaises(ValueError):s.import_selection_actions(DLL,EXE,EXE.parent/'forbidden-selection.actions')
if __name__=='__main__':unittest.main()
