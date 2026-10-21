import hashlib,json,os,shutil,struct,sys,tempfile,unittest
from pathlib import Path
STAGE=Path(__file__).resolve().parents[2];sys.path.insert(0,str(STAGE/'tools/import'))
import sonic_actor_resources as s
DLL=Path('reference/updated-pc-reference/game/system/CHRMODELS_orig.dll');EXE=Path('reference/updated-pc-reference/game/sonic.exe')
class Changed:
 def __init__(self,image,address,data):self.image=image;self.address=address;self.data=data;self.base=image.base;self.sections=image.sections
 def read(self,address,size):
  data=bytearray(self.image.read(address,size));x=max(address,self.address);y=min(address+size,self.address+len(self.data))
  if x<y:data[x-address:y-address]=self.data[x-self.address:y-self.address]
  return bytes(data)
 def __getattr__(self,name):return getattr(self.image,name)
class Tests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.dll=s.PEImage(DLL);cls.exe=s.PEImage(EXE);cls.temp=tempfile.TemporaryDirectory();cls.output=Path(cls.temp.name)/'sonic-selection.actions';cls.result=s.import_sonic_actor_resources(DLL,EXE,cls.output)
 @classmethod
 def tearDownClass(cls):cls.temp.cleanup()
 def test_all_records_inverse_and_existing_namespace(self):
  p=self.result['proof'];self.assertEqual(self.result['records'],3302);self.assertEqual(self.result['bytes'],760168);self.assertTrue(p['round_trip_exact']);self.assertEqual(sum(not r['direct_image_bytes'] for r in p['records']),7)
  # Previously qualified SDXACT03 selection graph2949 records, excluding its368-byte descriptor header.
  self.assertEqual(hashlib.sha256(self.output.read_bytes()[368:687528]).hexdigest(),'58541b2d3367502c2221269512eed05d5c894eea99003334315ccc188433d045')
 def test_exact_requested_model_slots_and_initial_action(self):
  m=self.result['metadata'];self.assertEqual([r['index'] for r in m['objects']],[5,6,22]);proof={r['id']:r for r in self.result['proof']['records']};table=s.export(self.dll,'___SONIC_OBJECTS')
  for row in m['objects']:self.assertEqual(int(proof[row['object']]['source_address'],16),self.dll.unpack('<I',table+row['index']*4)[0])
  action=self.dll.unpack('<I',s.export(self.dll,'___SONIC_ACTIONS')+4)[0];self.assertEqual(int(proof[m['initial_animation_action']]['source_address'],16),action)
  self.assertEqual([int(proof[i]['source_address'],16) for i in m['direct_ahead_objects']],[self.dll.unpack('<I',table+4*i)[0] for i in [50,52,51]])
 def test_whole_declarations_inverse(self):
  records={r['id']:r for r in self.result['proof']['records']};pairs={r['id']:r for r in self.result['proof']['vertex_pair_sources']}
  for spec in s.CONSTRUCTORS:
   raw,_=s.declarations(self.dll,self.exe,spec);out=bytearray();entries=self.result['metadata']['joins'][spec[0]];self.assertEqual(len(entries),spec[4]);self.assertEqual(entries[-1]['object'],0)
   for e in entries:
    refs=[int(records[e[k]]['source_address'],16) if e[k] else 0 for k in ['object','source_object','destination_object']];index=int(pairs[e['vertex_pairs']]['source_address'],16) if e['vertex_pairs'] else 0
    out.extend(struct.pack('<IIIBbBBII',*refs,e['vertex_count'],e['input_mode'],e['source_node'],e['destination_node'],e['original_vertices'],index))
   self.assertEqual(bytes(out),raw)
 def test_pair_arrays_and_collision_inverse(self):
  meta=self.result['metadata'];proof=self.result['proof'];pairs={r['id']:r for r in proof['vertex_pair_sources']}
  for row in meta['vertex_pairs']:self.assertEqual(b''.join(struct.pack('<HH',*v) for v in row['values']),bytes.fromhex(pairs[row['id']]['bytes']))
  self.assertEqual(struct.pack('<24I',*meta['collision_words']),self.exe.read(0x91cbf0,96))
  self.assertEqual(struct.pack('<264I',*[w for row in meta['player_parameter_words'] for w in row]),self.exe.read(0x9154e8,1056))
 def test_changed_constructor_rejected(self):
  with self.assertRaisesRegex(ValueError,'initializer bytes'):s.declarations(self.dll,Changed(self.exe,0x7d0b50,b'\xa0'),s.CONSTRUCTORS[0])
 def test_partial_register_never_invents_high_bits(self):
  code=b'\xb0\x04\xa3'+struct.pack('<I',0x3c55e28)+b'\xc3';spec=('test',0x7d0b50,0x7d0b50+len(code),0x3c55e28,1,hashlib.sha256(code).hexdigest())
  with self.assertRaisesRegex(ValueError,'indeterminate register'):s.declarations(self.dll,Changed(self.exe,0x7d0b50,code),spec)
 def test_behavior_in_declaration_body_rejected(self):
  code=b'\x90\xc3';spec=('test',0x7d0b50,0x7d0b52,0x3c55e28,1,hashlib.sha256(code).hexdigest())
  with self.assertRaisesRegex(ValueError,'beyond static declarations'):s.declarations(self.dll,Changed(self.exe,0x7d0b50,code),spec)
 def test_hardlink_publication_preserves_install(self):
  with tempfile.TemporaryDirectory() as folder:
   root=Path(folder);owned=root/'owned';owned.mkdir();dll=owned/'CHRMODELS_orig.dll';exe=owned/'sonic.exe';shutil.copy2(DLL,dll);shutil.copy2(EXE,exe);out=root/'sonic-selection.actions';os.link(dll,out);os.link(exe,out.with_suffix('.provenance.json'));before={p:(p.read_bytes(),p.stat().st_mtime_ns,p.stat().st_ino) for p in [dll,exe]};s.import_sonic_actor_resources(dll,exe,out)
   for p,expect in before.items():self.assertEqual((p.read_bytes(),p.stat().st_mtime_ns,p.stat().st_ino),expect)
 def test_owned_directory_rejected(self):
  with self.assertRaises(ValueError):s.import_sonic_actor_resources(DLL,EXE,EXE.parent/'forbidden-sonic.actions')
if __name__=='__main__':unittest.main()
